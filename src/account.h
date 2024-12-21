#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "fs.h"
#include "util.h"
#include "error.h"
#include "database.h"
#include "bookstore.h"

#include <string>
#include <cstring>
#include <stack>

using privilege_t = int;
using identity_t = cstr<10>;
using userid_t = cstr<30>;
using username_t = cstr<30>;
using password_t = cstr<30>;

struct Account {
  int privilege;
  cstr<10> identity;
  cstr<30> userid, username, password;

  [[nodiscard]] bool validate_throw() const {
    Massert(privilege == 0 or privilege == 1 or privilege == 3 or privilege == 7, "privilege");
    Massert(cstr_end(userid) and cstr_end(identity) and cstr_end(username) and cstr_end(password), "cstr_end");
    Massert(identity == "admin" or identity == "clerk" or identity == "customer" or identity == "guest", "identity");
    Massert(valid_password(password), "password");
    Massert(valid_username(username), "username");
    Massert(valid_userid(userid), "userid");
    return true;
  }

  [[nodiscard]] bool validate() const {
    try {
      return validate_throw();
    } catch (...) {
      return false;
    }
  }
};

class AccountCenter {
private:
  Bfsp bf;

public:
  static AccountCenter &getInstance() {
    static AccountCenter me;
    return me;
  }
  void login(userid_t userid, password_t password);
  void logout();
  void regis(userid_t userid, password_t password, username_t username);
  void useradd(userid_t userid, password_t password, privilege_t privilege, username_t username);
  void changePassword(userid_t userid, password_t cur_pass, password_t new_pass);
  void erase(userid_t userid);
  void select(ISBN_t ISBN);

  // TODO: move into private scope
  std::vector<Account> login_stack;
  std::vector<bookid_t> select_stack;
  Database<cstr<30>, Account, std::less<cstr<30>>{}, std::equal_to<cstr<30>>{}> db;

private:
  void sync(Account &ac);
  AccountCenter();
};

#endif //ACCOUNT_H
