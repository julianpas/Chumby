TOOLCHAIN_PREFIX=../arm-2008q3/bin/arm-none-linux-gnueabi-
CXX=$(TOOLCHAIN_PREFIX)g++
CC=$(TOOLCHAIN_PREFIX)gcc
STRIP=$(TOOLCHAIN_PREFIX)strip
CPPFLAGS=-c -Wall
LDFLAGS=-lpthread -ldl
CXX_SOURCES=main.cpp message_queue.cpp message_loop.cpp event_manager.cpp input_device_handler.cpp task_base.cpp screen.cpp clock.cpp brightness_controller.cpp touch_screen_controller.cpp calib_display.cpp radio.cpp accel_handler.cpp mario.cpp world.cpp sprite.cpp button.cpp lights.cpp easybmp/EasyBMP.cpp accel/accel.cpp accel/i2c.cpp json/jsoncpp.cpp
CC_SOURCES=sqlite/sqlite3.c
OBJECTS=$(CC_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o)
EXECUTABLE=chumby

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	$(STRIP) -s $@

clean:
	rm *.o easybmp/*.o accel/*.o sqlite/*.o json/*.o chumby

deploy: $(EXECUTABLE)
	scp chumby root@chumby:/mnt/storage/jul/new_system/
