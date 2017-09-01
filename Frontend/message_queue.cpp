#include "message_queue.h"

MessageQueue::MessageQueue() {
  pthread_mutex_init(&events_lock_, NULL);
}

MessageQueue::~MessageQueue()
{
  pthread_mutex_destroy(&events_lock_);
}

void MessageQueue::PostMessage(const struct input_event& ev)
{
  pthread_mutex_lock(&events_lock_);
  events_.push(ev);
  pthread_mutex_unlock(&events_lock_);
  if (observer_)
    observer_->OnEvent();
}

struct input_event* MessageQueue::PeekMessage()
{
  input_event* event = NULL;
  pthread_mutex_lock(&events_lock_);
  if (!events_.empty())
    event = &events_.front();
  pthread_mutex_unlock(&events_lock_);
  return event;
}
struct input_event MessageQueue::PokeMessage()
{
  input_event event;
  pthread_mutex_lock(&events_lock_);
  if (!events_.empty())
    event = events_.front();
  events_.pop();
  pthread_mutex_unlock(&events_lock_);
  return event;
}

void MessageQueue::AddObserver(MessageObserver* observer)
{
  observer_ = observer;
}
