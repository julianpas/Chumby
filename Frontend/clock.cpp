#include "clock.h"

#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#include "easybmp/EasyBMP.h"
#include "event_manager.h"
#include "brightness_controller.h"
#include "accel_handler.h"
#include "screen.h"

namespace {

const int kFlashRate = 1;

const int kClockLine = 51;
const int kClockSize = 10;

const int kDateLine = 190;
const int kDateSize = 4;

}  // namespace

Clock::Clock(EventManager* event_manager) 
    : TaskBase(event_manager),
    active_(false),
    alarm_active_(false),
    alarm_snoozed_(false),
    setting_hours_(false),
    setting_min_(false),
    night_mode_(false),
    last_tilt_(0),
    tilting_(false),
    tilting_up_(false),
    tilting_down_(false),
    axis_(255), 
    temp_(0),
    last_temp_(0),
    animate_mario_(false) {
  touch_controller_ = static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));

  ReadSettings();

  mario_ = new ScreenBuffer(240, 43, 2);
  BMP mario;
  mario.ReadFromFile("/mnt/storage/jul/new_system/mario.bmp");
  for (int i = 0;i < 120; ++i) {
    for (int j = 0 ;j < 41; ++j) {
      mario_->PutPixel(i, j, Color(mario(i,j)->Red, mario(i,j)->Green, mario(i,j)->Blue));
      mario_->PutPixel(240 - i, j, Color(mario(i,j)->Red, mario(i,j)->Green, mario(i,j)->Blue));
    }
  }
  
  pthread_mutex_init(&data_lock_, NULL);

  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&clock_thread_, &thread_attr, 
                 &Clock::ClockThread, static_cast<void*>(this));
  pthread_attr_t temp_thread_attr;
  pthread_attr_init(&temp_thread_attr);
  pthread_attr_setstacksize(&temp_thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&temp_thread_, &temp_thread_attr, 
                 &Clock::TempThread, static_cast<void*>(this));

}

std::string Clock::GetName() { return std::string("Clock"); }

bool Clock::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  bool result = true;
  pthread_mutex_lock(&data_lock_);

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER) {
    if (!touch_controller_->IsTouching() && touch_controller_->GetX() < 40 && touch_controller_->GetY() < 40) {
      active_ = false;
      setting_hours_ = true;
      DrawAlarmTime();
    } if (!touch_controller_->IsTouching() && touch_controller_->GetX() > 40 && touch_controller_->GetX() < 80 && touch_controller_->GetY() < 40) {
      BrightnessController *controller = reinterpret_cast<BrightnessController*>(event_manager_->GetTask("Brightness Controller"));
      if (night_mode_) {
        controller->SetBrightnessLevel(normal_brightness_);
      } else {
        normal_brightness_ = controller->GetBrightnessLevel();
        controller->SetBrightnessLevel(20);
      }
      night_mode_ = !night_mode_;
      DrawNightMode();
    } else if (!touch_controller_->IsTouching() && touch_controller_->GetX() > 280 && touch_controller_->GetY() < 40) {
      event_manager_->SetActiveTask(event_manager_->GetTask("Radio"));
    } else if (!touch_controller_->IsTouching() && touch_controller_->GetX() > 280 && touch_controller_->GetY() > 200) {
      animate_mario_ = !animate_mario_;
    } else {
      result = false;
    }
  } else if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 0) {
    if (setting_hours_) {
      setting_hours_ = false;
      setting_min_ = true;
      DrawAlarmTime();
    } else if (setting_min_) {
      setting_min_ = false;
      active_ = true;
      SaveSettings();
    } else {
      time_t t = time(NULL);
      struct tm *ltm = localtime(&t);
      
      if (!alarm_snoozed_ && alarm_active_ && 
          ltm->tm_hour == alarm_hour_ && ltm->tm_min == alarm_min_) {
        alarm_snoozed_ = true;
      } else {
        alarm_active_ = !alarm_active_;
        SaveSettings();
      }
      DrawAlarm();
      DrawNightMode();
    }
  } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
    if (setting_hours_) {
      alarm_hour_ += ev.value;
      if (alarm_hour_ < 0) alarm_hour_ = 23;
      else if (alarm_hour_ > 23) alarm_hour_ = 0;
    } else if (setting_min_) {
      alarm_min_ += ev.value;
      if (alarm_min_ < 0) alarm_min_ = 59;
      else if (alarm_min_ > 59) alarm_min_ = 0;
    } else {
      result = false;
    }
    DrawAlarmTime();
  } else if (ev.type == 0x43) {
    int avg = ((ev.value & 0xff00) >> 8);
    if (avg - axis_ > 5 || axis_ - avg > 5) {
      axis_ = avg;
    } else if (avg > 12 && avg < 20 && !tilting_) {
      tilting_ = true;
      time_t now = time(NULL);
      if (now - last_tilt_ < 10 && tilting_down_ == true) {
        std::cout << "stop up" << std::endl;
        system("wget http://192.168.0.18/stop --post-data='' -q -T 1 -t 1 -O - &");
        tilting_down_ = false;
      } else {
        std::cout << "up" << std::endl;
        system("wget http://192.168.0.18/up --post-data='' -q -T 1 -t 1 -O - &");
        tilting_up_ = true;
      }
      last_tilt_ = time(NULL);
    } else if (avg > 220 && avg < 230 && !tilting_) {
      tilting_ = true;
      time_t now = time(NULL);
      if (now - last_tilt_ < 10 && tilting_up_ == true) {
        std::cout << "stop down" << std::endl;
        system("wget http://192.168.0.18/stop --post-data='' -q -T 1 -t 1 -O - &");
        tilting_up_ = false;
      } else {
        std::cout << "down" << std::endl;
        system("wget http://192.168.0.18/down --post-data='' -q -T 1 -t 1 -O - &");
        tilting_down_ = true;
      }
      last_tilt_ = time(NULL);
    } else if ((avg > 250 || avg < 0) && tilting_) {
      tilting_ = false;
      time_t now = time(NULL);
      if (now - last_tilt_ > 2) {
        std::cout << "stop" << std::endl;
        system("wget http://192.168.0.18/stop --post-data='' -q -T 1 -t 1 -O - &");
        last_tilt_ = 0;
      }
    }
    //std::cout << "Accel" << ((ev.value & 0xff0000) >> 16) << " " << ((ev.value & 0xff00) >> 8) << " " << (ev.value & 0xff);
  } else {
    result = false;
  }

  pthread_mutex_unlock(&data_lock_);

  return result;
}

