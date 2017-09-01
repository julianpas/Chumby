#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/time.h>
#include <pthread.h>

int playing = 0;
int fd_kbd;

void* kbdThread(void* threadId) {
  size_t rb;
  struct input_event ev[64];
  struct input_event last_down;
  int vol = 210;
  while(1) {
    rb=read(fd_kbd,ev,sizeof(struct input_event)*64);
    printf("rb = %d", rb);
    if (rb > 0 && rb >= (int) sizeof(struct input_event)) {
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
  }   
}

int main (int argc, char** argv) {
  if ((fd_kbd = open(argv[1], O_RDONLY)) < 0) {
    perror("Error opening kbd device");
    exit(1);
  }
  
  int fd_wheel;
  if ((fd_wheel = open(argv[2], O_RDONLY)) < 0) {
    perror("Error opening wheel device");
    exit(1);
  }
  
  int fd_bl;
  if ((fd_bl = open(argv[3], O_RDWR)) < 0) {
    perror("Error opening backlight control");
    exit(1);
  }
 
  pthread_t kbd_thread;
  pthread_create(&kbd_thread, NULL, &kbdThread, NULL);
  
  char str[20]; 
  size_t rb,rb2;
  struct input_event ev[64];
  struct input_event last_down;
  int vol = 210;
  while(1) {
    rb=read(fd_wheel,ev,sizeof(struct input_event)*64);
    if (rb > 0 && rb >= (int) sizeof(struct input_event)) {
      int i;        
      for (i = 0; i < (int)(rb / sizeof(struct input_event)); i++) {
        if (ev[i].type == EV_REL && ev[i].code == REL_WHEEL) {
          if (playing) {
            vol += ev[i].value;
            if (vol < 0) vol = 0;
            if (vol > 255) vol = 255;
            sprintf(str, "amixer sset DAC %d", vol);
            system(str);
          } else {    
            lseek(fd_bl, 0, SEEK_SET);
            rb2 = read(fd_bl, str, 19);
            str[rb2] = 0;
            int curr_brightness = atoi(str);
            curr_brightness += ev[i].value * (1 + curr_brightness / 30);
            if (curr_brightness > 100) curr_brightness = 100;
            if (curr_brightness < 0) curr_brightness = 0;
            sprintf(str, "%d", curr_brightness);
            rb2 = write(fd_bl, str, strlen(str));
          }  
        }
      }
    }
  }

  close(fd_kbd);
  close(fd_wheel);
  close(fd_bl);

  return 0;
}
