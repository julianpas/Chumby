#pragma once

#include <auto_ptr.h>
#include <linux/input.h>
#include <pthread.h>

#include "message_queue.h"

class EventManager;
class MessageLoopClient;

class MessageLoop : public MessageObserver {
 public:
  MessageLoop(std::auto_ptr<MessageQueue> queue);
  void Run();
  void Stop();

  void PostMessage(const struct input_event& ev);

  void AddClient(MessageLoopClient* client);
  
  void SetEventManager(EventManager* event_manager);
  
  virtual void OnEvent();
 private:
  static void* RunInternal(void* message_loop_ptr); 
   
  std::auto_ptr<MessageQueue> queue_;
  
  pthread_mutex_t event_present_lock_;
  pthread_cond_t event_present_condition_;

  pthread_t message_loop_thread_;
  
  EventManager* event_manager_;

  std::queue<MessageLoopClient*> clients_;

  static const int ML_TYPE = 0x42;
  static const int ML_QUIT = 0x01;
};
