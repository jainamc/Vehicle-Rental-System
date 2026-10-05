#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

namespace {
const char* dataPath = "roadwise_shared_data.json";
const char* backupPath = "roadwise_shared_data.json.bak";
const char* temporaryPath = "roadwise_shared_data.json.tmp";

bool fileExists(const char* path) {
#ifdef _WIN32
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
#else
    std::ifstream input(path, std::ios::binary);
    return input.good();
#endif
}

bool copyFile(const char* sourcePath, const char* destinationPath) {
    std::ifstream source(sourcePath, std::ios::binary);
    std::ofstream destination(destinationPath, std::ios::binary | std::ios::trunc);
    if (!source || !destination) return false;
    destination << source.rdbuf();
    destination.flush();
    return destination.good();
}

bool replaceFile(const char* sourcePath, const char* destinationPath, std::string& error) {
#ifdef _WIN32
    if (!MoveFileExA(sourcePath, destinationPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Windows could not replace the shared data file (error " +
                std::to_string(GetLastError()) + ").";
        return false;
    }
#else
    if (std::rename(sourcePath, destinationPath) != 0) {
        error = "Could not replace the shared data file.";
        return false;
    }
#endif
    return true;
}

bool isFiniteNumber(const Json& value) {
    if (!value.is_number()) return false;
    const double number = value.get<double>();
    return std::isfinite(number);
}

bool isDate(const Json& value) {
    if (!value.is_string()) return false;
    const std::string date = value.get<std::string>();
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (std::size_t index = 0; index < date.size(); ++index) {
        if (index != 4 && index != 7 && (date[index] < '0' || date[index] > '9')) return false;
    }
    return true;
}

bool isDateTime(const Json& value) {
    if (!value.is_string()) return false;
    const std::string dateTime = value.get<std::string>();
    if (dateTime.size() != 16 || dateTime[10] != 'T' || dateTime[13] != ':') return false;
    if (!isDate(Json(dateTime.substr(0, 10))) || dateTime[11] < '0' || dateTime[11] > '9' ||
        dateTime[12] < '0' || dateTime[12] > '9' || dateTime[14] < '0' || dateTime[14] > '9' ||
        dateTime[15] < '0' || dateTime[15] > '9') return false;
    const int hour = std::stoi(dateTime.substr(11, 2));
    const int minute = std::stoi(dateTime.substr(14, 2));
    return hour < 24 && minute < 60;
}

bool validateState(const Json& state, std::string& error) {
    if (!state.is_object() || !state.contains("vehicles") || !state["vehicles"].is_array() ||
        !state.contains("bookings") || !state["bookings"].is_array() ||
        !state.contains("extraRevenue") || !state["extraRevenue"].is_array() ||
        !state.contains("nextId") || !state["nextId"].is_number_integer() || state["nextId"].get<long long>() < 1) {
        error = "The shared data has an invalid structure.";
        return false;
    }

    std::set<std::string> vehicleIds;
    for (const Json& vehicle : state["vehicles"]) {
        if (!vehicle.is_object() || !vehicle.contains("id") || !vehicle["id"].is_string() ||
            !vehicle.contains("model") || !vehicle["model"].is_string() ||
            !vehicle.contains("type") || !vehicle["type"].is_string() ||
            !vehicle.contains("price") || !isFiniteNumber(vehicle["price"]) || vehicle["price"].get<double>() <= 0 ||
            !vehicle.contains("age") || !isFiniteNumber(vehicle["age"]) || vehicle["age"].get<double>() < 0 ||
            !vehicle.contains("seats") || !isFiniteNumber(vehicle["seats"]) || vehicle["seats"].get<double>() < 1 ||
            !vehicle.contains("range") || !isFiniteNumber(vehicle["range"]) || vehicle["range"].get<double>() < 1) {
            error = "A vehicle record is invalid.";
            return false;
        }
        const std::string id = vehicle["id"].get<std::string>();
        const std::string model = vehicle["model"].get<std::string>();
        const std::string type = vehicle["type"].get<std::string>();
        if (id.empty() || model.empty() || (type != "Hatchback" && type != "SUV" && type != "Premium" &&
            type != "Two-Wheeler" && type != "EV") ||
            !vehicleIds.insert(id).second) {
            error = "Vehicle IDs must be unique and vehicle details must be valid.";
            return false;
        }
    }

    std::set<std::string> bookingIds;
    for (const Json& booking : state["bookings"]) {
        if (!booking.is_object() || !booking.contains("id") || !booking["id"].is_string() ||
            !booking.contains("vehicleId") || !booking["vehicleId"].is_string() ||
            !booking.contains("customer") || !booking["customer"].is_string() ||
            !booking.contains("start") || !isDate(booking["start"]) ||
            !booking.contains("end") || !isDate(booking["end"]) ||
            !booking.contains("charge") || !isFiniteNumber(booking["charge"]) || booking["charge"].get<double>() < 0 ||
            !booking.contains("deposit") || !isFiniteNumber(booking["deposit"]) || booking["deposit"].get<double>() < 0 ||
            !booking.contains("returned") || !booking["returned"].is_boolean()) {
            error = "A booking record is invalid.";
            return false;
        }
        const bool hasTimeSlot = booking.contains("startAt") || booking.contains("endAt");
        if (hasTimeSlot && (!booking.contains("startAt") || !booking.contains("endAt") ||
            !isDateTime(booking["startAt"]) || !isDateTime(booking["endAt"]) ||
            booking["startAt"].get<std::string>() >= booking["endAt"].get<std::string>())) {
            error = "An hourly booking must have a valid, positive time slot.";
            return false;
        }
        const std::string id = booking["id"].get<std::string>();
        const std::string vehicleId = booking["vehicleId"].get<std::string>();
        const std::string customer = booking["customer"].get<std::string>();
        const std::string start = booking["start"].get<std::string>();
        const std::string end = booking["end"].get<std::string>();
        const bool sameDayHourly = hasTimeSlot && booking.contains("plan") && booking["plan"] == "Hourly" && start == end;
        if (id.empty() || customer.empty() || !vehicleIds.count(vehicleId) || start > end ||
            (start == end && !sameDayHourly) ||
            !bookingIds.insert(id).second) {
            error = "Booking IDs must be unique and bookings must reference valid vehicles and dates.";
            return false;
        }
    }

    for (std::size_t index = 0; index < state["bookings"].size(); ++index) {
        const Json& booking = state["bookings"][index];
        const std::string bookingStart = booking.contains("startAt") ? booking["startAt"].get<std::string>() :
            booking["start"].get<std::string>() + "T00:00";
        const std::string bookingEnd = booking.contains("endAt") ? booking["endAt"].get<std::string>() :
            booking["end"].get<std::string>() + "T00:00";
        for (std::size_t other = index + 1; other < state["bookings"].size(); ++other) {
            const Json& comparison = state["bookings"][other];
            const std::string comparisonStart = comparison.contains("startAt") ? comparison["startAt"].get<std::string>() :
                comparison["start"].get<std::string>() + "T00:00";
            const std::string comparisonEnd = comparison.contains("endAt") ? comparison["endAt"].get<std::string>() :
                comparison["end"].get<std::string>() + "T00:00";
            if (booking["vehicleId"] == comparison["vehicleId"] && bookingStart < comparisonEnd &&
                comparisonStart < bookingEnd) {
                error = "Bookings for the same vehicle cannot overlap.";
                return false;
            }
        }
    }

    for (const Json& entry : state["extraRevenue"]) {
        if (!entry.is_object() || !entry.contains("date") || !isDate(entry["date"]) ||
            !entry.contains("amount") || !isFiniteNumber(entry["amount"]) || entry["amount"].get<double>() < 0 ||
            !entry.contains("kind") || !entry["kind"].is_string()) {
            error = "A revenue record is invalid.";
            return false;
        }
    }
    return true;
}

bool readEnvelope(const char* path, Json& envelope, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Could not open the shared data file.";
        return false;
    }
    try {
        input >> envelope;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
    if (!envelope.is_object() || !envelope.contains("revision") ||
        !envelope["revision"].is_number_integer() || envelope["revision"].get<long long>() < 1 ||
        !envelope.contains("state")) {
        error = "The shared data file has an invalid envelope.";
        return false;
    }
    if (!validateState(envelope["state"], error)) return false;
    return true;
}

class SharedStore {
    Json state_ = nullptr;
    unsigned long long revision_ = 0;
    bool preserveBackupOnNextSave_ = false;

    Json envelope() const {
        return Json{{"revision", revision_}, {"state", state_}};
    }

    bool writeEnvelope(const Json& value, std::string& error) {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            error = "Could not create a temporary shared data file.";
            return false;
        }
        output << value.dump(2) << '\n';
        output.flush();
        if (!output) {
            error = "Could not write the shared data file.";
            output.close();
            std::remove(temporaryPath);
            return false;
        }
        output.close();

        if (fileExists(dataPath) && !preserveBackupOnNextSave_ && !copyFile(dataPath, backupPath)) {
            error = "Could not back up the previous shared data file.";
            std::remove(temporaryPath);
            return false;
        }
        if (!replaceFile(temporaryPath, dataPath, error)) {
            std::remove(temporaryPath);
            return false;
        }
        preserveBackupOnNextSave_ = false;
        return true;
    }

public:
    SharedStore() {
        Json loadedEnvelope;
        if (!fileExists(dataPath) && !fileExists(backupPath)) return;
        std::string primaryError;
        if (fileExists(dataPath) && readEnvelope(dataPath, loadedEnvelope, primaryError)) {
            revision_ = loadedEnvelope["revision"].get<unsigned long long>();
            state_ = loadedEnvelope["state"];
            return;
        }
        std::string backupError;
        if (fileExists(backupPath) && readEnvelope(backupPath, loadedEnvelope, backupError)) {
            revision_ = loadedEnvelope["revision"].get<unsigned long long>();
            state_ = loadedEnvelope["state"];
            preserveBackupOnNextSave_ = true;
            std::string restoreError;
            if (!writeEnvelope(envelope(), restoreError))
                throw std::runtime_error("Recovered the backup but could not restore the main data file: " + restoreError);
            std::cerr << "Recovered shared data from roadwise_shared_data.json.bak.\n";
            return;
        }
        throw std::runtime_error("Shared data could not be loaded. " + primaryError + " " + backupError);
    }

    Json snapshot() const {
        return envelope();
    }

    bool save(unsigned long long expectedRevision, const Json& nextState, Json& latest, std::string& error) {
        if (expectedRevision != revision_) {
            latest = envelope();
            return false;
        }
        Json nextEnvelope{{"revision", revision_ + 1}, {"state", nextState}};
        if (!writeEnvelope(nextEnvelope, error)) return false;
        ++revision_;
        state_ = nextState;
        latest = envelope();
        return true;
    }
};

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
};

bool receiveRequest(SOCKET client, HttpRequest& request, std::string& error) {
    const std::size_t maximumHeaderSize = 16 * 1024;
    const std::size_t maximumBodySize = 2 * 1024 * 1024;
    std::string incoming;
    char buffer[8192];
    std::size_t headerEnd = std::string::npos;
    while (headerEnd == std::string::npos) {
        const int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            error = "Request headers were incomplete.";
            return false;
        }
        incoming.append(buffer, static_cast<std::size_t>(received));
        headerEnd = incoming.find("\r\n\r\n");
        if (headerEnd == std::string::npos && incoming.size() > maximumHeaderSize) {
            error = "Request headers are too large.";
            return false;
        }
    }

