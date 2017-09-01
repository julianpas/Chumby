#include "screen.h"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "font.h"

#define ColorTo16(c) ((((c).r & 31) << 11) | (((c).g & 63) << 5) | ((c).b & 31))

Screen* gScreen = NULL;

ScreenBuffer::ScreenBuffer(int width, int height, int bpp)
    : width_(width),
      height_(height),
      bpp_(bpp) {
  back_buffer_ = reinterpret_cast<short int*>(new char[width_ * height_ * bpp_]);
  bzero(back_buffer_, width_ * height_ * bpp_);
}

ScreenBuffer::~ScreenBuffer() {
  delete[] back_buffer_;
}

void ScreenBuffer::PutPixel(int x, int y, const Color& c) {
    short pixel = ColorTo16(c);
    x %= width_;
    y %= height_;
    back_buffer_[(y * width_) + x] = pixel;
}

Color ScreenBuffer::GetPixel(int x, int y) {
    Color px;
    x %= width_;
    y %= height_;
    short p = back_buffer_[y * width_ + x];
    px.r = ((p>>11) & 31) << 3;
    px.g = ((p>>5) & 63) << 2;
    px.b = ((p>>0) & 31) << 3;
    return px;
}

void ScreenBuffer::DrawRectangle(int x, int y, int w, int h, const Color& c) {
  short pixel = ColorTo16(c);
  for (int j = y;j < y+h; j++) {
    for (int i = x;i < x+w; i++) {
      back_buffer_[(j * width_) + i] = pixel;
    }
  }
}

void ScreenBuffer::ClearRectangle(int x, int y, int w, int h) {
  int width = w * bpp_;
  for (int j = y;j < y+h; j++)
    bzero(back_buffer_ + width_ * j + x, width);
}

void ScreenBuffer::DrawDigit(int x, int y, int size, const Color& c, int d) {
  for (int i = 0;i < 8;i++) {
    for (int j = 0;j < 8;j++) {
      if (kDigits[d][i] & (1 << (7-j)))
	DrawRectangle(x + j*size, y + i*size, size, size, c);
    }
  }
}


void ScreenBuffer::DrawSymbol(int x, int y, int size, const Color& c, char s) {
  int i,j;
  for (i = 0;i < 16; ++i) {
    for (j = 0;j < 8;++j) {
      if (font[static_cast<unsigned char>(s)][i] & (1 << (7-j)))
        DrawRectangle(x + j*size, y + i*size, size, size, c);
    }
  }
}

void ScreenBuffer::DrawString(int x, int y, int size, const Color& c,
                        const std::string& s) {
  int i;
  int len = s.length();
  for (i = 0; i < len; ++i)
    DrawSymbol(x + i * size * 8, y, size, c, s[i]);
}

void ScreenBuffer::ClearBuffer() {
  bzero(back_buffer_, width_ * height_ * bpp_);
}

void ScreenBuffer::Swap(ScreenBuffer* other) {
  short int* tmp = back_buffer_;
  back_buffer_ = other->back_buffer_;
  other->back_buffer_ = tmp;
}

void ScreenBuffer::Copy(const ScreenBuffer& other) {
  memcpy(back_buffer_, other.back_buffer_, width_ * height_ * bpp_);
}

void ScreenBuffer::BitBlt(const ScreenBuffer& src, int sx, int sy,
                          int w, int h, int dx, int dy) {
  int copy_width = w * bpp_;
  for (int j = 0;j < h; j++) {
    memcpy(&back_buffer_ [(dy + j) * width_ + dx],
	   &src.back_buffer_ [(sy + j) * src.width_ + sx],
	   copy_width);
  }
}

void ScreenBuffer::BitBltTrans(const ScreenBuffer& src, int sx, int sy,
                               int w, int h, int dx, int dy, Color transkey) {
  short trans = ColorTo16(transkey);
  for (int j = 0;j < h; j++) {
    if (dy + j < 0 || dy + j > height_)
      continue;
    int soff = (sy + j) * src.width_ + sx;
    int doff = (dy + j) * width_ + dx;
    for (int i = 0; i < w; i++) {
      if (dx + i < 0 || dx + i > width_)
        continue;
      if (true || src.back_buffer_[soff + i] != trans)
        back_buffer_[doff + i] = src.back_buffer_[soff + i];
    }
  }
}

Screen::Screen(std::string frame_buffer, int width, int height, int bpp)
    : ScreenBuffer(width, height, bpp) {
  frame_buffer_ = open(frame_buffer.c_str(), O_RDWR);
  if (frame_buffer_ <= 0) {
    std::cout <<  "Unable to open screen";
    exit(0);
  }

  fb_buffer_ = static_cast<short*>(mmap(NULL, width_ * height_ * bpp_,
                                        PROT_READ | PROT_WRITE, MAP_SHARED,
				        frame_buffer_, 0));
  if(fb_buffer_ == MAP_FAILED) {
    std::cout <<  "Unable to mmap screen";
    close(frame_buffer_);
  }

  bzero(fb_buffer_, width_ * height_ * bpp_);

  signal(SIGPIPE, SIG_IGN);
}

Screen::~Screen() {
  close(frame_buffer_);
}

void Screen::FlipBuffer() {
  memcpy(fb_buffer_, back_buffer_, width_ * height_ * bpp_);
}
