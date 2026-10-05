require("dotenv").config();

const crypto = require("node:crypto");
const fs = require("node:fs");
const path = require("node:path");
const express = require("express");
const bcrypt = require("bcryptjs");
const { DatabaseSync } = require("node:sqlite");
const jwt = require("jsonwebtoken");
const Razorpay = require("razorpay");
const nodemailer = require("nodemailer");
const twilio = require("twilio");

const rootDir = path.resolve(__dirname, "..");
const dataDir = path.join(rootDir, "data");
fs.mkdirSync(dataDir, { recursive: true });
const database = new DatabaseSync(path.join(dataDir, "roadwise.sqlite"));
const port = Number(process.env.PORT || 3000);
const jwtSecret = process.env.JWT_SECRET || "roadwise-course-project-change-me";
const app = express();

app.use(express.json({ limit: "2mb" }));
app.use(express.static(rootDir));

database.exec("PRAGMA journal_mode = WAL");
database.exec(`
  CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    email TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    role TEXT NOT NULL CHECK (role IN ('admin', 'customer')),
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
  );
  CREATE TABLE IF NOT EXISTS app_state (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    revision INTEGER NOT NULL,
    state_json TEXT NOT NULL,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
  );
  CREATE TABLE IF NOT EXISTS notifications (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    title TEXT NOT NULL,
    detail TEXT NOT NULL,
    channel TEXT NOT NULL DEFAULT 'in-app',
    read_at TEXT,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
  );
  CREATE TABLE IF NOT EXISTS payments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    booking_id TEXT,
    provider TEXT NOT NULL,
    provider_order_id TEXT,
    amount INTEGER NOT NULL,
    currency TEXT NOT NULL DEFAULT 'INR',
    status TEXT NOT NULL DEFAULT 'created',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(id)
  );
`);

function seedDatabase() {
  const userCount = database.prepare("SELECT COUNT(*) AS count FROM users").get().count;
  if (!userCount) {
    const insertUser = database.prepare("INSERT INTO users (name, email, password_hash, role) VALUES (?, ?, ?, ?)");
    insertUser.run("Fleet manager", "admin@roadwise.test", bcrypt.hashSync("admin123", 10), "admin");
    insertUser.run("Customer account", "customer@roadwise.test", bcrypt.hashSync("customer123", 10), "customer");
  }
  const stateRow = database.prepare("SELECT id FROM app_state WHERE id = 1").get();
  if (!stateRow) {
    const sourcePath = path.join(rootDir, "roadwise_shared_data.json");
    const source = JSON.parse(fs.readFileSync(sourcePath, "utf8"));
    database.prepare("INSERT INTO app_state (id, revision, state_json) VALUES (1, ?, ?)").run(source.revision || 0, JSON.stringify(source.state || source));
  }
}
seedDatabase();

function publicUser(user) {
  return { id: user.id, name: user.name, email: user.email, role: user.role };
}

function issueToken(user) {
  return jwt.sign({ sub: user.id, role: user.role, email: user.email }, jwtSecret, { expiresIn: "8h" });
}

function authenticate(request, response, next) {
  const header = request.get("authorization") || "";
  const token = header.startsWith("Bearer ") ? header.slice(7) : "";
  if (!token) return response.status(401).json({ error: "Authentication required." });
  try {
    const claims = jwt.verify(token, jwtSecret);
    request.user = database.prepare("SELECT id, name, email, role FROM users WHERE id = ?").get(claims.sub);
    if (!request.user) return response.status(401).json({ error: "User account not found." });
    next();
  } catch (error) {
    return response.status(401).json({ error: "Session expired. Sign in again." });
  }
}

function requireAdmin(request, response, next) {
  if (request.user.role !== "admin") return response.status(403).json({ error: "Administrator access required." });
  next();
}

function readState() {
  const row = database.prepare("SELECT revision, state_json FROM app_state WHERE id = 1").get();
  return { revision: row.revision, state: JSON.parse(row.state_json) };
}

app.get("/api/health", (request, response) => response.json({ status: "ok", service: "roadwise-node", database: "sqlite" }));

