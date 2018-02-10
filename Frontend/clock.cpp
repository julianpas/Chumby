#include "clock.h"

#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "easybmp/EasyBMP.h"
#include "json/json/json.h"
#include "event_manager.h"
#include "brightness_controller.h"
#include "accel_handler.h"
#include "screen.h"
#include "button.h"

namespace {

int max(const int a, const int b) { return a > b ? a : b; }

const int kFlashRate = 1;

const int kClockLine = 51;
const int kClockSize = 10;

const int kDateLine = 190;
const int kDateSize = 4;

const int kTempBuckets = 48;
const int kMinPerBucket = 10;

const char config_file[] = "/mnt/storage/jul/clock_settings.txt";

}  // namespace

Clock::Clock(EventManager* event_manager) 
    : TaskBase(event_manager),
    active_(false),
    alarm_active_(false),
    alarm_snoozed_(false),
    setting_hours_(false),
    setting_min_(false),
    normal_brightness_(75),
    night_brightness_(17),
    last_tilt_(0),
    tilting_(false),
    tilting_up_(false),
    tilting_down_(false),
    axis_(255), 
    temp_(0),
    last_temps_(0),
    show_temps_(false),
    last_temp_(0) {
  touch_controller_ = 
      static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));

  reading_light_command_ = 
      "wget http://192.168.0.6:8000/cgi/action.py?action=jul_light -q -T 1 -t 1 -O - &";

  ReadSettings();

  net_button_ = new Button(307, 0, 13, 13, "/mnt/storage/jul/new_system/net.bmp", 0, -2);
  reading_light_ = new Button(240, 0, 40, 40, "/mnt/storage/jul/new_system/readinglight.bmp");
  reading_light_->SetCallback(&Clock::OnReadingLightButton, static_cast<void*>(this));
  ceiling_light_ = new Button(80, 0, 40, 40, "/mnt/storage/jul/new_system/lightbulb.bmp");
  ceiling_light_->SetCallback(&Clock::OnCeilingLightButton, static_cast<void*>(this));
  alarm_ = new Button(0, 0, 40, 40, "/mnt/storage/jul/new_system/alarm.bmp");
  alarm_->SetCallback(&Clock::OnAlarmButton, static_cast<void*>(this));
  night_mode_ = new Button(40, 0, 40, 40, "/mnt/storage/jul/new_system/night_mode.bmp");
  night_mode_->SetCallback(&Clock::OnNightModeButton, static_cast<void*>(this));
  radio_ = new Button(280, 0, 40, 40, "/mnt/storage/jul/new_system/radio_but.bmp");
  radio_->SetCallback(&Clock::OnRadioButton, static_cast<void*>(this));
  temperature_ = new Button(220, 160, 100, 80, "");
  temperature_->SetCallback(&Clock::OnTempButton, static_cast<void*>(this));

  temps_ = new int[kTempBuckets * 3];
  hums_ = temps_ + kTempBuckets;
  measures_ = temps_ + kTempBuckets * 2;
  memset(temps_, 0, sizeof(int) * kTempBuckets * 3);
  last_temps_time_ = time(NULL);

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
                 &Clock::DataThread, static_cast<void*>(this));
}

std::string Clock::GetName() { return std::string("Clock"); }

