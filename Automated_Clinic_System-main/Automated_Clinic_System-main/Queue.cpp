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
void Queue::enqueue(Patient* p) {
    Node* n = new Node(p);
    if (isEmpty()) {
        front = rear = n;
    } else {
        rear->next = n;
        rear = n;
    }
    ++size;
}

// Dequeue
// Removes the front patient from the queue
// O(1) operation
// Handles empty queue safely
Patient* Queue::dequeue() {
    if (isEmpty()) {
        return nullptr;
    }

    Node* temp = front;
    Patient* out = temp->value;
    front = front->next; // move front pointer
    delete temp;         // free node memory (not the Patient*)
    --size;

    if (front == nullptr) { // queue became empty
        rear = nullptr;
    }
    return out;
}

// Peek
// Returns pointer to front patient without removing
// O(1) operation
Patient* Queue::peek() const {
    if (isEmpty()) {
        return nullptr;
    }
    return front->value;
}

// isEmpty
bool Queue::isEmpty() const {
    return front == nullptr;
}


// getSize

int Queue::getSize() const {
    return size;
}


// Display
// Prints all patients in queue in order (front -> rear)
// O(n) operation
void Queue::display() const {
    if (isEmpty()) {
        std::cout << "Queue is empty\n";
        return;
    }

    Node* temp = front;
    while (temp != nullptr) {
        if (temp->value) {
            std::cout << "ID: " << temp->value->getId()
                      << " | Name: " << temp->value->getName()
                      << " | ClinicId: " << temp->value->getClinicId()
                      << " | Priority: " << temp->value->getPriorityScore()
                      << "\n";
        } else {
            std::cout << "(null patient)\n";
        }
        temp = temp->next;
    }
    std::cout << "END\n";
}

std::vector<Patient*> Queue::toVector() const {
    std::vector<Patient*> out;
    out.reserve((size_t)size);
    Node* cur = front;
    while (cur) {
        out.push_back(cur->value);
        cur = cur->next;
    }
    return out;
}

// Destructor
// Frees all memory to prevent leaks
// O(n) operation
Queue::~Queue() {
    while (front != nullptr) {
        Node* tmp = front;
        front = front->next;
        delete tmp;
    }
    rear = nullptr;
    size = 0;
}