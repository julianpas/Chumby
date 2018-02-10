#pragma once

#include <string>

class ScreenBuffer;

class Button {
 public:
  Button(int x, int y, int w, int h, const std::string& images, int state = 0, int touch_extra = 0);

  void Draw() const;

  bool OnPress(int x, int y) const;

  void SetState(int state);
  int GetState() const;
  void SetEnabled(bool enabled);
  void SetCallback(bool (*callback)(void*), void* param);

private:
  int x_, y_;
  int w_, h_;
  int touch_extra_;
  ScreenBuffer* icons_;
  int state_;
  bool (*callback_)(void*);
  void* param_;
  bool enabled_;
};

