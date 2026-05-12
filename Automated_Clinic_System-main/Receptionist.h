#ifndef RECEPTIONIST_H
#define RECEPTIONIST_H

#include "User.h"
#include "Patient.h"
#include "Clinic.h"
#include "Queue.h"
#include <string>
#include <iostream>
#include <utility>

// Receptionist Class
// Manages two responsibilities:
//
//   1. FIFO Pending Queue (backed by Queue):
//      Patients who selected "Other" are enqueued here in
//      strict arrival order. The receptionist dequeues them
//      one by one, assigns a Category based on visual triage,
//      then routes them into the correct Clinic's PriorityQueue.
//      The Queue class owns the linked-list logic — Receptionist
//      simply calls enqueue/dequeue/display on it.
//
//   2. Triage:
//      assignUrgencyAndRoute() sets the patient's Condition,
//      computes their priority score, and calls
//      clinic->enqueuePatient(). Until this step the patient
//      has priorityScore = -1 and is NOT in any PriorityQueue.
//
// This design directly demonstrates FIFO vs. Priority Queue
// comparison as required by the challenge section.
class Receptionist : public User {
private:
    Queue              pendingQueue;   // FIFO queue — no custom linked list needed
    std::pair<int,int> workingHours;
    bool               onDuty;

public:
    Receptionist(int id, const std::string& name,
                 std::pair<int,int> hours = {7, 20})
        : User(id, name, UserRole::RECEPTIONIST),
          workingHours(hours), onDuty(true)
    {}

    // Destructor is default — Queue's own destructor cleans up nodes.

    // ---- FIFO Pending Queue -------------------------------------

    // Called when patient picks "Other" — O(1)
    void receivePendingPatient(Patient* p) {
        pendingQueue.enqueue(p);
        std::cerr << "  [Receptionist " << name << "] Received pending patient: "
                  << p->getName() << " (ID " << p->getId() << ")\n";
    }

    // Peek at next patient without removing — O(1)
    Patient* peekNextPending() const {
        return pendingQueue.peek();
    }

    // Remove and return next pending patient — O(1)
    Patient* dequeuePending() {
        return pendingQueue.dequeue();
    }

    // ---- Triage -------------------------------------------------
    // Dequeues the next pending patient, assigns a Category and
    // estimated treatment time, then routes them to their Clinic.
    // Returns the triaged patient, or nullptr if queue is empty.
    Patient* assignUrgencyAndRoute(UrgencyLevel chosenLevel,
                                   int estimatedTreatmentMinutes,
                                   Clinic* clinic) {
        Patient* p = dequeuePending();
        if (!p) {
            return nullptr;
        }
        Condition c("Other (assessed)", chosenLevel, estimatedTreatmentMinutes);
        p->setCondition(c);
        clinic->enqueuePatient(p);
        std::cerr << "  [Receptionist " << name << "] Triaged " << p->getName()
                  << " → " << c.category.name
                  << " | Routed to " << clinic->getName() << "\n";
        return p;
    }

    // ---- Accessors ----------------------------------------------
    int  getPendingCount() const { return pendingQueue.getSize(); }
    bool hasPending()      const { return !pendingQueue.isEmpty(); }
    bool isOnDuty()        const { return onDuty; }
    void setOnDuty(bool v)       { onDuty = v; }

    void displayPendingQueue() const {
        std::cout << "  [Receptionist " << name << "] Pending queue ("
                  << pendingQueue.getSize() << "):\n";
        pendingQueue.display();
    }

    void displayInfo() const override {
        std::cout << "+-----------------------------------------+\n";
        std::cout << "  [Receptionist]\n";
        std::cout << "  ID          : " << id << "\n";
        std::cout << "  Name        : " << name << "\n";
        std::cout << "  Shift       : " << workingHours.first << ":00 - "
                  << workingHours.second << ":00\n";
        std::cout << "  On Duty     : " << (onDuty ? "Yes" : "No") << "\n";
        std::cout << "  Pending     : " << pendingQueue.getSize() << "\n";
        std::cout << "+-----------------------------------------+\n";
    }
};

#endif
