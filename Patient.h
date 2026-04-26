#ifndef PATIENT_H
#define PATIENT_H

#include "User.h"
#include "Condition.h"
#include <iostream>
#include <iomanip>

// ============================================================
// Patient Class
// Core entity managed by the priority queue system.
//
// Priority formula:
//   priorityScore = urgencyLevel * 1,000,000
//                 - arrivalTime          (earlier arrival = higher priority)
//                 + agingCounter * 100   (starvation prevention)
//
// Rule 1: Higher urgency ALWAYS wins (1,000,000 gap)
// Rule 2: Same urgency → earlier arrival wins
// Rule 3: Same urgency + arrival → longer wait wins (aging)
//
// PENDING patients sit in the Receptionist's FIFO Queue.
// They are NOT inserted into a Clinic's PriorityQueue until
// a Category is assigned via setCondition().
// ============================================================
class Patient : public User {
private:
    Condition condition;
    int       arrivalTime;
    int       serviceStartTime;
    int       waitTime;
    int       agingCounter;
    int       assignedClinicId;
    bool      served;
    int       priorityScore;

public:
    Patient(int id, const std::string& name, int arrivalTime, int clinicId)
        : User(id, name, UserRole::PATIENT),
          arrivalTime(arrivalTime),
          serviceStartTime(-1),
          waitTime(0),
          agingCounter(0),
          assignedClinicId(clinicId),
          served(false),
          priorityScore(-1)
    {}

    // ---- Getters ------------------------------------------------
    const Condition& getCondition()     const { return condition; }
    int  getArrivalTime()               const { return arrivalTime; }
    int  getServiceStartTime()          const { return serviceStartTime; }
    int  getWaitTime()                  const { return waitTime; }
    int  getAgingCounter()              const { return agingCounter; }
    int  getClinicId()                  const { return assignedClinicId; }
    bool isServed()                     const { return served; }
    int  getPriorityScore()             const { return priorityScore; }
    bool isPending()                    const { return condition.category.isPending(); }

    // ---- Setters ------------------------------------------------
    void setCondition(const Condition& c) {
        condition = c;
        computePriority();
    }

    void markServed(int currentTick) {
        serviceStartTime = currentTick;
        waitTime         = currentTick - arrivalTime;
        served           = true;
    }

    // ---- Aging --------------------------------------------------
    void incrementAging() {
        agingCounter++;
        computePriority();
    }

    // ---- Priority Formula ---------------------------------------
    void computePriority() {
        if (condition.category.isPending()) {
            priorityScore = -1;
            return;
        }
        priorityScore = (condition.category.getPriorityValue() * 1000000)
                      - arrivalTime
                      + (agingCounter * 100);
    }

    // ---- Display ------------------------------------------------
    // Bug fix: removed erroneous "Patient::" qualifier inside class body
    void displayInfo() const override {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "  [Patient]\n";
        std::cout << "  ID          : " << id << "\n";
        std::cout << "  Name        : " << name << "\n";
        std::cout << "  Clinic ID   : " << assignedClinicId << "\n";
        std::cout << "  Condition   : " << condition.name << "\n";
        std::cout << "  Urgency     : " << condition.category.name << "\n";
        std::cout << "  Est. Time   : " << condition.estimatedTreatmentMinutes << " min\n";
        std::cout << "  Priority    : " << priorityScore << "\n";
        std::cout << "  Arrived At  : tick " << arrivalTime << "\n";
        std::cout << "  Wait Time   : " << waitTime << " ticks\n";
        std::cout << "  Aging       : " << agingCounter << "\n";
        std::cout << "  Served      : " << (served ? "Yes" : "No") << "\n";
        std::cout << "+-----------------------------------------+\n";
    }
};

#endif
