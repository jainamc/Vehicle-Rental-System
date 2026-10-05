# Roadwise Presentation Slides

## Slide 1: Title

**Roadwise Vehicle Rental Platform**

A smart fleet operations and customer booking system.

## Slide 2: Problem

- Manual vehicle availability tracking causes booking conflicts.
- Fixed prices do not reflect demand or seasonality.
- Customer payments, deposits, and damage charges are difficult to track.
- Fleet compliance and maintenance tasks can be missed.

## Slide 3: Solution

- One dashboard for fleet managers.
- Customer portal for vehicle discovery and reservations.
- Dynamic pricing engine.
- Availability and conflict detection.
- Payment, invoice, return, and notification workflows.

## Slide 4: Object-Oriented Design

- `Vehicle` base class with specialized vehicle types.
- Pricing strategy abstraction for dynamic pricing.
- Booking and return flows model real rental operations.
- C++17 demonstrates inheritance, polymorphism, and encapsulation.

## Slide 5: Admin Dashboard

- Fleet status and utilization
- Revenue reports
- KYC verification
- Maintenance readiness
- Fastag balances
- Overdue and payment notifications

## Slide 6: Customer Portal

- Search available vehicles
- Compare live quotes
- Apply loyalty discounts
- Create reservations
- View payment status and invoices

## Slide 7: Backend Upgrade

- Node.js and Express API
- SQLite database
- JWT authentication
- Bcrypt password hashing
- Admin/customer role separation
- API endpoints for users, state, payments, and notifications

## Slide 8: Payment and Notifications

- Razorpay test-mode order creation
- Payment verification endpoint
- Optional email through SMTP
- Optional SMS through Twilio
- In-app notification center

## Slide 9: Deployment

- Railway-ready `railway.json`
- Environment variables for secrets
- Health endpoint at `/api/health`
- SQLite for the course demo
- PostgreSQL recommended for production scaling

## Slide 10: Demonstration Flow

1. Sign in as admin.
2. Review fleet alerts.
3. Open the customer portal.
4. Search for a vehicle and view a quote.
5. Sign out and sign in as customer.
6. Create a reservation.
7. Review the invoice and notification flow.

## Slide 11: Testing

- Recommendation tests
- Availability conflict checks
- Shared state revision checks
- Authentication API checks to add before final production release

## Slide 12: Conclusion

Roadwise demonstrates how a basic C++ rental program can evolve into a complete fleet operations product with a customer portal, backend services, payments, notifications, and deployment readiness.
