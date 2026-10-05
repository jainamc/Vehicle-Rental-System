# Roadwise Vehicle Rental Demo

Roadwise has two versions:

- `index.html`: browser interface served by the C++ shared server.
- `.vscode/oops.cpp`: C++ console version. Compile it with a C++17 compiler such as MinGW g++.
- `.vscode/shared_server.cpp`: C++ HTTP server for sharing browser data across devices on one local network.

## Run on this computer

In VS Code, choose **Tasks: Run Task** and run `Run Shared Rental Web App (C++)`. Keep its terminal running. On the host computer open `http://127.0.0.1:8080/`, or run `Open Rental Web App` as a separate task to open it.

### One-click dashboard start on Windows

Double-click `START_DASHBOARD.bat`. It builds the C++ shared server, starts it, and opens the dashboard at `http://127.0.0.1:8080/`. Keep the server window open while using the dashboard. Close that server window when you are finished.

`localhost` is not an error: it means the dashboard is running privately on your own computer. To let another device on the same Wi-Fi open it, use the host computer's local IPv4 address instead of `127.0.0.1`, for example `http://192.168.1.5:8080/`.

For another device on the same trusted Wi-Fi, get the host computer's IPv4 address with `ipconfig` and open `http://HOST-IP:8080/` on the other device. Allow the server through Windows Firewall for private networks if prompted. The host computer must stay on with the server running. Connected pages check for updates every two seconds.

## C++ data storage

The browser app's shared server saves data to `roadwise_shared_data.json` in the project folder and keeps the previous valid version in `roadwise_shared_data.json.bak`. Writes are version-checked to avoid silently overwriting a newer change from another device.

The separate C++ console app loads and saves `rental_data.txt`; it does not currently use the browser server's shared data file.

This is a LAN classroom demo without accounts or HTTPS. Use a trusted private network only; do not expose it to the public internet or enter real customer or payment data. Internet-wide access needs a hosted HTTPS server, authentication, and persistent database.

## Node backend upgrade

The project also includes a course-friendly production foundation in `backend/server.js`.
It adds SQLite persistence, JWT authentication, role checks, Razorpay test-order support, optional SMTP/Twilio notifications, and Railway deployment configuration.

```powershell
npm install
Copy-Item .env.example .env
npm start
```

Open `http://127.0.0.1:3000/`. The Node backend seeds the demo accounts `admin@roadwise.test` / `admin123` and `customer@roadwise.test` / `customer123` into `data/roadwise.sqlite`.

Do not open `index.html` with VS Code Live Server for the full application. Live Server only serves the frontend and cannot provide `/api/state`, authentication, bookings, or payments. Use `npm start` locally, or deploy the repository as a Railway Node service. Railway uses the configured `PORT` automatically and exposes the same backend URL for both the UI and API.

Keep payment, SMTP, Twilio, and JWT values in `.env` or Railway environment variables. Never commit `.env` or real provider keys. See [PROJECT_REPORT.md](PROJECT_REPORT.md) and [PRESENTATION.md](PRESENTATION.md) for the report and slide outline.

To build and run the C++ version manually:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic .vscode\oops.cpp -o .vscode\oops.exe
.\.vscode\oops.exe
```

## Share the browser demo

For devices on the same network, share the host URL above while the C++ server is running. Opening `index.html` directly or through Five Server will not connect to shared data. GitHub Pages can host static files but cannot run this C++ server or synchronize rental records.

The browser demo also works in **Local demo mode** when no server is available. Vehicle additions, bookings, returns, and other changes are saved in the browser's `localStorage`, so the UI does not block the OOP demonstration. The C++ files remain the main course implementation; the Node backend is an optional extension for authentication and hosted persistence.

## Share the C++ console version

Share `.vscode/oops.cpp` with the recipient. They need a C++17 compiler installed. On Windows with MinGW g++, they can run the build commands above from the project folder. VS Code tasks are optional and only work when opening the whole project folder in VS Code.

## Optional GitHub upload

This folder is currently not connected to a Git repository. To upload it:

```powershell
git init
git add .
git commit -m "Submit Roadwise vehicle rental project"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/YOUR-REPOSITORY.git
git push -u origin main
```

Do not upload `.env`, real provider keys, or private customer data. Create the GitHub repository first, replace the remote URL, and sign in when Git asks.