#pragma once

#include <linux/input.h>

class EventHandlerInterface {
public:
  ~EventHandlerInterface() {}
  virtual bool OnEventReceived(const input_event& ev) = 0;
};
