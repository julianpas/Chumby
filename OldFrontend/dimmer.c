#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/time.h>
#include <errno.h>

int main (int argc, char** argv) {
  int fd;
  if ((fd = open(argv[1], O_RDONLY)) < 0) {
    perror("evdev open");
    exit(1);
  }
  
  int bl;
  if ((bl = open(argv[2], O_RDWR)) < 0) {
    perror("evdev open");
    exit(1);
  }
  
  char str[20]; 
  size_t rb,rb2;
  struct input_event ev[64];
  
  while(1) {
    rb=read(fd,ev,sizeof(struct input_event)*64);
    if (rb < (int) sizeof(struct input_event)) {
      printf("%d", rb);
      perror("evtest: short read");
      exit (1);
    }
    
    int i;        
    for (i = 0; i < (int)(rb / sizeof(struct input_event)); i++) {
      if (ev[i].type == EV_REL && ev[i].code == REL_WHEEL) {
        lseek(bl, 0, SEEK_SET);
        rb2 = read(bl, str, 19);
        str[rb2] = 0;
        int curr_brightness = atoi(str);
        curr_brightness += ev[i].value * 3;
        if (curr_brightness > 100) curr_brightness = 100;
        if (curr_brightness < 0) curr_brightness = 0;
        sprintf(str, "%d", curr_brightness);
        rb2 = write(bl, str, strlen(str));
      }
    }
  }

  close(fd);
  close(bl);

  return 0;
}
