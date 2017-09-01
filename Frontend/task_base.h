#pragma once

#include <string>

#include "event_handler_interface.h"

class EventManager;

class TaskBase : public EventHandlerInterface {
 public:
  TaskBase(EventManager* event_manager);
  virtual ~TaskBase();
  
  virtual std::string GetName() = 0;

  bool Focus();
  bool HasFocus();
  
  virtual bool OnEventReceived(const input_event& ev);
  
  virtual void OnLooseFocus(TaskBase* new_focused_task);
  virtual void OnReceiveFocus();
 protected:
  EventManager* event_manager_;
};
