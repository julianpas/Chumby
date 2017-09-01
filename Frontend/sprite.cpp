#include "sprite.h"

static int sin_table[360] = {
  0,    17,    34,    52,    69,
  87,   104,   121,   139,   156,
  173,   190,   207,   224,   241,
  258,   275,   292,   309,   325,
  342,   358,   374,   390,   406,
  422,   438,   453,   469,   484,
  499,   515,   529,   544,   559,
  573,   587,   601,   615,   629,
  642,   656,   669,   681,   694,
  707,   719,   731,   743,   754,
  766,   777,   788,   798,   809,
  819,   829,   838,   848,   857,
  866,   874,   882,   891,   898,
  906,   913,   920,   927,   933,
  939,   945,   951,   956,   961,
  965,   970,   974,   978,   981,
  984,   987,   990,   992,   994,
  996,   997,   998,   999,   999,
  1000,   999,   999,   998,   997,
  996,   994,   992,   990,   987,
  984,   981,   978,   974,   970,
  965,   961,   956,   951,   945,
  939,   933,   927,   920,   913,
  906,   898,   891,   882,   874,
  866,   857,   848,   838,   829,
  819,   809,   798,   788,   777,
  766,   754,   743,   731,   719,
  707,   694,   681,   669,   656,
  642,   629,   615,   601,   587,
  573,   559,   544,   529,   515,
  499,   484,   469,   453,   438,
  422,   406,   390,   374,   358,
  342,   325,   309,   292,   275,
  258,   241,   224,   207,   190,
  173,   156,   139,   121,   104,
  87,    69,    52,    34,    17,
  0,   -17,   -34,   -52,   -69,
  -87,  -104,  -121,  -139,  -156,
  -173,  -190,  -207,  -224,  -241,
  -258,  -275,  -292,  -309,  -325,
  -342,  -358,  -374,  -390,  -406,
  -422,  -438,  -453,  -469,  -484,
  -500,  -515,  -529,  -544,  -559,
  -573,  -587,  -601,  -615,  -629,
  -642,  -656,  -669,  -681,  -694,
  -707,  -719,  -731,  -743,  -754,
  -766,  -777,  -788,  -798,  -809,
  -819,  -829,  -838,  -848,  -857,
  -866,  -874,  -882,  -891,  -898,
  -906,  -913,  -920,  -927,  -933,
  -939,  -945,  -951,  -956,  -961,
  -965,  -970,  -974,  -978,  -981,
  -984,  -987,  -990,  -992,  -994,
  -996,  -997,  -998,  -999,  -999,
  -1000,  -999,  -999,  -998,  -997,
  -996,  -994,  -992,  -990,  -987,
  -984,  -981,  -978,  -974,  -970,
  -965,  -961,  -956,  -951,  -945,
  -939,  -933,  -927,  -920,  -913,
  -906,  -898,  -891,  -882,  -874,
  -866,  -857,  -848,  -838,  -829,
  -819,  -809,  -798,  -788,  -777,
  -766,  -754,  -743,  -731,  -719,
  -707,  -694,  -681,  -669,  -656,
  -642,  -629,  -615,  -601,  -587,
  -573,  -559,  -544,  -529,  -515,
  -500,  -484,  -469,  -453,  -438,
  -422,  -406,  -390,  -374,  -358,
  -342,  -325,  -309,  -292,  -275,
  -258,  -241,  -224,  -207,  -190,
  -173,  -156,  -139,  -121,  -104,
  -87,   -69,   -52,   -34,   -17};
static int cos_table[360] = {
  1000,   999,   999,   998,   997,
  996,   994,   992,   990,   987,
  984,   981,   978,   974,   970,
  965,   961,   956,   951,   945,
  939,   933,   927,   920,   913,
  906,   898,   891,   882,   874,
  866,   857,   848,   838,   829,
  819,   809,   798,   788,   777,
  766,   754,   743,   731,   719,
  707,   694,   681,   669,   656,
  642,   629,   615,   601,   587,
  573,   559,   544,   529,   515,
  500,   484,   469,   453,   438,
  422,   406,   390,   374,   358,
  342,   325,   309,   292,   275,
  258,   241,   224,   207,   190,
  173,   156,   139,   121,   104,
  87,    69,    52,    34,    17,
  0,   -17,   -34,   -52,   -69,
  -87,  -104,  -121,  -139,  -156,
  -173,  -190,  -207,  -224,  -241,
  -258,  -275,  -292,  -309,  -325,
  -342,  -358,  -374,  -390,  -406,
  -422,  -438,  -453,  -469,  -484,
  -499,  -515,  -529,  -544,  -559,
  -573,  -587,  -601,  -615,  -629,
  -642,  -656,  -669,  -681,  -694,
  -707,  -719,  -731,  -743,  -754,
  -766,  -777,  -788,  -798,  -809,
  -819,  -829,  -838,  -848,  -857,
  -866,  -874,  -882,  -891,  -898,
  -906,  -913,  -920,  -927,  -933,
  -939,  -945,  -951,  -956,  -961,
  -965,  -970,  -974,  -978,  -981,
  -984,  -987,  -990,  -992,  -994,
  -996,  -997,  -998,  -999,  -999,
  -1000,  -999,  -999,  -998,  -997,
  -996,  -994,  -992,  -990,  -987,
  -984,  -981,  -978,  -974,  -970,
  -965,  -961,  -956,  -951,  -945,
  -939,  -933,  -927,  -920,  -913,
  -906,  -898,  -891,  -882,  -874,
  -866,  -857,  -848,  -838,  -829,
  -819,  -809,  -798,  -788,  -777,
  -766,  -754,  -743,  -731,  -719,
  -707,  -694,  -681,  -669,  -656,
  -642,  -629,  -615,  -601,  -587,
  -573,  -559,  -544,  -529,  -515,
  -500,  -484,  -469,  -453,  -438,
  -422,  -406,  -390,  -374,  -358,
  -342,  -325,  -309,  -292,  -275,
  -258,  -241,  -224,  -207,  -190,
  -173,  -156,  -139,  -121,  -104,
  -87,   -69,   -52,   -34,   -17,
  0,    17,    34,    52,    69,
  87,   104,   121,   139,   156,
  173,   190,   207,   224,   241,
  258,   275,   292,   309,   325,
  342,   358,   374,   390,   406,
  422,   438,   453,   469,   484,
  500,   515,   529,   544,   559,
  573,   587,   601,   615,   629,
  642,   656,   669,   681,   694,
  707,   719,   731,   743,   754,
  766,   777,   788,   798,   809,
  819,   829,   838,   848,   857,
  866,   874,   882,   891,   898,
  906,   913,   920,   927,   933,
  939,   945,   951,   956,   961,
  965,   970,   974,   978,   981,
  984,   987,   990,   992,   994,
  996,   997,   998,   999,   999};

