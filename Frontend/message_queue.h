#pragma once

#include <queue>

#include <linux/input.h>
#include <pthread.h>

class MessageObserver{
 public:
  ~MessageObserver() {}; 
  virtual void OnEvent() = 0;
};

class MessageQueue {
 public:
  MessageQueue();
  ~MessageQueue();
   
  void PostMessage(const struct input_event& ev);
  struct input_event* PeekMessage();
  struct input_event PokeMessage();
  
  void AddObserver(MessageObserver* observer);
 private:
  pthread_mutex_t events_lock_;
  std::queue<struct input_event> events_;
  
  MessageObserver* observer_;
};