    std::istringstream headers(incoming.substr(0, headerEnd));
    std::string requestLine;
    std::getline(headers, requestLine);
    std::istringstream requestParts(requestLine);
    std::string target, httpVersion;
    if (!(requestParts >> request.method >> target >> httpVersion) || httpVersion.find("HTTP/") != 0) {
        error = "The HTTP request line is invalid.";
        return false;
    }
    const std::size_t queryStart = target.find('?');
    request.path = target.substr(0, queryStart);

    std::size_t contentLength = 0;
    std::string header;
    while (std::getline(headers, header)) {
        if (!header.empty() && header.back() == '\r') header.pop_back();
        const std::size_t separator = header.find(':');
        if (separator == std::string::npos) continue;
        std::string name = header.substr(0, separator);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        std::string value = header.substr(separator + 1);
        const std::size_t first = value.find_first_not_of(" \t");
        value = first == std::string::npos ? "" : value.substr(first);
        if (name == "content-length") {
            char* end = nullptr;
            const unsigned long long parsedLength = std::strtoull(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0' || parsedLength > maximumBodySize) {
                error = "The request body is too large or has an invalid length.";
                return false;
            }
            contentLength = static_cast<std::size_t>(parsedLength);
        } else if (name == "transfer-encoding" && value.find("chunked") != std::string::npos) {
            error = "Chunked request bodies are not supported.";
            return false;
        }
    }

