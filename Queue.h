#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
using namespace std;

// ---------------------------
// Patient Struct
// ---------------------------
// Represents a patient in the queue
// Each patient has:
// - id: unique identifier
// - priorityScore: for priority handling in future use
// - arrivalTime: time patient arrived, used for FIFO order
// - next: pointer to next patient in linked list
struct Patient {
    int id;                 // Patient ID
    int priorityScore;      // Patient priority (higher = more urgent)
    int arrivalTime;        // Patient arrival time for FIFO
    Patient* next;          // Pointer to next patient in queue

    // Constructor
    Patient(int id, int priorityScore, int arrivalTime) {
        this->id = id;
        this->priorityScore = priorityScore;
        this->arrivalTime = arrivalTime;
        this->next = nullptr;
    }
};

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
    Patient* front;  // Pointer to first patient
    Patient* rear;   // Pointer to last patient
    int size;        // Number of patients in queue

public:
    Queue();  // Constructor

    // Enqueue a new patient to the rear of the queue
    // O(1) operation
    void enqueue(int id, int priorityScore, int arrivalTime);

    // Dequeue the front patient
    // O(1) operation
    void dequeue();

    // Peek at front patient without removing
    // O(1) operation
    Patient* peek();

    // Check if the queue is empty
    bool isEmpty();

    // Get number of patients in queue
    int getSize();

    // Display all patients in queue (front -> rear)
    void display();

    // Destructor frees all nodes to prevent memory leaks
    ~Queue();
};

#endif