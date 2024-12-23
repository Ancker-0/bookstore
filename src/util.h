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

static const char userid_chars[] = "1234567890zxcvbnmasdfghjklqwertyuiopZXCVBNMASDFGHJKLQWERTYUIOP_";
static const char username_chars[] = " 0@P`p!1AQaq\"2BRbr#3CScs$4DTdt%5EUeu&6FVfv'7GWgw(8HXhx)9IYiy*:JZjz+;K[k{,<L\\l|-=M]m}.>N^n~/?O_o";  // also used by ISBN
static const char bookname_chars[] = " 0@P`p!1AQaq2BRbr#3CScs$4DTdt%5EUeu&6FVfv'7GWgw(8HXhx)9IYiy*:JZjz+;K[k{,<L\\l|-=M]m}.>N^n~/?O_o";

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
  std::vector<std::string> ret;
  std::string now;
  for (int i = 0; i < (int)keyword.size(); ++i)
    if (keyword[i] == '|') {
      Massert(i > 0 and now != "" and i + 1 != (int)keyword.size() and keyword[i + 1] != '|', "parse fail");
      ret.push_back(now);
      now = "";
    } else
      now += keyword[i];
  if (now != "")
    ret.push_back(now);
  for (int i = 1; i < (int)ret.size(); ++i)
    for (int j = 0; j < i; ++j)
      Massert(ret[i] != ret[j], "duplicated keyword");
  return ret;
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

static std::string double2string(double r) {
  std::stringstream ss;
  ss << r;
  std::string ret;
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
  // return std::atoi(s.c_str());
  Massert(s.size() > 0, "empty string");
  Massert(s.size() <= 10, "too large");
  for (int i = 0; i < (int)s.size(); ++i)
    Massert('0' <= s[i] and s[i] <= '9', "not number");
  std::stringstream ss;
  ss << s;
  long long ret;
  ss >> ret;
  Massert(ret <= 2147483647, "too large");
  return ret;
}

static bool valid_userid(cstr<30> s);

static bool valid_userid(const std::string &s) {
  return valid_userid(string2cstr<30>(s));
}

static bool valid_userid(cstr<30> s) {
  if (not cstr_end(s))
    return false;
  for (int i = 0; s[i]; ++i)
    if (not inside(s[i], userid_chars))
      return false;
  return true;
}

static bool valid_password(auto s) {
  return valid_userid(s);
}

static bool valid_username(cstr<30> s);

static bool valid_username(const std::string &s) {
  return valid_username(string2cstr<30>(s));
}

static bool valid_username(cstr<30> s) {
  if (not cstr_end(s))
    return false;
  for (int i = 0; s[i]; ++i)
    if (not inside(s[i], username_chars))
      return false;
  return true;
}

static bool valid_bookname(const cstr<60> &s);

static bool valid_bookname(const std::string &s) {
  errf("specific bookname\n");
  return valid_bookname(string2cstr<60>(s));
}

static bool valid_bookname(const cstr<60> &s) {
  // if (not std::is_convertible_v<decltype(s), cstr<60>> or not cstr_end(s))
  //   return false;
  if (not cstr_end(s))
    return false;
  for (int i = 0; s[i]; ++i)
    if (not inside(s[i], bookname_chars)) {
      errf("bad char %c %d\n", s[i], s[i]);
      return false;
    }
  return true;
}

static bool valid_ISBN(cstr<20> s);
static bool valid_ISBN(const std::string &s) {
  return valid_ISBN(string2cstr<20>(s));
}

static bool valid_ISBN(cstr<20> s) {
  if (not cstr_end(s))
    return false;
  for (int i = 0; s[i]; ++i)
    if (not inside(s[i], username_chars))
      return false;
  return true;
}

static bool valid_author(auto s) {
  return valid_bookname(s);
}

static bool valid_keyword(auto s) {
  return valid_bookname(s);
}

#endif //UTIL_H
