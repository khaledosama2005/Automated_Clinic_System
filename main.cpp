#include "HospitalSystem.h"
#include <iostream>
#include <string>
#include <limits>
#include <iomanip>

int getInt(const std::string& prompt, int lo, int hi) {
    int val;
    while (true) {
        std::cout << prompt;
        if (std::cin >> val && val >= lo && val <= hi) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  Invalid. Enter " << lo << "-" << hi << ".\n";
    }
}

std::string getString(const std::string& prompt) {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    return s;
}

void printHeader() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║      HOSPITAL PRIORITY QUEUE MANAGEMENT SYSTEM       ║\n";
    std::cout << "║         CSCE 2211 — Applied Data Structures          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
}

void printMainMenu(int tick) {
    std::cout << "\n┌─────────────────────────────────────────────────────┐\n";
    std::cout << "│  MAIN MENU                           tick: " << std::setw(6) << tick << "   │\n";
    std::cout << "├─────────────────────────────────────────────────────┤\n";
    std::cout << "│  [1] Register new patient                           │\n";
    std::cout << "│  [2] Triage next pending patient (Receptionist)     │\n";
    std::cout << "│  [3] Serve next patient (Doctor)                    │\n";
    std::cout << "│  [4] Advance simulation clock (tick)                │\n";
    std::cout << "│  [5] Update patient priority dynamically            │\n";
    std::cout << "│  [6] View all queue states                          │\n";
    std::cout << "│  [7] View doctor status                             │\n";
    std::cout << "│  [8] View statistics and FIFO comparison            │\n";
    std::cout << "│  [9] Run automated demo scenario                    │\n";
    std::cout << "│  [0] Exit                                           │\n";
    std::cout << "└─────────────────────────────────────────────────────┘\n";
    std::cout << "  Choice: ";
}

void actionRegister(HospitalSystem& sys) {
    std::cout << "\n=== PATIENT REGISTRATION ===\n";
    std::string name = getString("  Patient name: ");
    if (name.empty()) { std::cout << "  Name cannot be empty.\n"; return; }

    sys.displayClinicsMenu();
    int clinicIdx = getInt("  Select clinic: ", 0, sys.getClinicCount()-1);

    Clinic* clinic = sys.getClinic(clinicIdx);
    clinic->displayConditionMenu();

    int condCount = (int)clinic->getCommonConditions().size();
    int condChoice = getInt("  Select condition: ", 1, condCount+1);
    int condIdx = (condChoice == condCount+1) ? -1 : (condChoice - 1);
    sys.registerPatient(name, clinicIdx, condIdx);
}

void actionTriage(HospitalSystem& sys) {
    Receptionist* r = sys.getReceptionist();
    if (!r->hasPending()) { std::cout << "\n  No pending patients.\n"; return; }

    std::cout << "\n=== RECEPTIONIST TRIAGE ===\n";
    r->displayPendingQueue();
    Patient* next = r->peekNextPending();
    std::cout << "\n  Triaging: " << next->getName() << " (ID " << next->getId() << ")\n";

    std::cout << "  Urgency: [1]Critical [2]Urgent [3]Semi-Urgent [4]Non-Urgent [5]Elective\n";
    int lvlChoice = getInt("  Level: ", 1, 5);
    UrgencyLevel levels[] = {
        UrgencyLevel::CRITICAL, UrgencyLevel::URGENT,
        UrgencyLevel::SEMI_URGENT, UrgencyLevel::NON_URGENT, UrgencyLevel::ELECTIVE
    };

    int treatMins = getInt("  Estimated treatment time (minutes): ", 1, 300);
    sys.displayClinicsMenu();
    int clinicIdx = getInt("  Route to clinic: ", 0, sys.getClinicCount()-1);
    sys.triageNextPending(levels[lvlChoice-1], treatMins, clinicIdx);
}

void actionServeNext(HospitalSystem& sys) {
    std::cout << "\n=== SERVE NEXT PATIENT ===\n";
    sys.displayClinicsMenu();
    int clinicIdx = getInt("  Select clinic: ", 0, sys.getClinicCount()-1);
    sys.serveNext(clinicIdx);
}

void actionTick(HospitalSystem& sys) {
    int steps = getInt("\n  Advance by how many ticks? (1-100): ", 1, 100);
    sys.tick(steps);
}

void actionUpdatePriority(HospitalSystem& sys) {
    std::cout << "\n=== DYNAMIC PRIORITY UPDATE ===\n";
    int patientId = getInt("  Enter patient ID: ", 1, 9999);
    std::cout << "  [1]Critical [2]Urgent [3]Semi-Urgent [4]Non-Urgent [5]Elective\n";
    int lvl = getInt("  New level: ", 1, 5);
    UrgencyLevel levels[] = {
        UrgencyLevel::CRITICAL, UrgencyLevel::URGENT,
        UrgencyLevel::SEMI_URGENT, UrgencyLevel::NON_URGENT, UrgencyLevel::ELECTIVE
    };
    sys.updatePatientPriority(patientId, levels[lvl-1]);
}

void runDemo(HospitalSystem& sys) {
    std::cout << "\n=== AUTOMATED DEMO SCENARIO ===\n";
    std::cout << "  Simulating a busy hospital morning...\n\n";

    sys.registerPatient("Ahmed Hassan",   0, 0);  // Emergency, Cardiac Arrest CRITICAL
    sys.registerPatient("Sara Ali",       2, 0);  // General, Mild Fever NON_URGENT
    sys.registerPatient("Omar Khaled",    1, 0);  // Cardiology, Chest Pain URGENT
    sys.registerPatient("Layla Nour",     0, 5);  // Emergency, Deep Laceration SEMI_URGENT
    sys.registerPatient("Youssef Salem",  2, -1); // General, Other pending
    sys.registerPatient("Dina Farid",     1, 4);  // Cardiology, Shortness of Breath URGENT

    std::cout << "\n[Tick +5]\n";
    sys.tick(5);
    sys.displayAllQueues();

    std::cout << "\n[Triage Youssef → Semi-Urgent → General]\n";
    sys.triageNextPending(UrgencyLevel::SEMI_URGENT, 25, 2);

    std::cout << "\n[Serving from each clinic]\n";
    sys.serveNext(0);
    sys.serveNext(1);
    sys.serveNext(2);

    std::cout << "\n[Tick +10]\n";
    sys.tick(10);

    std::cout << "\n[Dynamic update: escalate Sara to CRITICAL]\n";
    sys.updatePatientPriority(2, UrgencyLevel::CRITICAL);

    std::cout << "\n[Serve remaining]\n";
    sys.serveNext(0);
    sys.serveNext(1);
    sys.serveNext(2);

    sys.displayStats();
}

int main() {
    printHeader();
    HospitalSystem sys;
    std::cout << "\n  System initialized: 3 clinics, 6 doctors, 1 receptionist ready.\n";

    bool running = true;
    while (running) {
        printMainMenu(sys.getCurrentTick());
        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: actionRegister(sys);       break;
            case 2: actionTriage(sys);         break;
            case 3: actionServeNext(sys);      break;
            case 4: actionTick(sys);           break;
            case 5: actionUpdatePriority(sys); break;
            case 6: sys.displayAllQueues();    break;
            case 7: sys.displayDoctors();      break;
            case 8: sys.displayStats();        break;
            case 9: runDemo(sys);              break;
            case 0: running = false;           break;
            default: std::cout << "  Invalid choice.\n";
        }
    }
    std::cout << "\n  Goodbye.\n";
    return 0;
}
