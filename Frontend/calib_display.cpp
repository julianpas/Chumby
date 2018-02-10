#include "calib_display.h"

#include <stdio.h>

#include "easybmp/EasyBMP.h"
#include "event_manager.h"
#include "screen.h"

CalibDisplay::CalibDisplay(EventManager* event_manager) : TaskBase(event_manager) {
  step_ = 0;
  if (event_manager->GetActiveTask() == this)
    OnReceiveFocus();
}

std::string CalibDisplay::GetName() { return std::string("Display Calibration"); }

bool CalibDisplay::OnEventReceived(const input_event& ev) {
  static int x, y;
  static int last_x, last_y;
  if (ev.type == EV_ABS) {
    switch (ev.code) {
      case ABS_X:
        x = ev.value;
        break;
      case ABS_Y:
        y = ev.value;
        break;
    }
  } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
    if (ev.value) {
      last_x = x;
      last_y = y;
    } else {
      std::cout << "Touch calib point " << step_ << " << : x = " << last_x << " y = " << last_y << std::endl;
      touch_points_[step_][0] = last_x;
      touch_points_[step_][1] = last_y;
      step_++;
      if (step_ == 3) {
        ComputeParams();
        step_ = 0;
      }
    }
  }
  
  return false;
}

void CalibDisplay::OnReceiveFocus() {
  std::cout << "Got the focus!" << std::endl;
  BMP background;
  background.ReadFromFile("calib.bmp");

  for (int i = 0;i < 320; ++i)
    for (int j = 0 ;j < 240; ++j)
      gScreen->PutPixel(i, j, Color(background(i,j)->Red, background(i,j)->Green, background(i,j)->Blue));
  gScreen->FlipBuffer();  
  
  step_ = 0;
}

void CalibDisplay::ComputeParams() {
  float calib_points_[3][2] = {{28.,32.}, {289., 81.}, {105., 216.}};
  float params_[6];
  float K = (touch_points_[0][0] - touch_points_[2][0]) * (touch_points_[1][1] - touch_points_[2][1]) - 
            (touch_points_[1][0] - touch_points_[2][0]) * (touch_points_[0][1] - touch_points_[2][1]);

  params_[0] = ((calib_points_[0][0] - calib_points_[2][0]) * (touch_points_[1][1] - touch_points_[2][1]) - 
                (calib_points_[1][0] - calib_points_[2][0]) * (touch_points_[0][1] - touch_points_[2][1])) / K;
  params_[1] = ((touch_points_[0][0] - touch_points_[2][0]) * (calib_points_[1][0] - calib_points_[2][0]) - 
                (calib_points_[0][0] - calib_points_[2][0]) * (touch_points_[1][0] - touch_points_[2][0])) / K;
  params_[2] = (touch_points_[0][1] * (touch_points_[2][0] * calib_points_[1][0] - touch_points_[1][0] * calib_points_[2][0]) + 
                touch_points_[1][1] * (touch_points_[0][0] * calib_points_[2][0] - touch_points_[2][0] * calib_points_[0][0]) + 
                touch_points_[2][1] * (touch_points_[1][0] * calib_points_[0][0] - touch_points_[0][0] * calib_points_[1][0])) / K;
  params_[3] = ((calib_points_[0][1] - calib_points_[2][1]) * (touch_points_[1][1] - touch_points_[2][1]) - 
                (calib_points_[1][1] - calib_points_[2][1]) * (touch_points_[0][1] - touch_points_[2][1])) / K;
  params_[4] = ((touch_points_[0][0] - touch_points_[2][0]) * (calib_points_[1][1] - calib_points_[2][1]) - 
                (calib_points_[0][1] - calib_points_[2][1]) * (touch_points_[1][0] - touch_points_[2][0])) / K;
  params_[5] = (touch_points_[0][1] * (touch_points_[2][0] * calib_points_[1][1] - touch_points_[1][0] * calib_points_[2][1]) + 
                touch_points_[1][1] * (touch_points_[0][0] * calib_points_[2][1] - touch_points_[2][0] * calib_points_[0][1]) + 
                touch_points_[2][1] * (touch_points_[1][0] * calib_points_[0][1] - touch_points_[0][0] * calib_points_[1][1])) / K;

  FILE *f = fopen("/mnt/storage/jul/ts_calib.txt", "wt");
  std::cout << "Calib params : ";
  for (int i = 0; i < 6; i++) {
    fprintf(f, "%f\n", params_[i]);
    std::cout << params_[i] << " ";
  }
  std::cout << std::endl;
  fclose(f);
  
  event_manager_->RemoveTask(this);
}
