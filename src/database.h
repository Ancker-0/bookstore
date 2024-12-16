#ifndef DATABASE_H
#define DATABASE_H

#include "fs.h"
#include "error.h"

#include <cassert>
#include <functional>
#include <cstring>
#include <vector>
#include <string>

// TODO: Get rid of macros?
#define NWITH_T(bf, id, T, name, expr) \
({ T name{}; \
bf.getT(id, name); \
expr; \
bf.putT(id, name); })
#define NWITH_TR(bf, id, T, name, expr) \
({ T name{}; \
bf.getT(id, name); \
expr; })
#define NWITH_ET(bf, id, name, expr) \
({ bf.getT(id, name); \
expr; \
bf.putT(id, name); })
#define NWITH_ETR(bf, id, name, expr) \
({ bf.getT(id, name); \
expr; })
#define NWITH_ETW(bf, id, name, expr) \
({ expr; \
bf.putT(id, name); })

#define WITH_T(id, T, name, expr) \
({ T name{}; \
bf.getT(id, name); \
expr; \
bf.putT(id, name); })
#define WITH_TR(id, T, name, expr) \
({ T name{}; \
bf.getT(id, name); \
expr; })
#define WITH_ET(id, name, expr) \
({ bf.getT(id, name); \
expr; \
bf.putT(id, name); })
#define WITH_ETR(id, name, expr) \
({ bf.getT(id, name); \
expr; })
#define WITH_ETW(id, name, expr) \
({ expr; \
bf.putT(id, name); })

constexpr size_t child_cnt = 16;

// TODO: Add more static assertions to type Key and Val
template <class Key, class Val, auto KeyCmp, auto KeyEq>
class Database {
static_assert(std::is_convertible_v<decltype(KeyCmp), std::function<bool(Key, Key)>>
              && std::is_convertible_v<decltype(KeyEq), std::function<bool(Key, Key)>>);

public:
  Database() = default;
  Database(std::string filename);
  void insert(Key key, Val val);

  Val get(Key key);
  std::pair<pos_t, Val> getLow(Key key);
  void modify(Key key, Val val);

  Bfsp bf;

private:

  struct BHeader {
    int depth;
    pos_t root;
    int timestamp;
  } header;

  struct Node {
    Key key[child_cnt - 1];
    size_t size;
    pos_t fa, chd[child_cnt];
  };
};

template<class Key, class Val, auto KeyCmp, auto KeyEq>
Database<Key, Val, KeyCmp, KeyEq>::Database(std::string filename) : bf(filename) {
  bf.getHeaderT(0, header);
  if (header.root == 0) {
    errf("initializing tree\n");
    Node node{{}, 0, nullpos, {}};
    header.root = bf.allocT(node);
    header.depth = 0;
    bf.putT(0, header);
  }
}

