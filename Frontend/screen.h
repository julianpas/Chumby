#pragma once

#include <string>

struct Color {
  Color(int rp, int gp, int bp) : r(rp / 8), g(gp / 4), b(bp / 8) {}
  Color() : r(0), g(0), b(0) {}
  int r, g, b;
};

const char kDigits[14][8] = {{0,60,66,74,82,66,60,0},  // 0
                             {0,6,2,2,2,2,2,0},        // 1
                             {0,60,66,6,56,64,126,0},  // 2
                             {0,60,66,4,2,66,60,0},    // 3
                             {0,34,66,66,126,2,2,0},   // 4
                             {0,126,64,124,2,2,124,0}, // 5
                             {0,60,64,124,66,66,60,0}, // 6
                             {0,62,2,4,4,8,8,0},       // 7
                             {0,60,66,60,66,66,60,0},  // 8
                             {0,60,66,62,2,2,124,0},   // 9
                             {0,24,24,0,0,24,24,0},    // :
                             {0,0,0,0,0,24,24,0},      // .
                             {2,5,2,0,0,0,0,0},        // ´
                             {0,0,0,0,0,0,0,0}};       //

const Color kBlack(0,0,0);
const Color kWhite(255,255,255);
const Color kBlue(100,100,255);
const Color kGreen(150,255,50);
const Color kCyan(100,255,255);
const Color kGrey(100,100,100);

class ScreenBuffer {
 public:
  ScreenBuffer(int width, int height, int bpp);
  ~ScreenBuffer();

  void PutPixel(int x, int y, const Color& c);
  Color GetPixel(int x, int y);

  void DrawRectangle(int x, int y, int w, int h, const Color& c);
  void ClearRectangle(int x, int y, int w, int h);

  void DrawDigit(int x,int y, int size, const Color& c, int d);
  void DrawSymbol(int x, int y, int size, const Color& c, char s);
  void DrawString(int x, int y, int size, const Color& c, const std::string& s);

  void ClearBuffer();

  void Swap(ScreenBuffer* other);
  void Copy(const ScreenBuffer& other);
  void BitBlt(const ScreenBuffer& src, int sx, int sy, int w, int h,
	      int dx, int dy);
  void BitBltTrans(const ScreenBuffer& src, int sx, int sy, int w, int h,
	           int dx, int dy, Color transkey);

 protected:
  int width_;
  int height_;
  int bpp_;
  short int* back_buffer_;
};

class Screen : public ScreenBuffer {
 public:
  Screen(std::string frame_buffer, int width, int height, int bpp);
  ~Screen();

  void FlipBuffer();

 private:
  int frame_buffer_;
  short int* fb_buffer_;
};

extern Screen* gScreen;
