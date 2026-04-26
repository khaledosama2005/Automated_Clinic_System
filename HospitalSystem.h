#ifndef HOSPITALSYSTEM_H
#define HOSPITALSYSTEM_H

// ============================================================
// HospitalSystem.h
// Central controller — owns all Clinics, Doctors, Patients,
// and the Receptionist. Drives the simulation tick and
// exposes the actions that main.cpp's CLI calls.
// ============================================================

#include "Clinic.h"
#include "Receptionist.h"
#include "HASHMAP.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

class HospitalSystem {
private:
    // ── Clinics ──────────────────────────────────────────────
    std::vector<Clinic*>       clinics;
    std::vector<Doctor*>       allDoctors;
    std::vector<Patient*>      allPatients;      // owns all Patient memory
    Receptionist*              receptionist;

    // ── HashMap: patientId (string) → clinic index (int) ────
    // Enables O(1) lookup: "which clinic is P0012 in?"
    HashMap                    patientClinicMap;

    // ── Simulation state ─────────────────────────────────────
    int    currentTick;
    int    nextPatientId;

    // ── Stats for FIFO comparison ─────────────────────────────
    // We keep a parallel served-order log to compare PQ vs FIFO
    std::vector<Patient*>      servedLog;        // ordered by actual serve time
    std::vector<int>           fifoServeOrder;   // what FIFO order would have been

public:
    HospitalSystem() : receptionist(nullptr), currentTick(0), nextPatientId(1) {
        setupClinics();
        setupDoctors();
        setupReceptionist();
    }

    ~HospitalSystem() {
        for (Patient* p : allPatients) delete p;
        for (Clinic*  c : clinics)     delete c;
        for (Doctor*  d : allDoctors)  delete d;
        delete receptionist;
    }

    // ════════════════════════════════════════════════════════
    //  SETUP
    // ════════════════════════════════════════════════════════

    void setupClinics() {
        // Clinic 0 — Emergency
        Clinic* em = new Clinic(0, "Emergency", "Emergency Medicine");
        em->addCommonCondition({"Cardiac Arrest",     UrgencyLevel::CRITICAL,   15});
        em->addCommonCondition({"Severe Trauma",      UrgencyLevel::CRITICAL,   30});
        em->addCommonCondition({"Stroke",             UrgencyLevel::CRITICAL,   20});
        em->addCommonCondition({"Severe Burn",        UrgencyLevel::URGENT,     45});
        em->addCommonCondition({"Broken Bone",        UrgencyLevel::URGENT,     60});
        em->addCommonCondition({"Deep Laceration",    UrgencyLevel::SEMI_URGENT,40});
        em->addCommonCondition({"High Fever",         UrgencyLevel::SEMI_URGENT,30});
        clinics.push_back(em);

        // Clinic 1 — Cardiology
        Clinic* ca = new Clinic(1, "Cardiology", "Heart & Vascular");
        ca->addCommonCondition({"Chest Pain",         UrgencyLevel::URGENT,     45});
        ca->addCommonCondition({"Palpitations",       UrgencyLevel::SEMI_URGENT,30});
        ca->addCommonCondition({"Hypertension Crisis",UrgencyLevel::URGENT,     60});
        ca->addCommonCondition({"Arrhythmia",         UrgencyLevel::SEMI_URGENT,40});
        ca->addCommonCondition({"Shortness of Breath",UrgencyLevel::URGENT,     35});
        ca->addCommonCondition({"Routine Checkup",    UrgencyLevel::ELECTIVE,   20});
        clinics.push_back(ca);

        // Clinic 2 — General
        Clinic* gn = new Clinic(2, "General", "General Practice");
        gn->addCommonCondition({"Mild Fever",         UrgencyLevel::NON_URGENT, 20});
        gn->addCommonCondition({"Cold / Flu",         UrgencyLevel::NON_URGENT, 15});
        gn->addCommonCondition({"Headache",           UrgencyLevel::NON_URGENT, 15});
        gn->addCommonCondition({"Stomach Pain",       UrgencyLevel::SEMI_URGENT,25});
        gn->addCommonCondition({"Routine Checkup",    UrgencyLevel::ELECTIVE,   20});
        gn->addCommonCondition({"Prescription Refill",UrgencyLevel::ELECTIVE,   10});
        clinics.push_back(gn);
    }

