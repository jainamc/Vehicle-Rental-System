# Roadwise Viva Preparation

## 30-second introduction

Roadwise is a vehicle-rental management platform. It combines a C++17 object-oriented rental engine with an HTML/CSS/JavaScript browser interface and a Node.js/Express backend. The system manages vehicles, availability, bookings, payments, notifications, and role-based access for administrators and customers.

## Important questions and answers

### What problem does the project solve?

Manual rental records can cause double bookings, incorrect pricing, missed maintenance, and poor payment tracking. Roadwise centralizes fleet availability, reservations, pricing, and operational alerts.

### Which OOP concepts are demonstrated?

Encapsulation keeps vehicle and booking data controlled through methods. Inheritance allows specialized vehicle classes to derive from a common `Vehicle` base class. Polymorphism lets different vehicle types use a common interface. Abstraction hides pricing and rental implementation details behind clear operations.

### How is double booking prevented?

For a requested interval `[start, end)`, a booking overlaps when:

```text
requestedStart < existingEnd
AND existingStart < requestedEnd
```

The API checks active bookings for the same vehicle before creating a reservation. The C++ shared server also uses revision checks so one device cannot silently overwrite another device's newer state.

### How does vehicle recommendation work?

The system filters by required range and passenger capacity, removes vehicles with overlapping bookings, calculates a quote, removes quotes over budget, and sorts by price and score. It returns at most three recommendations.

### How does authentication work?

Passwords are hashed with bcryptjs. On successful login, the server issues an eight-hour JWT. Protected routes verify the Bearer token, load the user, and apply role checks such as administrator-only state updates.

### What is the difference between authentication and authorization?

Authentication verifies who the user is. Authorization verifies what that user may do. For example, both roles can authenticate, but only an administrator can update shared state or send notifications.

### Why is a revision number used?

The revision number provides optimistic concurrency control. A client sends the revision it last read. If it differs from the database revision, the server returns HTTP 409 instead of overwriting newer data.

### What is stored in SQLite?

Users, the current rental state and revision, notifications, and payment records are stored in SQLite. The rental state contains vehicles and bookings as JSON while operational records use relational tables.

### How are payments handled?

With Razorpay credentials, the backend creates test orders. Without credentials, it creates a clearly marked mock order for classroom demonstration. When configured, payment verification uses an HMAC-SHA256 signature.

### What are the project limitations?

The course demo uses local SQLite and optional external notification providers. Production improvements would include HTTPS, rate limiting, stronger secret management, audit logging, webhook handling, and a managed database.

## Demonstration sequence

1. Start with `npm.cmd start`.
2. Open `http://127.0.0.1:3000/`.
3. Admin login: `admin@roadwise.test` / `admin123`.
4. Show fleet status, alerts, and admin-only actions.
5. Customer login: `customer@roadwise.test` / `customer123`.
6. Search for a vehicle and show quote and availability filtering.
7. Create a reservation and show the invoice/notification flow.
8. Explain the C++ classes and overlap formula.

## Commands

```powershell
npm.cmd test
npm.cmd start
```

Health endpoint:

```text
GET http://127.0.0.1:3000/api/health
```
