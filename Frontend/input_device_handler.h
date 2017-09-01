#pragma once

#include <string>

#include <linux/input.h>
#include <pthread.h>

#include "message_loop_client.h"

class MessageLoop;

const int MAX_EVENTS = 16;
const int EVENT_STRUCT_SIZE = (int)sizeof(struct input_event);

class InputDeviceHandler : public MessageLoopClient {
 public:
  InputDeviceHandler(MessageLoop* message_loop, const std::string& input_device_file);

  virtual void OnLoopQuitting();
 private:
  static void* InputThread(void* instance);

  MessageLoop* message_loop_;

  std::string input_device_file_;
  int input_device_fd_;  

  pthread_t input_thread_;

  struct input_event events_[MAX_EVENTS];
};

