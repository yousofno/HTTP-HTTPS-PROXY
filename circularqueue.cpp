#include "circularqueue.h"

CircularQueue::CircularQueue() : head(0), tail(0), size(0) {}

void CircularQueue::enqueue(const char* str) {
    // Copy the string to the buffer, overwriting if necessary
    std::strncpy(buffer[tail], str, StringLength - 1);
    buffer[tail][StringLength - 1] = '\0'; // Ensure null termination

    // Move the tail forward
    tail = (tail + 1) % Capacity;

    // If the buffer is full, move the head forward to overwrite
    if (size >= Capacity) {
        head = (head + 1) % Capacity; // Overwrite the oldest element
        size = 0;
    } else {
        size++;
    }
}

const char* CircularQueue::dequeue() {
    if (size == 0) {
        throw std::underflow_error("Queue is empty");
    }

    const char* result = buffer[head];
    head = (head + 1) % Capacity;
    size--;
    return result; // Note: Caller should not delete this pointer
}

bool CircularQueue::isEmpty() const {
    return size == 0;
}

bool CircularQueue::isFull() const {
    return size == Capacity;
}
