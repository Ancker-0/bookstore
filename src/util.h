#ifndef UTIL_H
#define UTIL_H

#include "error.h"

#include <array>
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

static int string2int(std::string s) {
  return std::atoi(s.c_str());
}

bool valid_userid(auto s) {
	if (not std::is_same_v<decltype(s), cstr<30>> or not cstr_end(s))
		return false;
	for (int i = 0; s[i]; ++i)
		if (not inside(s[i], userid_chars))
			return false;
  return true;
}

bool valid_password(auto s) {
	return valid_userid(s);
}

bool valid_username(auto s) {
  // TODO
  return true;
}

#endif //UTIL_H
