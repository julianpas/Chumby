#include "lights.h"

#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "easybmp/EasyBMP.h"
#include "json/json/json.h"
#include "httpclient/HTTPClient.h"
#include "event_manager.h"
#include "brightness_controller.h"
#include "screen.h"
#include "button.h"

namespace {

int max(const int a, const int b) { return a > b ? a : b; }

typedef struct {
  int x;
  int y;
  int image;
  std::string name;
  Lights* instance;
  Button* button;
} LightDef;

LightDef lights[] = {
  { 64, 57, 1, "Hallway mirror", NULL, NULL},
  { 105, 79, 0, "Hallway", NULL, NULL},

  { 14, 126, 1, "Luna", NULL, NULL},
  { 14, 175, 1, "Jul", NULL, NULL},
  { 50, 149, 0, "Bedroom", NULL, NULL},

  { 229, 84, 0, "Living Room Dining", NULL, NULL},
  { 248, 160, 0, "Living room", NULL, NULL},
  { 283, 182, 2, "Couch", NULL, NULL},

  { 163, 149, 0, "office_top", NULL, NULL},
  { 188, 124, 2, "Office", NULL, NULL}
};

const int kNumButtons = 10;

std::string light_icons[3] = {
  "/mnt/storage/jul/new_system/smallbulb.bmp",
  "/mnt/storage/jul/new_system/rsmallbulb.bmp",
  "/mnt/storage/jul/new_system/lsmallbulb.bmp",
};

}  // namespace

Lights::Lights(EventManager* event_manager) 
    : TaskBase(event_manager) {
  touch_controller_ = 
      static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));

  for (int i = 0; i < kNumButtons; ++i) {
    Button* button = new Button(lights[i].x, lights[i].y, 17, 17, light_icons[lights[i].image], 0, 8);
    lights[i].instance = this;
    lights[i].button = button;
    button->SetCallback(&Lights::OnButton, static_cast<void*>(&lights[i]));
    buttons_.push_back(button);
  }
  clock_ = new Button(320 - 37, 3, 34, 34, "/mnt/storage/jul/new_system/clock.bmp", 0 , 3);
  clock_->SetCallback(&Lights::OnClock, static_cast<void*>(this));

  pthread_mutex_init(&data_lock_, NULL);

  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setstacksize(&thread_attr , PTHREAD_STACK_MIN + 0x2000);
  pthread_create(&data_thread_, &thread_attr, 
                 &Lights::DataThread, static_cast<void*>(this));
}

std::string Lights::GetName() { return std::string("Lights"); }

bool Lights::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  bool result = false;
  pthread_mutex_lock(&data_lock_);

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER && !touch_controller_->IsTouching()) {
    for (int i = 0; i < kNumButtons; ++i) {
      result |= buttons_[i]->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
    }
    result |= clock_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
  } else if (ev.type == EV_KEY && ev.code == KEY_ENTER && ev.value == 0) {
  } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
  } //else if (ev.type == EV_ACCEL) { } 
  pthread_mutex_unlock(&data_lock_);

  return result;
}

void Lights::OnLooseFocus(TaskBase* new_focused_task) {
}

void Lights::OnReceiveFocus() {
  BMP background;
  background.ReadFromFile("/mnt/storage/jul/new_system/lights.bmp");

  for (int i = 0;i < 320; ++i) {
    for (int j = 0 ;j < 240; ++j) {
      gScreen->PutPixel(i, j, 
                        Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
    }
  }
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
  
  for (int i = 0; i < kNumButtons; ++i) {
    buttons_[i]->Draw();
  }
  clock_->Draw();
  gScreen->FlipBuffer();
}

// static
void Lights::GetLights(Lights* self) {
  HTTPClient client;
  char* server_reply = new char[4096];
  
  HTTPText data(server_reply, 4096);
  if (HTTP_OK == client.get("http://192.168.0.6/cgi/action.py?action=get_hue_groups", &data)) {
    int len = strlen(server_reply);
    if(len >= 0) {
      std::cout << len << " " << server_reply << std::endl;
      server_reply[len] = 0;
      char *pos = strstr(server_reply, "{");
      if (pos != NULL) {
        Json::Value root;
        Json::CharReaderBuilder builder;
        Json::CharReader* reader(builder.newCharReader());
        if (reader->parse(pos, pos + strlen(pos), &root, NULL)) {
          pthread_mutex_lock(&self->data_lock_);
          Json::ValueConstIterator iter = root.begin();
          for (; iter != root.end(); ++iter) {
            for (int i = 0; i < kNumButtons; ++i) {
              if (lights[i].name == (*iter)["name"].asString()) {
                lights[i].button->SetState((*iter)["state"]["any_on"].asBool() ? 1 : 0);
              }
            }
          }
          self->DrawUI();
          pthread_mutex_unlock(&self->data_lock_);
        }
        delete reader;
      }
    }
  }
/*  
  const char request[] = "GET /cgi/action.py?action=get_hue_groups\n\n";

  int sock;
  struct sockaddr_in server;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return;

  server.sin_addr.s_addr = inet_addr("192.168.0.6");
  server.sin_family = AF_INET;
  server.sin_port = htons(8000);

  if (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0) {
    if(send(sock, request, strlen(request), 0) >= 0) {
      int len = recv(sock, server_reply, 4096, MSG_WAITALL);
      if(len >= 0) {
        server_reply[len] = 0;
        char *pos = strstr(server_reply, "{");
        if (pos != NULL) {
          Json::Value root;
          Json::CharReaderBuilder builder;
          Json::CharReader* reader(builder.newCharReader());
          if (reader->parse(pos, pos + strlen(pos), &root, NULL)) {
            pthread_mutex_lock(&self->data_lock_);
            Json::ValueConstIterator iter = root.begin();
            for (; iter != root.end(); ++iter) {
              for (int i = 0; i < kNumButtons; ++i) {
                if (lights[i].name == (*iter)["name"].asString()) {
                  lights[i].button->SetState((*iter)["state"]["any_on"].asBool() ? 1 : 0);
                }
              }
            }
            self->DrawUI();
            pthread_mutex_unlock(&self->data_lock_);
          }
          delete reader;
        }
      }
    }
  }
  close(sock);*/
  delete[] server_reply;
}

// static
bool Lights::OnButton(void* data) {
  LightDef* light = reinterpret_cast<LightDef*>(data);

  light->button->SetState((light->button->GetState() + 1) % 2);

  char request[80];
  sprintf(request, "GET /cgi/action.py?action=set_hue_group_%s\n\n", light->name.c_str());  

  int sock;
  struct sockaddr_in server;

  //Create socket
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return true;

  server.sin_addr.s_addr = inet_addr("192.168.0.6");
  server.sin_family = AF_INET;
  server.sin_port = htons(8000);
  
  char* server_reply = new char[1024];
  if (connect(sock, (struct sockaddr*)&server, sizeof(server)) >= 0) {
    if(send(sock, request, strlen(request), 0) >= 0)
      recv(sock, server_reply, 1024, 0);
  }
  close(sock);
  delete[] server_reply;
  
  light->instance->DrawUI();
  return true;
}


// static
bool Lights::OnClock(void* data) {
  Lights* self = reinterpret_cast<Lights*>(data);

  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Clock"));  
  return true;
}