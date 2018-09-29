#pragma once

#include <auto_ptr.h>
#include <pthread.h>
#include <vector>

#include "screen.h"
#include "task_base.h"
#include "tcp_connection.h"
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
  
  void GetHueLights();
  void GetJulLights();

  static void GetLights(Lights* self);

  static bool OnButton(void* data);
  static bool OnClock(void* data);
  
  pthread_t data_thread_;
  TouchScreenController* touch_controller_;

  pthread_mutex_t data_lock_;

  std::vector<Button*> buttons_;
  Button* clock_;
  
  std::auto_ptr<TcpConnection> connection_;
};
