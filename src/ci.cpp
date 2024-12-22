// TODO: ISBN can be surrounded by quotes?

#include "ci.h"
#include "error.h"
#include "account.h"
#include "config.h"
#include "finance.h"
#include "bookstore.h"

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
      if (cur != "")
        res.command.push_back(cur);
    } else {
      std::string key, val;
      cmd = false;
      for (i_ = i + 1; i_ < len; ++i_) {
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
    exit(0);
#if ECHO
#if ECHO_ERR
  std::cerr << s << std::endl;
#else
  std::cout << s << std::endl;
#endif
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
    // for (auto &c : tk.command)
    //   errf("#%s ", c.c_str());
    // errf("\n");
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
    AccountCenter &ac = AccountCenter::getInstance();
    assert(ac.login_stack.size() == ac.select_stack.size());
    for (int i = 0; i < (int)ac.login_stack.size(); ++i)
      printf("(%s %d) ", ac.login_stack[i].userid.data(), ac.select_stack[i]);
    puts("");
  } else if (tk.command.at(0) == "debug") {
    AccountCenter::getInstance().db.printKeys();
  } else if (tk.command.at(0) == "select") {
    Massert(tk.param.empty(), "expect no params");
    Massert(tk.command.size() == 2, "invalid param");
    AccountCenter::getInstance().select((ISBN_t)string2cstr<20>(tk.command.at(1)));
  } else if (tk.command.at(0) == "modify") {
    static const std::vector<std::string> modify_allow_fields = { "ISBN", "name", "author", "keyword", "price" };
    // for (auto [k, _] : tk.param)
    //   errf("%s ", k.c_str());
    // errf("\n");
    Massert(param_inside(tk, modify_allow_fields), "unexpected param");
    Massert(tk.command.size() == 1, "expect no subcommand");
    Massert(AccountCenter::getInstance().login_stack.back().privilege >= 3, "access denied");
    Massert(AccountCenter::getInstance().select_stack.size() > 1 and AccountCenter::getInstance().select_stack.back() != nullpos, "not selecting any book");
    Bookstore::getInstance().modify(AccountCenter::getInstance().select_stack.back(), tk.param);
  } else if (tk.command.at(0) == "show" and tk.command.size() >= 2 and tk.command.at(1) == "finance") {
    Massert(tk.command.size() <= 3, "command");
    Massert(acci.login_stack.size() > 1 and acci.login_stack.back().privilege == 7, "access denied");
    Massert(tk.param.empty(), "param");
    if (tk.command.size() == 2)
      fnce.showAll();
    else
      fnce.show(string2int(tk.command.at(2)));
  } else if (tk.command.at(0) == "show") {
    static const std::vector<std::string> show_allow_fields = { "ISBN", "name", "author", "keyword" };
    Massert(tk.param.empty() or (tk.param.size() == 1 and param_inside(tk, show_allow_fields)), "can't show");
    Massert(acci.login_stack.size() > 1 and acci.login_stack.back().privilege >= 1, "access denied");

    if (tk.param.count("ISBN")) {
      Bookstore::getInstance().showByISBN(string2cstr<20>(tk.param["ISBN"]));
    } else if (tk.param.count("name")) {
      Bookstore::getInstance().showByName(string2cstr<60>(tk.param["name"]));
    } else if (tk.param.count("author")) {
      Bookstore::getInstance().showByAuthor(string2cstr<60>(tk.param["author"]));
    } else if (tk.param.count("keyword")) {
      Bookstore::getInstance().showByKeyword(string2cstr<60>(tk.param["keyword"]));
    } else {
      assert(tk.param.empty());
      Bookstore::getInstance().showAll();
    }
  } else if (tk.command.at(0) == "import") {
    Massert(tk.command.size() == 3, "invalid param");
    Massert(tk.param.empty(), "expect no params");
    Massert(AccountCenter::getInstance().login_stack.back().privilege >= 3, "access denied");
    Massert(AccountCenter::getInstance().select_stack.size() > 1 and AccountCenter::getInstance().select_stack.back() != nullpos, "not selecting any book");
    Bookstore::getInstance().import_book(AccountCenter::getInstance().select_stack.back(), string2int(tk.command.at(1)), string2double(tk.command.at(2)));
  } else if (tk.command.at(0) == "buy") {
    Massert(acci.login_stack.size() > 1, "not logged in");
    Massert(tk.param.empty(), "param");
    Massert(tk.command.size() == 3, "command");
    bkst.buy(string2cstr<20>(tk.command.at(1)), string2int(tk.command.at(2)));
  } else if (tk.command.at(0) == ".print") {
    Massert(AccountCenter::getInstance().select_stack.size() > 1 and AccountCenter::getInstance().select_stack.back() != nullpos, "not selecting any book");
    Book b = Bookstore::getInstance().askByBookid(AccountCenter::getInstance().select_stack.back());
    b.print();
    // printf("(book %s %s %s %s)\n", b.ISBN.data(), b.bookname.data(), b.author.data(), b.keyword.data());
  } else
    throw Error("unrecognized command");
}
