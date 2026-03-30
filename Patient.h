#ifndef PATIENT_H
#define PATIENT_H

#include "User.h"
#include "Condition.h"

// ============================================================
// Patient Class
// The core entity managed by the priority queue system.
//
// Priority is computed via a three-rule formula:
//   priorityScore = urgencyLevel * 1,000,000
//                 - arrivalTime                  (tie-break: earlier = higher)
//                 + agingCounter * 100            (starvation prevention)
//
// This guarantees:
//   Rule 1: Higher urgency ALWAYS wins (dominates by 1,000,000)
//   Rule 2: Same urgency → earlier arrival wins
//   Rule 3: Same urgency + arrival → longer wait wins (aging)
//
// PENDING patients are held at the Receptionist's FIFO queue
// and are NOT enqueued in the Clinic's PriorityQueue until
// a Category is assigned.
// ============================================================
class Patient : public User {
private:
    Condition condition;
    int       arrivalTime;        // simulation tick when patient registered
    int       serviceStartTime;   // tick when a doctor began seeing them (-1 if unserved)
    int       waitTime;           // serviceStartTime - arrivalTime (set on service)
    int       agingCounter;       // increments each tick; boosts priority to prevent starvation
    int       assignedClinicId;
    bool      served;
    int       priorityScore;      // computed; updated dynamically

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

    // Called after Receptionist assigns a Category to an "Other" patient,
    // or immediately after patient picks a preset Condition.
    void setCondition(const Condition& c) {
        condition = c;
        computePriority();
    }

    // Called by Doctor when they start seeing this patient.
    void markServed(int currentTick) {
        serviceStartTime = currentTick;
        waitTime         = currentTick - arrivalTime;
        served           = true;
    }

    // ---- Aging --------------------------------------------------
    // Called each simulation tick for every patient waiting in the queue.
    // Increments agingCounter and recomputes priorityScore so that
    // long-waiting patients eventually get served (starvation prevention).
    void incrementAging() {
        agingCounter++;
        computePriority();
    }

    // ---- Priority Formula ---------------------------------------
    void computePriority() {
        if (condition.category.isPending()) {
            priorityScore = -1; // not eligible for PriorityQueue yet
            return;
        }
        priorityScore = (condition.category.getPriorityValue() * 1000000)
                      - arrivalTime
                      + (agingCounter * 100);
    }

    void displayInfo() const override;
};

#endif
