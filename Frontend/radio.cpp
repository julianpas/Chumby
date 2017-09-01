#include "radio.h"

#include <stdlib.h>

#include "event_manager.h"
#include "screen.h"

namespace {

const char kRadio1[] = "http://live.btvradio.bg/z-rock.mp3\"";
const char kRadio2[] = "http://pulsar.atlantis.bg:8000/starfm\"";
const char kRadio3[] = "http://darik.hothost.bg/low\"";

}  // namespace

Radio::Radio(EventManager* event_manager)
    : TaskBase(event_manager),
      volume_(210),
      previous_volume_(100),
      volume_bar_(new ScreenBuffer(100, 40, 2)),
      volume_bar_off_(new ScreenBuffer(100, 40, 2)),
      background_(new ScreenBuffer(320, 240, 2)) {
  previous_volume_ = 0;
  touch_controller_ = static_cast<TouchScreenController*>(event_manager->GetTask("TouchScreen Controller"));
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
  BMP background;
  background.ReadFromFile("/mnt/storage/jul/new_system/radio.bmp");
  for (int i = 0;i < 320; ++i)
    for (int j = 0 ;j < 240; ++j)
      background_->PutPixel(i, j, Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
}

std::string Radio::GetName() { return std::string("Radio"); }

bool Radio::OnEventReceived(const input_event& ev) {
  if (!HasFocus())
    return false;
  
  if (ev.type == EV_KEY && ev.code == BTN_TRIGGER) {
    if (!touch_controller_->IsTouching()) {
      if (touch_controller_->GetX() > 280 && touch_controller_->GetY() < 40) {
	event_manager_->SetActiveTask(event_manager_->GetTask("Clock"));
      } else if (touch_controller_->GetX() > 30 && touch_controller_->GetY() > 50 &&
	         touch_controller_->GetX() < 290 && touch_controller_->GetY() < 90) {
	if (selected_radio_ == 1)
	  selected_radio_ = 0;
	else
	  selected_radio_ = 1;
	PlayRadio();
      } else if (touch_controller_->GetX() > 30 && touch_controller_->GetY() > 96 &&
	         touch_controller_->GetX() < 290 && touch_controller_->GetY() < 131) {
	if (selected_radio_ == 2)
	  selected_radio_ = 0;
	else
	  selected_radio_ = 2;
	PlayRadio();
      } else if (touch_controller_->GetX() > 30 && touch_controller_->GetY() > 138 &&
	         touch_controller_->GetX() < 290 && touch_controller_->GetY() < 174) {
	if (selected_radio_ == 3)
	  selected_radio_ = 0;
	else
	  selected_radio_ = 3;
	PlayRadio();
      } else if (touch_controller_->GetX() < 100 && touch_controller_->GetY() < 40) {
        volume_ = ((touch_controller_->GetX() - 4) * 155) / 91 + 100;
	SetVolume();
      }
      return true;      
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
  gScreen->Copy(*background_);
  gScreen->FlipBuffer();  

  std::cout << "Set volume now" << std::endl;
  SetVolume();
}

void Radio::PlayRadio() {
  char cmd[100] = "btplay --passthru=\"play * ";
  switch(selected_radio_) {
    case 0:
      system("btplay --passthru=\"stop\"");
      return;
    case 1:
      strcat(cmd, kRadio1);
      break;
    case 2:
      strcat(cmd, kRadio2);
      break;
    case 3:
      strcat(cmd, kRadio3);
      break;
  }
  std::cout << "Playing " << cmd << std::endl;
  system(cmd);
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
  std::cout << "Draw from " << min << " to " << max << std::endl;
  if (volume_ >= previous_volume_)
    gScreen->BitBlt(*volume_bar_, min, 0, max - min + 1, 40, min, 0);
  else
    gScreen->BitBlt(*volume_bar_off_, min, 0, max - min + 1, 40, min, 0);
  gScreen->FlipBuffer(); 
  previous_volume_ = volume_;
}