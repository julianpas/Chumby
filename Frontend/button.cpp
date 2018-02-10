#include "button.h"

#include "easybmp/EasyBMP.h"
#include "screen.h"

Button::Button(int x, int y, int w, int h, const std::string& images, int state, int touch_extra) 
    : x_(x), y_(y),
    w_(w), h_(h),
    touch_extra_(touch_extra),
    state_(state),
    enabled_(true) {
  if (images.length()) {
    BMP icons;
    icons.ReadFromFile(images.c_str());
    icons_ = new ScreenBuffer(icons.TellWidth(), icons.TellHeight(), 2);
    for (int i = 0;i < icons.TellWidth(); ++i) {
      for (int j = 0 ;j < icons.TellHeight(); ++j) {
        icons_->PutPixel(i, j, Color(icons(i,j)->Red, icons(i,j)->Green, icons(i,j)->Blue));
      }
    }
  }
}

void Button::Draw() const {
  if (!icons_)
    return;
  gScreen->BitBlt(*icons_, state_ * w_, 0, w_, h_, x_,y_);
}

bool Button::OnPress(int x, int y) const {
  if (enabled_ &&
      x >= x_ - touch_extra_ && x <= x_ + w_ + touch_extra_ && 
      y >= y_ - touch_extra_ && y <= y_ + w_ + touch_extra_) {
    if (callback_)
      return callback_(param_);
    return true;
  }
  return false;
}

void Button::SetState(int state) {
  state_ = state;
}

int Button::GetState() const {
  return state_;
}

void Button::SetEnabled(bool enabled) {
  enabled_ = enabled;
}

void Button::SetCallback(bool (*callback)(void*), void* param) {
  callback_ = callback;
  param_ = param;
}
