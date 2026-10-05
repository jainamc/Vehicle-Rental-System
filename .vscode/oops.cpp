#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

struct Date {
    int year;
    unsigned month;
    unsigned day;
    int serial;
};

int daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const int adjustedMonth = static_cast<int>(month) + (month > 2 ? -3 : 9);
    const unsigned dayOfYear = (153 * adjustedMonth + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + static_cast<int>(dayOfEra) - 719468;
}

Date dateFromSerial(int serial) {
    const int shifted = serial + 719468;
    const int era = (shifted >= 0 ? shifted : shifted - 146096) / 146097;
    const unsigned dayOfEra = static_cast<unsigned>(shifted - era * 146097);
    const unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    int year = static_cast<int>(yearOfEra) + era * 400;
    const unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned monthPrime = (5 * dayOfYear + 2) / 153;
    const unsigned day = dayOfYear - (153 * monthPrime + 2) / 5 + 1;
    const int month = static_cast<int>(monthPrime) + (monthPrime < 10 ? 3 : -9);
    year += month <= 2;
    return {year, static_cast<unsigned>(month), day, serial};
}

bool parseDate(const std::string& input, Date& result) {
    if (input.size() != 10 || input[4] != '-' || input[7] != '-') return false;
    for (std::size_t index = 0; index < input.size(); ++index) {
        if (index != 4 && index != 7 && !std::isdigit(static_cast<unsigned char>(input[index]))) return false;
    }
    try {
        const int year = std::stoi(input.substr(0, 4));
        const int month = std::stoi(input.substr(5, 2));
        const int day = std::stoi(input.substr(8, 2));
        if (year < 1900 || month < 1 || month > 12 || day < 1) return false;
        static const int monthLengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int maxDay = monthLengths[month - 1];
        const bool leapYear = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        if (month == 2 && leapYear) ++maxDay;
        if (day > maxDay) return false;
        result = {year, static_cast<unsigned>(month), static_cast<unsigned>(day), daysFromCivil(year, month, day)};
        return true;
    } catch (...) {
        return false;
    }
}

std::string dateText(int serial) {
    const Date date = dateFromSerial(serial);
    std::ostringstream output;
    output << std::setfill('0') << std::setw(4) << date.year << '-'
           << std::setw(2) << date.month << '-' << std::setw(2) << date.day;
    return output.str();
}

int todaySerial() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (!local) return daysFromCivil(2026, 1, 1);
    return daysFromCivil(local->tm_year + 1900, static_cast<unsigned>(local->tm_mon + 1),
                         static_cast<unsigned>(local->tm_mday));
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

int readInt(const std::string& prompt) {
    while (true) {
        const std::string input = readLine(prompt);
        try {
            std::size_t used = 0;
            const int value = std::stoi(input, &used);
            if (used == input.size()) return value;
        } catch (...) {
        }
        std::cout << "Enter a whole number.\n";
    }
}

double readDouble(const std::string& prompt) {
    while (true) {
    
    
        const std::string input = readLine(prompt);
        try {
            std::size_t used = 0;
            const double value = std::stod(input, &used);
            if (used == input.size() && std::isfinite(value)) return value;
        } catch (...) {
        }
        std::cout << "Enter a valid number.\n";
    }
}

int readDate(const std::string& prompt) {
    while (true) {
        Date date;
        if (parseDate(readLine(prompt), date)) return date.serial;
        std::cout << "Use a valid date in YYYY-MM-DD format.\n";
    }
}

std::string clippedText(const std::string& value, std::size_t width) {
    if (value.size() <= width) return value;
    if (width < 4) return value.substr(0, width);
    return value.substr(0, width - 3) + "...";
}

std::string moneyText(double amount) {
    std::ostringstream output;
    output << '$' << std::fixed << std::setprecision(2) << amount;
    return output.str();
}

void printSectionHeader(const std::string& title, const std::string& subtitle = "") {
    const std::string rule(72, '=');
    std::cout << '\n' << '+' << rule << "+\n"
              << "| " << std::left << std::setw(70) << title << " |\n";
    if (!subtitle.empty()) std::cout << "| " << std::left << std::setw(70) << subtitle << " |\n";
    std::cout << '+' << std::string(72, '-') << "+\n";
}

void printField(const std::string& label, const std::string& value) {
    std::cout << "  " << std::left << std::setw(22) << label << value << '\n';
}

