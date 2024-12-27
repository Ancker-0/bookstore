#include "bookstore.h"
#include "util.h"
#include "finance.h"

void Book::print() {
  printf("%s\t%s\t%s\t%s\t%.2lf\t%d\n", ISBN.data(), bookname.data(), author.data(), keyword.data(), price, quantity);
}

Bookstore::Bookstore() : bf("bookstore.db"), db_ISBN(bf), db_bookname(bf), db_author(bf), db_keyword(bf), db_bookid(bf) {}

Book Bookstore::askByISBN(ISBN_t ISBN) {
  BookPtr pos = db_ISBN.get(ISBN);
  Book ret{};
  bf.getT(pos, ret);
  return ret;
}

Book Bookstore::askByBookid(bookid_t bookid) {
  BookPtr pos = db_bookid.get(bookid);
  Book ret{};
  bf.getT(pos, ret);
  return ret;
}

bookid_t Bookstore::select(ISBN_t ISBN) {
  if (db_ISBN.exist(ISBN))
    return askByISBN(ISBN).bookid;
  // errf("bs::select creating new book\n");
  Book newbook = default_book();
  newbook.ISBN = ISBN;
  pos_t pos = bf.allocT(newbook);
  newbook.bookid = pos;  // TODO: maybe there's better way to determine bookid
  bf.putT(pos, newbook);
  db_ISBN.insert(ISBN, pos);
  db_bookid.insert(newbook.bookid, pos);
  return newbook.bookid;
}

void Bookstore::eraseBook(BookPtr bp) {
  Book b{};
  bf.getT(bp, b);
  // printf("erase "); b.print();
  db_ISBN.erase(b.ISBN);
  if (not cstr_null(b.bookname))
    db_bookname.erase(b.bookname, b.bookid);
  if (not cstr_null(b.author))
    db_author.erase(b.author, b.bookid);
  if (not cstr_null(b.keyword))
    for (auto &k : split_keyword(b.keyword.data()))
      db_keyword.erase(string2keyword(k), b.bookid);
  db_bookid.erase(b.bookid);
}

void Bookstore::insertBook(BookPtr bp) {
  Book b{};
  bf.getT(bp, b);
  // printf("insert "); b.print();
  db_ISBN.insert(b.ISBN, bp);
  if (not cstr_null(b.bookname))
    db_bookname.insert(b.bookname, bp, b.bookid);
  if (not cstr_null(b.author))
    db_author.insert(b.author, bp, b.bookid);
  if (not cstr_null(b.keyword))
    for (auto &k : split_keyword(b.keyword.data()))
      db_keyword.insert(string2keyword(k), bp, b.bookid);
  db_bookid.insert(b.bookid, bp);
}

void Bookstore::modify(bookid_t bookid, const std::map<std::string, std::string> &map) {
  pos_t pos = db_bookid.get(bookid);
  Book b{};
  bf.getT(pos, b);
  Book newb = b;

  if (map.count("ISBN")) {
    newb.ISBN = string2ISBN(map.at("ISBN"));
    Massert(newb.ISBN != b.ISBN, "modify to the same ISBN");
    Massert(not cstr_null(newb.ISBN), "null param");
    Massert(valid_ISBN(newb.ISBN), "bad ISBN");
    Massert(not db_ISBN.exist(newb.ISBN), "duplicated ISBN");
  }
  if (map.count("name")) {
    newb.bookname = string2bookname(map.at("name"));
    Massert(valid_bookname(newb.bookname), "bad bookname");
    Massert(not cstr_null(newb.bookname), "null param");
  }
  if (map.count("author")) {
    newb.author = string2author(map.at("author"));
    Massert(valid_author(newb.author), "bad author");
    Massert(not cstr_null(newb.author), "null param");
  }
  if (map.count("keyword")) {
    split_keyword(map.at("keyword"));
    newb.keyword = string2keyword(map.at("keyword"));
    Massert(valid_keyword(newb.keyword), "bad keyword");
    Massert(not cstr_null(newb.keyword), "null param");
  }
  if (map.count("price")) {
    Massert(valid_price(map.at("price")), "bad price");
    newb.price = (price_t)string2double(map.at("price"));
  }

  eraseBook(pos);
  bf.putT(pos, newb);
  insertBook(pos);
}

