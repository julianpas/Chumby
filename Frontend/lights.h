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
   Lights(EventManager* event_manager, const std::string& level);

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
  virtual void OnLooseFocus(TaskBase* new_focused_task);

  static void* DataThread(void* data);

 private:
  class LightDef {
   public:
    LightDef(int x, int y, int image, const std::string& name)
      : x(x), y(y), image(image), name(name), instance(NULL), button(NULL) {}
    int x;
    int y;
    int image;
    std::string name;
    Lights* instance;
    Button* button;
  };

  void DrawUI();
  
  void ReadSettings(const std::string& filename);
  void GetHueLights();

  static void GetLights(Lights* self);

  static bool OnButton(void* data);
  static bool OnClock(void* data);
  static bool OnOG(void* data);
  static bool OnEG(void* data);
  static bool OnUG(void* data);
  
  pthread_t data_thread_;
  TouchScreenController* touch_controller_;

  pthread_mutex_t data_lock_;

  std::vector<LightDef> lights_;
  std::vector<Button*> buttons_;
  Button* clock_;
  Button* ug_;
  Button* eg_;
  Button* og_;

  std::string task_name_;
  std::string image_file_;
  
  std::auto_ptr<TcpConnection> connection_;
};
