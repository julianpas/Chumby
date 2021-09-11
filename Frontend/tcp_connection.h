#pragma once

#include <arpa/inet.h>

#include <string>

#include "json/json/json.h"

class TcpConnection {
 public:  
  TcpConnection(const std::string& ip, int port);
  ~TcpConnection();
  
  bool connect();
  bool send(const std::string& data);
  int receive(size_t maxsizekb, std::string* output);
  void close();
  
  static bool getJson(const std::string data, Json::Value* root);
 
  static bool hostnameToIp(const std::string& hostname , std::string* ip);
  
 private:
  struct sockaddr_in server_;
  int sock_;
  char* buffer_;
};

