#include "Queue.h"

// Constructor
// Initializes an empty queue
Queue::Queue() {
    front = nullptr;
    rear = nullptr;
    size = 0;
}

// Enqueue
// Adds a patient to the rear of the queue
// O(1) operation
void Queue::enqueue(int id, int priorityScore, int arrivalTime) {
    Patient* newPatient = new Patient(id, priorityScore, arrivalTime);

    if (isEmpty()) {
        front = rear = newPatient; // first element
    }
    else {
        rear->next = newPatient;  // link new patient
        rear = newPatient;        // update rear pointer
    }
    size++;
}

// Dequeue
// Removes the front patient from the queue
// O(1) operation
// Handles empty queue safely
void Queue::dequeue() {
    if (isEmpty()) {
        cout << "Queue is empty, cannot dequeue\n";
        return;
    }

    Patient* temp = front;
    front = front->next; // move front pointer
    delete temp;         // free memory
    size--;

    if (front == nullptr) { // queue became empty
        rear = nullptr;
    }
}

// Peek
// Returns pointer to front patient without removing
// O(1) operation
Patient* Queue::peek() {
    if (isEmpty()) {
        cout << "Queue is empty\n";
        return nullptr;
    }
    return front;
}

// isEmpty
bool Queue::isEmpty() {
    return front == nullptr;
}


// getSize

int Queue::getSize() {
    return size;
}


// Display
// Prints all patients in queue in order (front -> rear)
// O(n) operation
void Queue::display() {
    if (isEmpty()) {
        cout << "Queue is empty\n";
        return;
    }

    Patient* temp = front;
    while (temp != nullptr) {
        cout << "ID: " << temp->id
            << " | Priority: " << temp->priorityScore
            << " | Arrival: " << temp->arrivalTime << endl;
        temp = temp->next;
    }
    cout << "END\n";
}

// Destructor
// Frees all memory to prevent leaks
// O(n) operation
Queue::~Queue() {
    while (!isEmpty()) {
        dequeue();
    }
}