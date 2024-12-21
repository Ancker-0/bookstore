#ifndef UTIL_H
#define UTIL_H

#include "error.h"
#include "ci.h"

#include <array>
#include <sstream>
#include <cstring>

template <size_t size>
class cstr : public std::array<char, size + 1> {
};

const char userid_chars[] = "1234567890zxcvbnmasdfghjklqwertyuiopZXCVBNMASDFGHJKLQWERTYUIOP_";

static bool inside(char c, const char *s) {
  for (; *s; ++s)
    if (c == *s)
      return true;
  return false;
}

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
bool operator!=(const cstr<sizeu> &u, const cstr<sizev> &v) {
  return not (u == v);
}  // TODO: does C++ derive operator!= from operator== ?

template <size_t sizeu, size_t sizev>
bool operator<(const cstr<sizeu> &u, const cstr<sizev> &v) {
  return strcmp(u.data(), v.data()) < 0;
}

static std::vector<std::string> split_keyword(std::string keyword) {
  // TODO;
}

template <size_t size>
bool cstr_end(const cstr<size> s) {
  return strnlen(s.data(), s.max_size()) < s.max_size();
}

template <size_t size>
bool cstr_null(const cstr<size> s) {
  return strnlen(s.data(), s.max_size()) == 0;
}

template <size_t size>
static cstr<size> string2cstr(std::string s) {
  Massert(s.size() <= size, "cstr cannot fit");
  cstr<size> ret;
  strncpy(ret.data(), s.c_str(), size + 1);
  return ret;
}

static double string2double(std::string s) {
  std::stringstream ss{s};
  double ret{};
  ss >> ret;
  return ret;
}

static bool param_inside(Tokenized &tk, const std::vector<std::string> &allow) {
  for (auto &[k, _] : tk.param) {
    bool find = false;
    for (auto &s : allow)
      if (s == k) {
        find = true;
        break;
      }
    if (not find)
      return false;
  }
  return true;
}

static int string2int(std::string s) {
  return std::atoi(s.c_str());
}

static bool valid_userid(auto s) {
  if (not std::is_same_v<decltype(s), cstr<30>> or not cstr_end(s))
    return false;
  for (int i = 0; s[i]; ++i)
    if (not inside(s[i], userid_chars))
      return false;
  return true;
}

static bool valid_password(auto s) {
  return valid_userid(s);
}

static bool valid_username(auto s) {
  // TODO
  return true;
}

#endif //UTIL_H
