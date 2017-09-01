#pragma once

#include <pthread.h>

#include "task_base.h"

class TouchScreenController : public TaskBase {
 public:
  TouchScreenController(EventManager* event_manager);
  ~TouchScreenController();

  bool IsTouching() { return touched_; }
  int GetX() { return current_x_; }
  int GetY() { return current_y_; }
  
  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
 private:
  int current_x_;
  int current_y_;
  int touched_;
};