void Bookstore::import_book(bookid_t bookid, quantity_t quantity, totalcost_t totalcost) {
  Massert(quantity > 0, "bad quantity");
  Massert(totalcost > 0, "bad totalcost");
  pos_t pos = db_bookid.get(bookid);
  Book b{}; bf.getT(pos, b);
  b.quantity += quantity;
  b.totalcost += totalcost;
  bf.putT(pos, b);
  fnce.outcome(totalcost);
}

void Bookstore::showByISBN(ISBN_t ISBN) {
  Massert(valid_ISBN(ISBN), "bad ISBN");
  try {
    askByISBN(ISBN).print();
  } catch(const Error &) {
    puts("");  // book does not exist
  }
}

void Bookstore::showByName(bookname_t bookname) {
  Massert(valid_bookname(bookname), "bad bookname");
  try {
    auto bookptrs = db_bookname.get(bookname);
    std::vector<Book> books;
    std::transform(bookptrs.begin(), bookptrs.end(), std::back_inserter(books), [&](pos_t p) {
        Book ret{}; bf.getT(p, ret);
        return ret;
        });
    if (books.empty())
      puts("");
    else {
      std::sort(books.begin(), books.end(), [&](auto &u, auto &v) { return u.ISBN < v.ISBN; });
      for (auto &b : books)
        b.print();
    }
  } catch(const Error &) {
    puts("");
  }
}

void Bookstore::showByAuthor(author_t author) {
  Massert(valid_author(author), "bad author");
  try {
    auto bookptrs = db_author.get(author);
    std::vector<Book> books;
    std::transform(bookptrs.begin(), bookptrs.end(), std::back_inserter(books), [&](pos_t p) {
        Book ret{}; bf.getT(p, ret);
        return ret;
        });
    if (books.empty())
      puts("");
    else {
      std::sort(books.begin(), books.end(), [&](auto &u, auto &v) { return u.ISBN < v.ISBN; });
      for (auto &b : books)
        b.print();
    }
  } catch(const Error &) {
    puts("");
  }
}

void Bookstore::showByKeyword(keyword_t keyword) {
  Massert(valid_keyword(keyword), "bad keyword");
  try {
    auto bookptrs = db_keyword.get(keyword);
    std::vector<Book> books;
    std::transform(bookptrs.begin(), bookptrs.end(), std::back_inserter(books), [&](pos_t p) {
        Book ret{}; bf.getT(p, ret);
        return ret;
        });
    if (books.empty())
      puts("");
    else {
      std::sort(books.begin(), books.end(), [&](auto &u, auto &v) { return u.ISBN < v.ISBN; });
      for (auto &b : books)
        b.print();
    }
  } catch(const Error &) {
    puts("");
  }
}

void Bookstore::showAll() {
  try {
    auto bookptrs = db_bookid.getAll();
    std::vector<Book> books;
    std::transform(bookptrs.begin(), bookptrs.end(), std::back_inserter(books), [&](pos_t p) {
        Book ret{}; bf.getT(p, ret);
        return ret;
        });
    if (books.empty())
      puts("");
    else {
      std::sort(books.begin(), books.end(), [&](auto &u, auto &v) { return u.ISBN < v.ISBN; });
      for (auto &b : books)
        b.print();
    }
  } catch(const Error &) {
    puts("");
  }
}

void Bookstore::buy(ISBN_t ISBN, int quantity) {
  Massert(quantity > 0, "bad quantity");
  pos_t pos = db_ISBN.get(ISBN);
  Book b{}; bf.getT(pos, b);
  Massert(quantity <= b.quantity, "no enough book");
  eraseBook(pos);
  b.quantity -= quantity;
  bf.putT(pos, b);
  insertBook(pos);
  printf("%.2lf\n", quantity * b.price);
  fnce.income(quantity * b.price);
}
