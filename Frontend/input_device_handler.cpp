#include "input_device_handler.h"

#include <iostream>

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "message_loop.h"

InputDeviceHandler::InputDeviceHandler(MessageLoop* message_loop, const std::string& input_device_file) : message_loop_(message_loop), input_device_file_(input_device_file) {
  if ((input_device_fd_ = open(input_device_file_.c_str(), O_RDONLY)) < 0) {
    std::cout << "Device failed to open " << input_device_file << std::endl;
    return;
  }

  message_loop_->AddClient(this);
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&input_thread_, &thread_attr, &InputDeviceHandler::InputThread, static_cast<void*>(this));
}

void InputDeviceHandler::OnLoopQuitting() {
  void* status;
  pthread_cancel(input_thread_);
  pthread_join(input_thread_, &status);

  close(input_device_fd_);
}

// static
void* InputDeviceHandler::InputThread(void* instance) {
  InputDeviceHandler* self = reinterpret_cast<InputDeviceHandler*>(instance);
  
  int retval;
  while (true) {
    retval = read(self->input_device_fd_, self->events_, EVENT_STRUCT_SIZE * MAX_EVENTS);
    if (retval < EVENT_STRUCT_SIZE)
      continue;
    for (int i = 0; i < (retval / EVENT_STRUCT_SIZE); ++i)
      self->message_loop_->PostMessage(self->events_[i]);
  }
}
