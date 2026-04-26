#ifndef QUEUE_H
#define QUEUE_H

// ============================================================
// Queue.h — Linked-list FIFO Queue holding Patient* pointers
// Used by Receptionist for the pending (untriaged) queue.
// Enqueue: O(1)  |  Dequeue: O(1)  |  Peek: O(1)
// ============================================================

// Forward declare — Queue.h must not include Patient.h
// to avoid circular dependency (Patient.h → Condition.h etc.)
class Patient;

struct QNode {
    Patient* data;
    QNode*   next;
    explicit QNode(Patient* p) : data(p), next(nullptr) {}
};

class Queue {
private:
    QNode* front;
    QNode* rear;
    int    size;

public:
    Queue();
    ~Queue();

    void     enqueue(Patient* p);   // O(1)
    Patient* dequeue();             // O(1) — returns nullptr if empty
    Patient* peek() const;          // O(1) — returns nullptr if empty
    bool     isEmpty() const;
    int      getSize() const;
    void     display() const;       // O(n) — prints queue front→rear
};

#endif
