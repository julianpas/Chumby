#include "brightness_controller.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char kBrightnessFile[] =
    "/sys/devices/platform/stmp3xxx-bl/backlight/stmp3xxx-bl/brightness";

BrightnessController::BrightnessController(EventManager* event_manager) 
    : TaskBase(event_manager) {
  backlight_file_ = open(kBrightnessFile, O_RDWR);
}

BrightnessController::~BrightnessController() {
  close(backlight_file_);
}

std::string BrightnessController::GetName() {
  return std::string("Brightness Controller");
}

bool BrightnessController::OnEventReceived(const input_event& ev) {
  if (ev.type == EV_REL && ev.code == REL_WHEEL) {
    int curr_brightness = GetBrightnessLevel();
    curr_brightness += ev.value * (1 + curr_brightness / 30);
    if (curr_brightness > 100) curr_brightness = 100;
    if (curr_brightness < 0) curr_brightness = 0;
    SetBrightnessLevel(curr_brightness);
    
    return true;
  }
  return false;
}

int BrightnessController::GetBrightnessLevel() {
  char str[20]; 
  size_t rb;
  lseek(backlight_file_, 0, SEEK_SET);
  rb = read(backlight_file_, str, 19);
  str[rb] = 0;
  return atoi(str);
}

void BrightnessController::SetBrightnessLevel(int level) {
  char str[20]; 
  sprintf(str, "%d", level);
  write(backlight_file_, str, strlen(str));
}