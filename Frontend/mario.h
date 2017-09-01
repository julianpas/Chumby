#pragma once

#include <auto_ptr.h>
#include <pthread.h>

#include "screen.h"
#include "task_base.h"
#include "touch_screen_controller.h"
#include "world.h"

class Mario : public TaskBase {
 public:
  Mario(EventManager* event_manager);

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
  virtual void OnLooseFocus(TaskBase* new_focused_task);

  static void* MarioThread(void* data);

 private:
  void SaveSettings();
  void ReadSettings();

  pthread_t mario_thread_;
  World world_;
  TouchScreenController* touch_controller_;
};