class Vehicle {
protected:
    std::string id;
    std::string model;
    double pricePerDay;
    int ageYears;

public:
    Vehicle(std::string vehicleId, std::string vehicleModel, double price, int age)
        : id(std::move(vehicleId)), model(std::move(vehicleModel)), pricePerDay(price), ageYears(age) {}
    virtual ~Vehicle() = default;
    const std::string& getId() const { return id; }
    const std::string& getModel() const { return model; }
    double getPricePerDay() const { return pricePerDay; }
    int getAgeYears() const { return ageYears; }
    virtual std::string typeName() const = 0;
    virtual int passengerCapacity() const = 0;
    virtual int rangeKm() const = 0;
    virtual double categoryMultiplier() const { return 1.0; }
    virtual double rentalFee() const { return 0.0; }
    virtual double deposit() const { return 0.0; }
    virtual std::string specialRule() const { return "No special fee"; }

    virtual void displayDetails() const {
        std::cout << '[' << typeName() << "] " << id << " | " << model
                  << " | $" << std::fixed << std::setprecision(2) << pricePerDay << "/day"
                  << " | age " << ageYears << "y | seats " << passengerCapacity()
                  << " | range " << rangeKm() << "km | " << specialRule() << '\n';
    }
};

class Car : public Vehicle {
    int seats;
public:
    Car(std::string id, std::string model, double price, int age, int seatCount)
        : Vehicle(std::move(id), std::move(model), price, age), seats(seatCount) {}
    std::string typeName() const override { return "Car"; }
    int passengerCapacity() const override { return seats; }
    int rangeKm() const override { return 800; }
};

class Bike : public Vehicle {
    int engineCc;
public:
    Bike(std::string id, std::string model, double price, int age, int cc)
        : Vehicle(std::move(id), std::move(model), price, age), engineCc(cc) {}
    std::string typeName() const override { return "Bike"; }
    int passengerCapacity() const override { return 2; }
    int rangeKm() const override { return 450; }
    int getEngineCc() const { return engineCc; }
    double deposit() const override { return 100.0; }
    std::string specialRule() const override { return "Helmet deposit $100"; }
    void displayDetails() const override {
        Vehicle::displayDetails();
        std::cout << "    Engine: " << engineCc << "cc\n";
    }
};

class EV : public Vehicle {
    int seats;
    int range;
public:
    EV(std::string id, std::string model, double price, int age, int seatCount, int vehicleRange)
        : Vehicle(std::move(id), std::move(model), price, age), seats(seatCount), range(vehicleRange) {}
    std::string typeName() const override { return "EV"; }
    int passengerCapacity() const override { return seats; }
    int rangeKm() const override { return range; }
    double rentalFee() const override { return 25.0; }
    std::string specialRule() const override { return "$25 charging fee per rental"; }
};

class Luxury : public Vehicle {
    int seats;
public:
    Luxury(std::string id, std::string model, double price, int age, int seatCount)
        : Vehicle(std::move(id), std::move(model), price, age), seats(seatCount) {}
    std::string typeName() const override { return "Luxury"; }
    int passengerCapacity() const override { return seats; }
    int rangeKm() const override { return 700; }
    double categoryMultiplier() const override { return 1.35; }
    std::string specialRule() const override { return "Premium class pricing"; }
};

class PricingStrategy {
public:
    virtual ~PricingStrategy() = default;
    virtual double quote(const Vehicle& vehicle, int startDay, int rentalDays, double demand) const = 0;
};

class DynamicPricingStrategy : public PricingStrategy {
public:
    double quote(const Vehicle& vehicle, int startDay, int rentalDays, double demand) const override {
        const double ageFactor = std::max(0.70, 1.0 - vehicle.getAgeYears() * 0.02);
        double total = 0.0;
        for (int offset = 0; offset < rentalDays; ++offset) {
            const int serial = startDay + offset;
            const Date date = dateFromSerial(serial);
            const int weekday = (serial + 4) % 7;
            const bool weekend = weekday == 0 || weekday == 6;
            const bool peakSeason = date.month == 6 || date.month == 7 || date.month == 8 || date.month == 12;
            double daily = vehicle.getPricePerDay() * vehicle.categoryMultiplier() * ageFactor;
            daily *= 1.0 + std::min(0.50, demand * 0.50);
            if (weekend) daily *= 1.20;
            if (peakSeason) daily *= 1.25;
            total += daily;
        }
        if (rentalDays >= 30) total *= 0.80;
        else if (rentalDays >= 7) total *= 0.90;
        return total;
    }
};

struct Booking {
    std::string id;
    std::string vehicleId;
    std::string customer;
    int startDay;
    int endDay;
    int days;
    double charge;
    double deposit;
    bool returned = false;
};

struct RevenueEntry {
    int date;
    double amount;
};

enum class DataLoadResult { Loaded, Missing, Error };

class RentalSystem {
    std::vector<std::unique_ptr<Vehicle>> fleet;
    std::vector<Booking> bookings;
    std::vector<RevenueEntry> revenueEntries;
    std::map<std::string, int> completedRentals;
    std::unique_ptr<PricingStrategy> pricing = std::make_unique<DynamicPricingStrategy>();
    int nextBookingNumber = 1;
    const std::string dataPath = "rental_data.txt";
    const std::string backupPath = "rental_data.txt.bak";
    bool preserveBackupOnNextSave = false;