    void setupDoctors() {
        // Emergency doctors
        addDoctor(new Doctor(1, "Dr. Youssef Kamal",  "Emergency Medicine", 0, {0,24}));
        addDoctor(new Doctor(2, "Dr. Layla Hassan",   "Emergency Medicine", 0, {0,24}));

        // Cardiology doctors
        addDoctor(new Doctor(3, "Dr. Omar Nasser",    "Cardiology",         1, {8,20}));
        addDoctor(new Doctor(4, "Dr. Nour Saleh",     "Cardiology",         1, {8,20}));

        // General doctors
        addDoctor(new Doctor(5, "Dr. Ahmed Ibrahim",  "General Practice",   2, {8,18}));
        addDoctor(new Doctor(6, "Dr. Sara Mostafa",   "General Practice",   2, {8,18}));
    }

    void addDoctor(Doctor* d) {
        clinics[d->getClinicId()]->addDoctor(d);
        allDoctors.push_back(d);
    }

    void setupReceptionist() {
        receptionist = new Receptionist(100, "Rania Ahmed", {7, 21});
    }

    // ════════════════════════════════════════════════════════
    //  PATIENT REGISTRATION FLOW
    // ════════════════════════════════════════════════════════

    // Step 1: Create patient and assign clinic + condition
    // If conditionIdx == -1 → "Other" → goes to pending queue
    Patient* registerPatient(const std::string& name, int clinicIdx, int conditionIdx) {
        if (clinicIdx < 0 || clinicIdx >= (int)clinics.size()) return nullptr;

        Patient* p = new Patient(nextPatientId++, name, currentTick, clinicIdx);
        allPatients.push_back(p);

        Clinic* clinic = clinics[clinicIdx];

        if (conditionIdx < 0) {
            // "Other" — send to receptionist FIFO pending queue
            p->setCondition(Condition()); // PENDING condition
            receptionist->receivePendingPatient(p);
            std::cout << "\n  [Registration] " << name
                      << " (ID " << p->getId() << ") added to pending queue.\n";
        } else {
            const Condition* cond = clinic->getConditionByIndex(conditionIdx);
            if (!cond) return nullptr;
            p->setCondition(*cond);
            clinic->enqueuePatient(p);
            // Track insertion order for FIFO comparison
            fifoServeOrder.push_back(p->getId());
            // HashMap: map patient ID → clinic index
            patientClinicMap.insert(std::to_string(p->getId()), clinicIdx);
            std::cout << "\n  [Registration] " << name
                      << " (ID " << p->getId() << ") → "
                      << clinic->getName() << " | "
                      << cond->category.name
                      << " | Priority: " << p->getPriorityScore() << "\n";
        }
        return p;
    }

    // Step 2: Receptionist triages next pending patient
    Patient* triageNextPending(UrgencyLevel level, int treatmentMinutes, int clinicIdx) {
        if (clinicIdx < 0 || clinicIdx >= (int)clinics.size()) return nullptr;
        Patient* p = receptionist->assignUrgencyAndRoute(level, treatmentMinutes, clinics[clinicIdx]);
        if (p) {
            fifoServeOrder.push_back(p->getId());
            patientClinicMap.insert(std::to_string(p->getId()), clinicIdx);
        }
        return p;
    }

    // Step 3: Serve next patient in a clinic
    // Doctor treats the top-priority patient
    bool serveNext(int clinicIdx) {
        if (clinicIdx < 0 || clinicIdx >= (int)clinics.size()) return false;
        Clinic* clinic = clinics[clinicIdx];

        if (!clinic->hasWaitingPatients()) {
            std::cout << "  [" << clinic->getName() << "] No patients waiting.\n";
            return false;
        }

        // Find available doctor
        int hour = 9 + (currentTick / 60) % 15;  // simulation starts at 9:00am
        Doctor* doc = clinic->getAvailableDoctor(hour);
        if (!doc) {
            std::cout << "  [" << clinic->getName() << "] No doctors available right now.\n";
            return false;
        }

        Patient* p = clinic->dequeuePatient();
        p->markServed(currentTick);
        doc->assignPatient(p->getCondition().estimatedTreatmentMinutes);
        clinic->recordServedPatient(p->getWaitTime(), p->getCondition().estimatedTreatmentMinutes);
        servedLog.push_back(p);

        std::cout << "\n  ✓ [" << clinic->getName() << "] Serving: "
                  << p->getName() << " (ID " << p->getId() << ")"
                  << "\n    Urgency   : " << p->getCondition().category.name
                  << "\n    Wait time : " << p->getWaitTime() << " ticks"
                  << "\n    Doctor    : " << doc->getName()
                  << "\n    Est. time : " << p->getCondition().estimatedTreatmentMinutes << " min\n";
        return true;
    }

