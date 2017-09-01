#include <list>

#include "sprite.h"

class World {
 public:
  World();

  //void AddSprite(Sprite* sprite);
  void GenerateNewColumn(int col);

  void Tick(long curr_time);
  void Draw();

 private:
  std::list<Sprite*> sprites_;
  SpriteMap* background_elements_;
  int background_[15][24];
  long viewport_x_;
  long viewport_y_;
  long current_viewport_tile_;
  long world_time_;
  long last_move_time_;
};
