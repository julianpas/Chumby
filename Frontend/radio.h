#pragma once

#include <auto_ptr.h>
#include <pthread.h>

#include "button.h"
#include "easybmp/EasyBMP.h"
#include "screen.h"
#include "task_base.h"
#include "touch_screen_controller.h"

class Radio : public TaskBase {
 public:
  Radio(EventManager* event_manager);
  
  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();

 private:
  TouchScreenController* touch_controller_;
  
  void SetVolume();
  void DrawVolume();

  static bool OnButton(void* data);
  static bool OnClock(void* data);
  
  bool selected_radio_;
  int volume_;
  int previous_volume_;
  std::auto_ptr<ScreenBuffer> volume_bar_;
  std::auto_ptr<ScreenBuffer> volume_bar_off_;
  
  ButtonDef button_defs_[4];
  std::auto_ptr<Button> clock_;
};
