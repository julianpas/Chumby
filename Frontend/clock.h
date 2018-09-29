#pragma once

#include <auto_ptr.h>
#include <pthread.h>

#include "screen.h"
#include "task_base.h"
#include "touch_screen_controller.h"

class Button;
class TcpConnection;

class Clock : public TaskBase {
 public:
  Clock(EventManager* event_manager);

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
  virtual void OnLooseFocus(TaskBase* new_focused_task);

  static void* ClockThread(void* data);
  static void* DataThread(void* data);
  static void* SunlightThread(void*);

 private:
  void SaveSettings();
  void ReadSettings();

  void DrawUI();
  void DrawAlarmTime();
    
  void SetBrightness();
  
  static void GetBedroomTemp(Clock* self, TcpConnection* connection);
  static void GetBedroomLights(Clock* self, TcpConnection* connection);

  static bool OnAlarmButton(void* data);
  static bool OnNightModeButton(void* data);
  static bool OnCeilingLightButton(void* data);
  static bool OnAllLightsButton(void* data);
  static bool OnRadioButton(void* data);
  static bool OnTempButton(void* data);
  static bool OnTvButton(void* data);
  
  static bool InvokeAction(const std::string& action);
  
  pthread_t clock_thread_;
  pthread_t temp_thread_;
  pthread_t sunlight_thread_;
  TouchScreenController* touch_controller_;

  pthread_mutex_t data_lock_;

  bool active_;
  bool force_draw_;

  bool alarm_active_;
  bool alarm_snoozed_;
  bool setting_hours_;
  bool setting_min_;
  int alarm_hour_;
  int alarm_min_;

  bool night_mode_active_;
  int night_mode_observations_;
  int normal_brightness_;
  int night_brightness_;
  std::string reading_light_command_;

  time_t last_tilt_;
  bool tilting_;
  int axis_;

  long last_press_;
  int press_count_;
  
  int temp_;
  int *temps_;
  int *hums_;
  int *measures_;
  int last_temps_;
  time_t last_temps_time_;
  bool show_temps_;
  time_t last_temp_;

  Button* net_button_;
  Button* all_lights_;
  Button* ceiling_light_;
  //Button* alarm_;
  Button* night_mode_;
  Button* radio_;
  Button* temperature_;
  Button* tv_button_;
};
