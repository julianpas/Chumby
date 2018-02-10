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

int main(int argc, char **argv) {
  gScreen = new Screen("/dev/fb0", 320, 240, 2);

  MessageLoop message_loop(std::auto_ptr<MessageQueue>(new MessageQueue()));

  InputDeviceHandler wheel(&message_loop, "/dev/input/by-id/soc-noserial-event-joystick");
  InputDeviceHandler button(&message_loop, "/dev/input/by-id/soc-noserial-event-kbd");
  InputDeviceHandler touch(&message_loop, "/dev/input/by-id/soc-noserial-event-ts");
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

  Lights lights(&manager);
  manager.AddTask(&lights);
  
  
  //Mario mario(&manager);
  //manager.AddTask(&mario);

  manager.SetActiveTask(&clock);

  message_loop.SetEventManager(&manager);

  message_loop.Run();
  return 0;
}
