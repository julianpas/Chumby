#include <iostream>

#include "accel_handler.h"
#include "brightness_controller.h"
#include "calib_display.h"
#include "clock.h"
#include "input_device_handler.h"
#include "event_manager.h"
#include "mario.h"
#include "message_loop.h"
#include "message_queue.h"
#include "radio.h"
#include "lights.h"
#include "screen.h"
#include "touch_screen_controller.h"

const char wheel_dev[] = "/dev/input/by-id/soc-noserial-event-joystick";
const char button_dev[] = "/dev/input/by-id/soc-noserial-event-kbd";
const char touch_dev[] = "/dev/input/by-id/soc-noserial-event-ts";
const char fb_dev[] = "/dev/fb0";

int main(int argc, char **argv) {
  MessageLoop message_loop(std::auto_ptr<MessageQueue>(new MessageQueue()));

  gScreen = new Screen("/dev/fb0", 320, 240, 2);

  InputDeviceHandler wheel(&message_loop, wheel_dev);
  InputDeviceHandler button(&message_loop, button_dev);
  InputDeviceHandler touch(&message_loop, touch_dev);
  AccelHandler accel(&message_loop);

  EventManager manager(&message_loop);

  //CalibDisplay calib(&manager);
  TouchScreenController touch_screen(&manager);
  manager.AddTask(&touch_screen);

  BrightnessController brightness(&manager);
  manager.AddTask(&brightness);

  Radio radio(&manager);
  manager.AddTask(&radio);

  Clock clock(&manager);
  manager.AddTask(&clock);

  Lights lights_og(&manager, "og");
  manager.AddTask(&lights_og);
  Lights lights_eg(&manager, "eg");
  manager.AddTask(&lights_eg);
  Lights lights_ug(&manager, "ug");
  manager.AddTask(&lights_ug);
  
  
  //Mario mario(&manager);
  //manager.AddTask(&mario);

  manager.SetActiveTask(&clock);

  message_loop.SetEventManager(&manager);

  message_loop.Run();
  return 0;
}