app.post("/api/auth/register", (request, response) => {
  const name = String(request.body.name || "").trim();
  const email = String(request.body.email || "").trim().toLowerCase();
  const password = String(request.body.password || "");
  if (name.length < 2 || !email.includes("@") || password.length < 8) return response.status(400).json({ error: "Use a name, valid email, and password of at least 8 characters." });
  try {
    const result = database.prepare("INSERT INTO users (name, email, password_hash, role) VALUES (?, ?, ?, 'customer')").run(name, email, bcrypt.hashSync(password, 10));
    const user = database.prepare("SELECT id, name, email, role FROM users WHERE id = ?").get(result.lastInsertRowid);
    response.status(201).json({ user: publicUser(user), token: issueToken(user) });
  } catch (error) {
    response.status(409).json({ error: "An account with that email already exists." });
  }
});

app.post("/api/auth/login", (request, response) => {
  const email = String(request.body.email || "").trim().toLowerCase();
  const password = String(request.body.password || "");
  const user = database.prepare("SELECT * FROM users WHERE email = ?").get(email);
  if (!user || !bcrypt.compareSync(password, user.password_hash)) return response.status(401).json({ error: "Invalid email or password." });
  response.json({ user: publicUser(user), token: issueToken(user) });
});

app.get("/api/me", authenticate, (request, response) => response.json({ user: publicUser(request.user) }));

app.get("/api/state", authenticate, (request, response) => response.json(readState()));
app.put("/api/state", authenticate, requireAdmin, (request, response) => {
  const expectedRevision = Number(request.body.revision);
  const nextState = request.body.state;
  const current = readState();
  if (!Number.isSafeInteger(expectedRevision) || !nextState || !Array.isArray(nextState.vehicles) || !Array.isArray(nextState.bookings)) return response.status(400).json({ error: "Expected a revision and valid rental state." });
  if (expectedRevision !== current.revision) return response.status(409).json(current);
  const nextRevision = current.revision + 1;
  database.prepare("UPDATE app_state SET revision = ?, state_json = ?, updated_at = CURRENT_TIMESTAMP WHERE id = 1").run(nextRevision, JSON.stringify(nextState));
  response.json({ revision: nextRevision, state: nextState });
});

app.post("/api/bookings", authenticate, (request, response) => {
  const current = readState();
  const payload = request.body || {};
  const vehicle = current.state.vehicles.find((item) => item.id === payload.vehicleId);
  const start = String(payload.start || "");
  const end = String(payload.end || "");
  if (!vehicle || !/^\d{4}-\d{2}-\d{2}$/.test(start) || !/^\d{4}-\d{2}-\d{2}$/.test(end) || start >= end) return response.status(400).json({ error: "Vehicle and valid rental dates are required." });
  const overlaps = current.state.bookings.some((booking) => !booking.returned && booking.vehicleId === vehicle.id && start < (booking.end || booking.endAt) && (booking.start || booking.startAt) < end);
  if (overlaps) return response.status(409).json({ error: "Those dates overlap another reservation for this vehicle." });
  const booking = {
    ...payload,
    id: `RW-${current.state.nextId++}`,
    customer: request.user.role === "customer" ? request.user.name : String(payload.customer || request.user.name),
    returned: false,
    created: new Date().toISOString().slice(0, 10)
  };
  current.state.bookings.push(booking);
  database.prepare("UPDATE app_state SET revision = ?, state_json = ?, updated_at = CURRENT_TIMESTAMP WHERE id = 1").run(current.revision + 1, JSON.stringify(current.state));
  database.prepare("INSERT INTO notifications (user_id, title, detail, channel) SELECT NULL, ?, ?, 'in-app' FROM users WHERE role = 'admin' LIMIT 1").run("New reservation", `${booking.id} · ${booking.customer} booked ${vehicle.model}.`);
  response.status(201).json({ revision: current.revision + 1, booking, state: current.state });
});

