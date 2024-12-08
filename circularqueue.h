#ifndef CIRCULARQUEUE_H
#define CIRCULARQUEUE_H
#include <QString>
#include <iostream>
#include <stdexcept>
#include <QDebug>

// C++ program to implement the circular queue using array
#include <bits/stdc++.h>

// defining the max size of the queue
#define Capacity 100
#define StringLength 500


// class that represents queue
class CircularQueue {
public:
    CircularQueue();
    void enqueue(const char* str);
    const char* dequeue() ;
    bool isEmpty() const;
    bool isFull() const;

private:
    char buffer[Capacity][StringLength]; // Fixed-size buffer for strings
    size_t head;                          // Index of the head of the buffer
    size_t tail;                          // Index of the tail of the buffer
    size_t size;                          // Current number of items in the buffer
};

#endif // CIRCULARQUEUE_H