template<class Key, class Val, auto KeyCmp, auto KeyEq>
void Database<Key, Val, KeyCmp, KeyEq>::insert(Key key, Val val) {
    pos_t now = header.root;
  int depth = 0;
  Node node{};
  while (depth < header.depth) {
    WITH_ETR(now, node, );
    size_t k{};
    for (k = 0; k + 1 < node.size; ++k)
      if (!KeyCmp(node.key[k], key))
        break;
    now = node.chd[k];
    ++depth;
  }
  WITH_ETR(now, node, );
  size_t k;
  for (k = 0; k < node.size; ++k) {
    // errf("(insert 0 %s)\n", node.key[k]);
    if (!KeyCmp(node.key[k], key))
      break;
  }
  if (k < node.size && KeyEq(node.key[k], key)) {  // exist
    throw Error("Key already exists");
  } else if (node.size + 1 < child_cnt) {
    for (int i = node.size; i > k; --i) {
      memcpy(&node.key[i], &node.key[i - 1], sizeof(node.key[i]));
      memcpy(&node.chd[i], &node.chd[i - 1], sizeof(node.chd[i]));
    }
    // memcpy(&node.key[k], key.c_str(), sizeof(Key));
    strcpy(node.key[k], key.c_str());
    ++header.timestamp;
    bf.putT(0, header);
    node.chd[k] = bf.allocT(val);
    ++node.size;
    WITH_ETW(now, node, );
  } else {
    assert(node.size == child_cnt - 1);
    ++header.timestamp;
    bf.putT(0, header);
    pos_t dhd_pos = bf.allocT(val);

    Key ktmp[child_cnt + 1]{};
    pos_t kchd[child_cnt + 1]{};
    /* TODO: these memcpy are stupid... improvements are to be made */
    memcpy(&ktmp[0], &node.key[0], sizeof(Key) * k);
    memcpy(&kchd[0], &node.chd[0], sizeof(kchd[0]) * k);
    strcpy(ktmp[k], key.c_str());
    memcpy(&kchd[k], &dhd_pos, sizeof(kchd[0]));
    memcpy(&ktmp[k + 1], &node.key[k], sizeof(Key) * (node.size - k));
    memcpy(&kchd[k + 1], &node.chd[k], sizeof(kchd[0]) * (node.size - k));

    size_t mid = child_cnt / 2;
    Node rnode{};
    node.size = mid;
    rnode.size = child_cnt - mid;
    rnode.fa = node.fa;
    memcpy(&node.key[0], &ktmp[0], sizeof(Key) * mid);
    memcpy(&rnode.key[0], &ktmp[mid], sizeof(Key) * (child_cnt - mid));
    memcpy(&node.chd[0], &kchd[0], sizeof(kchd[0]) * mid);
    memcpy(&rnode.chd[0], &kchd[mid], sizeof(kchd[0]) * (child_cnt - mid));
    pos_t rnode_pos = bf.allocT(rnode);
    pos_t node_pos = now;
    WITH_ETW(now, node, );
    // std::string old_key = key;
    key = ktmp[mid - 1];

    // for the loop to run properly, we need to maintain:
    // + key
    // + // old_key
    // + node
    // + node_pos
    // + // rnode
    // + rnode_pos
    for (; depth > 0; --depth) {
      Node fa{};
      WITH_ETR(node.fa, fa, );
      if (fa.size + 1 <= child_cnt) {
        pos_t k{};
        for (k = 0; k + 1 < fa.size; ++k)
          if (!KeyCmp(fa.key[k], key))
            break;
        for (int i = fa.size; i > k; --i) {
          if (i != fa.size)
            memcpy(&fa.key[i], &fa.key[i - 1], sizeof(Key));
          memcpy(&fa.chd[i], &fa.chd[i - 1], sizeof(fa.chd[i]));
        }
        strcpy(fa.key[k], key.c_str());
        // if (k + 1 < child_cnt - 1)
        //   strcpy(fa.key[k + 1], old_key);
        memcpy(&fa.chd[k + 1], &rnode_pos, sizeof(rnode_pos));

        WITH_ETW(node.fa, fa, fa.size++);
        break;
      } else {
        assert(fa.size == child_cnt);
        pos_t k{};
        for (k = 0; k + 1 < fa.size; ++k)
          if (!KeyCmp(fa.key[k], key))
            break;
        // node_pos = fa.chd[k];
        memcpy(&ktmp[0], &fa.key[0], sizeof(Key) * k);
        memcpy(&kchd[0], &fa.chd[0], sizeof(fa.chd[0]) * (k + 1));
        // if (k + 1 < child_cnt)
          strcpy(ktmp[k], key.c_str());
        memcpy(&kchd[k + 1], &rnode_pos, sizeof(rnode_pos));
        assert(kchd[k] == node_pos);
        if (k + 1 < child_cnt) {
          memcpy(&kchd[k + 2], &fa.chd[k + 1], sizeof(fa.chd[0]) * ((int)child_cnt - k - 1));
          memcpy(&ktmp[k + 1], &fa.key[k], sizeof(Key) * ((int)child_cnt - 1 - k));
        }

        mid = (child_cnt + 1) / 2;
        key = ktmp[mid - 1];
        pos_t old_node_pos = node_pos, old_rnode_pos = rnode_pos;
        node_pos = node.fa;
        node = fa;
        node.size = mid;
        memcpy(&node.key[0], &ktmp[0], sizeof(Key) * (node.size - 1));
        memcpy(&node.chd[0], &kchd[0], sizeof(kchd[0]) * node.size);
        rnode.fa = node.fa;  // TODO: this is not true
        rnode.size = child_cnt + 1 - mid;
        memcpy(&rnode.key[0], &ktmp[mid], sizeof(Key) * (rnode.size - 1));
        memcpy(&rnode.chd[0], &kchd[mid], sizeof(kchd[0]) * rnode.size);
        rnode_pos = bf.allocT(rnode);
        WITH_ETW(node_pos, node, );
        // for (int i = 0; i < node.size; ++i)  // TODO: this is too slow. probably there is better way?
        //   WITH_T(node.chd[i], Node, ntmp, ntmp.fa = node_pos);
        for (int i = 0; i < rnode.size; ++i)
          WITH_T(rnode.chd[i], Node, ntmp, ntmp.fa = rnode_pos);
        WITH_T(old_node_pos, Node, ntmp, ntmp.fa = k < mid ? node_pos : rnode_pos);
        WITH_T(old_rnode_pos, Node, ntmp, ntmp.fa = (k + 1) < mid ? node_pos : rnode_pos);
      }
    }
    if (depth == 0) {
      errf("(growing! %d)\n", header.depth + 1);
      Node newroot{{}, 2, nullpos, {node_pos, rnode_pos}};
      strcpy(newroot.key[0], key.c_str());
      pos_t newroot_pos = bf.allocT(newroot);
      WITH_ET(node_pos, node, node.fa = newroot_pos);
      WITH_ET(rnode_pos, rnode, rnode.fa = newroot_pos);
      ++header.depth;
      header.root = newroot_pos;
      bf.putT(0, header);
    }
  }
}