    const std::size_t bodyStart = headerEnd + 4;
    while (incoming.size() - bodyStart < contentLength) {
        const int received = recv(client, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            error = "The request body was incomplete.";
            return false;
        }
        incoming.append(buffer, static_cast<std::size_t>(received));
    }
    request.body.assign(incoming, bodyStart, contentLength);
    return true;
}

void sendResponse(SOCKET client, int status, const std::string& contentType, const std::string& body) {
    const char* statusText = status == 200 ? "OK" : status == 400 ? "Bad Request" :
        status == 404 ? "Not Found" : status == 405 ? "Method Not Allowed" :
        status == 409 ? "Conflict" : "Internal Server Error";
    std::ostringstream header;
    header << "HTTP/1.1 " << status << ' ' << statusText << "\r\n"
           << "Content-Type: " << contentType << "\r\n"
           << "Content-Length: " << body.size() << "\r\n"
           << "Cache-Control: no-store\r\n"
           << "X-Content-Type-Options: nosniff\r\n"
           << "Connection: close\r\n\r\n";
    const std::string response = header.str() + body;
    std::size_t sent = 0;
    while (sent < response.size()) {
        const int count = send(client, response.data() + sent,
                               static_cast<int>(response.size() - sent), 0);
        if (count <= 0) return;
        sent += static_cast<std::size_t>(count);
    }
}

