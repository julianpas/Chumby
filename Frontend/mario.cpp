#include "mario.h"

#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

#include "easybmp/EasyBMP.h"
#include "event_manager.h"
#include "screen.h"

namespace {

const int kFlashRate = 1;

volatile bool active = false;
}  // namespace

Mario::Mario(EventManager* event_manager) : TaskBase(event_manager) {
  touch_controller_ = static_cast<TouchScreenController*>(
      event_manager->GetTask("TouchScreen Controller"));

  ReadSettings();

  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&mario_thread_, &thread_attr,
                 &Mario::MarioThread, static_cast<void*>(this));
}

std::string Mario::GetName() { return std::string("Mario"); }

bool Mario::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER) {
    if (!touch_controller_->IsTouching()) {
      //&& touch_controller_->GetX() < 40 && touch_controller_->GetY() < 40) {
    } else {
      return false;
    }
  } else if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 1) {
  } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
  } else if (ev.type == 0x43) {
  } else {
    return false;
  }
  return true;
}

void Mario::OnLooseFocus(TaskBase* new_focused_task) {
  active = false;
}

void Mario::OnReceiveFocus() {
  active = true;
}


// static
void* Mario::MarioThread(void* data) {
  Mario* self = reinterpret_cast<Mario*>(data);
  long time = 0;

  while(1) {
    if (active) {
      self->world_.Tick(time);
      usleep(10 * 1000);
      time += 10;
      if (time % 50 == 0)
        self->world_.Draw();
    } else {
      usleep(1000 * 1000);
    }
  }
}

void Mario::SaveSettings() {
  FILE *f = fopen("/mnt/storage/jul/mario.txt", "wt");
  if (f) {
    //fprintf(f, "%d %d %d", alarm_hour, alarm_min, (int)alarm_active);
    fclose(f);
  }
}

void Mario::ReadSettings() {
  FILE *f = fopen("/mnt/storage/jul/mario.txt", "rt");
  if (f) {
    //fscanf(f, "%d %d %d", &alarm_hour, &alarm_min, &on);
    fclose(f);
  }
}
