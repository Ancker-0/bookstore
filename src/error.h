#ifndef ERROR_H
#define ERROR_H

#include "config.h"

#include <exception>
#include <string>

#if NO_ERRF
#define errf(...) 42
#else
#define errf(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
#endif

class Error : std::exception {
public:
  explicit Error(const std::string &msg_) : msg(msg_) {}
  std::string msg;
};

inline void Eassert(bool condition, std::string msg = "") {
  if (not condition)
    throw Error(msg);
}

#define Massert(condition, msg) \
  ({ if (not (condition)) throw Error(std::string(__func__) + ": " + (msg)); })

#endif //ERROR_H
