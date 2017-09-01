#include "touch_screen_controller.h"

#include <iostream>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "event_manager.h"
#include "screen.h"

const int kXFactor = 10;
const int kXOffset = 1500;
const int kXDivisor = 118;

const int kYFactor = 100;
const int kYOffset = 28000;
const int kYDivisor = 1515;

float params[6] = {1, 0, 0, 0, 1, 0};

TouchScreenController::TouchScreenController(EventManager* event_manager) 
    : TaskBase(event_manager) {
  FILE *f = fopen("/mnt/storage/jul/ts_calib.txt", "rt");
  if (f) {
    std::cout << "Loaded TS params : ";
    for(int i = 0; i < 6; ++i) {
      fscanf(f, "%f", &params[i]);
      std::cout << params[i] << " ";
    }
    fclose(f);
    std::cout << std::endl;
  }
}

TouchScreenController::~TouchScreenController() {
}

std::string TouchScreenController::GetName() {
  return std::string("TouchScreen Controller");
}

bool TouchScreenController::OnEventReceived(const input_event& ev) {
  static int x, y;
  if (ev.type == EV_ABS) {
    switch (ev.code) {
      case ABS_X:
        x = ev.value;
        break;
      case ABS_Y:
        y = ev.value;
        break;
      case ABS_PRESSURE:
        break;
    }
  } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
    touched_ = ev.value;
  } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
    if (touched_) {
      current_x_ = params[0] * x + params[1] * y + params[2];
      current_y_ = params[3] * x + params[4] * y + params[5];
      /*if (event_manager_->GetActiveTask() == this) {
        gScreen->PutPixel(current_x_, current_y_, Color(255, 228, 228));
        gScreen->FlipBuffer();
      }*/
    }
    input_event ne;
    ne.type = EV_KEY;
    ne.code = BTN_TRIGGER;
    ne.value = 1;
    event_manager_->OnEventReceived(ne);
    std::cout << "Touch event : x = " << current_x_ << " y = " << current_y_ << " pressing : " << touched_ << std::endl;
  }
  return false;
}

