#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <vector>
#include <iostream>
#include <utility>
#include "Patient.h"

// ============================================================
// PriorityQueue (Max-Heap) of Patient*
//
// Ordering:
//   Higher Patient::getPriorityScore() wins.
// Notes:
//   - Patient::computePriority() already encodes urgency + FIFO + aging.
//   - This queue does NOT own Patient* memory.
// ============================================================
class PriorityQueue {
private:
    std::vector<Patient*> heap;
    int capacity;

    static bool higher(Patient* a, Patient* b) {
        if (!a) return false;
        if (!b) return true;
        return a->getPriorityScore() > b->getPriorityScore();
    }

    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (!higher(heap[i], heap[p])) break;
            std::swap(heap[i], heap[p]);
            i = p;
        }
    }

    void siftDown(int i) {
        int n = (int)heap.size();
        while (true) {
            int l = 2 * i + 1;
            int r = 2 * i + 2;
            int best = i;
            if (l < n && higher(heap[l], heap[best])) best = l;
            if (r < n && higher(heap[r], heap[best])) best = r;
            if (best == i) break;
            std::swap(heap[i], heap[best]);
            i = best;
        }
    }

    void heapify() {
        for (int i = (int)heap.size() / 2 - 1; i >= 0; --i) {
            siftDown(i);
        }
    }

public:
    explicit PriorityQueue(int cap = 64) : capacity(cap) {
        heap.reserve((size_t)cap);
    }

    bool isEmpty() const { return heap.empty(); }
    int  getSize() const { return (int)heap.size(); }
    const std::vector<Patient*>& items() const { return heap; }

    Patient* peek() const {
        if (heap.empty()) return nullptr;
        return heap[0];
    }

    void insert(Patient* p) {
        if ((int)heap.size() >= capacity) {
            std::cerr << "[PriorityQueue] Queue is full!\n";
            return;
        }
        heap.push_back(p);
        siftUp((int)heap.size() - 1);
    }

    Patient* extractMax() {
        if (heap.empty()) return nullptr;
        Patient* top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) siftDown(0);
        return top;
    }

    // Applies one aging tick to all waiting patients and rebuilds heap
    // to reflect updated priorityScore.
    void applyAgingAndRebuild() {
        for (Patient* p : heap) {
            if (p) p->incrementAging();
        }
        heapify();
    }

    void display() const {
        if (heap.empty()) {
            std::cout << "  (empty)\n";
            return;
        }
        // Print in heap order (not sorted) — good enough for debugging.
        for (Patient* p : heap) {
            if (!p) continue;
            std::cout << "  - ID " << p->getId()
                      << " | " << p->getName()
                      << " | pr=" << p->getPriorityScore()
                      << " | clinic=" << p->getClinicId()
                      << "\n";
        }
    }
};

#endif

