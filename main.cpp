#include "Queue.h"
#include <iostream>
using namespace std;

int main() {
    Queue q; // Create a FIFO queue

        // Enqueue sample patients
        q.enqueue(1, 50, 10); // ID=1, priority=50, arrival=10
    q.enqueue(2, 50, 11); // ID=2, priority=50, arrival=11
    q.enqueue(3, 50, 12); // ID=3, priority=50, arrival=12

    cout << "--- Queue after enqueue ---\n";
    q.display(); // Expect: 1 -> 2 -> 3

    
    // Dequeue one patient
    
    q.dequeue();
    cout << "--- Queue after 1 dequeue ---\n";
    q.display(); // Expect: 2 -> 3

    // Dequeue another patient
    
    q.dequeue();
    cout << "--- Queue after 2 dequeues ---\n";
    q.display(); // Expect: 3

    
    // Peek at front patient
    
    Patient* p = q.peek();
    if (p != nullptr) {
        cout << "Next patient to process: ID=" << p->id
            << ", Priority=" << p->priorityScore
            << ", Arrival=" << p->arrivalTime << endl;
    }

    
    // Final state of queue
    
    q.dequeue(); // Remove last patient
    cout << "--- Queue after removing all patients ---\n";
    q.display(); // Expect: empty

    return 0;
}