    static bool fileExists(const std::string& path) {
#ifdef _WIN32
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
        std::ifstream input(path.c_str(), std::ios::binary);
        return input.good();
#endif
    }

    static bool copyFile(const std::string& sourcePath, const std::string& destinationPath) {
        std::ifstream source(sourcePath.c_str(), std::ios::binary);
        std::ofstream destination(destinationPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!source || !destination) return false;
        destination << source.rdbuf();
        destination.flush();
        return source.good() && destination.good();
    }

    bool writeDataFile(std::string& error) {
        const std::string temporaryPath = dataPath + ".tmp";
        std::ofstream output(temporaryPath.c_str(), std::ios::trunc);
        if (!output) {
            error = "Could not open the temporary data file for writing.";
            return false;
        }

        output << std::setprecision(17) << "ROADWISE 1\n" << "NEXT " << nextBookingNumber << '\n';
        for (const auto& vehicle : fleet) {
            int engineCc = 0;
            if (const auto* bike = dynamic_cast<const Bike*>(vehicle.get())) engineCc = bike->getEngineCc();
            output << "VEHICLE " << std::quoted(vehicle->typeName()) << ' '
                   << std::quoted(vehicle->getId()) << ' ' << std::quoted(vehicle->getModel()) << ' '
                   << vehicle->getPricePerDay() << ' ' << vehicle->getAgeYears() << ' '
                   << vehicle->passengerCapacity() << ' ' << vehicle->rangeKm() << ' ' << engineCc << '\n';
        }
        for (const auto& booking : bookings) {
            output << "BOOKING " << std::quoted(booking.id) << ' ' << std::quoted(booking.vehicleId) << ' '
                   << std::quoted(booking.customer) << ' ' << booking.startDay << ' ' << booking.endDay << ' '
                   << booking.days << ' ' << booking.charge << ' ' << booking.deposit << ' '
                   << (booking.returned ? 1 : 0) << '\n';
        }
        for (const auto& entry : revenueEntries) output << "REVENUE " << entry.date << ' ' << entry.amount << '\n';
        output.flush();
        if (!output) {
            error = "Could not write all rental data to the temporary file.";
            output.close();
            std::remove(temporaryPath.c_str());
            return false;
        }
        output.close();

        if (fileExists(dataPath) && !preserveBackupOnNextSave) {
            if (!copyFile(dataPath, backupPath)) {
                error = "Could not create a backup of the existing rental data.";
                std::remove(temporaryPath.c_str());
                return false;
            }
        }

#ifdef _WIN32
        if (!MoveFileExA(temporaryPath.c_str(), dataPath.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            error = "Could not safely replace the rental data file (Windows error " +
                    std::to_string(GetLastError()) + ").";
            std::remove(temporaryPath.c_str());
            return false;
        }
#else
        if (std::rename(temporaryPath.c_str(), dataPath.c_str()) != 0) {
            error = "Could not safely replace the rental data file.";
            std::remove(temporaryPath.c_str());
            return false;
        }
#endif
    preserveBackupOnNextSave = false;
        return true;
    }

    bool readDataFile(const std::string& path, std::string& error) {
        std::ifstream input(path.c_str());
        if (!input) {
            error = "Could not open " + path + ".";
            return false;
        }

        std::string line;
        if (!std::getline(input, line) || line != "ROADWISE 1") {
            error = "The data file has an unsupported or invalid header.";
            return false;
        }
        if (!std::getline(input, line)) {
            error = "The data file is incomplete.";
            return false;
        }
        std::istringstream nextRecord(line);
        std::string tag;
        int loadedNextBookingNumber = 0;
        nextRecord >> tag >> loadedNextBookingNumber;
        nextRecord >> std::ws;
        if (tag != "NEXT" || loadedNextBookingNumber < 1 || !nextRecord.eof()) {
            error = "The next-booking record is invalid.";
            return false;
        }

        std::vector<std::unique_ptr<Vehicle>> loadedFleet;
        std::vector<Booking> loadedBookings;
        std::vector<RevenueEntry> loadedRevenue;
        std::map<std::string, int> loadedCompletedRentals;
        std::map<std::string, bool> vehicleIds;
        std::map<std::string, bool> bookingIds;
        std::size_t lineNumber = 2;
        while (std::getline(input, line)) {
            ++lineNumber;
            if (line.empty()) continue;
            std::istringstream record(line);
            record >> tag;
            bool valid = false;
            if (tag == "VEHICLE") {
                std::string type, id, model;
                double price = 0.0;
                int age = 0, capacity = 0, range = 0, engineCc = 0;
                record >> std::quoted(type) >> std::quoted(id) >> std::quoted(model)
                       >> price >> age >> capacity >> range >> engineCc;
                record >> std::ws;
                if (!record.fail() && record.eof() && !id.empty() && !model.empty() &&
                    std::isfinite(price) && price > 0.0 && age >= 0 && capacity > 0 &&
                    !vehicleIds.count(id)) {
                    if (type == "Car" && range == 800 && engineCc == 0)
                        loadedFleet.push_back(std::make_unique<Car>(id, model, price, age, capacity));
                    else if (type == "Bike" && range == 450 && capacity == 2 && engineCc > 0)
                        loadedFleet.push_back(std::make_unique<Bike>(id, model, price, age, engineCc));
                    else if (type == "EV" && range > 0 && engineCc == 0)
                        loadedFleet.push_back(std::make_unique<EV>(id, model, price, age, capacity, range));
                    else if (type == "Luxury" && range == 700 && engineCc == 0)
                        loadedFleet.push_back(std::make_unique<Luxury>(id, model, price, age, capacity));
                    valid = !loadedFleet.empty() && loadedFleet.back()->getId() == id;
                    if (valid) vehicleIds[id] = true;
                }
            } else if (tag == "BOOKING") {
                Booking booking{};
                int returned = 0;
                record >> std::quoted(booking.id) >> std::quoted(booking.vehicleId) >> std::quoted(booking.customer)
                       >> booking.startDay >> booking.endDay >> booking.days >> booking.charge >> booking.deposit >> returned;
                record >> std::ws;
                if (!record.fail() && record.eof() && !booking.id.empty() && !booking.customer.empty() &&
                    booking.startDay >= daysFromCivil(1900, 1, 1) && booking.endDay > booking.startDay &&
                    booking.days > 0 && booking.endDay - booking.startDay == booking.days &&
                    std::isfinite(booking.charge) && booking.charge >= 0.0 &&
                    std::isfinite(booking.deposit) && booking.deposit >= 0.0 &&
                    (returned == 0 || returned == 1) && !bookingIds.count(booking.id)) {
                    booking.returned = returned == 1;
                    loadedBookings.push_back(booking);
                    bookingIds[booking.id] = true;
                    if (booking.returned) ++loadedCompletedRentals[booking.customer];
                    valid = true;
                }
            } else if (tag == "REVENUE") {
                RevenueEntry entry{};
                record >> entry.date >> entry.amount;
                record >> std::ws;
                if (!record.fail() && record.eof() && std::isfinite(entry.amount) && entry.amount >= 0.0) {
                    loadedRevenue.push_back(entry);
                    valid = true;
                }
            }
            if (!valid) {
                error = "Invalid record on line " + std::to_string(lineNumber) + ".";
                return false;
            }
        }
        if (input.bad()) {
            error = "An error occurred while reading the data file.";
            return false;
        }
        for (std::size_t index = 0; index < loadedBookings.size(); ++index) {
            const Booking& booking = loadedBookings[index];
            if (!vehicleIds.count(booking.vehicleId)) {
                error = "Booking " + booking.id + " refers to a vehicle that is not in the fleet.";
                return false;
            }
            for (std::size_t other = index + 1; other < loadedBookings.size(); ++other) {
                const Booking& comparison = loadedBookings[other];
                if (booking.vehicleId == comparison.vehicleId && booking.startDay < comparison.endDay &&
                    comparison.startDay < booking.endDay) {
                    error = "The data file contains overlapping bookings for " + booking.vehicleId + ".";
                    return false;
                }
            }
        }

        fleet = std::move(loadedFleet);
        bookings = std::move(loadedBookings);
        revenueEntries = std::move(loadedRevenue);
        completedRentals = std::move(loadedCompletedRentals);
        nextBookingNumber = loadedNextBookingNumber;
        return true;
    }

    Vehicle* findVehicle(const std::string& id) const {
        for (const auto& vehicle : fleet) if (vehicle->getId() == id) return vehicle.get();
        return nullptr;
    }

    Booking* findBooking(const std::string& id) {
        for (auto& booking : bookings) if (booking.id == id) return &booking;
        return nullptr;
    }

    bool overlaps(const std::string& vehicleId, int start, int end) const {
        for (const auto& booking : bookings) {
            if (!booking.returned && booking.vehicleId == vehicleId &&
                start < booking.endDay && booking.startDay < end) {
                return true;
            }
        }
        return false;
    }

    double peakDemand(int start, int end) const {
        if (fleet.empty()) return 0.0;
        double peak = 0.0;
        for (int day = start; day < end; ++day) {
            int reserved = 0;
            for (const auto& booking : bookings) {
                if (!booking.returned && booking.startDay <= day && day < booking.endDay) ++reserved;
            }
            peak = std::max(peak, static_cast<double>(reserved) / fleet.size());
        }
        return peak;
    }

    int customerRentals(const std::string& customer) const {
        const auto found = completedRentals.find(customer);
        return found == completedRentals.end() ? 0 : found->second;
    }

    static std::string monthKey(int serial) {
        const Date date = dateFromSerial(serial);
        std::ostringstream output;
        output << std::setfill('0') << std::setw(4) << date.year << '-' << std::setw(2) << date.month;
        return output.str();
    }

    static void printBar(const std::string& label, double value, double scale) {
        const int bars = std::max(0, std::min(40, static_cast<int>(value * scale)));
        std::cout << std::left << std::setw(14) << label << " | " << std::string(bars, '#')
                  << " " << std::fixed << std::setprecision(2) << value << '\n';
    }

public:
    bool saveData(std::string& error) { return writeDataFile(error); }

    DataLoadResult loadData(std::string& error) {
        if (fileExists(dataPath)) {
            if (readDataFile(dataPath, error)) return DataLoadResult::Loaded;
            const std::string primaryError = error;
            if (fileExists(backupPath)) {
                std::string backupError;
                if (readDataFile(backupPath, backupError)) {
                    preserveBackupOnNextSave = true;
                    if (writeDataFile(error)) {
                        std::cout << "Warning: recovered rental data from the backup file.\n";
                        return DataLoadResult::Loaded;
                    }
                    error = "Recovered the backup data, but could not restore the main file: " + error;
                    return DataLoadResult::Error;
                }
                error = primaryError + " Backup file is also invalid: " + backupError;
            }
            return DataLoadResult::Error;
        }
        if (fileExists(backupPath)) {
            if (!readDataFile(backupPath, error)) return DataLoadResult::Error;
            preserveBackupOnNextSave = true;
            if (!writeDataFile(error)) {
                error = "Recovered the backup data, but could not restore the main file: " + error;
                return DataLoadResult::Error;
            }
            std::cout << "Warning: recovered rental data from the backup file.\n";
            return DataLoadResult::Loaded;
        }
        return DataLoadResult::Missing;
    }

    bool addVehicle(std::unique_ptr<Vehicle> vehicle) {
        if (!vehicle || findVehicle(vehicle->getId())) return false;
        fleet.push_back(std::move(vehicle));
        return true;
    }

    bool addVehicleAndSave(std::unique_ptr<Vehicle> vehicle, std::string& error) {
        if (!vehicle) {
            error = "Vehicle details are invalid.";
            return false;
        }
        if (findVehicle(vehicle->getId())) {
            error = "That vehicle ID already exists.";
            return false;
        }
        fleet.push_back(std::move(vehicle));
        if (!writeDataFile(error)) {
            fleet.pop_back();
            return false;
        }
        return true;
    }

    void displayFleet() const {
        printSectionHeader("FLEET INVENTORY", std::to_string(fleet.size()) + " vehicle(s) registered");
        std::cout << std::left << std::setw(8) << "ID" << std::setw(10) << "CLASS"
                  << std::setw(23) << "VEHICLE" << std::right << std::setw(11) << "RATE/DAY"
                  << std::setw(7) << "SEATS" << std::setw(9) << "RANGE" << std::setw(5) << "AGE" << '\n'
                  << std::string(73, '-') << '\n';
        if (fleet.empty()) std::cout << "No vehicles are registered yet.\n";
        for (const auto& vehicle : fleet) {
            std::cout << std::left << std::setw(8) << clippedText(vehicle->getId(), 7)
                      << std::setw(10) << vehicle->typeName()
                      << std::setw(23) << clippedText(vehicle->getModel(), 22)
                      << std::right << std::setw(11) << moneyText(vehicle->getPricePerDay())
                      << std::setw(7) << vehicle->passengerCapacity()
                      << std::setw(7) << vehicle->rangeKm() << " km"
                      << std::setw(4) << vehicle->getAgeYears() << "y\n";
            if (const auto* bike = dynamic_cast<const Bike*>(vehicle.get()))
                std::cout << "        " << bike->getEngineCc() << "cc engine  |  " << bike->specialRule() << '\n';
            else if (vehicle->typeName() != "Car")
                std::cout << "        " << vehicle->specialRule() << '\n';
        }
        std::cout << std::string(73, '-') << "\n  Data is saved automatically after changes.\n";
    }

    void showAvailability(int start, int days) const {
        if (days <= 0 || days > std::numeric_limits<int>::max() - start) {
            std::cout << "Number of days is outside the supported range.\n";
            return;
        }
        const int end = start + days;
        printSectionHeader("AVAILABILITY CHECK", dateText(start) + " through " + dateText(end) +
                           "  |  return date excluded");
        std::cout << std::left << std::setw(14) << "STATUS" << std::setw(10) << "ID" << "VEHICLE\n"
                  << std::string(72, '-') << '\n';
        for (const auto& vehicle : fleet) {
            std::cout << std::left << std::setw(14)
                      << (overlaps(vehicle->getId(), start, end) ? "BOOKED" : "AVAILABLE")
                      << std::setw(10) << vehicle->getId() << vehicle->getModel() << '\n';
        }
    }

    bool bookVehicle(const std::string& vehicleId, const std::string& customer, int start, int days) {
        Vehicle* vehicle = findVehicle(vehicleId);
        if (!vehicle) {
            std::cout << "Error: vehicle ID not found.\n";
            return false;
        }
        if (customer.empty() || days <= 0) {
            std::cout << "Customer name and a positive rental length are required.\n";
            return false;
        }
        if (days > std::numeric_limits<int>::max() - start) {
            std::cout << "Error: rental length is too large for the selected date.\n";
            return false;
        }
        const int end = start + days;
        if (overlaps(vehicleId, start, end)) {
            std::cout << "Error: this vehicle already has a booking that overlaps those dates.\n";
            return false;
        }

        const int pastRentals = customerRentals(customer);
        const double discount = pastRentals >= 10 ? 0.15 : pastRentals >= 5 ? 0.10 : pastRentals >= 2 ? 0.05 : 0.0;
        const double quote = pricing->quote(*vehicle, start, days, peakDemand(start, end));
        const double charge = quote * (1.0 - discount) + vehicle->rentalFee();
        const double deposit = vehicle->deposit();
        std::ostringstream bookingId;
        bookingId << 'R' << std::setfill('0') << std::setw(4) << nextBookingNumber;
        bookings.push_back({bookingId.str(), vehicleId, customer, start, end, days, charge, deposit, false});
        revenueEntries.push_back({start, charge});
        ++nextBookingNumber;
        std::string saveError;
        if (!writeDataFile(saveError)) {
            bookings.pop_back();
            revenueEntries.pop_back();
            --nextBookingNumber;
            std::cout << "Booking was not saved: " << saveError << '\n';
            return false;
        }

        const std::string tier = pastRentals >= 10 ? "Platinum" : pastRentals >= 5 ? "Gold" : pastRentals >= 2 ? "Silver" : "Standard";
        printSectionHeader("RESERVATION CONFIRMED", bookingId.str() + "  |  " + customer);
        printField("Vehicle", vehicle->getModel() + " (" + vehicle->getId() + ")");
        printField("Rental dates", dateText(start) + " to " + dateText(end));
        printField("Length", std::to_string(days) + " day(s), return day excluded");
        printField("Customer tier", tier + "  /  " + std::to_string(static_cast<int>(discount * 100)) + "% discount");
        printField("Rental charge", moneyText(charge));
        printField("Refundable deposit", moneyText(deposit));
        std::cout << std::string(72, '-') << '\n';
        printField("TOTAL DUE", moneyText(charge + deposit));
        std::cout << "  Saved to rental_data.txt\n";
        return true;
    }

    void returnVehicle(const std::string& bookingId, int returnDay, bool damaged,
                       const std::string& description, double damageCost) {
        Booking* booking = findBooking(bookingId);
        if (!booking) {
            std::cout << "Error: booking ID not found.\n";
            return;
        }
        if (booking->returned) {
            std::cout << "Error: this booking has already been returned.\n";
            return;
        }
        if (returnDay < booking->startDay) {
            std::cout << "Error: return date cannot be before rental start.\n";
            return;
        }
        const int lateDays = std::max(0, returnDay - booking->endDay);
        const double latePenalty = lateDays * (booking->charge / booking->days) * 1.5;
        const double assessedDamage = damaged ? std::max(0.0, damageCost) : 0.0;
        const Booking previousBooking = *booking;
        const std::size_t previousRevenueCount = revenueEntries.size();
        booking->returned = true;
        ++completedRentals[booking->customer];
        if (latePenalty > 0.0) revenueEntries.push_back({returnDay, latePenalty});
        if (assessedDamage > 0.0) revenueEntries.push_back({returnDay, assessedDamage});
        std::string saveError;
        if (!writeDataFile(saveError)) {
            *booking = previousBooking;
            revenueEntries.resize(previousRevenueCount);
            auto customerCount = completedRentals.find(booking->customer);
            if (customerCount != completedRentals.end() && --customerCount->second == 0)
                completedRentals.erase(customerCount);
            std::cout << "Return was not saved: " << saveError << '\n';
            return;
        }
        printSectionHeader("RETURN COMPLETE", booking->id + "  |  " + booking->customer);
        printField("Late days", std::to_string(lateDays));
        printField("Late fee", moneyText(latePenalty));
        printField("Damage report", damaged ? (description.empty() ? "No description supplied" : description) : "None");
        printField("Repair assessment", moneyText(assessedDamage));
        printField("Deposit returned", moneyText(std::max(0.0, booking->deposit - assessedDamage)));
        printField("Additional due", moneyText(latePenalty + std::max(0.0, assessedDamage - booking->deposit)));
        std::cout << "  Return saved to rental_data.txt\n";
    }

    void recommend(double budget, int distance, int passengers) const {
        const int start = todaySerial();
        const int days = std::max(1, static_cast<int>(std::ceil(distance / 250.0)));
        std::vector<std::pair<double, const Vehicle*>> candidates;
        for (const auto& vehicle : fleet) {
            const int end = start + days;
            if (vehicle->passengerCapacity() < passengers || vehicle->rangeKm() < distance ||
                overlaps(vehicle->getId(), start, end)) continue;
            const double quote = pricing->quote(*vehicle, start, days, peakDemand(start, end)) + vehicle->rentalFee();
            if (quote <= budget) candidates.push_back({quote, vehicle.get()});
        }
        std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
            return left.first < right.first;
        });
        printSectionHeader("TRIP RECOMMENDATIONS", std::to_string(distance) + " km  |  " +
                           std::to_string(passengers) + " passenger(s)  |  " + std::to_string(days) + " day(s)");
        if (candidates.empty()) {
            std::cout << "No available vehicle meets the capacity, range, and budget requirements.\n";
            return;
        }
        for (std::size_t index = 0; index < std::min<std::size_t>(3, candidates.size()); ++index) {
            std::cout << "  " << index + 1 << "  " << std::left << std::setw(9)
                      << candidates[index].second->getId() << std::setw(27)
                      << clippedText(candidates[index].second->getModel(), 26)
                      << "Estimate  " << moneyText(candidates[index].first) << '\n';
        }
    }

    void adminDashboard() const {
        std::map<std::string, double> monthlyRevenue;
        for (const auto& entry : revenueEntries) monthlyRevenue[monthKey(entry.date)] += entry.amount;
        std::map<std::string, int> rentalCounts;
        int active = 0;
        int overdue = 0;
        const int today = todaySerial();
        for (const auto& booking : bookings) {
            ++rentalCounts[booking.vehicleId];
            if (!booking.returned && booking.startDay <= today) {
                if (today >= booking.endDay) ++overdue;
                else ++active;
            }
        }
        printSectionHeader("FLEET PERFORMANCE", dateText(today) + "  |  operating summary");
        printField("Fleet size", std::to_string(fleet.size()) + " vehicle(s)");
        printField("Rentals in progress", std::to_string(active));
        printField("Overdue returns", std::to_string(overdue));
        std::cout << "\nRevenue by month\n";
        if (monthlyRevenue.empty()) std::cout << "No revenue recorded yet.\n";
        for (const auto& month : monthlyRevenue) printBar(month.first, month.second, 0.10);
        const double utilization = fleet.empty() ? 0.0 : 100.0 * (active + overdue) / fleet.size();
        std::cout << "\nFleet utilization today: " << std::fixed << std::setprecision(1) << utilization << "%\n";
        printBar("Utilization", utilization, 0.40);
        std::cout << "Most-rented vehicles:\n";
        std::vector<std::pair<int, std::string>> ranked;
        for (const auto& item : rentalCounts) ranked.push_back({item.second, item.first});
        std::sort(ranked.begin(), ranked.end(), [](const auto& left, const auto& right) {
            return left.first == right.first ? left.second < right.second : left.first > right.first;
        });
        if (ranked.empty()) std::cout << "No bookings yet.\n";
        for (std::size_t index = 0; index < std::min<std::size_t>(5, ranked.size()); ++index) {
            std::cout << "  " << ranked[index].second << " | " << ranked[index].first << " booking(s)\n";
        }
        std::cout << "\n" << std::string(72, '-') << "\n";
    }
};

