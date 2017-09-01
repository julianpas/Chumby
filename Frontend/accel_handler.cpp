#include "accel_handler.h"

#include <iostream>

#include <limits.h>

#include "accel/accel.h"
#include "message_loop.h"

AccelHandler::AccelHandler(MessageLoop* message_loop) : message_loop_(message_loop) {
  init_acc();
  
  message_loop_->AddClient(this);
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&input_thread_, &thread_attr, &AccelHandler::InputThread, static_cast<void*>(this));
}

void AccelHandler::OnLoopQuitting() {
  void* status;
  pthread_cancel(input_thread_);
  pthread_join(input_thread_, &status);

  close_acc();
}

// static
void* AccelHandler::InputThread(void* instance) {
  AccelHandler* self = reinterpret_cast<AccelHandler*>(instance);
  
  int retval;
  while (true) {
    long int msg;
    signed char xyz[3];
    retval = read_acc_xyz(xyz);
    if (retval > 0)
      continue;
    msg = (xyz[0] << 16) + (xyz[1] << 8) + xyz[2];
    
    struct input_event ev;
    ev.type = 0x43;
    ev.code = ACC_XYZ;
    ev.value = msg;
    self->message_loop_->PostMessage(ev);
    
    usleep(50 * 1000);  // 20Hz
  }
}
