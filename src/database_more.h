#ifndef DATABASE_MORE_H
#define DATABASE_MORE_H

#include "database.h"

#include <vector>
#include <functional>
#include <algorithm>
#include <tuple>

struct Data {
  pos_t beg;
  int size, capacity;
};

template <class Key, class ID, class Val, auto KeyCmp = std::less<Key>{}, auto KeyEq = std::equal_to<Key>{}, auto IDEq = std::equal_to<ID>{}, int header_id = 0>
class DBMore {
  static_assert(std::is_convertible_v<decltype(IDEq), std::function<bool(ID, ID)>>);
public:
  explicit DBMore(std::string filename);
  explicit DBMore(Bfsp &bf);

  std::vector<Val> get(Key key);
  void insert(Key key, Val val, ID id);
  void erase(Key key, ID id);

private:
  // using Cell = std::pair<ID, Val>;
  using Cell = std::tuple<ID, Val, bool>;
  Cell make_cell(const ID &u, const Val &v);
  Database<Key, Data, KeyCmp, KeyEq, header_id> db;
  void push_back(Data &dt, const Cell &cell);
};

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::DBMore(std::string filename) : db(filename) {
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::DBMore(Bfsp &bf) : db(bf) {
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
std::vector<Val> DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::get(Key key) {
  Data dt = db.get(key);
  std::vector<Cell> cell(dt.size);
  std::vector<Val> ret; ret.reserve(dt.size);
  db.bf.get(dt.beg, reinterpret_cast<char*>(&cell[0]), sizeof(Cell) * dt.size);
  cell.erase(std::remove_if(cell.begin(), cell.end(), [](const Cell &u) { return std::get<2>(u); }), cell.end());
  std::transform(cell.begin(), cell.end(), std::back_inserter(ret), [](const Cell &u) { return std::get<1>(u); });
  return ret;
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
void DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::insert(Key key, Val val, ID id) {
  try {
    auto [pos, dt] = db.getLow(key);
    push_back(dt, make_cell(id, val));
    db.bf.putT(pos, dt);
  } catch (const Error &e) {
    if (e.msg != "getLow: not found")
      throw;
    errf("DBMore::insert ing... %s %d\n", key.data(), id);
    Data dt{nullpos, 0, 0};
    push_back(dt, make_cell(id, val));
    db.insert(key, dt);
    db.get(key);
    // TODO: do something
  }
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
void DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::erase(Key key, ID id) {
  auto [pos, dt] = db.getLow(key);
  std::vector<Cell> vec(dt.size);
  db.bf.get(dt.beg, reinterpret_cast<char*>(&vec[0]), sizeof(Cell) * dt.size);
  int i;
  for (i = 0; i < (int)dt.size; ++i)
    if (std::get<0>(vec[i]) == id and not std::get<2>(vec[i])) {
      std::get<2>(vec[i]) = true;
      break;
    }
  Massert(i < dt.size, "fail");
  db.bf.putT(dt.beg + sizeof(Cell) * i, vec[i]);
  db.bf.putT(pos, dt);
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
typename DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::Cell DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq,
  header_id>::make_cell(const ID &u, const Val &v) {
  // return std::make_pair<ID, Val>(u, v);
  return {u, v, false};
}

template<class Key, class ID, class Val, auto KeyCmp, auto KeyEq, auto IDEq, int header_id>
void DBMore<Key, ID, Val, KeyCmp, KeyEq, IDEq, header_id>::push_back(Data &s, const Cell &cell) {
  if (s.size == s.capacity || !s.capacity) {
    s.capacity = std::max(4, int(s.capacity * 2));
    pos_t new_beg = db.bf.allocEmpty(sizeof(Cell) * s.capacity);
    if (s.size)
      db.bf.memcpy(new_beg, s.beg, sizeof(Cell) * s.size);
    s.beg = new_beg;
  }
  assert(s.size < s.capacity);
  db.bf.putT(s.beg + sizeof(Cell) * (s.size++), cell);
}


#endif //DATABASE_MORE_H
