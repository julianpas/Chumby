#include "lights.h"

#include <auto_ptr.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "brightness_controller.h"
#include "button.h"
#include "easybmp/EasyBMP.h"
#include "event_manager.h"
#include "json/json/json.h"
#include "screen.h"

namespace {

int max(const int a, const int b) { return a > b ? a : b; }
/*
typedef struct {
  LightDef(int x, int y, int image, const std::string& name)
    : x(x), y(y), image(image), name(name), instance(nullptr), button(nullptr) {}
  int x;
  int y;
  int image;
  std::string name;
  Lights* instance;
  Button* button;
} LightDef;

LightDef lights[] = {
  { 105, 79, 0, "hallway_light", NULL, NULL},

  { 14, 126, 1, "luna_light", NULL, NULL},
  { 14, 175, 1, "jul_light", NULL, NULL},
  { 50, 149, 0, "bedroom_light", NULL, NULL},

  { 229, 84, 0, "diningroom_light", NULL, NULL},
  { 248, 160, 0, "livingroom_light", NULL, NULL},
  { 283, 182, 2, "livingroom_tv_light", NULL, NULL},

  { 188, 124, 2, "office_hue_light", NULL, NULL}
};

const int kNumButtons = sizeof(lights) / sizeof(LightDef);
*/
std::string light_icons[3] = {
  "/mnt/storage/jul/new_system/smallbulb.bmp",
  "/mnt/storage/jul/new_system/rsmallbulb.bmp",
  "/mnt/storage/jul/new_system/lsmallbulb.bmp",
};

}  // namespace

Lights::Lights(EventManager* event_manager, const std::string& level)
    : TaskBase(event_manager) {
  touch_controller_ = 
      static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));

  std::string config_file(std::string("/mnt/storage/jul/new_system/").append(level).append(".txt"));
  image_file_ = std::string("/mnt/storage/jul/new_system/").append(level).append(".bmp");
  task_name_ = std::string("Lights ").append(level);
  ReadSettings(config_file);

  for (size_t i = 0; i < lights_.size(); ++i) {
    Button* button = new Button(lights_[i].x, lights_[i].y, 17, 17, light_icons[lights_[i].image], 0, 8);
    lights_[i].instance = this;
    lights_[i].button = button;
    button->SetCallback(&Lights::OnButton, static_cast<void*>(&lights_[i]));
    buttons_.push_back(button);
  }
  clock_ = new Button(320 - 37, 3, 34, 34, "/mnt/storage/jul/new_system/clock.bmp", 0, 3);
  clock_->SetCallback(&Lights::OnClock, static_cast<void*>(this));
  og_ = new Button(274, 118, 20, 26, "/mnt/storage/jul/new_system/og_but.bmp", level == "og" ? 1 : 0);
  og_->SetCallback(&Lights::OnOG, static_cast<void*>(this));
  eg_ = new Button(274, 157, 20, 26, "/mnt/storage/jul/new_system/eg_but.bmp", level == "eg" ? 1 : 0);
  eg_->SetCallback(&Lights::OnEG, static_cast<void*>(this));
  ug_ = new Button(274, 195, 20, 26, "/mnt/storage/jul/new_system/ug_but.bmp", level == "ug" ? 1 : 0);
  ug_->SetCallback(&Lights::OnUG, static_cast<void*>(this));
  
  connection_.reset(new TcpConnection("192.168.0.6", 8000));

  pthread_mutex_init(&data_lock_, NULL);

  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&data_thread_, &thread_attr, 
                 &Lights::DataThread, static_cast<void*>(this));
}

std::string Lights::GetName() { return task_name_; }

void Lights::ReadSettings(const std::string& filename) {

  FILE *f = fopen(filename.c_str(), "rt");
  if (f) {
    int lines;
    int x, y, image;
    char name[100];
    fscanf(f, "%d", &lines);
    for (int i = 0; i < lines; ++i) {
      fscanf(f, "%d %d %d %s", &x, &y, &image, name);
      lights_.push_back(LightDef(x, y, image, name));
    }
    fclose(f);
  }
  std::cout << task_name_ << ":\n\tFile   : " << filename 
            << "\n\tLights : " << lights_.size() << std::endl;
}

bool Lights::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  bool result = false;
  pthread_mutex_lock(&data_lock_);

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER && !touch_controller_->IsTouching()) {
    for (size_t i = 0; i < buttons_.size(); ++i) {
      result |= buttons_[i]->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
    }
    result |= clock_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
    result |= og_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
    result |= eg_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
    result |= ug_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
  } else if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 0) {
  } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
  } //else if (ev.type == EV_ACCEL) { } 
  pthread_mutex_unlock(&data_lock_);

  return result;
}

void Lights::OnLooseFocus(TaskBase* new_focused_task) { }

void Lights::OnReceiveFocus() {
  BMP background;
  background.ReadFromFile(image_file_.c_str());

  for (int i = 0;i < 320; ++i) {
    for (int j = 0 ;j < 240; ++j) {
      gScreen->PutPixel(i, j, 
                        Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
    }
  }
  DrawUI();
  GetLights(this);
}

// static
void* Lights::DataThread(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  while(true) {
    if (self->HasFocus())
      GetLights(self);
    sleep(10);
  }

  return NULL;
}

void Lights::DrawUI() {
  if (!HasFocus())
    return;
  
  for (size_t i = 0; i < buttons_.size(); ++i)
    buttons_[i]->Draw();
  clock_->Draw();
  og_->Draw();
  eg_->Draw();
  ug_->Draw();
  gScreen->FlipBuffer();
}

void Lights::GetHueLights() {
  const char request[] = "GET /cgi/action.py?action=hue_lights_status\n\n";

  pthread_mutex_lock(&data_lock_);
  if (connection_->connect() && connection_->send(request)) {
    std::string output;
    connection_->receive(20, &output);
    Json::Value root;
    if (TcpConnection::getJson(output, &root)) {
      Json::ValueConstIterator iter = root.begin();
      for (; iter != root.end(); ++iter) {
        for (size_t i = 0; i < lights_.size(); ++i) {
          if (lights_[i].name == iter.name())
            lights_[i].button->SetState((*iter)["state"]["any_on"].asBool() ? 1 : 0);
        }
      }
      DrawUI();
    }
  }
  connection_->close();
  pthread_mutex_unlock(&data_lock_);
}

// static
void Lights::GetLights(Lights* self) {
  self->GetHueLights();
}

// static
bool Lights::OnButton(void* data) {
  LightDef* light = reinterpret_cast<LightDef*>(data);
  light->button->SetState((light->button->GetState() + 1) % 2);
  light->instance->DrawUI();
  
  char request[100];
  sprintf(request, "GET /cgi/action.py?action=%s", light->name.c_str());  
  TcpConnection connection("192.168.0.6", 8000);
  connection.connect();
  connection.send(request);
  return true;
}

// static
bool Lights::OnClock(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Clock"));  
  return true;
}

// static
bool Lights::OnOG(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Lights og"));  
  return true;
}

// static
bool Lights::OnEG(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Lights eg"));  
  return true;
}

// static
bool Lights::OnUG(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Lights ug"));  
  return true;
}
