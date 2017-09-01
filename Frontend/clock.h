#pragma once

#include <auto_ptr.h>
#include <pthread.h>

#include "screen.h"
#include "task_base.h"
#include "touch_screen_controller.h"

class Clock : public TaskBase {
 public:
  Clock(EventManager* event_manager);

  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
  virtual void OnLooseFocus(TaskBase* new_focused_task);

  static void* ClockThread(void* data);
  static void* TempThread(void* data);

 private:
  void SaveSettings();
  void ReadSettings();

  void DrawNightMode();
  void DrawNet();
  void DrawAlarm();
  void DrawAlarmTime();

  pthread_t clock_thread_;
  pthread_t temp_thread_;
  TouchScreenController* touch_controller_;

  pthread_mutex_t data_lock_;

  bool active_;
  bool alarm_active_;
  bool alarm_snoozed_;
  bool setting_hours_;
  bool setting_min_;
  int alarm_hour_;
  int alarm_min_;

  bool night_mode_;
  int normal_brightness_;

  time_t last_tilt_;
  bool tilting_;
  bool tilting_up_;
  bool tilting_down_;
  int axis_;

  int temp_;
  time_t last_temp_;

  bool animate_mario_;
  ScreenBuffer* mario_;
};