void displayMenu() {
    printSectionHeader("ROADWISE  /  RENTAL OPERATIONS", "C++ CONSOLE SYSTEM  |  " + dateText(todaySerial()));
    std::cout << "| " << std::left << std::setw(33) << "  1  Browse fleet"
              << "| " << std::setw(33) << "  5  Smart recommendations" << " |\n"
              << "| " << std::setw(33) << "  2  Create reservation"
              << "| " << std::setw(33) << "  6  Performance dashboard" << " |\n"
              << "| " << std::setw(33) << "  3  Check availability"
              << "| " << std::setw(33) << "  7  Add vehicle" << " |\n"
              << "| " << std::setw(33) << "  4  Process vehicle return"
              << "| " << std::setw(33) << "  8  Exit" << " |\n"
              << '+' << std::string(72, '-') << "+\n";
}

int main() {
    RentalSystem system;
    std::string dataError;
    const DataLoadResult loadResult = system.loadData(dataError);
    if (loadResult == DataLoadResult::Error) {
        std::cerr << "Could not load rental_data.txt: " << dataError << '\n'
                  << "The file was left untouched. Fix or restore it before restarting.\n";
        return 1;
    }
    if (loadResult == DataLoadResult::Missing) {
        system.addVehicle(std::make_unique<Car>("C001", "Toyota Corolla", 65.0, 3, 5));
        system.addVehicle(std::make_unique<Car>("C002", "Ford Mustang", 150.0, 2, 4));
        system.addVehicle(std::make_unique<Bike>("B001", "Yamaha R1", 80.0, 2, 1000));
        system.addVehicle(std::make_unique<Bike>("B002", "Honda Rebel", 65.0, 4, 500));
        system.addVehicle(std::make_unique<EV>("E001", "Tesla Model 3", 120.0, 1, 5, 450));
        system.addVehicle(std::make_unique<Luxury>("L001", "Mercedes S-Class", 220.0, 1, 5));
        if (!system.saveData(dataError)) {
            std::cerr << "Could not initialize rental_data.txt: " << dataError << '\n';
            return 1;
        }
        std::cout << "Created rental_data.txt with the starter fleet.\n";
    } else {
        std::cout << "Loaded saved rental data from rental_data.txt.\n";
    }

    while (true) {
        displayMenu();
        const int choice = readInt("  Select an action [1-8] > ");
        if (choice == 8) {
            std::cout << "Goodbye.\n";
            break;
        }
        switch (choice) {
            case 1:
                system.displayFleet();
                break;
            case 2: {
                const std::string id = readLine("Vehicle ID: ");
                const std::string customer = readLine("Customer name: ");
                const int start = readDate("Start date (YYYY-MM-DD): ");
                system.bookVehicle(id, customer, start, readInt("Rental length in days: "));
                break;
            }
            case 3: {
                const int start = readDate("Start date (YYYY-MM-DD): ");
                const int days = readInt("Number of days: ");
                if (days > 0) system.showAvailability(start, days);
                else std::cout << "Number of days must be positive.\n";
                break;
            }
            case 4: {
                const std::string id = readLine("Booking ID: ");
                const int returned = readDate("Actual return date (YYYY-MM-DD): ");
                const std::string answer = readLine("Report damage? (y/n): ");
                const bool damaged = !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
                std::string description;
                double cost = 0.0;
                if (damaged) {
                    description = readLine("Damage description: ");
                    cost = std::max(0.0, readDouble("Assessed repair cost: $"));
                }
                system.returnVehicle(id, returned, damaged, description, cost);
                break;
            }
            case 5: {
                const double budget = readDouble("Budget ($): ");
                const int distance = readInt("Trip distance (km): ");
                const int passengers = readInt("Passenger count: ");
                if (budget >= 0 && distance > 0 && passengers > 0) system.recommend(budget, distance, passengers);
                else std::cout << "Budget must be nonnegative; distance and passengers must be positive.\n";
                break;
            }
            case 6:
                system.adminDashboard();
                break;
            case 7: {
                std::cout << "Vehicle types: 1 Car, 2 Bike, 3 EV, 4 Luxury\n";
                const int type = readInt("Type: ");
                const std::string id = readLine("Unique ID: ");
                const std::string model = readLine("Model: ");
                const double price = readDouble("Base price per day ($): ");
                const int age = readInt("Vehicle age in years: ");
                if (price <= 0 || age < 0) {
                    std::cout << "Price must be positive and age cannot be negative.\n";
                    break;
                }
                std::unique_ptr<Vehicle> vehicle;
                if (type == 1) {
                    const int seats = readInt("Passenger capacity: ");
                    if (seats > 0) vehicle = std::make_unique<Car>(id, model, price, age, seats);
                } else if (type == 2) {
                    const int cc = readInt("Engine capacity (cc): ");
                    if (cc > 0) vehicle = std::make_unique<Bike>(id, model, price, age, cc);
                } else if (type == 3) {
                    const int seats = readInt("Passenger capacity: ");
                    const int range = readInt("Range (km): ");
                    if (seats > 0 && range > 0) vehicle = std::make_unique<EV>(id, model, price, age, seats, range);
                } else if (type == 4) {
                    const int seats = readInt("Passenger capacity: ");
                    if (seats > 0) vehicle = std::make_unique<Luxury>(id, model, price, age, seats);
                }
                if (!vehicle) std::cout << "Invalid type or vehicle details.\n";
                else {
                    std::string saveError;
                    if (!system.addVehicleAndSave(std::move(vehicle), saveError))
                        std::cout << "Vehicle was not added: " << saveError << '\n';
                    else std::cout << "Vehicle added and saved.\n";
                }
                break;
            }
            default:
                std::cout << "Choose an option from 1 to 8.\n";
        }
    }
    return 0;
}