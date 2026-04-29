#ifndef CLINIC_H
#define CLINIC_H

#include "PriorityQueue.h"
#include "Doctor.h"
#include "Condition.h"
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

// ============================================================
// Clinic Class
// Represents one hospital department (e.g. Emergency, Cardiology).
//
// Each Clinic owns:
//   • A PriorityQueue<Patient*> for confirmed patients
//   • A vector<Doctor*> of assigned doctors
//   • A preset list of common Conditions patients can select
//
// Multi-level priority: patients are first routed to the correct
// clinic, then compete within that clinic's priority queue.
// This directly satisfies the challenge section requirement.
// ============================================================
class Clinic {
private:
    int                    id;
    std::string            name;
    std::string            specialty;
    PriorityQueue          patientQueue;
    std::vector<Doctor*>   doctors;
    std::vector<Condition> commonConditions;

    // Statistics
    int    totalPatientsServed;
    int    totalWaitTime;
    int    totalThroughputMinutes; // sum of treatment times for served patients

public:
    Clinic(int id, const std::string& name, const std::string& specialty)
        : id(id), name(name), specialty(specialty),
          patientQueue(64),
          totalPatientsServed(0),
          totalWaitTime(0),
          totalThroughputMinutes(0)
    {}

    // ---- Preset Conditions --------------------------------------
    void addCommonCondition(const Condition& c) {
        commonConditions.push_back(c);
    }

    const std::vector<Condition>& getCommonConditions() const {
        return commonConditions;
    }

    bool removeConditionByIndex(int idx) {
        if (idx < 0 || idx >= (int)commonConditions.size()) return false;
        commonConditions.erase(commonConditions.begin() + idx);
        return true;
    }

    void displayConditionMenu() const {
        std::cout << "\n  == " << name << " — Select Your Condition ==\n";
        for (int i = 0; i < (int)commonConditions.size(); i++) {
            std::cout << "  [" << (i + 1) << "] "
                      << std::left << std::setw(25) << commonConditions[i].name
                      << " | " << commonConditions[i].category.name
                      << " | ~" << commonConditions[i].estimatedTreatmentMinutes << " min\n";
        }
        std::cout << "  [" << (commonConditions.size() + 1) << "] Other (assessed by receptionist)\n";
    }

    // Returns nullptr if index is out of range (means "Other" selected).
    const Condition* getConditionByIndex(int idx) const {
        if (idx < 0 || idx >= (int)commonConditions.size()) return nullptr;
        return &commonConditions[idx];
    }

    // ---- Doctor Management --------------------------------------
    void addDoctor(Doctor* d) { doctors.push_back(d); }

    // Returns first available doctor on duty, nullptr if none.
    Doctor* getAvailableDoctor(int currentHour) const {
        for (Doctor* d : doctors) {
            if (d->isAvailable() && d->isOnDuty(currentHour))
                return d;
        }
        return nullptr;
    }

    const std::vector<Doctor*>& getDoctors() const { return doctors; }

    // ---- Queue Operations ---------------------------------------
    void enqueuePatient(Patient* p) {
        patientQueue.insert(p);
    }

    // Dequeue highest-priority patient.
    Patient* dequeuePatient() {
        return patientQueue.extractMax();
    }

    Patient* peekNextPatient() const {
        return patientQueue.peek();
    }

    bool hasWaitingPatients() const {
        return !patientQueue.isEmpty();
    }

    int getQueueSize() const {
        return patientQueue.getSize();
    }

    const std::vector<Patient*>& getQueueItems() const {
        return patientQueue.items();
    }

    // ---- Aging --------------------------------------------------
    // Called each simulation tick. Applies aging to all waiting patients
    // and rebuilds the heap to reflect updated priority scores.
    void applyAgingTick() {
        patientQueue.applyAgingAndRebuild();
    }

    // ---- Statistics ---------------------------------------------
    void recordServedPatient(int waitTime, int treatmentMinutes) {
        totalPatientsServed++;
        totalWaitTime         += waitTime;
        totalThroughputMinutes += treatmentMinutes;
    }

    double getAverageWaitTime() const {
        if (totalPatientsServed == 0) return 0.0;
        return (double)totalWaitTime / totalPatientsServed;
    }

    double getThroughput() const { // patients per hour equivalent
        if (totalThroughputMinutes == 0) return 0.0;
        return (double)totalPatientsServed / totalThroughputMinutes * 60.0;
    }

    // ---- Getters ------------------------------------------------
    int         getId()        const { return id; }
    std::string getName()      const { return name; }
    std::string getSpecialty() const { return specialty; }

    void displayInfo() const {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "  [Clinic] " << name << " (ID: " << id << ")\n";
        std::cout << "  Specialty      : " << specialty << "\n";
        std::cout << "  Doctors        : " << doctors.size() << "\n";
        std::cout << "  Queue Size     : " << patientQueue.getSize() << "\n";
        std::cout << "  Patients Served: " << totalPatientsServed << "\n";
        std::cout << "  Avg Wait       : " << std::fixed << std::setprecision(1)
                  << getAverageWaitTime() << " ticks\n";
        std::cout << "  Throughput     : " << std::fixed << std::setprecision(2)
                  << getThroughput() << " patients/hr\n";
        std::cout << "+-----------------------------------------+\n";
    }

    void displayQueue() const {
        std::cout << "  [" << name << " Queue]\n";
        patientQueue.display();
    }
};

#endif
