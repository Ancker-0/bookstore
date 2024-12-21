#include "bookstore.h"
#include "util.h"

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
  printf("erase "); b.print();
  db_ISBN.erase(b.ISBN);
  if (not cstr_null(b.bookname))
    db_bookname.erase(b.bookname, b.bookid);
  if (not cstr_null(b.author))
    db_author.erase(b.author, b.bookid);
  if (not cstr_null(b.keyword))  // TODO: parse keyword
    db_keyword.erase(b.keyword, b.bookid);
  db_bookid.erase(b.bookid);
}

void Bookstore::insertBook(BookPtr bp) {
  Book b{};
  bf.getT(bp, b);
  printf("insert "); b.print();
  db_ISBN.insert(b.ISBN, bp);
  if (not cstr_null(b.bookname))
    db_bookname.insert(b.bookname, bp, b.bookid);
  if (not cstr_null(b.author))
    db_author.insert(b.author, bp, b.bookid);
  if (not cstr_null(b.keyword))  // TODO: parse keyword
    db_keyword.insert(b.keyword, bp, b.bookid);
  db_bookid.insert(b.bookid, bp);
}

void Bookstore::modify(bookid_t bookid, const std::map<std::string, std::string> &map) {
  // TODO: some validate chore

  pos_t pos = db_bookid.get(bookid);
  Book b{};
  bf.getT(pos, b);
  Book newb = b;

  if (map.count("ISBN"))
    newb.ISBN = (ISBN_t)string2cstr<20>(map.at("ISBN"));
  if (map.count("name"))
    newb.bookname = (bookname_t)string2cstr<60>(map.at("name"));
  if (map.count("author"))
    newb.author = (author_t)string2cstr<60>(map.at("author"));
  if (map.count("keyword"))
    newb.keyword = (keyword_t)string2cstr<60>(map.at("keyword"));
  if (map.count("price"))
    newb.price = (price_t)string2double(map.at("price"));

  eraseBook(pos);
  bf.putT(pos, newb);
  insertBook(pos);
}

void Bookstore::import_book(bookid_t bookid, quantity_t quantity, totalcost_t totalcost) {
  pos_t pos = db_bookid.get(bookid);
  Book b{}; bf.getT(pos, b);
  b.quantity += quantity;
  b.totalcost += totalcost;
  bf.putT(pos, b);
}

void Bookstore::showByISBN(ISBN_t ISBN) {
  try {
    askByISBN(ISBN).print();
  } catch(const Error &) {
    puts("");  // book does not exist
  }
}

void Bookstore::showByName(bookname_t bookname) {
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
}

void Bookstore::showByAuthor(author_t author) {
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
}

void Bookstore::showByKeyword(keyword_t keyword) {
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
}

void Bookstore::showAll() {
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
}