void sendJsonResponse(SOCKET client, int status, const Json& body) {
    sendResponse(client, status, "application/json; charset=utf-8", body.dump());
}

bool serveStaticFile(SOCKET client, const std::string& relativePath) {
    std::string safePath = relativePath;
    if (safePath == "/") safePath = "/index.html";
    if (safePath.size() > 1 && safePath[0] == '/') safePath = safePath.substr(1);
    if (safePath.find("..") != std::string::npos) {
        sendResponse(client, 404, "text/plain; charset=utf-8", "Requested file is not allowed.");
        return false;
    }
    const std::string lower = safePath;
    const bool allowed = lower.size() > 4 && (
        lower.substr(lower.size() - 5) == ".html" ||
        lower.substr(lower.size() - 3) == ".js" ||
        lower.substr(lower.size() - 4) == ".css" ||
        lower.substr(lower.size() - 5) == ".json" ||
        lower.substr(lower.size() - 4) == ".png" ||
        lower.substr(lower.size() - 4) == ".jpg" ||
        lower.substr(lower.size() - 5) == ".jpeg" ||
        lower.substr(lower.size() - 4) == ".gif" ||
        lower.substr(lower.size() - 5) == ".webp");
    if (!allowed) {
        sendResponse(client, 404, "text/plain; charset=utf-8", "Requested file type is not allowed.");
        return false;
    }

    std::ifstream input(safePath, std::ios::binary);
    if (!input) {
        sendResponse(client, 404, "text/plain; charset=utf-8",
                     (safePath + " was not found in the server working directory."));
        return false;
    }

    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::string contentType = "application/octet-stream";
    if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".html") contentType = "text/html; charset=utf-8";
    else if (lower.size() >= 3 && lower.substr(lower.size() - 3) == ".js") contentType = "application/javascript; charset=utf-8";
    else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".css") contentType = "text/css; charset=utf-8";
    else if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".json") contentType = "application/json; charset=utf-8";
    else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".png") contentType = "image/png";
    else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".jpg") contentType = "image/jpeg";
    else if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".jpeg") contentType = "image/jpeg";
    else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == ".gif") contentType = "image/gif";
    else if (lower.size() >= 5 && lower.substr(lower.size() - 5) == ".webp") contentType = "image/webp";

    sendResponse(client, 200, contentType, contents);
    return true;
}

