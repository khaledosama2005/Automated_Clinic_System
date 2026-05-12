#include <iostream>
using namespace std;

// stores info about each patient
struct Patient
{
    int id;            // unique identifier
    int priorityScore; // higher value = higher priority
};

// max heap based priority queue
// the patient with the highest priority score is always processed first
class PriorityQueue
{
private:
    Patient *arr; // array to store the heap
    int capacity; // max number of patients
    int size;     // current number of patients

    int parent(int i)     { return (i - 1) / 2; }
    int leftChild(int i)  { return (2 * i) + 1; }
    int rightChild(int i) { return (2 * i) + 2; }

    // returns true if a has higher priority than b
    bool hasHigherPriority(Patient a, Patient b)
    {
        return a.priorityScore > b.priorityScore;
    }

    // moves a newly inserted patient up to its correct position
    void fixUp(int i)
    {
        while (i > 0 && hasHigherPriority(arr[i], arr[parent(i)]))
        {
            Patient temp   = arr[i];
            arr[i]         = arr[parent(i)];
            arr[parent(i)] = temp;
            i = parent(i);
        }
    }

    // moves the root patient down to its correct position after removal
    void fixDown(int i)
    {
        while (true)
        {
            int left         = leftChild(i);
            int right        = rightChild(i);
            int highestIndex = i;

            if (left < size && hasHigherPriority(arr[left], arr[highestIndex]))
                highestIndex = left;
            if (right < size && hasHigherPriority(arr[right], arr[highestIndex]))
                highestIndex = right;

            if (highestIndex != i)
            {
                Patient temp      = arr[i];
                arr[i]            = arr[highestIndex];
                arr[highestIndex] = temp;
                i = highestIndex;
            }
            else
                break;
        }
    }

public:
    PriorityQueue(int cap)
    {
        capacity = cap;
        arr      = new Patient[capacity];
        size     = 0;
    }

    ~PriorityQueue() { delete[] arr; }

    // inserts a patient into the queue
    void enqueue(Patient p)
    {
        if (size == capacity) { cout << "Queue is full!\n"; return; }
        arr[size++] = p;
        fixUp(size - 1);
    }

    // removes and returns the patient with the highest priority
    Patient dequeue()
    {
        if (size == 0) { cout << "Queue is empty!\n"; return {-1, -1}; }
        Patient top = arr[0];
        arr[0] = arr[--size];
        fixDown(0);
        return top;
    }

    // returns true if the queue is empty
    bool isEmpty() { return size == 0; }
};