#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/time.h>

int main (int argc, char** argv) {
  int fd;
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("evdev open");
    exit(1);
  }
  
  size_t rb;
  struct input_event ev[64];
  struct input_event last_down;
  int playing = 0;
  while(1) {
    rb=read(fd,ev,sizeof(struct input_event)*64);
    if (rb < (int) sizeof(struct input_event)) {
      perror("evtest: short read");
      exit (1);
    }
    
    int i;        
    for (i = 0; i < (int)(rb / sizeof(struct input_event)); i++) {
      if (ev[i].type == EV_KEY && ev[i].code == KEY_ENTER) {
        if (ev[i].value) {
          last_down = ev[i];
          system("aplay /mnt/storage/sounds/error.wav&");
        } else {
          if (ev[i].time.tv_sec - last_down.time.tv_sec > 2) {
            system("restart_network");
            system("aplay /mnt/storage/sounds/error.wav&");
          } else {
            if (!playing) {
              playing = 1;
              system("btplay --passthru=\"play * `cat /mnt/storage/jul/station`\"");
            } else {
              playing = 0;
              system("btplay --passthru=\"stop\"");
            }
          }
        }      
      }    
    }
  }

  close(fd);

  return 0;
}
