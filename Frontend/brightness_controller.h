#pragma once

#include <pthread.h>

#include "task_base.h"

class BrightnessController : public TaskBase {
 public:
  BrightnessController(EventManager* event_manager);
  ~BrightnessController();

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  
  int GetBrightnessLevel();
  void SetBrightnessLevel(int level);
 private:
  int backlight_file_; 
};
