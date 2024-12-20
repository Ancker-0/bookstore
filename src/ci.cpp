#include "ci.h"
#include "error.h"

#include <string>

using namespace ci;

Tokenized ci::tokenize(std::string s) {
  Tokenized res;
  int len = (int)s.size();
  bool cmd = true;
  for (int i = 0, i_; i < len; i = i_) {
    while (i < len && s[i] == ' ')
      ++i;
    bool inside_quote = false;
    bool backslash = false;
    if (s[i] != '-') {
      std::string cur;
      if (not cmd)
        throw Error("unexpected token");
      for (i_ = i; i_ < len; ++i_) {
        if (backslash) {
          cur += s[i_];
          backslash = false;
        } else if (s[i_] == '"') {
          if (inside_quote) {
            inside_quote = false;
          } else {
            inside_quote = true;
          }
        } else if (s[i_] == '\\') {
          backslash = true;
        } else if (s[i_] == ' ') {
          if (inside_quote)
            cur += s[i_];
          else
            break;
        } else
          cur += s[i_];
      }
      if (backslash)
        throw Error("tokenize: backslash expected one charactor, read eol");
      res.command.push_back(cur);
    } else {
      std::string key, val;
      cmd = false;
      for (i_ = i; i_ < len; ++i_) {
        if (backslash) {
          key += s[i_];
          backslash = false;
        } else if (s[i_] == '"') {
          if (inside_quote) {
            inside_quote = false;
          } else {
            inside_quote = true;
          }
        } else if (s[i_] == '\\') {
          backslash = true;
        } else if (s[i_] == ' ') {
          if (inside_quote)
            key += s[i_];
          else {
            ++i_;
            break;
          }
        } else if (s[i_] == '=') {
          if (not inside_quote) {
            ++i_;
            break;
          }
        } else
          key += s[i_];
      }
      if (backslash)
        throw Error("tokenize: backslash expected one charactor, read eol");
      for (; i_ < len; ++i_) {
        if (backslash) {
          val += s[i_];
          backslash = false;
        } else if (s[i_] == '"') {
          if (inside_quote) {
            inside_quote = false;
          } else {
            inside_quote = true;
          }
        } else if (s[i_] == '\\') {
          backslash = true;
        } else if (s[i_] == ' ') {
          if (inside_quote)
            val += s[i_];
          else
            break;
        } else if (s[i_] == '=') {
          if (not inside_quote)
            break;
        } else
          val += s[i_];
      }
      if (backslash)
        throw Error("tokenize: backslash expected one charactor, read eol");
      if (res.param.count(key))
        throw Error("tokenize: duplicated parameter");
      res.param[key] = val;
    }
  }
  return res;
}

/*
# ??????
su [UserID] ([Password])?
logout
register [UserID] [Password] [Username]
passwd [UserID] ([CurrentPassword])? [NewPassword]
useradd [UserID] [Password] [Privilege] [Username]
delete [UserID]
*/

void Ci::process_one() {
  std::string s;
  std::getline(is, s);
  Tokenized tk = tokenize(s);
  // if (tk.command.empty())
  //   throw Error("ci: not specify any command");
  Massert(not tk.command.empty(), "not specify any command");
  if (tk.command[0] == "exit" or tk.command[0] == "quit") {
    Massert(tk.param.empty() and tk.command.size() == 1, "expect no params");
    exit(0);
  }
}
