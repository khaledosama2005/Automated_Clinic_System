#include "Queue.h"
#include "Patient.h"
#include <iostream>
#include <iomanip>

Queue::Queue() : front(nullptr), rear(nullptr), size(0) {}

Queue::~Queue() {
    while (front) {
        QNode* tmp = front->next;
        delete front;
        front = tmp;
    }
}

void Queue::enqueue(Patient* p) {
    QNode* node = new QNode(p);
    if (!rear) { front = rear = node; }
    else        { rear->next = node; rear = node; }
    ++size;
}

Patient* Queue::dequeue() {
    if (!front) return nullptr;
    Patient* p   = front->data;
    QNode*   tmp = front;
    front = front->next;
    if (!front) rear = nullptr;
    delete tmp;
    --size;
    return p;
}

Patient* Queue::peek() const { return front ? front->data : nullptr; }
bool     Queue::isEmpty()    const { return front == nullptr; }
int      Queue::getSize()    const { return size; }

void Queue::display() const {
    if (isEmpty()) { std::cout << "    (empty)\n"; return; }
    std::cout << std::left
              << std::setw(6)  << "  Pos"
              << std::setw(6)  << "ID"
              << std::setw(20) << "Name"
              << std::setw(10) << "Arrived\n";
    std::cout << "  " << std::string(40,'-') << "\n";
    QNode* cur = front; int pos = 1;
    while (cur) {
        Patient* p = cur->data;
        std::cout << "  " << std::setw(4) << pos++
                  << std::setw(6)  << p->getId()
                  << std::setw(20) << p->getName()
                  << std::setw(10) << p->getArrivalTime() << "\n";
        cur = cur->next;
    }
}