    // ════════════════════════════════════════════════════════
    //  SIMULATION TICK
    // ════════════════════════════════════════════════════════

    void tick(int steps = 1) {
        for (int s = 0; s < steps; ++s) {
            currentTick++;
            // Age all patients in all clinic queues
            for (Clinic* c : clinics) c->applyAgingTick();
            // Countdown all doctors
            for (Doctor* d : allDoctors) d->tick();
        }
        std::cout << "  [Tick] Simulation clock: " << currentTick << "\n";
    }

    // ════════════════════════════════════════════════════════
    //  DYNAMIC PRIORITY UPDATE
    // ════════════════════════════════════════════════════════

    bool updatePatientPriority(int patientId, UrgencyLevel newLevel) {
        int* clinicIdx = patientClinicMap.search(std::to_string(patientId));
        if (!clinicIdx) {
            std::cout << "  Patient " << patientId << " not found in any clinic queue.\n";
            return false;
        }
        Clinic* clinic = clinics[*clinicIdx];

        // Find patient in allPatients and update condition
        for (Patient* p : allPatients) {
            if (p->getId() == patientId && !p->isServed()) {
                Condition c(p->getCondition().name, newLevel,
                            p->getCondition().estimatedTreatmentMinutes);
                p->setCondition(c);
                clinic->getQueueSize(); // keep reference alive
                // Trigger re-heap in the clinic PQ
                // PriorityQueue::updatePriority does fixUp+fixDown
                // We call it through Clinic — need access to internal PQ
                // Workaround: applyAgingAndRebuild rebuilds the whole heap O(n)
                // This guarantees correctness after any priority change
                clinic->applyAgingTick();
                std::cout << "  [Update] Patient " << p->getName()
                          << " priority → " << c.category.name << "\n";
                return true;
            }
        }
        return false;
    }

    // ════════════════════════════════════════════════════════
    //  STATS & COMPARISON
    // ════════════════════════════════════════════════════════

    void displayStats() const {
        std::cout << "\n╔══════════════════════════════════════════════════╗\n";
        std::cout <<   "║            SYSTEM STATISTICS                    ║\n";
        std::cout <<   "╚══════════════════════════════════════════════════╝\n";
        std::cout << "  Current tick : " << currentTick << "\n";
        std::cout << "  Total served : " << servedLog.size() << "\n\n";

        // Per-clinic stats
        for (Clinic* c : clinics) c->displayInfo();

        // FIFO vs PQ comparison
        displayFifoComparison();
    }

