#pragma once

#include <list>
#include <map>
#include <string>

#include <linux/input.h>

#include "event_handler_interface.h"

class MessageLoop;
class TaskBase;

class EventManager : public EventHandlerInterface {
 public:
  EventManager(MessageLoop* message_loop);
  ~EventManager();

  void AddTask(TaskBase* task);
  void RemoveTask(TaskBase* task);
  void SetActiveTask(TaskBase* task);
  TaskBase* GetActiveTask();
  TaskBase* GetTask(std::string task);
  
  virtual bool OnEventReceived(const input_event& ev);
 private:
   MessageLoop* message_loop_;
   std::list<TaskBase*> tasks_;
   std::map<std::string, TaskBase*> tasksmap_;
};