void Clock::OnLooseFocus(TaskBase* new_focused_task) {
  active_ = false;
}

void Clock::OnReceiveFocus() {
  BMP background;
  background.ReadFromFile("/mnt/storage/jul/new_system/clock_bg.bmp");

  for (int i = 0;i < 320; ++i)
    for (int j = 0 ;j < 240; ++j)
      gScreen->PutPixel(i, j, Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
  DrawAlarm();
  DrawNightMode();
  DrawNet();
  // gScreen->BitBlt(*mario_, 0, 0, 30, 41, 280, 195);
  
  active_ = true;
}


// static
void* Clock::ClockThread(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  bool blink = true;
  int frame_dir = 1;
  int mario_dir = 1;
  int mario_pos = 60, mario_frame = 0;

  time_t t = time(NULL);
  struct tm *ltm = localtime(&t);

  while(1) {
    if (mario_pos % 20 == 0) {
      t = time(NULL);
      ltm = localtime(&t);
      ltm->tm_mon++;
    }

    pthread_mutex_lock(&self->data_lock_);
    if (self->active_) {
      if (mario_pos % 20 == 10) {
        gScreen->ClearRectangle(0,kClockLine,320,kClockSize * 8);
        blink = !blink;
        if (blink)
          gScreen->DrawDigit(120, kClockLine, kClockSize, kWhite, 10);

        gScreen->DrawDigit(0,kClockLine, kClockSize, kWhite, ltm->tm_hour / 10);
        gScreen->DrawDigit(7 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_hour % 10);
        gScreen->DrawDigit(18 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_min / 10);
        gScreen->DrawDigit(25 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_min % 10);
      }

      if (mario_pos % 40 == 0) {
        gScreen->ClearRectangle(0,kDateLine,319,40);
        gScreen->DrawDigit(0, kDateLine, kDateSize, kWhite, ltm->tm_mday / 10);
        gScreen->DrawDigit(7 *kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mday % 10);
        gScreen->DrawDigit(12 * kDateSize, kDateLine, kDateSize, kWhite, 11);
        gScreen->DrawDigit(18 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mon / 10);
        gScreen->DrawDigit(25 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mon % 10);
        gScreen->DrawDigit(30 * kDateSize, kDateLine, kDateSize, kWhite, 11);
        gScreen->DrawDigit(36 * kDateSize, kDateLine, kDateSize, kWhite, (ltm->tm_year / 10) % 10);
        gScreen->DrawDigit(43 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_year % 10);

        if (self->temp_ > 0) {
          gScreen->DrawDigit(60 * kDateSize, kDateLine, kDateSize, kWhite, self->temp_ / 100);
          gScreen->DrawDigit(67 * kDateSize, kDateLine, kDateSize, kWhite, (self->temp_ % 100) / 10);
          gScreen->DrawDigit(74 * kDateSize, kDateLine, 2, kWhite, self->temp_ % 10);
        }
      }
     
      if (self->animate_mario_) {
        gScreen->BitBlt(*self->mario_, (mario_frame + (mario_dir > 0 ? 0 : 5)) * 30, 0, 30, 41, mario_pos, 20);
      } else {
        gScreen->ClearRectangle(80, 20, 160, 41);
      }

      mario_frame = mario_frame + frame_dir;
      if (mario_frame == 3) {
        frame_dir = -1;
        mario_frame = 1;
      }
      else if (mario_frame == -1) {
        mario_frame = 1;
        frame_dir = 1;
      }
      mario_pos+= mario_dir;
      if(mario_pos > 159) {
        mario_dir = -1;
      } else if (mario_pos < 81) {
        mario_dir = 1;
      }
      
      gScreen->FlipBuffer();
    }
    if (self->setting_hours_ || self->setting_min_) {
    } else {
      if (!self->alarm_snoozed_ && self->alarm_active_ && 
          ltm->tm_hour == self->alarm_hour_ && 
          ltm->tm_min == self->alarm_min_ && ltm->tm_sec % 5 == 0) {
        system("amixer sset DAC 255");
        system("aplay /mnt/storage/sounds/shutdown1.wav");
      }
      if (self->alarm_snoozed_ && 
          (ltm->tm_hour != self->alarm_hour_ || ltm->tm_min != self->alarm_min_)) {
        self->alarm_snoozed_ = false;
      }
    }
    pthread_mutex_unlock(&self->data_lock_);
    
    usleep(50 * 1000 / kFlashRate);
  }
}

// static
void* Clock::TempThread(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);
  
  const char request[] = "GET /cgi/action.py?action=bedroom_temp\n\n";

  while(true) {
    int sock;
    struct sockaddr_in server;
    char server_reply[200];

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      printf("Could not create socket");
      return NULL;
    }

    server.sin_addr.s_addr = inet_addr("192.168.0.6");
    server.sin_family = AF_INET;
    server.sin_port = htons(8000);
 
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0) {
      if(send(sock, request, strlen(request), 0) >= 0) {
        if(recv(sock, server_reply, 200, 0) >= 0) {
          char *pos = strstr(server_reply, "{\"temp\": ");
          if (pos != NULL) {
            pos += 9;
            pos[strlen(pos) - 2] = 0;
            pthread_mutex_lock(&self->data_lock_);
            self->temp_ = (int)(round(atof(pos) * 10));
            self->last_temp_ = time(NULL);
            self->DrawNet();
            pthread_mutex_unlock(&self->data_lock_);
          }
        }
      }
    }

    close(sock);
    sleep(60);
  }
  return NULL;  
}


