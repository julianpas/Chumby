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
  char evtype_b[4];
  memset(evtype_b, 0, sizeof(evtype_b));
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evtype_b) < 0) {
    perror("evdev ioctl");
  }

  printf("Supported event types: %02X%02X%02X%02X\n",(int)evtype_b[0], (int)evtype_b[1], (int)evtype_b[2], (int)evtype_b[3]);

  fd_set fdset;
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  
  struct timeval tv;
  
  
  while(1) {
    memset(&tv, 0, sizeof(tv));
    int retval = select(1, &fdset, NULL, NULL, &tv);
    printf("RV: %d (%d)\n", retval, (int)FD_ISSET(fd, &fdset));
    
    /* how many bytes were read */
    size_t rb;
    /* the events (up to 64 at once) */
    struct input_event ev[64];
    
    rb=read(fd,ev,sizeof(struct input_event)*64);
    if (rb < (int) sizeof(struct input_event)) {
      perror("evtest: short read");
      exit (1);
    }
    
    int yalv;        
    for (yalv = 0; yalv < (int) (rb / sizeof(struct input_event)); yalv++) {
      printf("%ld.%06ld ", ev[yalv].time.tv_sec, ev[yalv].time.tv_usec);
      printf("type %d code %d value %d\n", ev[yalv].type, ev[yalv].code, ev[yalv].value);
    }
  }

  close(fd);

  return 0;
}
