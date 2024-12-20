#ifndef UTIL_H
#define UTIL_H

#include "error.h"

#include <array>
#include <cstring>

template <size_t size>
class cstr : public std::array<char, size + 1> {
};

template <size_t size>
bool operator==(const cstr<size> &u, const std::string &v) {
  return u.data() == v;
}

template <size_t size>
bool operator==(const std::string &v, const cstr<size> &u) {
  return u.data() == v;
}

template <size_t sizeu, size_t sizev>
bool operator==(const cstr<sizeu> &u, const cstr<sizev> &v) {
  return strcmp(u.data(), v.data()) == 0;
}

template <size_t sizeu, size_t sizev>
bool operator<(const cstr<sizeu> &u, const cstr<sizev> &v) {
  return strcmp(u.data(), v.data()) < 0;
}

template <size_t size>
bool cstr_end(const cstr<size> s) {
  return strnlen(s.data(), s.max_size()) < s.max_size();
}

template <size_t size>
cstr<size> string2cstr(std::string s) {
  Massert(s.size() <= size, "cstr cannot fit");
  cstr<size> ret;
  strcpy(ret.data(), s.c_str());
  return ret;
}

// TODO: what?
// int string2int(std::string s) {
//   return std::atoi(s.c_str());
// }

bool valid_password(auto s) {
  // TODO
  return true;
}

bool valid_username(auto s) {
  // TODO
  return true;
}

bool valid_userid(auto s) {
  // TODO
  return true;
}

#endif //UTIL_H
