#include "radio.h"

#include <stdlib.h>

#include "button.h"
#include "event_manager.h"
#include "screen.h"

namespace {

const int kNumButtons = 4;
  
ButtonDef buttons[kNumButtons] = {
  { 20, 40, 130, 90, 0, 0, "/mnt/storage/jul/new_system/zrock.bmp", "http://live.btvradio.bg/z-rock.mp3", NULL, NULL, NULL},
  { 170, 40, 130, 90, 0, 0, "/mnt/storage/jul/new_system/starfm.bmp", "http://pulsar.atlantis.bg:8000/starfm", NULL, NULL, NULL},
  { 20, 140, 130, 90, 0, 0, "/mnt/storage/jul/new_system/darik.bmp", "http://darik.hothost.bg/low", NULL, NULL, NULL},
  { 170, 140, 130, 90, 0, 0, "/mnt/storage/jul/new_system/foundation.bmp", "http://94.26.63.158:8000/;?type=http&nocache=808", NULL, NULL, NULL}
};

}  // namespace

Radio::Radio(EventManager* event_manager)
    : TaskBase(event_manager),
      selected_radio_(false),
      volume_(210),
      previous_volume_(0),
      volume_bar_(new ScreenBuffer(100, 40, 2)),
      volume_bar_off_(new ScreenBuffer(100, 40, 2)) {
  touch_controller_ = static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));

  for (int i = 0; i < kNumButtons; ++i) {
    buttons[i].instance = static_cast<void*>(this);
    buttons[i].callback = &Radio::OnButton;
    buttons[i].button = new Button(&buttons[i]);
  }
  clock_.reset(new Button(320 - 37, 3, 34, 34, "/mnt/storage/jul/new_system/clock.bmp", 0 , 3));
  clock_->SetCallback(&Radio::OnClock, static_cast<void*>(this));

  BMP volume_bar;
  volume_bar.ReadFromFile("/mnt/storage/jul/new_system/radio_v.bmp");
  for (int i = 0;i < 100; ++i)
    for (int j = 0 ;j < 40; ++j)
      volume_bar_->PutPixel(i, j, Color(volume_bar(i,j)->Red, volume_bar(i,j)->Green, volume_bar(i,j)->Blue));
  BMP volume_bar_off;
  volume_bar_off.ReadFromFile("/mnt/storage/jul/new_system/radio_v_o.bmp");
  for (int i = 0;i < 100; ++i)
    for (int j = 0 ;j < 40; ++j)
      volume_bar_off_->PutPixel(i, j, Color(volume_bar_off(i,j)->Red, volume_bar_off(i,j)->Green, volume_bar_off(i,j)->Blue));
}

std::string Radio::GetName() { return std::string("Radio"); }

bool Radio::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;

  bool result = false;

  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER) {
    if (!touch_controller_->IsTouching()) {
      for (int i = 0; i < kNumButtons; ++i) {
        result |= buttons[i].button->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
      }
      result |= clock_->OnPress(touch_controller_->GetX(), touch_controller_->GetY());
      
      if (touch_controller_->GetX() < 100 && touch_controller_->GetY() < 40) {
        volume_ = ((touch_controller_->GetX() - 4) * 155) / 91 + 100;
        SetVolume();
        result = true;
      }
      return result;
    } else {
      if (touch_controller_->GetX() < 100 && touch_controller_->GetY() < 40) {
          volume_ = ((touch_controller_->GetX() - 4) * 155 ) / 91 + 100;
        DrawVolume();
        return true;
      }
      return false;
    }
  } else if (ev.type == EV_REL && ev.code == REL_WHEEL) {
    volume_ += ev.value > 0 ? 5 : -5;
    SetVolume();
  } else {
    return false;
  }
  return true;
}

void Radio::OnReceiveFocus() {
  previous_volume_ = 100;
  BMP background;
  background.ReadFromFile("/mnt/storage/jul/new_system/radio.bmp");
  for (int i = 0;i < 320; ++i)
    for (int j = 0 ;j < 240; ++j)
      gScreen->PutPixel(i, j, Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
    
  for (int i = 0; i < kNumButtons; ++i)
    buttons[i].button->Draw();
  clock_->Draw();

  gScreen->FlipBuffer();

  SetVolume();
}

void Radio::SetVolume() {
  char str[50];
  if (volume_ < 100) volume_ = 100;
  if (volume_ > 255) volume_ = 255;
  DrawVolume();
  sprintf(str, "amixer sset DAC %d", volume_);
  system(str);
}

void Radio::DrawVolume() {
  int min = volume_ < previous_volume_ ? volume_ : previous_volume_;
  int max = volume_ > previous_volume_ ? volume_ : previous_volume_;
  min = 4 + ((min-100) * 91) / 155;
  max = 4 + ((max-100) * 91) / 155;
  if (volume_ >= previous_volume_)
    gScreen->BitBlt(*volume_bar_, min, 0, max - min + 1, 40, min, 0);
  else
    gScreen->BitBlt(*volume_bar_off_, min, 0, max - min + 1, 40, min, 0);
  gScreen->FlipBuffer(); 
  previous_volume_ = volume_;
}


// static
bool Radio::OnButton(void* data) {
  ButtonDef* button = reinterpret_cast<ButtonDef*>(data);
  Radio* self = reinterpret_cast<Radio*>(button->instance);

  std::cout << button->name << std::endl;
  
  if (self->selected_radio_) {
    system("btplay --passthru=\"stop\"");
  } else {
    char cmd[100];
    sprintf(cmd, "btplay --passthru=\"play * %s\"", button->name.c_str());
    std::cout << cmd << std::endl;
    system(cmd);
  }
  self->selected_radio_ = !self->selected_radio_;

  return true;
}


// static
bool Radio::OnClock(void* data) {
  Radio* self = reinterpret_cast<Radio*>(data);
  
  self->event_manager_->SetActiveTask(self->event_manager_->GetTask("Clock"));  
  return true;
}