void handleRequest(SOCKET client, SharedStore& store) {
    HttpRequest request;
    std::string requestError;
    if (!receiveRequest(client, request, requestError)) {
        sendJsonResponse(client, 400, Json{{"error", requestError}});
        return;
    }
    if (request.method == "GET" && (request.path == "/" || request.path == "/index.html" || request.path == "/recommendations.js" || request.path == "/roadwise_shared_data.json")) {
        serveStaticFile(client, request.path);
        return;
    }
    if (request.method == "GET" && request.path == "/api/state") {
        sendJsonResponse(client, 200, store.snapshot());
        return;
    }
    if (request.method == "GET" && request.path == "/api/health") {
        sendJsonResponse(client, 200, Json{{"status", "ok"}});
        return;
    }
    if (request.method == "PUT" && request.path == "/api/state") {
        Json payload;
        try {
            payload = Json::parse(request.body);
        } catch (const std::exception& exception) {
            sendJsonResponse(client, 400, Json{{"error", std::string("Invalid JSON: ") + exception.what()}});
            return;
        }
        if (!payload.is_object() || !payload.contains("revision") || !payload["revision"].is_number_integer() ||
            payload["revision"].get<long long>() < 0 || !payload.contains("state")) {
            sendJsonResponse(client, 400, Json{{"error", "Expected a revision and state."}});
            return;
        }
        std::string validationError;
        if (!validateState(payload["state"], validationError)) {
            sendJsonResponse(client, 400, Json{{"error", validationError}});
            return;
        }
        Json latest;
        std::string saveError;
        const unsigned long long expected = payload["revision"].get<unsigned long long>();
        if (!store.save(expected, payload["state"], latest, saveError)) {
            if (saveError.empty()) sendJsonResponse(client, 409, latest);
            else sendJsonResponse(client, 500, Json{{"error", saveError}});
            return;
        }
        sendJsonResponse(client, 200, latest);
        return;
    }
    if (request.method == "GET") {
        serveStaticFile(client, request.path);
        return;
    }
    const int status = request.method == "PUT" ? 404 : 405;
    sendJsonResponse(client, status, Json{{"error", status == 404 ? "Route not found." : "Method not allowed."}});
}
}

int main() {
    try {
        SharedStore store;
        WSADATA winsockData;
        if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0)
            throw std::runtime_error("Windows networking could not be initialized.");
        SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Could not create the web server socket.");
        }
        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(8080);
        if (bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
            listen(listener, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(listener);
            WSACleanup();
            throw std::runtime_error("Could not listen on port 8080. The port may already be in use.");
        }

        std::cout << "Shared server listening on port 8080.\n"
                  << "On this computer: http://127.0.0.1:8080/\n"
                  << "On other devices, open http://<this-computer-IPv4>:8080/\n"
                  << "Use ipconfig to find this computer's IPv4 address. Keep this server running.\n"
                  << "LAN demo only: use a trusted private network, not public Wi-Fi or real customer data.\n";
        while (true) {
            SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET) continue;
            DWORD timeout = 5000;
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
            setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
            handleRequest(client, store);
            shutdown(client, SD_BOTH);
            closesocket(client);
        }
    } catch (const std::exception& exception) {
        std::cerr << "Shared server stopped: " << exception.what() << '\n';
        return 1;
    }
}