app.get("/api/notifications", authenticate, (request, response) => {
  const rows = database.prepare("SELECT id, title, detail, channel, read_at, created_at FROM notifications WHERE user_id IS NULL OR user_id = ? ORDER BY created_at DESC LIMIT 50").all(request.user.id);
  response.json({ notifications: rows });
});
app.post("/api/notifications", authenticate, requireAdmin, (request, response) => {
  const title = String(request.body.title || "").trim();
  const detail = String(request.body.detail || "").trim();
  const channel = String(request.body.channel || "in-app").trim();
  if (!title || !detail) return response.status(400).json({ error: "Notification title and detail are required." });
  const result = database.prepare("INSERT INTO notifications (user_id, title, detail, channel) VALUES (?, ?, ?, ?)").run(request.body.userId || null, title, detail, channel);
  response.status(201).json({ id: result.lastInsertRowid, title, detail, channel });
});
app.patch("/api/notifications/:id/read", authenticate, (request, response) => {
  database.prepare("UPDATE notifications SET read_at = CURRENT_TIMESTAMP WHERE id = ? AND (user_id IS NULL OR user_id = ?)").run(request.params.id, request.user.id);
  response.status(204).end();
});

const razorpay = process.env.RAZORPAY_KEY_ID && process.env.RAZORPAY_KEY_SECRET
  ? new Razorpay({ key_id: process.env.RAZORPAY_KEY_ID, key_secret: process.env.RAZORPAY_KEY_SECRET })
  : null;
app.post("/api/payments/order", authenticate, async (request, response) => {
  const amount = Math.round(Number(request.body.amount));
  if (!Number.isInteger(amount) || amount < 100) return response.status(400).json({ error: "Payment amount must be at least ₹100." });
  const receipt = String(request.body.bookingId || `roadwise-${Date.now()}`);
  try {
    const order = razorpay
      ? await razorpay.orders.create({ amount, currency: "INR", receipt })
      : { id: `mock_order_${crypto.randomUUID()}`, amount, currency: "INR", status: "created", mock: true };
    database.prepare("INSERT INTO payments (user_id, booking_id, provider, provider_order_id, amount, status) VALUES (?, ?, ?, ?, ?, ?)").run(request.user.id, request.body.bookingId || null, razorpay ? "razorpay" : "mock", order.id, amount, "created");
    response.status(201).json({ order, keyId: process.env.RAZORPAY_KEY_ID || null, mock: !razorpay });
  } catch (error) {
    response.status(502).json({ error: "Payment provider could not create an order." });
  }
});
app.post("/api/payments/verify", authenticate, (request, response) => {
  const { orderId, paymentId, signature } = request.body;
  if (!orderId || !paymentId || !signature) return response.status(400).json({ error: "Payment verification fields are required." });
  const expected = process.env.RAZORPAY_KEY_SECRET ? crypto.createHmac("sha256", process.env.RAZORPAY_KEY_SECRET).update(`${orderId}|${paymentId}`).digest("hex") : signature;
  if (expected !== signature) return response.status(400).json({ error: "Payment signature is invalid." });
  database.prepare("UPDATE payments SET status = 'paid' WHERE provider_order_id = ? AND user_id = ?").run(orderId, request.user.id);
  response.json({ status: "paid" });
});

async function sendExternalNotification({ email, phone, subject, message }) {
  const results = [];
  if (email && process.env.SMTP_HOST && process.env.SMTP_USER && process.env.SMTP_PASS) {
    const transporter = nodemailer.createTransport({ host: process.env.SMTP_HOST, port: Number(process.env.SMTP_PORT || 587), secure: process.env.SMTP_SECURE === "true", auth: { user: process.env.SMTP_USER, pass: process.env.SMTP_PASS } });
    await transporter.sendMail({ from: process.env.SMTP_FROM || process.env.SMTP_USER, to: email, subject, text: message });
    results.push("email");
  }
  if (phone && process.env.TWILIO_ACCOUNT_SID && process.env.TWILIO_AUTH_TOKEN && process.env.TWILIO_FROM_NUMBER) {
    const client = twilio(process.env.TWILIO_ACCOUNT_SID, process.env.TWILIO_AUTH_TOKEN);
    await client.messages.create({ body: message, from: process.env.TWILIO_FROM_NUMBER, to: phone });
    results.push("sms");
  }
  return results;
}
app.post("/api/notifications/send", authenticate, requireAdmin, async (request, response) => {
  try {
    const channels = await sendExternalNotification(request.body);
    response.json({ sent: channels, configured: Boolean(channels.length) });
  } catch (error) {
    response.status(502).json({ error: "External notification provider failed." });
  }
});

app.get("*", (request, response) => response.sendFile(path.join(rootDir, "index.html")));
app.listen(port, "0.0.0.0", () => console.log(`Roadwise Node backend listening on port ${port}`));
