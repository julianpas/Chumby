#include "message_loop.h"

#include <iostream>

#include <limits.h>

#include "event_manager.h"
#include "message_loop_client.h"

MessageLoop::MessageLoop(std::auto_ptr<MessageQueue> queue) : queue_(queue) {
  pthread_mutex_init(&event_present_lock_, NULL);
  pthread_cond_init(&event_present_condition_, NULL);
  queue_->AddObserver(this);
}

void MessageLoop::Run()
{
  void* status;
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&message_loop_thread_, &thread_attr, &MessageLoop::RunInternal, static_cast<void*>(this));
  pthread_join(message_loop_thread_, &status);
  while(!clients_.empty()) {
    clients_.front()->OnLoopQuitting();
    clients_.pop();
  }
}

void MessageLoop::Stop()
{
  struct input_event ev;
  ev.type = ML_TYPE;
  ev.code = ML_QUIT;
  queue_->PostMessage(ev);
}

void MessageLoop::PostMessage(const struct input_event& ev) {
  queue_->PostMessage(ev);
}

void MessageLoop::AddClient(MessageLoopClient* client) {
  clients_.push(client);
}

void MessageLoop::SetEventManager(EventManager* event_manager) {
  event_manager_ = event_manager;
}

void MessageLoop::OnEvent()
{
  pthread_mutex_lock(&event_present_lock_);
  pthread_cond_signal(&event_present_condition_);
  pthread_mutex_unlock(&event_present_lock_);
}

// static
void* MessageLoop::RunInternal(void* message_loop_ptr) {
  MessageLoop* self = reinterpret_cast<MessageLoop*>(message_loop_ptr);

  while (true) {
    pthread_mutex_lock(&self->event_present_lock_);
    pthread_cond_wait(&self->event_present_condition_, &self->event_present_lock_);
    pthread_mutex_unlock (&self->event_present_lock_);
    
    while (self->queue_->PeekMessage()) {
      struct input_event ev = self->queue_->PokeMessage();
    
      //std::cout << "New event : " << ev.time.tv_sec << ":" << ev.time.tv_usec << " " << ev.type << "-" << ev.code << " = " << ev.value << std::endl;

      if (ev.type == ML_TYPE && ev.code == ML_QUIT) {
        std::cout << "STOP!" << std::endl;
        return NULL;
      }
      
      self->event_manager_->OnEventReceived(ev);
    }    
  }
}
