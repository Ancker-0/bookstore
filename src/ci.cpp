#include "ci.h"
#include "error.h"
#include "account.h"
#include "config.h"

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
	// Massert(not is.eof(), "input end");
	if (is.eof())
		exit(1);
#if ECHO
	std::cout << s << std::endl;
#endif
  Tokenized tk = tokenize(s);
  // if (tk.command.empty())
  //   throw Error("ci: not specify any command");
  // Massert(not tk.command.empty(), "not specify any command");
	if (tk.command.empty())
		return;
  if (tk.command[0] == "exit" or tk.command[0] == "quit") {
    Massert(tk.param.empty() and tk.command.size() == 1, "expect no params");
    exit(0);
  } else if (tk.command.at(0) == "su") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() >= 2 and tk.command.size() <= 3, "invalid param");
    AccountCenter::getInstance().login((userid_t)string2cstr<30>(tk.command.at(1)), (password_t)string2cstr<30>(tk.command.size() >= 3 ? tk.command.at(2) : ""));
  } else if (tk.command.at(0) == "register") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() == 4, "invalid param");
    AccountCenter::getInstance().regis((userid_t)string2cstr<30>(tk.command.at(1)), (password_t)string2cstr<30>(tk.command.at(2)), (username_t)string2cstr<30>(tk.command.at(3)));
  } else if (tk.command.at(0) == "passwd") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() == 3 or tk.command.size() == 4, "invalid param");
    if (tk.command.size() == 3)
      AccountCenter::getInstance().changePassword((userid_t)string2cstr<30>(tk.command.at(1)), (password_t)string2cstr<30>(""), (password_t)string2cstr<30>(tk.command.at(2)));
    else
      AccountCenter::getInstance().changePassword((userid_t)string2cstr<30>(tk.command.at(1)), (password_t)string2cstr<30>(tk.command.at(2)), (password_t)string2cstr<30>(tk.command.at(3)));
  } else if (tk.command.at(0) == "logout") {
    Massert(tk.param.empty() and tk.command.size() == 1, "expect no params");
    AccountCenter::getInstance().logout();
  } else if (tk.command.at(0) == "useradd") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() == 5, "invalid param");
    Massert(tk.command.at(3).size() == 1 and '0' <= tk.command.at(3)[0] and tk.command.at(3)[0] <= '9', "privilege should be a one-digit number");
    AccountCenter::getInstance().useradd((userid_t)string2cstr<30>(tk.command.at(1)), (password_t)string2cstr<30>(tk.command.at(2)), (privilege_t)(tk.command.at(3)[0] - '0'), (username_t)string2cstr<30>(tk.command.at(4)));
  } else if (tk.command.at(0) == "delete") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() == 2, "invalid param");
    AccountCenter::getInstance().erase((userid_t)string2cstr<30>(tk.command.at(1)));
  } else if (tk.command.at(0) == "stack") {
    for (auto s : AccountCenter::getInstance().login_stack)
      printf("%s ", s.userid.data());
    puts("");
  } else if (tk.command.at(0) == "debug") {
    AccountCenter::getInstance().db.printKeys();
  } else
		throw Error("unrecognized command");
}
