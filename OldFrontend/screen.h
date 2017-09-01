#pragma once

class Screen {
 public:
  Screen(std::string frame_buffer, int width, int height, int bpp);
  ~Screen();

 private:
  int width, height;
  int bpp;
  short *pixels;
};

struct pixel {
    int r, g, b;
};

char digits[12][8] = {{0,60,66,74,82,66,60,0},
                      {0,6,2,2,2,2,2,0},
                      {0,60,66,6,56,64,126,0},
                      {0,60,66,4,2,66,60,0},
                      {0,34,66,66,126,2,2,0},
                      {0,126,64,124,2,2,124,0},
                      {0,60,64,124,66,66,60,0},
                      {0,62,2,4,4,8,8,0},
                      {0,60,66,60,66,66,60,0},
                      {0,60,66,62,2,2,124,0},
                      {0,24,24,0,0,24,24,0},
                      {0,0,0,0,0,0,0,0}};

static inline void put_pixel(struct screen *scr, int x, int y, struct pixel *px) {
    char r, g, b;
    // rrrr rggg gggb bbbb
    r>>=3;
    g>>=2;
    b>>=3;
    short pixel = ((px->r&31)<<11) | ((px->g&63)<<5) | (px->b&31);

    if(y<0) {
        fprintf(stderr, "y out of bounds: %d\n", y);
        y=0;
    }
    if(y>=scr->height) {
        fprintf(stderr, "y out of bounds: %d\n", y);
        y=scr->height-1;
    }
    if(x<0) {
        fprintf(stderr, "x out of bounds: %d\n", x);
        x=0;
    }
    if(x>=scr->width) {
        fprintf(stderr, "x out of bounds: %d\n", x);
        x = scr->width-1;
    }
    scr->pixels[(y*scr->width)+x] = pixel;
}

static inline struct pixel get_pixel(struct screen *scr, int x, int y) {
    struct pixel px;
    short p = scr->pixels[y*scr->width+x];
    px.r = ((p>>11) & 31)<<3;
    px.g = ((p>>5) & 63)<<2;
    px.b = ((p>>0) & 31)<<3;
    return px;
}

void draw_block(int set, int x, int y, struct screen* scr) {
  int i,j;                              
  struct pixel px = {set ? 255 : 0, set ? 255 : 0, set ? 255 : 0};                        
  for (i = 0;i < 8;i++) {                                       
    for (j = 0;j < 8;j++) {                                     
      put_pixel(scr, x + i, y + j, &px);
    }                                                                 
  }                                                                   
}

void draw_big_digit(int symbol, int x, int y, struct screen* scr) {
  int i,j;
  for (i = 0;i < 8;i++) {
    for (j = 0;j < 8;j++) {
      draw_block(digits[symbol][i] & (1 << (7-j)), x + j*8, y + i*8, scr);
    }
  }
}
      
struct pixel black = {0,0,0};      
      
void draw_symbol(unsigned char symbol, int x, int y, struct pixel* c, struct screen* scr) {
  int i,j;
  for (i = 0;i < 16; ++i) {
    for (j = 0;j < 8;++j) {
      if (font[symbol][i] & (1 << (7-j)))
        put_pixel(scr, x + j, y + i, c);
      else  
        put_pixel(scr, x + j, y + i, &black);
    }
  }
}

void draw_string(char* str, int x, int y, struct pixel* c, struct screen* scr) {
  int i;
  int len = strlen(str);
  for (i = 0; i < len; ++i)
    draw_symbol((unsigned char)str[i], x + i*8, y, c, scr);
}    

int main(int argc, char **argv) {
    struct screen scr;
    int fd;
    int column = 0;
    int connection = 0;

    scr.width  = 320;
    scr.height = 240;
    scr.bpp    = 2;

    fd = open("/dev/fb0", O_RDWR);
    if(fd<=0) {
        perror("Unable to open screen");
        return 1;
    }

    scr.pixels = mmap(NULL, scr.width*scr.height*scr.bpp,
                       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if((int)scr.pixels == -1) {
        perror("Unable to mmap screen");
        close(fd);
        return 1;
    }

    // Clear the screen to black.
    bzero(scr.pixels, scr.width*scr.height*scr.bpp);

    signal(SIGPIPE, SIG_IGN);
/*    
    draw_symbol(0,0,0, &scr);
    draw_symbol(1,64,0, &scr);
    draw_symbol(2,128,0, &scr);
    draw_symbol(3,192,0, &scr);
    draw_symbol(4,256,0, &scr);
    draw_symbol(5,0,80, &scr);
    draw_symbol(6,64,80, &scr);
    draw_symbol(7,128,80, &scr);
    draw_symbol(8,192,80, &scr);
    draw_symbol(9,256,80, &scr);
*/
    struct pixel red = {255, 0, 0};
    struct pixel green = {0, 200, 0};
    // Begin looping.
    char blink = 1;
    while(1) {
      time_t t = time(NULL);
      struct tm *ltm = localtime(&t);
      
      blink++;
      draw_big_digit(10 + (blink%2), 128, 40, &scr);
      
      draw_big_digit(ltm->tm_hour / 10, 0 + 16,40, &scr);
      draw_big_digit(ltm->tm_hour % 10, 64 + 16, 40, &scr);
      draw_big_digit(ltm->tm_min / 10, 192 - 16, 40, &scr);
      draw_big_digit(ltm->tm_min % 10, 256 - 16, 40, &scr);

      draw_big_digit(ltm->tm_mday / 10, 0, 134, &scr);
      draw_big_digit(ltm->tm_mday % 10, 64, 134, &scr);
      //draw_big_digit(10, 0, 40, &scr);
      draw_big_digit(ltm->tm_mon / 10, 192, 134, &scr);
      draw_big_digit(ltm->tm_mon % 10, 256, 134, &scr);
     
      FILE *f = fopen("/tmp/nonet", "rt");
      if (f != NULL) {
        draw_string("No net!", 0, 0, &red, &scr);
        fclose(f);
      }
      else
        draw_string("Net ok ", 0, 0, &green, &scr);
      
      usleep(1000000/FLASH_RATE);
    }

    return 0;
}