template<class Key, class Val, auto KeyCmp, auto KeyEq>
Val Database<Key, Val, KeyCmp, KeyEq>::get(Key key) {
  int depth = 0;
  pos_t pos = header.root;
  Node node{};
  WITH_ETR(pos, node,);
  pos_t k{};
  while (depth <= header.depth) {
    WITH_ETR(pos, node,);
    for (k = 0; k + 1 < node.size; ++k)
      if (!KeyCmp(node.key[k], key))
        break;
    pos = node.chd[k];
    ++depth;
  }

  if (k < child_cnt - 1 && KeyEq(node.key[k], key)) {
    Val val;
    NWITH_ETR(bf, pos, val,);
    return val;
  }
  throw Error("get: not found");
}

template<class Key, class Val, auto KeyCmp, auto KeyEq>
std::pair<pos_t, Val> Database<Key, Val, KeyCmp, KeyEq>::getLow(Key key) {
  int depth = 0;
  pos_t pos = header.root;
  Node node{};
  WITH_ETR(pos, node,);
  pos_t k{};
  while (depth <= header.depth) {
    WITH_ETR(pos, node,);
    for (k = 0; k + 1 < node.size; ++k)
      if (!KeyCmp(node.key[k], key))
        break;
    pos = node.chd[k];
    ++depth;
  }

  if (k < child_cnt - 1 && KeyEq(node.key[k], key)) {
    Val val;
    NWITH_ETR(bf, pos, val,);
    return {pos, val};
  }
  throw Error("getLow: not found");
}

template<class Key, class Val, auto KeyCmp, auto KeyEq>
void Database<Key, Val, KeyCmp, KeyEq>::modify(Key key, Val val) {
  int depth = 0;
  pos_t pos = header.root;
  Node node{};
  WITH_ETR(pos, node,);
  pos_t k{};
  while (depth <= header.depth) {
    WITH_ETR(pos, node,);
    for (k = 0; k + 1 < node.size; ++k)
      if (!KeyCmp(node.key[k], key))
        break;
    pos = node.chd[k];
    ++depth;
  }

  if (k < child_cnt - 1 && KeyEq(node.key[k], key))
    bf.putT(pos, val);
  else
    throw Error("modify: not found");
}

#undef NWITH_T
#undef NWITH_TR
#undef NWITH_ET
#undef NWITH_ETR
#undef NWITH_ETW
#undef WITH_T
#undef WITH_TR
#undef WITH_ET
#undef WITH_ETR
#undef WITH_ETW

#endif //DATABASE_H