#pragma once

#include <string>

#include <pthread.h>

#include "message_loop_client.h"

#define EV_ACCEL 0x43
#define ACC_XYZ 0x01

class MessageLoop;

class AccelHandler : public MessageLoopClient {
 public:
  AccelHandler(MessageLoop* message_loop);

  virtual void OnLoopQuitting();
 private:
  static void* InputThread(void* instance);

  MessageLoop* message_loop_;

  pthread_t input_thread_;
};