void Clock::SaveSettings() {
  FILE *f = fopen("/mnt/storage/jul/clock_alarm.txt", "wt");
  if (f) {
    fprintf(f, "%d %d %d", alarm_hour_, alarm_min_, (int)alarm_active_);
    fclose(f);
  }
}

void Clock::ReadSettings() {
  FILE *f = fopen("/mnt/storage/jul/clock_alarm.txt", "rt");
  if (f) {
    int on;
    fscanf(f, "%d %d %d", &alarm_hour_, &alarm_min_, &on);
    fclose(f);
    if (on)
      alarm_active_ = true;
  }
}

void Clock::DrawNightMode() {
  BMP night;
  night.ReadFromFile(night_mode_ ? "/mnt/storage/jul/new_system/night_a.bmp" : "/mnt/storage/jul/new_system/night.bmp");

  for (int i = 0;i < 40; ++i)
    for (int j = 0 ;j < 40; ++j)
      gScreen->PutPixel(i + 40, j, Color(night(i,j)->Red, night(i,j)->Green, night(i,j)->Blue));
}

void Clock::DrawNet() {
  BMP net;
  
  time_t now = time(NULL);
  
  net.ReadFromFile(now - last_temp_ < 120 ? "/mnt/storage/jul/new_system/net_a.bmp" : "/mnt/storage/jul/new_system/net_o.bmp");
  
  for (int i = 0;i < 40; ++i)
    for (int j = 0 ;j < 40; ++j)
      gScreen->PutPixel(i + 240, j, Color(net(i,j)->Red, net(i,j)->Green, net(i,j)->Blue));
}

void Clock::DrawAlarm() {
  BMP alarm;
  alarm.ReadFromFile(alarm_active_ ? "/mnt/storage/jul/new_system/clock_a.bmp" : "/mnt/storage/jul/new_system/clock_a_o.bmp");

  for (int i = 0;i < 40; ++i)
    for (int j = 0 ;j < 40; ++j)
      gScreen->PutPixel(i, j, Color(alarm(i,j)->Red, alarm(i,j)->Green, alarm(i,j)->Blue));
}

void Clock::DrawAlarmTime() {
  gScreen->ClearRectangle(0,kClockLine,320,80);
  gScreen->DrawDigit(120, kClockLine, 10, kWhite, 10);
  
  Color ch(kWhite);
  if (setting_hours_)
    ch.b -= 50;
  gScreen->DrawDigit(0,kClockLine, kClockSize, ch, alarm_hour_ / 10);
  gScreen->DrawDigit(70, kClockLine, kClockSize, ch, alarm_hour_ % 10);
  Color cm(kWhite);
  if (setting_min_)
    cm.b -= 50;
  gScreen->DrawDigit(180, kClockLine, 10, cm, alarm_min_ / 10);
  gScreen->DrawDigit(250, kClockLine, 10, cm, alarm_min_ % 10);
  gScreen->FlipBuffer();
}


