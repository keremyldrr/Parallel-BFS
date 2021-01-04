
#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>

using namespace std;


class Queue {
private:
    int N;
    int *arr;
    int front;
    int end;

public:
    Queue(int n);
    ~Queue();
    void enqueue(int i);
    int dequeue();
    bool isEmpty();
};

Queue::Queue(int n) {
    N = n;
    arr = new int[N];
    front = -1;
    end = -1;
    for(int i = 0; i < N; i++)
        arr[i] = -1;
}

Queue::~Queue() {
    delete arr;
}

void Queue::enqueue(int i) {
    if(isEmpty()) {
        front = 0;
        end = 0;
        arr[end] = i;
        return;
    }

    end = (end+1) % N;
    arr[end] = i;
    return;
}

int Queue::dequeue() {
    if(isEmpty()) {
        cout << "Empty boi" << endl;
        return -1;
    }

    int num = arr[front];
    arr[front] = -1;
    if(front == end) {
        front = -1;
        end = -1;
    }
    else
        front = (front+1)%N;

    return num;
}

bool Queue::isEmpty() {
    return front==-1 && end==-1;
}



/*struct qnode {
    int id;
    qnode *next;

    qnode(int i, qnode *ptr=nullptr): id(i), next(ptr) {};
};

class Queue {
private:
    qnode *head;
    qnode *tail;

public:
    Queue();
    ~Queue();
    void enqueue(int i);
    int dequeue();
    bool isEmpty();
    void deleteall();
};

Queue::Queue() {
    head = nullptr;
    tail = nullptr;
}

Queue::~Queue() {
    tail = nullptr;
    while(head != nullptr) {
        qnode *ptr = head;
        head = head->next;
        delete ptr;
    }
}

void Queue::enqueue(int i) {
    if(isEmpty()) {
        head = new qnode(i,nullptr);
        tail = head;
        return;
    }

    tail->next = new qnode(i,nullptr);
    tail = tail->next;
}

int Queue::dequeue() {
    if(isEmpty())
        return -1;
    if(head == tail) {// 1 element
        int num = head->id;
        tail = nullptr;
        qnode *ptr = head;
        head = nullptr;
        delete ptr;
        return num;
    }

    int num = head->id;
    qnode *ptr = head;
    head = head->next;
    delete ptr;
    return num;
}

bool Queue::isEmpty() {
    return head == nullptr;
}

void Queue::deleteall() {
    tail = nullptr;
    while(head != nullptr) {
        qnode *ptr = head;
        head = head->next;
        delete ptr;
    }
    head = nullptr;
} */




#endif
