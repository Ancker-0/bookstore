#ifndef BOOKSTORE_H
#define BOOKSTORE_H

#include "util.h"
#include "database.h"
#include "database_more.h"

#include <cstdint>

#define DF(n, len) using n##_t = cstr<len>;

DF(ISBN, 20);
DF(bookname, 60);
DF(author, 60);
DF(keyword, 60);
using quantity_t = uint32_t;
using price_t = double;
using totalcost_t = double;
using bookid_t = pos_t;

static constexpr bookid_t nullid = -1;

struct Book {
#define DFF(n) n##_t n;
  DFF(ISBN);
  DFF(bookname);
  DFF(author);
  DFF(keyword);
  DFF(quantity);
  DFF(price);
  DFF(totalcost);
  DFF(bookid);
  void print();
};

static Book default_book() { return Book{ "", "", "", "", 0, 0, 0, nullpos }; }

typedef pos_t BookPtr;

class Bookstore {
private:
  Bfsp bf;
#define DFDB(fld, headerid) DBMore<fld##_t, bookid_t, BookPtr, std::less<fld##_t>{}, std::equal_to<fld##_t>{}, std::equal_to<bookid_t>{}, headerid> db_##fld;
  // DFDB(ISBN, 0);      // db_ISBN
  Database<ISBN_t, BookPtr> db_ISBN;
  DFDB(bookname, 1);  // db_bookname
  DFDB(author, 2);    // db_author
  DFDB(keyword, 3);   // db_keyword
  Database<bookid_t, BookPtr, std::less<bookid_t>{}, std::equal_to<bookid_t>{}, 4> db_bookid;

  Bookstore();
  void eraseBook(BookPtr bp);
  void insertBook(BookPtr bp);

public:
  static Bookstore &getInstance() {
    static Bookstore me;
    return me;
  }
  Book askByISBN(ISBN_t ISBN);
  Book askByBookid(bookid_t bookid);
  bookid_t select(ISBN_t ISBN);
  void modify(bookid_t bookid, const std::map<std::string, std::string> &map);
  void import_book(bookid_t bookid, quantity_t quantity, totalcost_t totalcost);
  void showByISBN(ISBN_t ISBN);
  void showByName(bookname_t bookname);
  void showByAuthor(author_t author);
  void showByKeyword(keyword_t keyword);
  void showAll();
};

#endif