    void displayFifoComparison() const {
        if (servedLog.empty()) { std::cout << "  No patients served yet.\n"; return; }

        std::cout << "\n  ┌─────────────────────────────────────────────┐\n";
        std::cout <<   "  │  Priority Queue vs FIFO Comparison          │\n";
        std::cout <<   "  └─────────────────────────────────────────────┘\n";

        // PQ stats: actual served order
        double totalPQWait = 0;
        double maxPQWait   = 0;
        for (Patient* p : servedLog) {
            totalPQWait += p->getWaitTime();
            if (p->getWaitTime() > maxPQWait) maxPQWait = p->getWaitTime();
        }
        double avgPQWait = totalPQWait / servedLog.size();

        // FIFO simulation: patients served in insertion order
        // Compute what wait would have been if served strictly in arrival order
        // by sorting a copy by arrivalTime and assigning sequential serve slots
        // We approximate: FIFO_wait[i] ≈ position_in_fifo * avg_treatment_time
        // Use actual patient data for a realistic estimate
        double totalFIFOWait = 0;
        int n = servedLog.size();
        // Build arrival-ordered list
        std::vector<int> arrivalOrder(n);
        for (int i = 0; i < n; ++i) arrivalOrder[i] = i;
        // Simple insertion sort by arrivalTime (n is small in demo)
        for (int i = 1; i < n; ++i) {
            int key = arrivalOrder[i];
            int j = i - 1;
            while (j >= 0 && servedLog[arrivalOrder[j]]->getArrivalTime() >
                              servedLog[key]->getArrivalTime()) {
                arrivalOrder[j+1] = arrivalOrder[j]; --j;
            }
            arrivalOrder[j+1] = key;
        }
        // FIFO: each patient served at tick = arrivalTime + sum of previous treatments
        int fifoTick = 0;
        for (int i = 0; i < n; ++i) {
            Patient* p = servedLog[arrivalOrder[i]];
            int fifoStart = fifoTick > p->getArrivalTime() ? fifoTick : p->getArrivalTime();
            totalFIFOWait += fifoStart - p->getArrivalTime();
            fifoTick = fifoStart + p->getCondition().estimatedTreatmentMinutes;
        }
        double avgFIFOWait = totalFIFOWait / n;

        std::cout << std::fixed << std::setprecision(1);
        std::cout << "\n  Metric                PQ (Ours)     FIFO\n";
        std::cout << "  " << std::string(44, '-') << "\n";
        std::cout << "  Avg wait time      " << std::setw(10) << avgPQWait
                  << "    " << std::setw(10) << avgFIFOWait << "\n";
        std::cout << "  Max wait time      " << std::setw(10) << maxPQWait << "\n";
        std::cout << "  Patients served    " << std::setw(10) << n << "\n";

        // Per-urgency breakdown
        std::cout << "\n  Per-urgency average wait (PQ):\n";
        const char* labels[] = {"", "Elective","Non-Urgent","Semi-Urgent","Urgent","Critical"};
        for (int u = 5; u >= 1; --u) {
            double tot = 0; int cnt = 0;
            for (Patient* p : servedLog) {
                if (p->getCondition().category.getPriorityValue() == u) {
                    tot += p->getWaitTime(); cnt++;
                }
            }
            if (cnt > 0)
                std::cout << "  " << std::setw(14) << labels[u]
                          << " : " << tot/cnt << " ticks avg (" << cnt << " patients)\n";
        }

        double improvement = avgFIFOWait - avgPQWait;
        std::cout << "\n  Improvement over FIFO: " << improvement << " ticks avg wait\n";
        if (improvement > 0)
            std::cout << "  ✓ Priority Queue reduced average waiting time.\n";
        else
            std::cout << "  (FIFO competitive at low patient counts — PQ shines at scale)\n";
    }

    // ════════════════════════════════════════════════════════
    //  DISPLAY HELPERS
    // ════════════════════════════════════════════════════════

    void displayAllQueues() const {
        std::cout << "\n══════════════════════════════════════════\n";
        std::cout << "  CURRENT QUEUE STATE  (tick " << currentTick << ")\n";
        std::cout << "══════════════════════════════════════════\n";
        for (Clinic* c : clinics) {
            std::cout << "\n  [" << c->getName() << "]  ("
                      << c->getQueueSize() << " waiting)\n";
            c->displayQueue();
        }
        std::cout << "\n  [Pending (untriaged)]  ("
                  << receptionist->getPendingCount() << " waiting)\n";
        receptionist->displayPendingQueue();
    }

    void displayDoctors() const {
        std::cout << "\n══════ DOCTOR STATUS ══════\n";
        for (Doctor* d : allDoctors) d->displayInfo();
    }

    void displayClinicsMenu() const {
        std::cout << "\n  Available clinics:\n";
        for (int i = 0; i < (int)clinics.size(); ++i)
            std::cout << "  [" << i << "] " << clinics[i]->getName()
                      << " (" << clinics[i]->getSpecialty() << ")\n";
    }

    Clinic*       getClinic(int i)       { return (i>=0&&i<(int)clinics.size()) ? clinics[i] : nullptr; }
    Receptionist* getReceptionist()      { return receptionist; }
    int           getCurrentTick() const { return currentTick; }
    int           getClinicCount() const { return (int)clinics.size(); }
};

#endif
