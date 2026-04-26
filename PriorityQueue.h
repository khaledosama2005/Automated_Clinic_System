#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

// ============================================================
// PriorityQueue.h — Max-Heap Priority Queue for Patient*
// Operates on Patient* pointers using Patient::getPriorityScore()
// Insert:      O(log n)
// ExtractMax:  O(log n)
// UpdateKey:   O(n) find + O(log n) fix = O(n) — acceptable per spec
// Peek:        O(1)
// AgingRebuild:O(n) — called once per tick
// ============================================================

// Forward declare to break circular include (Patient.h → Condition.h → Category.h)
class Patient;

class PriorityQueue {
private:
    Patient** arr;
    int       capacity;
    int       size;

    int parent(int i)     const { return (i - 1) / 2; }
    int leftChild(int i)  const { return 2 * i + 1; }
    int rightChild(int i) const { return 2 * i + 2; }

    bool higherPriority(int a, int b) const;   // defined in PriorityQueue.cpp

    void fixUp(int i) {
        while (i > 0 && higherPriority(i, parent(i))) {
            swap(i, parent(i));
            i = parent(i);
        }
    }

    void fixDown(int i) {
        while (true) {
            int best = i;
            int l = leftChild(i), r = rightChild(i);
            if (l < size && higherPriority(l, best)) best = l;
            if (r < size && higherPriority(r, best)) best = r;
            if (best == i) break;
            swap(i, best);
            i = best;
        }
    }

    void swap(int a, int b) {
        Patient* tmp = arr[a];
        arr[a] = arr[b];
        arr[b] = tmp;
    }

    void grow() {
        capacity *= 2;
        Patient** newArr = new Patient*[capacity];
        for (int i = 0; i < size; ++i) newArr[i] = arr[i];
        delete[] arr;
        arr = newArr;
    }

public:
    explicit PriorityQueue(int cap = 64)
        : capacity(cap), size(0) {
        arr = new Patient*[capacity];
    }

    ~PriorityQueue() { delete[] arr; }

    // Insert patient — O(log n)
    void insert(Patient* p) {
        if (size == capacity) grow();
        arr[size++] = p;
        fixUp(size - 1);
    }

    // Remove and return highest-priority patient — O(log n)
    Patient* extractMax() {
        if (size == 0) return nullptr;
        Patient* top = arr[0];
        arr[0] = arr[--size];
        if (size > 0) fixDown(0);
        return top;
    }

    // Update priority of patient with given id — O(n)
    // Needed for dynamic priority changes (challenge section)
    bool updatePriority(int patientId, int newScore);  // defined in PriorityQueue.cpp

    // Peek without removing — O(1)
    Patient* peek() const { return size > 0 ? arr[0] : nullptr; }

    bool isEmpty()  const { return size == 0; }
    int  getSize()  const { return size; }

    // Called each simulation tick — increments aging on all patients
    // then rebuilds heap to reflect updated priority scores — O(n)
    void applyAgingAndRebuild();   // defined in PriorityQueue.cpp

    // Display all entries (for debugging/demo)
    void display() const;          // defined in PriorityQueue.cpp
};

#endif
