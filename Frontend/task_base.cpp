#include "task_base.h"

#include <iostream>

#include "event_manager.h"

TaskBase::TaskBase(EventManager* event_manager) 
    : event_manager_(event_manager) {
}

TaskBase::~TaskBase() {
  event_manager_->RemoveTask(this);
}

bool TaskBase::Focus() {
  event_manager_->SetActiveTask(this);
  return true;
}

bool TaskBase::HasFocus() {
  return event_manager_->GetActiveTask() == this;
}

bool TaskBase::OnEventReceived(const input_event& ev) {
  std::cout << GetName() << " silently dropped event " << ev.type << std::endl;
  return false;
}

void TaskBase::OnLooseFocus(TaskBase* new_focused_task) {
}

void TaskBase::OnReceiveFocus() {
}

