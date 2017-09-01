#include "world.h"

#include <iostream>

#include <stdlib.h>

const int seeweed_prob = RAND_MAX / 3;  // 33%
const int coin_prob = RAND_MAX / 20; // 5%

World::World() : world_time_(0), last_move_time_(0) {
  background_elements_ =
      new SpriteMap("mario_sprites.bmp", 16, 16, 16, 2, 0, 0, Color(0,255, 0));
  for (int i = 0; i < 24; i+= 2)
    GenerateNewColumn(i);
  std::cout << "World::World finish" << std::endl;
}

void World::GenerateNewColumn(int col) {
  std::cout << "World::GenerateNewColumn begin" << std::endl;
  int watertop = (rand() % 8) << 1;
  background_[0][col] = watertop;
  background_[0][col + 1] = watertop+1;
  background_[14][col] = 20;
  background_[14][col + 1] = 20;
  for (int i = 1; i < 14; i++) {
     background_[i][col] = rand() % 3 + 16;
     background_[i][col + 1] = rand() % 3 + 16;
  }
  if (rand() < seeweed_prob) {
    int height = rand() % 10 + 4;
    for (int i = height; i < 14; i++)
      background_[i][col] = 19;
  }
  if (rand() < seeweed_prob) {
    int height = rand() % 10 + 4;
    for (int i = height; i < 14; i++)
      background_[i][col + 1] = 19;
  }
  if (rand() < coin_prob) {
    Sprite* sprite = new Sprite(background_elements_, 0,
                                viewport_x_ + 320 + rand() % 16, rand() % 200 + 20, 0, 0, 40);
    sprites_.push_back(sprite);
  }
  std::cout << "World::GenerateNewColumn finish" << std::endl;
}

void World::Tick(long curr_time) {
  std::cout << "World::Tick begin" << std::endl;
  if (curr_time < world_time_)
    world_time_ = 0;
  if (curr_time - world_time_ > 66) {
    world_time_ = curr_time;
    for (int j = 0;j < 24; j++)
      background_[0][j] = (background_[0][j] + 2) % 16;
  }
  std::list<Sprite*>::iterator it = sprites_.begin();
  for (;it != sprites_.end(); ++it) {
    if ((*it)->GetX() < viewport_x_ - 16) {
      it = sprites_.erase(it);
      if (it == sprites_.end())
        break;
    }
    (*it)->UpdateSprite(curr_time);
  }

  int world_moved = (curr_time - last_move_time_) / 50;
  if (world_moved > 1) {
    last_move_time_ += world_moved * 50;
    viewport_x_ += world_moved;
    if (current_viewport_tile_ < viewport_x_ - 32) {
      for (int i = 0; i < 22; i+= 2) {
        for (int j = 0; j < 15; j++) {
          background_[j][i+0] = background_[j][i+2];
          background_[j][i+1] = background_[j][i+3];
        }
      }
      GenerateNewColumn(22);
      current_viewport_tile_ += 32;
    }
  }
  std::cout << "World::Tick finish" << std::endl;
}

void World::Draw() {
  std::cout << "World::Draw begin" << std::endl;
  //std::cout << current_viewport_tile_ << " " << viewport_x_ << current_viewport_tile_ - viewport_x_ << std::endl;
  for (int i = 0; i < 22; i++) {
    for (int j = 0; j < 15; j++) {
      //std::cout << background_[j][i] << std::endl;
      background_elements_->DrawSprite(gScreen,
                                       current_viewport_tile_ - viewport_x_ + i*16,
                                       j * 16, background_[j][i]);
    }
  }
  std::list<Sprite*>::iterator it = sprites_.begin();
  for (;it != sprites_.end(); ++it) {
    (*it)->DrawSprite(gScreen, (*it)->GetX() - viewport_x_, (*it)->GetY());
  }
  gScreen->FlipBuffer();
  std::cout << "World::Draw finish" << std::endl;
}
