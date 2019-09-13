#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <string.h>

class Queue  {  
public: 
  Queue() {
    numUsedSlots = 0;
    head = 0;
    tail = 0;
  }

  ~Queue() {}

  // Returns nullptr if queue is empty.
  const char* pop() {
    // Is empty?
    if (numUsedSlots == 0) { 
      return nullptr;
    }

    char* string = store[head];
    head++;
    head %= kNumSlots;
    numUsedSlots--;
    return string;
  }

  // Returns nullptr if queue is empty.
  const char* peek() {
    // Is empty?
    if (numUsedSlots == 0) { 
      return nullptr;
    }
    char* string = store[head];
    return string;
  }

  // Returns false if queue is full. 
  bool push(const char* string) {
    // Is full?
    if (numUsedSlots == kNumSlots) {
      return false;
    }

    // Write string in slot.
    store[tail][0] = '\0';
    strncat(store[tail], string, kSlotSize -1);
    

    // More forward.
    tail++;
    tail %= kNumSlots;
    numUsedSlots++;

    return true;
  }

  void flush() {
    numUsedSlots = 0;
    head = 0;
    tail = 0;
  }

  size_t getNumFreeSlots() {
    return kNumSlots - numUsedSlots;
  }

  size_t getNumUsedSlots() {
    return numUsedSlots;
  }

  static constexpr size_t kSlotSize = 160; // adjusted to size of url params to store in the queue
  static constexpr size_t kNumSlots = 60; // ajusted to leave 50% of memory left on ESP8266
private:
  char store[kNumSlots][kSlotSize];

  size_t numUsedSlots;
  size_t head;
  size_t tail;
};

#endif