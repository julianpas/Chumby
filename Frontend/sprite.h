#include <string>

#include "easybmp/EasyBMP.h"

#include "screen.h"

class SpriteMap : public ScreenBuffer{
 public:
  SpriteMap(std::string file, int width, int height,
            int gridx, int gridy, int startx, int starty, Color transkey);

  void DrawSprite(ScreenBuffer* dst, int dx, int dy, int index);

  int GetSpriteCount() { return sprite_count_; }
 private:
  Color trans_key_;
  int sprite_width_;
  int sprite_count_;
};

class Sprite {
 public:
  Sprite(SpriteMap* sprite_map, int initial_state, int initial_x, int initial_y,
         int initial_velocity, int initial_vector, int animation_speed);
  Sprite(const BMP& sprite_bitmap, int initial_state, int initial_x, int initial_y,
         int initial_velocity, int initial_vector, int animation_speed);

  void SetVelocity(int velocity) { velocity_ = velocity; }
  int GetVelocity() { return velocity_; }

  void SetVectory(int vector) { vector_ = vector; }
  int GetVector() { return vector_; }

  int GetX() { return pos_x_; }
  int GetY() { return pos_y_; }

  void UpdateSprite(long curr_time);
  void DrawSprite(ScreenBuffer* dst, int dx, int dy);
  void RemoveSprite(ScreenBuffer* dst, int dx, int dy);
 private:
  SpriteMap* sprite_map_;
  ScreenBuffer* backgroud_;
  int state_;
  int velocity_;
  int vector_;
  int animation_speed_;
  int pos_x_;
  int pos_y_;
  long move_sprite_time_;
  long anim_sprite_time_;
};
