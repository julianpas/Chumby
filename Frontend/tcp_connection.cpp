#include "tcp_connection.h"

#include <auto_ptr.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

TcpConnection::TcpConnection(const std::string& ip, int port) : sock_(-1), buffer_(NULL) {
  server_.sin_addr.s_addr = inet_addr(ip.c_str());
  server_.sin_family = AF_INET;
  server_.sin_port = htons(port);
}

TcpConnection::~TcpConnection() {
  if (buffer_)
    delete[] buffer_;
  
  if (sock_ != -1)
    close();
}

bool TcpConnection::connect() {
  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_ == -1)
    return false;
  
  return (::connect(sock_, (struct sockaddr*)&server_, sizeof(server_)) >= 0);
}

bool TcpConnection::send(const std::string& data) {
  if (sock_ == -1)
    return false;

  return (::send(sock_, data.data(), data.size(), 0) >= 0);
}

int TcpConnection::receive(size_t maxsizekb, std::string* output) {
  if (sock_ == -1)
    return false;

  if (!buffer_) {
    buffer_ = new char[1024];
  }
  int len = 0;
  output->clear();
  do {
    len = ::recv(sock_, buffer_, 1024, 0);
    if (len > 0)
      output->append(buffer_, len);
  } while(len > 0 && output->size() <= maxsizekb * 1024);
  return output->size();
}

void TcpConnection::close() {
  if (sock_ != -1)
    ::close(sock_);
  sock_ = -1;
}

// static 
bool TcpConnection::getJson(const std::string data, Json::Value* root) {
  std::string::size_type pos = data.find("\r\n\r\n");
  if (pos == std::string::npos) {
    pos = data.find("\n\n");
    if (pos == std::string::npos)
      return false;
    else
      pos += 2;
  } else {
    pos += 4;
  }
  Json::CharReaderBuilder builder;
  std::auto_ptr<Json::CharReader> reader(builder.newCharReader());
  return reader->parse(data.data() + pos, data.data() + data.size(), root, NULL);
}

// static 
bool TcpConnection::hostnameToIp(const std::string& hostname , std::string* ip) {
  hostent *he;
  he = gethostbyname(hostname.c_str());
  if (!he) 
    return false;

  in_addr **addr_list = reinterpret_cast<in_addr**>(he->h_addr_list);
  if (addr_list[0] != NULL) {
      *ip = inet_ntoa(*addr_list[0]);
      return true;
  }
  return false;
}