bool Clock::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  bool result = true;
  pthread_mutex_lock(&data_lock_);

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER && !touch_controller_->IsTouching()) {
    result = 
        alarm_->OnPress(touch_controller_->GetX(), touch_controller_->GetY()) ||
        night_mode_->OnPress(touch_controller_->GetX(), touch_controller_->GetY()) ||
        ceiling_light_->OnPress(touch_controller_->GetX(), touch_controller_->GetY()) ||
        reading_light_->OnPress(touch_controller_->GetX(), touch_controller_->GetY()) ||
        radio_->OnPress(touch_controller_->GetX(), touch_controller_->GetY()) ||
        temperature_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
  } else if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 0) {
    if (setting_hours_) {
      setting_hours_ = false;
      setting_min_ = true;
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
  } else if (ev.type == EV_ACCEL) {
    int avg = AccelHandler::GetY(ev.value);
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

  for (int i = 0;i < 320; ++i) {
    for (int j = 0 ;j < 240; ++j) {
      gScreen->PutPixel(i, j, 
                        Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
    }
  }
  DrawUI();
  active_ = true;
}


// static
void* Clock::ClockThread(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  bool blink = true;
  bool needs_draw = false;
  int frame = 0;
  const int clock_redraw = 20;
  const int date_redraw = 100;

  time_t t = time(NULL);
  struct tm *ltm = localtime(&t);

  while(1) {
    needs_draw = false;
    if (frame % clock_redraw == 0) {
      t = time(NULL);
      ltm = localtime(&t);
      ltm->tm_mon++;
    }

    pthread_mutex_lock(&self->data_lock_);
    if (self->active_) {
      if (!self->show_temps_ && !self->setting_hours_ && !self->setting_min_) {
        if (frame % clock_redraw == 0) {
          gScreen->ClearRectangle(0,kClockLine,320,108);
          blink = !blink;
          if (blink)
            gScreen->DrawDigit(120, kClockLine, kClockSize, kWhite, 10);

          gScreen->DrawDigit(0,kClockLine, kClockSize, kWhite, ltm->tm_hour / 10);
          gScreen->DrawDigit(7 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_hour % 10);
          gScreen->DrawDigit(18 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_min / 10);
          gScreen->DrawDigit(25 * kClockSize, kClockLine, kClockSize, kWhite, ltm->tm_min % 10);
          needs_draw = true;
        }
      } else {
        if (self->show_temps_) {
          if (frame % date_redraw == 0) {
            gScreen->ClearRectangle(0, 46, 320, 108);
            
            for (int i = 0; i < kTempBuckets; ++i) {
              for (int j = 0; j < 18; j++) {
                if (self->temps_[(self->last_temps_ + i + 1) % kTempBuckets] >= (190 + j * 5)) {
                  if (self->hums_[(self->last_temps_ + i + 1) % kTempBuckets] >= (30 + j * 3))
                    gScreen->DrawRectangle(16 + i * 6, 154 - j * 6, 5, 5, kCyan);
                  else
                    gScreen->DrawRectangle(16 + i * 6, 154 - j * 6, 5, 5, kBlue);
                } else if (self->hums_[(self->last_temps_ + i + 1) % kTempBuckets] >= (30 + j * 3)) {
                  gScreen->DrawRectangle(16 + i * 6, 154 - j * 6, 5, 5, kGreen);
                } else if (j == 7) {
                  gScreen->DrawRectangle(16 + i * 6 + 1, 154 - j * 6 - 1, 3, 3, kGrey);
                }
              }
            }
            needs_draw = true;
          }
        } else {
          self->DrawAlarmTime();
          needs_draw = true;
        }
      }

      if (frame % date_redraw == 0) {
        gScreen->ClearRectangle(0,kDateLine,319,40);
        gScreen->DrawDigit(0, kDateLine, kDateSize, kWhite, ltm->tm_mday / 10);
        gScreen->DrawDigit(7 *kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mday % 10);
        gScreen->DrawDigit(12 * kDateSize, kDateLine, kDateSize, kWhite, 11);
        gScreen->DrawDigit(18 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mon / 10);
        gScreen->DrawDigit(25 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_mon % 10);
        gScreen->DrawDigit(30 * kDateSize, kDateLine, kDateSize, kWhite, 11);
        gScreen->DrawDigit(36 * kDateSize, kDateLine, kDateSize, kWhite, (ltm->tm_year / 10)% 10);
        gScreen->DrawDigit(43 * kDateSize, kDateLine, kDateSize, kWhite, ltm->tm_year % 10);

        if (self->temp_ > 0) {
          gScreen->DrawDigit(60 * kDateSize, kDateLine, kDateSize, kWhite, self->temp_ / 100);
          gScreen->DrawDigit(67 * kDateSize, kDateLine, kDateSize, kWhite, (self->temp_ % 100)/ 10);
          gScreen->DrawDigit(74 * kDateSize, kDateLine, 2, kWhite, self->temp_ % 10);
        }

        self->DrawUI();
        needs_draw = true;
      }

      frame++;
      if (frame == 1000)
        frame = 0;

      if (needs_draw || self->force_draw_) {
        self->force_draw_ = false;
        gScreen->FlipBuffer();
      }
    }
    if (!self->setting_hours_ && !self->setting_min_) {
      if (!self->alarm_snoozed_ && self->alarm_active_ && 
          ltm->tm_hour == self->alarm_hour_ && ltm->tm_sec % 5 == 0) {
        if (ltm->tm_min == self->alarm_min_) {
          system("amixer sset DAC 255");
          system("aplay /mnt/storage/sounds/shutdown1.wav");
        }
        if (ltm->tm_min == max(0, self->alarm_min_ - 5) && ltm->tm_sec == 0) {
          pthread_attr_t sunlight_thread_attr;
          pthread_attr_init(&sunlight_thread_attr);
          pthread_attr_setstacksize(&sunlight_thread_attr , PTHREAD_STACK_MIN + 0x2000);
          pthread_create(&self->sunlight_thread_, &sunlight_thread_attr, 
                        &Clock::SunlightThread, static_cast<void*>(self));
        }
      }
      if (self->alarm_snoozed_ && 
          (ltm->tm_hour != self->alarm_hour_ || ltm->tm_min != self->alarm_min_)) {
        self->alarm_snoozed_ = false;
      }
    }
    pthread_mutex_unlock(&self->data_lock_);

    usleep(50 * 1000 / kFlashRate);
  }

  return NULL;
}

// static
void* Clock::DataThread(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  while(true) {
    GetBedroomTemp(self);
    GetBedroomLights(self);
    sleep(20);
  }

  return NULL;  
}

// static
void* Clock::SunlightThread(void* /*data*/) {
  system("wget http://192.168.0.18/up --post-data='' -q -T 1 -t 1 -O - &");
  sleep(15);
  system("wget http://192.168.0.18/stop --post-data='' -q -T 1 -t 1 -O - &");

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
    std::cout << "Alarm:\n\tHour   : " << alarm_hour_ 
              << "\n\tMinute : " << alarm_min_ 
              << "\n\tEnabled: " << alarm_active_ << std::endl;
  }
  
  f = fopen(config_file, "rt");
  if (f) {
    char cmd[400];
    fgets(cmd, 400, f);
    if (cmd[strlen(cmd)-1] == '\n')
      cmd[strlen(cmd) - 1] = 0;
    reading_light_command_ = cmd;
    fgets(cmd, 400, f);
    if (cmd[strlen(cmd)-1] == '\n')
      cmd[strlen(cmd) - 1] = 0;
    reading_light_name_ = cmd;
    fscanf(f, "%d %d", &normal_brightness_, &night_brightness_);
    fclose(f);
    std::cout << "Settings:\n\tReading light command: " << reading_light_command_ 
              << "\n\tReading light name: " << reading_light_name_ 
              << "\n\tDefault normal brightness level: " << normal_brightness_ 
              << "\n\tDark brightness level: " << night_brightness_ << std::endl;
  }
}

void Clock::DrawUI() {
  reading_light_->Draw();
  ceiling_light_->Draw();
  alarm_->SetState(alarm_active_ ? 1 : 0);
  alarm_->Draw();
  night_mode_->SetState(night_mode_active_ ? 1 : 0);
  night_mode_->Draw();
  radio_->Draw();

  time_t now = time(NULL);
  net_button_->SetState((now - last_temp_) > 120 ? 0 : 1);
  net_button_->Draw();
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
}

// static 
void Clock::GetBedroomTemp(Clock* self) {
  const char request[] = "GET /cgi/action.py?action=bedroom_status\n\n";

  int sock;
  struct sockaddr_in server;
  char* server_reply = new char[1024];

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return;

  server.sin_addr.s_addr = inet_addr("192.168.0.6");
  server.sin_family = AF_INET;
  server.sin_port = htons(8000);

  if (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0) {
    if(send(sock, request, strlen(request), 0) >= 0) {
      if(recv(sock, server_reply, 200, 0) >= 0) {
        char *pos = strstr(server_reply, "{");
        if (pos != NULL) {
          Json::Value root;
          Json::CharReaderBuilder builder;
          Json::CharReader* reader(builder.newCharReader());
          if (reader->parse(pos, pos + strlen(pos), &root, NULL)) {
            pthread_mutex_lock(&self->data_lock_);
            self->temp_ = (int)(round(root["temperature"].asFloat() * 10));

            if ((time(NULL) - self->last_temps_time_) > (kMinPerBucket * 60)) {
              self->last_temps_time_ = time(NULL);
              self->last_temps_++;
              if (self->last_temps_ == kTempBuckets)
                self->last_temps_ = 0;
              self->measures_[self->last_temps_] = 0;
            } 
            self->temps_[self->last_temps_] = (self->measures_[self->last_temps_] * self->temps_[self->last_temps_] + self->temp_) / (self->measures_[self->last_temps_] + 1);
            self->hums_[self->last_temps_] = (self->measures_[self->last_temps_] * self->hums_[self->last_temps_] + (int)(round(root["humidity"].asFloat()))) / (self->measures_[self->last_temps_] + 1);
            self->measures_[self->last_temps_]++;

            self->last_temp_ = time(NULL);
            bool night_mode = 
                root["light"].asString().find("dark") != std::string::npos;
            if (night_mode != self->night_mode_active_) {
              self->night_mode_observations_++;
              if (self->night_mode_observations_ >= 3) {
                self->night_mode_active_ = night_mode;
                self->SetBrightness();
                self->night_mode_observations_ = 0;
              }
            } else {
              self->night_mode_observations_ = 0;
            }
            pthread_mutex_unlock(&self->data_lock_);
          }
          delete reader;
        }
      }
    }
  }
  close(sock);
  delete[] server_reply;
}

// static
void Clock::GetBedroomLights(Clock* self) {
  const char request[] = "GET /cgi/action.py?action=bedroom_lights_status\n\n";

  int sock;
  struct sockaddr_in server;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return;

  server.sin_addr.s_addr = inet_addr("192.168.0.6");
  server.sin_family = AF_INET;
  server.sin_port = htons(8000);

  char* server_reply = new char[1024];
  if (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0) {
    if(send(sock, request, strlen(request), 0) >= 0) {
      int len = recv(sock, server_reply, 1024, 0);
      if(len >= 0) {
        server_reply[len] = 0;
        char *pos = strstr(server_reply, "{");
        if (pos != NULL) {
          Json::Value root;
          Json::CharReaderBuilder builder;
          Json::CharReader* reader(builder.newCharReader());
          if (reader->parse(pos, pos + strlen(pos), &root, NULL)) {
            pthread_mutex_lock(&self->data_lock_);
            self->reading_light_->SetState(root[self->reading_light_name_.c_str()]["state"]["any_on"].asBool() ? 1 : 0);
            self->ceiling_light_->SetState(root["Bedroom"]["state"]["any_on"].asBool() ? 1 : 0);
            pthread_mutex_unlock(&self->data_lock_);
          }
          delete reader;
        }
      }
    }
  }
  close(sock);
  delete[] server_reply;
}

void Clock::SetBrightness() {
  BrightnessController *controller = 
      reinterpret_cast<BrightnessController*>(event_manager_->GetTask("Brightness Controller"));
  if (!night_mode_active_) {
    if (normal_brightness_ <= night_brightness_)
      normal_brightness_ = 75;
    controller->SetBrightnessLevel(normal_brightness_);
  } else {
    normal_brightness_ = controller->GetBrightnessLevel();
    controller->SetBrightnessLevel(night_brightness_);
  }
}

// static
bool Clock::OnAlarmButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  self->active_ = false;
  self->setting_hours_ = true;
  return true;
}

// static
bool Clock::OnNightModeButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  self->night_mode_active_ = !self->night_mode_active_;
  self->night_mode_observations_ = 0;
  self->SetBrightness();
  self->force_draw_ = true;
  return true;
}

// static
bool Clock::OnCeilingLightButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  if (!self->night_mode_active_)
    system("wget http://192.168.0.6:8000/cgi/action.py?action=bedroom_light -q -T 1 -t 1 -O - &");
  else
    system("wget http://192.168.0.6:8000/cgi/action.py?action=bedroom_night_light -q -T 1 -t 1 -O - &");
  self->ceiling_light_->SetState((self->ceiling_light_->GetState() + 1) % 2);
  self->force_draw_ = true;
  return true;
}

// static
bool Clock::OnReadingLightButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  system(self->reading_light_command_.c_str());
  self->reading_light_->SetState((self->reading_light_->GetState() + 1) % 2);
  self->force_draw_ = true;
  return true;
}

// static
bool Clock::OnRadioButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Radio"));  
  return true;
}

// static
bool Clock::OnTempButton(void* data) {
  Clock* self = reinterpret_cast<Clock*>(data);

  self->show_temps_ = !self->show_temps_;
  self->force_draw_ = true;
  return true;
}
