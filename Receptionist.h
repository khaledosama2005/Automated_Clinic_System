#ifndef RECEPTIONIST_H
#define RECEPTIONIST_H

#include "User.h"
#include "Patient.h"
#include "Clinic.h"
#include <string>
#include <iostream>
#include <utility>

// ============================================================
// PendingNode — internal linked-list node for the FIFO queue.
// Separate from teammates' Queue.h to avoid naming conflicts.
// ============================================================
struct PendingNode {
    Patient*     patient;
    PendingNode* next;
    explicit PendingNode(Patient* p) : patient(p), next(nullptr) {}
};

// ============================================================
// Receptionist Class
// Manages two responsibilities:
//
//   1. FIFO Pending Queue:
//      Patients who selected "Other" are enqueued here in
//      arrival order (pure FIFO — no priority). The receptionist
//      dequeues them one by one, assigns a Category based on
//      visual triage, then routes them into the correct Clinic's
//      PriorityQueue.
//
//   2. Triage:
//      assignUrgency() sets the patient's Condition (with the
//      chosen Category), computes their priority score, and
//      calls clinic->enqueuePatient(). Until this step, the
//      patient has priorityScore = -1 and is NOT in any
//      PriorityQueue.
//
// This design directly demonstrates FIFO vs. Priority Queue
// comparison as required by the challenge section.
// ============================================================
class Receptionist : public User {
private:
    // FIFO linked-list queue for pending "Other" patients
    PendingNode*       front;
    PendingNode*       rear;
    int                pendingCount;
    std::pair<int,int> workingHours;
    bool               onDuty;

public:
    Receptionist(int id, const std::string& name,
                 std::pair<int,int> hours = {7, 20})
        : User(id, name, UserRole::RECEPTIONIST),
          front(nullptr), rear(nullptr), pendingCount(0),
          workingHours(hours), onDuty(true)
    {}

    ~Receptionist() {
        // Free any remaining pending nodes (not the patients themselves)
        PendingNode* cur = front;
        while (cur) {
            PendingNode* tmp = cur->next;
            delete cur;
            cur = tmp;
        }
    }

    // ---- FIFO Pending Queue -------------------------------------

    // Called when patient picks "Other" — O(1)
    void receivePendingPatient(Patient* p) {
        PendingNode* node = new PendingNode(p);
        if (!rear) {
            front = rear = node;
        } else {
            rear->next = node;
            rear       = node;
        }
        pendingCount++;
        std::cout << "  [Receptionist " << name << "] Received pending patient: "
                  << p->getName() << " (ID " << p->getId() << ")\n";
    }

    // Peek at next patient to triage — O(1)
    Patient* peekNextPending() const {
        return front ? front->patient : nullptr;
    }

    // Dequeue next pending patient (internal) — O(1)
    Patient* dequeuePending() {
        if (!front) return nullptr;
        PendingNode* tmp = front;
        Patient*     p   = tmp->patient;
        front = front->next;
        if (!front) rear = nullptr;
        delete tmp;
        pendingCount--;
        return p;
    }

    // ---- Triage -------------------------------------------------
    // Dequeues the next pending patient, assigns a Category and
    // estimated treatment time, then routes them to their Clinic.
    // Returns the triaged patient or nullptr if queue empty.
    Patient* assignUrgencyAndRoute(UrgencyLevel chosenLevel,
                                   int estimatedTreatmentMinutes,
                                   Clinic* clinic) {
        Patient* p = dequeuePending();
        if (!p) {
            std::cout << "  [Receptionist] No pending patients.\n";
            return nullptr;
        }
        Condition c("Other (assessed)", chosenLevel, estimatedTreatmentMinutes);
        p->setCondition(c);
        clinic->enqueuePatient(p);
        std::cout << "  [Receptionist " << name << "] Triaged " << p->getName()
                  << " → " << c.category.name
                  << " | Routed to " << clinic->getName() << "\n";
        return p;
    }

    // ---- Accessors ----------------------------------------------
    int  getPendingCount() const { return pendingCount; }
    bool hasPending()      const { return pendingCount > 0; }
    bool isOnDuty()        const { return onDuty; }
    void setOnDuty(bool v)       { onDuty = v; }

    void displayPendingQueue() const {
        std::cout << "  [Receptionist " << name << "] Pending queue ("
                  << pendingCount << "):\n";
        PendingNode* cur = front;
        int pos = 1;
        while (cur) {
            std::cout << "    " << pos++ << ". "
                      << cur->patient->getName()
                      << " (ID " << cur->patient->getId() << ")\n";
            cur = cur->next;
        }
        if (pendingCount == 0)
            std::cout << "    (empty)\n";
    }

    void displayInfo() const override {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "  [Receptionist]\n";
        std::cout << "  ID          : " << id << "\n";
        std::cout << "  Name        : " << name << "\n";
        std::cout << "  Shift       : " << workingHours.first << ":00 - "
                  << workingHours.second << ":00\n";
        std::cout << "  On Duty     : " << (onDuty ? "Yes" : "No") << "\n";
        std::cout << "  Pending     : " << pendingCount << "\n";
        std::cout << "+-----------------------------------------+\n";
    }
};

#endif
