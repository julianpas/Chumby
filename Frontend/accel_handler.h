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
  
  static int GetX(int packed) { return (packed >> 16); }
  static int GetY(int packed) { return ((packed & 0xff00) >> 8); }
  static int GetZ(int packed) { return (packed & 0xff); }
private:
  static void* InputThread(void* instance);

  MessageLoop* message_loop_;

  pthread_t input_thread_;
};

