#pragma once

class MessageLoopClient {
 public:
  ~MessageLoopClient() {}
  virtual void OnLoopQuitting() = 0;
};
