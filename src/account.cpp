#include "account.h"

#include <cmath>

#include "error.h"

void AccountCenter::login(userid_t userid, password_t password) {
  if (not db.exist(userid))
    throw Error("login: account does not exist");
  auto ac = db.get(userid);
  if (password != "" and ac.password != password)
    throw Error("login: wrong password");
  // errf("DEBUG\n");
  auto &cur = login_stack.back();
  sync(cur);
  if (password == "" and ac.privilege >= cur.privilege)
    throw Error("login: not enough privilege");
  login_stack.push_back(ac);
  select_stack.push_back(nullid);
}

void AccountCenter::logout() {
  assert(not login_stack.empty());
  if (login_stack.size() == 1)
    throw Error("logout: no account logged in");
  login_stack.pop_back();
  select_stack.pop_back();
}

void AccountCenter::regis(userid_t userid, password_t password, username_t username) {
  if (db.exist(userid))
    throw Error("register: user exists");
  Account ac{1, "customer", userid, username, password};
  db.insert(userid, ac);
}

void AccountCenter::useradd(userid_t userid, password_t password, privilege_t privilege, username_t username) {
  if (db.exist(userid))
    throw Error("useradd: user exists");
  Massert(login_stack.back().privilege >= 3, "access denied");
  Massert(login_stack.back().privilege > privilege, "cannot create such privilege");
  Massert(valid_userid(userid), "bad userid");
  Massert(valid_password(password), "bad password");
  Massert(valid_username(username), "bad username");
  Account ac{privilege, (identity_t)string2cstr<10>(privilege == 1 ? "customer" : (privilege == 3 ? "clerk" : "admin")), userid, username, password};
  Massert(ac.validate() or privilege == 0, "invalid account info");
  db.insert(userid, ac);
}

void AccountCenter::changePassword(userid_t userid, password_t cur_pass, password_t new_pass) {
  Massert(db.exist(userid), "user does not exist");
  Massert(login_stack.back().privilege >= 1, "access denied");
  auto ac = db.get(userid);
  if (cur_pass != "" and cur_pass != ac.password)
    throw Error("changePassword: wrong password");
  if (cur_pass == "" and login_stack.back().privilege != 7)
    throw Error("changePassword: access denied");
  ac.password = new_pass;
  db.modify(userid, ac);
}

void AccountCenter::erase(userid_t userid) {
  Massert(db.exist(userid), "user not exists");
  Massert(login_stack.back().privilege >= 7, "access denied");
  for (int i = 1; i < (int)login_stack.size(); ++i)
    Massert(login_stack[i].userid != userid, "cannot erase a logged-in account");
  db.erase(userid);
}

void AccountCenter::select(ISBN_t ISBN) {
  Massert(login_stack.size() > 1, "not logged in");
  Massert(login_stack.back().privilege >= 3, "access denied");
  select_stack.back() = Bookstore::getInstance().select(ISBN);
}

void AccountCenter::sync(Account &ac) {
  try {
    ac = db.get(ac.userid);
  } catch (const Error &e) {
    if (e.msg != "get: not found")
      throw;
  }
}

AccountCenter::AccountCenter(): bf("account.db"), db(bf) {
  Account guest{0, "guest", "guest#unspecified", "guest#unspecified", "guest#unspecified"};
  login_stack.push_back(guest);
  select_stack.push_back(nullid);

  bool init = false;
  bf.getHeaderT(1, init);
  if (not init) {
    errf("initializing account\n");
    Account root{7, "admin", "root", "root#unspecified", "sjtu" };
    db.insert(root.userid, root);

    init = true;
    bf.putT(bf.getHeaderPos(1), init);
  }
}
