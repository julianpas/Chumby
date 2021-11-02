#include "event_manager.h"

#include <iostream>

#include "message_loop.h"
#include "task_base.h"

EventManager::EventManager(MessageLoop* message_loop) 
    : message_loop_(message_loop) { 
  message_loop->SetEventManager(this);   
};

EventManager::~EventManager() { }

void EventManager::AddTask(TaskBase* task) {
  if (tasks_.empty())
    task->OnReceiveFocus();
  tasks_.push_back(task);
  std::cout << "Adding " << task->GetName() << std::endl;
  tasksmap_.insert(std::pair<std::string, TaskBase*>(task->GetName(), task));
}

void EventManager::RemoveTask(TaskBase* task) {
  bool notify_new_task = false;
  if (!tasks_.empty() && tasks_.front() == task) {
    task->OnLooseFocus(task); // FIXME
    notify_new_task = true;
  }
  tasks_.remove(task);
  if (notify_new_task && !tasks_.empty()) {
    tasks_.front()->OnReceiveFocus();
  }
}

void EventManager::SetActiveTask(TaskBase* task) {
  if (!task) {
    std::cout << "Can't active null task." << std::endl;
    return;
  }
  if (!tasks_.empty() && tasks_.front() != task) {
    tasks_.front()->OnLooseFocus(NULL);
    tasks_.remove(task);
    tasks_.push_front(task);
    task->OnReceiveFocus();
  }
}

TaskBase* EventManager::GetActiveTask() {
  if (!tasks_.empty())
    return tasks_.front();
  return NULL;  
}

TaskBase* EventManager::GetTask(std::string task) {
  if (tasksmap_.find(task) != tasksmap_.end())
    return tasksmap_[task];
  std::cout << "NOT FOUND " << task << std::endl;
  return NULL;
}

bool EventManager::OnEventReceived(const input_event& ev) {
  std::list<TaskBase*>::iterator it = tasks_.begin();
  while(it != tasks_.end()) {
    if ((*it)->OnEventReceived(ev))
      return true;
    it++;
  }
  
  return true;
}
