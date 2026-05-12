#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <vector>
#include "Patient.h"

// ---------------------------
// Queue Class (FIFO)
// ---------------------------
// Implements a First-In-First-Out queue using a linked list
// - Enqueue: O(1)
// - Dequeue: O(1)
// - Peek: O(1)
// - Display: O(n)
// - Destructor: O(n)
// Linked list chosen for dynamic memory management and unlimited size
class Queue {
private:
    struct Node {
        Patient* value;
        Node* next;
        explicit Node(Patient* p) : value(p), next(nullptr) {}
    };

    Node* front;  // Pointer to first node
    Node* rear;   // Pointer to last node
    int size;        // Number of patients in queue

public:
    Queue();  // Constructor

    // Enqueue a new patient to the rear of the queue
    // O(1) operation
    void enqueue(Patient* p);

    // Dequeue the front patient
    // O(1) operation
    Patient* dequeue();

    // Peek at front patient without removing
    // O(1) operation
    Patient* peek() const;

    // Check if the queue is empty
    bool isEmpty() const;

    // Get number of patients in queue
    int getSize() const;

    // Display all patients in queue (front -> rear)
    void display() const;

    std::vector<Patient*> toVector() const;

    // Destructor frees all nodes to prevent memory leaks
    ~Queue();
};

#endif