# Roadwise Vehicle Rental Platform

## 1. Project Overview

Roadwise is a vehicle rental management platform for fleet operators and customers. The project combines a C++ rental engine with a browser dashboard and a Node.js service for authenticated, database-backed workflows.

## 2. Objectives

- Manage cars, SUVs, premium vehicles, EVs, and two-wheelers.
- Calculate demand-aware rental prices.
- Detect overlapping bookings.
- Track payments, deposits, late fees, tolls, and damage.
- Provide a customer booking portal.
- Provide role-aware admin and customer experiences.
- Demonstrate a path from local demo to hosted application.

## 3. Main Features

### Admin

- Fleet inventory and availability
- Reservation calendar and conflict detection
- Dynamic pricing and revenue reports
- KYC review and blocklist controls
- Maintenance and Fastag operations
- Notifications for overdue, payment, compliance, and maintenance events

### Customer

- Vehicle search by type, date, and duration
- Dynamic quote with loyalty discount
- Booking and invoice access
- Reservation history
- Customer-only portal navigation

### Backend foundation

- Node.js and Express HTTP service
- SQLite persistence through Node.js built-in `node:sqlite`
- Password hashing with `bcryptjs`
- JWT authentication and role checks
- Razorpay test-order integration point
- Optional SMTP email and Twilio SMS adapters
- Railway deployment configuration

## 4. Architecture

```text
Browser UI (index.html)
        |
        | HTTP / JSON
        v
Node.js + Express API
        |
        +-- SQLite: users, state, notifications, payments
        +-- Razorpay test API
        +-- SMTP / Twilio adapters

C++ engine and C++ shared server remain available as the original course implementation.
```

## 5. Technology Stack

- C++17 for object-oriented rental logic
- HTML, CSS, and JavaScript for the dashboard
- Node.js 20+ and Express
- SQLite
- JWT and bcryptjs
- Razorpay test mode
- Railway deployment

## 6. Running Locally

```powershell
npm install
Copy-Item .env.example .env
npm start
```

Open `http://127.0.0.1:3000/`.

Demo accounts:

- Admin: `admin@roadwise.test` / `admin123`
- Customer: `customer@roadwise.test` / `customer123`

The C++ browser demo remains available through the existing VS Code task on port 8080.

## 7. Testing

```powershell
npm test
```

The current automated tests cover vehicle recommendation and availability behavior. Additional API tests should be added before production deployment.

## 8. Security Note

The browser demo login is intentionally simple for classroom demonstration. The Node service contains the real authentication foundation, but production use still requires HTTPS, a strong `JWT_SECRET`, provider secrets stored in Railway variables, rate limiting, CSRF strategy where applicable, audit logging, and a managed database with backups.

## 9. Future Work

- Connect the browser login form to `/api/auth/login` in hosted mode.
- Store booking ownership server-side instead of exposing the complete demo state to customers.
- Add API integration tests.
- Add real Razorpay webhook verification.
- Configure SMTP and Twilio credentials.
- Move SQLite to PostgreSQL for multi-instance Railway deployment.
