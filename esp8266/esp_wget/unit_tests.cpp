#include "queue.h"
#include "stdio.h"

bool testPushPop() {
  const char* string = "thisis a test string";
  Queue queue;
  queue.push(string);
  const char* poped = queue.pop();
  bool failed = strcmp(poped, string);
  if (failed) {
  	printf("testPushPop failed.\n");
  	return false;
  }
  return true;
}

bool testMorePushThanPop() {
  Queue queue;
  queue.push("a");
  queue.push("a");
  queue.push("a");
  queue.pop();
  if (queue.getNumUsedSlots() != 2) {
  	printf("testMorePushThanPop failed.\n");
  	return false;
  }
  return true;
}

bool testWrapAround() {
  Queue queue;
  queue.push("a");
  queue.pop();
  for(size_t i = 0 ; i < Queue::kNumSlots ; i ++) {
  	bool success = queue.push("a");
  	if (!success) {
  	  printf("testWrapAround failed.\n");
  	  return false;
  	}
  }

  if (queue.getNumFreeSlots() != 0) {
  	printf("testMorePushThanPop failed.\n");
  	return false;
  }

  return true;
}

bool testPopOnEmpty() {
  Queue queue;
  const char* string = queue.pop();
  if (string != nullptr) {
  	printf("testPopOnEmpty failed.\n");
  	return false;
  }
  return true;
}

bool testPushOnFull() {
  Queue queue;
  for(size_t i = 0 ; i < Queue::kNumSlots ; i++) {
  	queue.push("a");
  }

  if (queue.getNumFreeSlots() != 0) {
  	printf("testPushOnFull1 failed.\n");
  	return false;
  }

  if (queue.getNumUsedSlots() != Queue::kNumSlots) {
  	printf("testPushOnFull2 failed.\n");
  	return false;
  }

  if (queue.push("a")) {
  	printf("testPushOnFull3 failed.\n");
  	return false;
  }
  return true;	
}

int main(int argc, char** argv) {
	printf("Unit test <queue.h> starting.\n");
	bool success = true;
    success &= testPushPop();
    success &= testMorePushThanPop();
    success &= testWrapAround();
    success &= testPopOnEmpty();
    success &= testPushOnFull();

    if (success) {
    	printf("All tests succeedeed.\n");
    	return 0;
    }
	return 1;
}