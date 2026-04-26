#include "PriorityQueue.h"
#include "Patient.h"
#include <iostream>
#include <iomanip>

// ── higherPriority ────────────────────────────────────────────
// Defined here (not in header) because Patient is a forward-declared
// incomplete type in PriorityQueue.h — we need the full definition.
bool PriorityQueue::higherPriority(int a, int b) const {
    return arr[a]->getPriorityScore() > arr[b]->getPriorityScore();
}

// ── updatePriority ────────────────────────────────────────────
// O(n) scan + O(log n) fix — acceptable per project spec
bool PriorityQueue::updatePriority(int patientId, int newScore) {
    for (int i = 0; i < size; ++i) {
        if (arr[i]->getId() == patientId) {
            // Patient::computePriority() must already be called by the caller
            // We just need to re-heapify from position i
            fixUp(i);
            fixDown(i);
            return true;
        }
    }
    return false;
}

// ── applyAgingAndRebuild ──────────────────────────────────────
// O(n): increment aging on every waiting patient, then heapify
void PriorityQueue::applyAgingAndRebuild() {
    for (int i = 0; i < size; ++i) {
        arr[i]->incrementAging();   // updates priorityScore inside
    }
    // Rebuild heap (Floyd's heapify — O(n))
    for (int i = size / 2 - 1; i >= 0; --i) {
        fixDown(i);
    }
}

// ── display ───────────────────────────────────────────────────
void PriorityQueue::display() const {
    if (size == 0) {
        std::cout << "    (empty)\n";
        return;
    }
    std::cout << std::left
              << std::setw(6)  << "  Pos"
              << std::setw(6)  << "ID"
              << std::setw(20) << "Name"
              << std::setw(14) << "Urgency"
              << std::setw(10) << "Score"
              << std::setw(8)  << "Aging"
              << std::setw(10) << "Arrived"
              << "\n";
    std::cout << "  " << std::string(68, '-') << "\n";
    for (int i = 0; i < size; ++i) {
        Patient* p = arr[i];
        std::cout << std::left
                  << "  " << std::setw(4)  << (i + 1)
                  << std::setw(6)  << p->getId()
                  << std::setw(20) << p->getName()
                  << std::setw(14) << p->getCondition().category.name
                  << std::setw(10) << p->getPriorityScore()
                  << std::setw(8)  << p->getAgingCounter()
                  << std::setw(10) << p->getArrivalTime()
                  << "\n";
    }
}
