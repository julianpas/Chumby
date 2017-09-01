#pragma once

#include "task_base.h"

class CalibDisplay : public TaskBase {
 public:
  CalibDisplay(EventManager* event_manager);
  
  virtual std::string GetName();
  virtual bool OnEventReceived(const input_event& ev);
  virtual void OnReceiveFocus();
 private:
  void ComputeParams();

  int step_;
  // Calib poiints : 
  //      X  Y
  //  0  28  32
  //  1 189  81 
  //  2 105 216
  int touch_points_[3][2];
};