SpriteMap::SpriteMap(std::string file, int width, int height,
                     int gridx, int gridy,
                     int startx, int starty, Color transkey)
    : ScreenBuffer(width * gridx * gridy, height, 2),
      trans_key_(transkey),
      sprite_width_(width),
      sprite_count_(gridx * gridy) {
  BMP sprite_bitmap;
  sprite_bitmap.ReadFromFile(file.c_str());
  for (int gy = 0; gy < gridy; gy++) {
    for (int gx = 0; gx < gridx; gx++) {
      int soffx = gx * width + startx;
      int soffy = gy * height + starty;
      int doffx = (gx + gy * gridx) * width;
      for (int i = 0;i < height; ++i) {
        for (int j = 0 ;j < width; ++j) {
          PutPixel(j + doffx, i,
                   Color(sprite_bitmap(j + soffx,i + soffy)->Red,
                         sprite_bitmap(j + soffx,i + soffy)->Green,
                         sprite_bitmap(j + soffx,i + soffy)->Blue));
        }
      }
    }
  }
}
/*
SpriteMap::SpriteMap(const BMP& sprite_bitmap, int width, int height,
                     int gridx, int gridy,
                     int startx, int starty, Color transkey)
    : ScreenBuffer(width * gridx * gridy, height, 2),
      trans_key_(transkey),
      sprite_width_(width),
      sprite_count_(gridx * gridy) {
  for (int gy = 0; gy < gridy; gy++) {
    for (int gx = 0; gx < gridx; gx++) {
      int soffx = gx * width + startx;
      int soffy = gy * height + starty;
      int doffx = (gx + gy * gridx) * width;
      for (int i = 0;i < height; ++i) {
        for (int j = 0 ;j < width; ++j) {
          PutPixel(j + doffx, i,
                   Color(sprite_bitmap(j + soffx,i + soffy)->Red,
                         sprite_bitmap(j + soffx,i + soffy)->Green,
                         sprite_bitmap(j + soffx,i + soffy)->Blue));
        }
      }
    }
  }
}
*/
void SpriteMap::DrawSprite(ScreenBuffer* dst, int dx, int dy, int index) {
  dst->BitBltTrans(*this, index * sprite_width_, 0, sprite_width_, height_,
                   dx, dy, trans_key_);
}

Sprite::Sprite(SpriteMap* sprite_map, int initial_state,
               int initial_x, int initial_y,
               int initial_velocity, int initial_vector, int animation_speed)
    : sprite_map_(sprite_map),
      state_(initial_state),
      velocity_(initial_velocity),
      vector_(initial_vector),
      animation_speed_(animation_speed),
      pos_x_(initial_x),
      pos_y_(initial_y),
      move_sprite_time_(0),
      anim_sprite_time_(0){
}

void Sprite::UpdateSprite(long curr_time) {
  if (velocity_) {
    int delta_pix = (curr_time - move_sprite_time_) / velocity_;
    int delta_x = delta_pix * cos_table[vector_] / 1000;
    int delta_y = delta_pix * sin_table[vector_] / 1000;
    pos_x_ += delta_x;
    pos_y_ += delta_y;
    move_sprite_time_ += delta_pix * velocity_;
  }
  int state_change = (curr_time - anim_sprite_time_) / animation_speed_;
  state_ += state_change;
  anim_sprite_time_ += state_change * animation_speed_;
  state_ %= sprite_map_->GetSpriteCount();
}

void Sprite::DrawSprite(ScreenBuffer* dst, int dx, int dy) {
  sprite_map_->DrawSprite(dst, dx, dy, state_);
}

void Sprite::RemoveSprite(ScreenBuffer* dst, int dx, int dy) {
}
