#pragma once

#include <pthread.h>
#include <vector>

#include "screen.h"
#include "task_base.h"
#include "touch_screen_controller.h"

class Button;

class Lights : public TaskBase {
 public:
   Lights(EventManager* event_manager);

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
  virtual void OnLooseFocus(TaskBase* new_focused_task);

  static void* DataThread(void* data);

 private:
  void DrawUI();

  static void GetLights(Lights* self);

  static bool OnButton(void* data);
  
  pthread_t data_thread_;
  TouchScreenController* touch_controller_;

  pthread_mutex_t data_lock_;

  std::vector<Button*> buttons_;
};
