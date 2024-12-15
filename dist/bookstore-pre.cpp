// Use B+ tree instead.
// Maintain a timestamp to make delete faster
// Allocating continuous block of the file, like std::vector
// Seperate another file. Doesn't seem to improve much.
// Add a stupid cache...

#define RESET 1

#pragma GCC optimize("Ofast")
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>
typedef int64_t pos_t;

constexpr pos_t nullpos = -1;
constexpr size_t header_size = 56;

#define errf(x, ...) fprintf(stderr, x, ##__VA_ARGS__)

struct BfspHeader {
  char data[header_size];
  pos_t nxt;
};

static_assert(sizeof(BfspHeader) == 64);

template <class T>
inline std::pair<T, T> cross(T l1, T r1, T l2, T r2) {
  return { std::max(l1, l2), std::min(r1, r2) };
}

class Bfsp {
public:
  Bfsp() = delete;

  ~Bfsp();

  explicit Bfsp(std::string filename, size_t cache_size_ = 1024 * 1024, pos_t cache_start = 0);

  template<class T>
  void getT(pos_t pos, T &x);

  void get(pos_t pos, char *x, ssize_t size);

  template<class T>
  pos_t allocT(const T &x);

  pos_t alloc(const char *x, ssize_t size);

  pos_t allocEmpty(ssize_t size);

  void memcpy(pos_t dest, pos_t src, ssize_t size);

  template<class T>
  void putT(pos_t pos, const T &x);

  void put(pos_t pos, const char *x, ssize_t size);

  void sync();

  void erase(pos_t pos, ssize_t size);

  template<class T>
  void getHeaderT(int id, T &x);

  void getHeader(int id, char *x, ssize_t sz);

  pos_t getHeaderId(int id);
  pos_t end;

private:
  const std::string filename;
  std::fstream fs;
  std::mutex alloc_mutex;
  const size_t cache_size;
  char *cache;
  pos_t cache_start;
};

Bfsp::~Bfsp() {
  sync();
  delete[] cache;
}

Bfsp::Bfsp(std::string filename_, size_t cache_size_, pos_t cache_start_) : filename(filename_),
                                    fs(filename, std::fstream::in | std::fstream::out | std::fstream::binary),
                                    cache_size(cache_size_), cache(new char[cache_size]), cache_start(cache_start_) {
#if RESET
  fs.close();
  {
    char cmd[1024];
    sprintf(cmd, "rm %s", filename.c_str());
    system(cmd);
  }
#endif

  if (!fs.is_open()) {
    fs.clear();
    fs.open(filename, std::fstream::out); // create file
    fs.close();
    fs.open(filename, std::fstream::in | std::fstream::out | std::fstream::binary);
  }
  assert(fs.is_open());
  fs.seekp(0, std::fstream::end);
  if (!fs.tellp()) {
    errf("initializing\n");
    std::unique_ptr<BfspHeader> hd(new BfspHeader);
    for (char &i: hd->data)
      i = 0;
    hd->nxt = nullpos;
    // putT(0, *hd);
    alloc(reinterpret_cast<const char*>(hd.get()), sizeof(BfspHeader));
  }
  assert(fs.is_open());
  assert(!fs.fail());
  end = fs.tellp();
  errf("(end %lu)\n", end);

  if (cache_start < end) {
    fs.seekg(cache_start);  // TODO: blah blah
    fs.read(cache, std::min(end - cache_start, (pos_t)cache_size));
  }
  assert(!fs.fail());
}

template<class T>
void Bfsp::getT(pos_t pos, T &x) {
  get(pos, reinterpret_cast<char *>(&x), sizeof(T));
}

template<class T>
pos_t Bfsp::allocT(const T &x) {
  return alloc(reinterpret_cast<const char *>(&x), sizeof(T));
}

template<class T>
void Bfsp::putT(pos_t pos, const T &x) {
  put(pos, reinterpret_cast<const char *>(&x), sizeof(T));
}

template<class T>
void Bfsp::getHeaderT(int id, T &x) {
  getHeader(id, reinterpret_cast<char *>(&x), sizeof(T));
}

void Bfsp::get(pos_t pos, char *x, ssize_t size) {
  // sync();
  if (cache_start <= pos && pos + size <= cache_start + cache_size) {
    ::memcpy(x, cache + pos - cache_start, size);
    return;
  }
  assert(!fs.fail());
  fs.seekg(pos);
  fs.read(x, size);
  assert(!fs.fail());
  auto [l, r] = cross(pos, pos + (pos_t)size, cache_start, cache_start + (pos_t)cache_size);
  if (l < r)
    ::memcpy(x + l - pos, cache + l - cache_start, r - l);
}

pos_t Bfsp::alloc(const char *x, ssize_t size) {
  std::lock_guard<std::mutex> guard(alloc_mutex);
  assert(!fs.fail());
  fs.seekp(0, std::fstream::end);
  pos_t ret = fs.tellp();
  fs.write(x, size);
  end = fs.tellp();
  assert(!fs.fail());
  auto [l, r] = cross(ret, ret + (pos_t)size, cache_start, cache_start + (pos_t)cache_size);
  if (l < r)
    ::memcpy(cache + l - cache_start, x + l - ret, r - l);
  // if (cache_start <= ret && ret + size <= cache_start + cache_size)
  //   ::memcpy(cache + ret - cache_start, x, size);
  return ret;
}

pos_t Bfsp::allocEmpty(ssize_t size) {  // TODO: can I alloc without filling each?
  std::lock_guard<std::mutex> guard(alloc_mutex);
  assert(!fs.fail());
  fs.seekp(0, std::fstream::end);
  pos_t ret = fs.tellp();
  for (int i = 0; i < size; ++i)
    fs.put('\0');
  end = fs.tellp();
  assert(end - ret == size);
  assert(!fs.fail());
  // if (cache_start <= ret && ret + size <= cache_start + cache_size)
  //   ::memset(cache + ret - cache_start, 0x00, size);
  auto [l, r] = cross(ret, ret + (pos_t)size, cache_start, cache_start + (pos_t)cache_size);
  if (l < r)
    ::memset(cache + l - cache_start, 0x00, r - l);
  return ret;
}

void Bfsp::memcpy(pos_t dest, pos_t src, ssize_t size) {  // TODO: WTF with my seekg and seekp. Are they using the same pointer?
  assert(!fs.fail());
  static constexpr ssize_t buf_size = 4096;
  static char buf[buf_size];
  ssize_t i;
  for (i = 0; i + buf_size <= size; i += buf_size) {
    // fs.seekg(src + i);
    // fs.read(buf, buf_size);
    get(src + i, buf, buf_size);
    // auto [sl, sr] = cross(src + i, src + i + buf_size, cache_start, cache_start + (pos_t)cache_size);
    // if (sl < sr)
    //   ::memcpy(buf, cache + sl - cache_start, sr - sl);
    put(dest + i, buf, buf_size);
    // auto [l, r] = cross(dest + i, dest + i + buf_size, cache_start, cache_start + (pos_t)cache_size);
    // if (l < r)
    //   ::memcpy(cache + l - cache_start, buf + l - dest - i, r - l);
    // if (l <= i && i + buf_size <= r)
    //   continue;
    // fs.seekp(dest + i);
    // fs.write(buf, buf_size);
  }
  assert(i <= size);
  if (i < size) {
    // errf("(P %ld G %ld)\n", fs.tellp(), fs.tellg());
    // fs.seekg(src + i);
    // fs.read(buf, size - i);
    get(src + i, buf, size - i);
    put(dest + i, buf, size - i);
    // auto [sl, sr] = cross(src + i, src + size, cache_start, cache_start + (pos_t)cache_size);
    // if (sl < sr)
    //   ::memcpy(buf, cache + sl - cache_start, sr - sl);
    // auto [l, r] = cross(dest + i, dest + size, cache_start, cache_start + (pos_t)cache_size);
    // if (l < r)
    //   ::memcpy(cache + l - cache_start, buf + l - dest - i, r - l);
    // if (not (l <= i && i + buf_size <= r)) {
    //   fs.seekp(dest + i);
    //   fs.write(buf, size - i);
    // }
    // sync();
    // errf("(P %ld G %ld)\n", fs.tellp(), fs.tellg());
    // errf("writing buf %x to %ld\n", buf[0], fs.tellp());
  }
  assert(!fs.fail());


  /*
  for (i = 0; i + 1024 <= size; i += 1024) {
    fs.read(buf, 1024);
    fs.write(buf, 1024);
  }
  assert(i <= size);
  if (i < size) {
    errf("(P %ld G %ld)\n", fs.tellp(), fs.tellg());
    fs.read(buf, size - i);
    fs.write(buf, size - i);
    // sync();
    errf("(P %ld G %ld)\n", fs.tellp(), fs.tellg());
    errf("writing buf %x to %ld\n", buf[0], fs.tellp());
  }
  assert(!fs.fail());
  */
}

void Bfsp::put(pos_t pos, const char *x, ssize_t size) {
  if (cache_start <= pos && pos + size <= cache_start + cache_size) {
    ::memcpy(cache + pos - cache_start, x, size);
    return;
  }
  assert(!fs.fail());
  fs.seekp(pos);
  fs.write(x, size);
  assert(!fs.fail());
  auto [l, r] = cross(pos, pos + (pos_t)size, cache_start, cache_start + (pos_t)cache_size);
  if (l < r)
    ::memcpy(cache + l - cache_start, x + l - pos, r - l);
}

void Bfsp::sync() {
  if (cache_start < end) {
    fs.seekp(cache_start);
    fs.write(cache, std::min((pos_t)cache_size, end - cache_start));
  }
  fs.sync();
}

void Bfsp::erase(pos_t pos, ssize_t size) {
  // TODO: reuse the space
}

void Bfsp::getHeader(int id, char *x, ssize_t sz) {
  assert(0 <= sz && sz <= header_size);
  get(getHeaderId(id), x, sz);
}

pos_t Bfsp::getHeaderId(int id) {
  assert(0 <= id);
  pos_t now = 0;
  int cnt = 0;
  std::unique_ptr<BfspHeader> hd(new BfspHeader);
  while ((cnt++) < id) {
    getT(now, *hd);
    now = hd->nxt;
    assert(now != nullpos);
  }
  return now;
}

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

Bfsp bf("beef.data", 256 * 1024);
Bfsp nd("noodle.data", 256 * 1024);
Bfsp lm("lemon.data", 512 * 1024);

struct BHeader {
  int depth;
  pos_t root;
  int timestamp;
} header;

constexpr size_t child_cnt = 16;  // TODO: test the best value
typedef char Key[65];
bool KeyCmp(const std::string &u, const std::string &v) {
  return u < v;
}
bool KeyEq(const std::string &u, const std::string &v) {
  return u == v;
}

struct Node {
  Key key[child_cnt - 1];
  size_t size;
  pos_t fa, chd[child_cnt];
};

struct Data {
  pos_t beg;
  int size, capacity;
};

struct Cell {
  int val;
  bool erase;
  int timestamp;
};

void push_back(Data &s, const Cell &cell) {
  if (s.size == s.capacity || !s.capacity) {
    s.capacity = std::max(4, int(s.capacity * 2));
    pos_t new_beg = lm.allocEmpty(sizeof(Cell) * s.capacity);
    // errf("(new_beg %ld)\n", new_beg);
    if (s.size) {
      // errf("memcpy %lu %ld %ld\n", sizeof(Cell) * s.size, s.beg, new_beg);
      lm.memcpy(new_beg, s.beg, sizeof(Cell) * s.size);
      // errf("DEBUG %d %d\n", NWITH_TR(nd, s.beg, Cell, c, c.val), NWITH_TR(nd, new_beg, Cell, c, c.val));
    }
    s.beg = new_beg;
  }
  assert(s.size < s.capacity);
  lm.putT(s.beg + sizeof(Cell) * (s.size++), cell);
}

void traceDfs(int dep, pos_t pos) {
  Node node{};
  WITH_ETR(pos, node, );
  if (dep == header.depth) {
    for (int i = 0; i < node.size; ++i)
      errf("%s ", node.key[i]);
    return;
  }
  for (int i = 0; i < node.size; ++i) {
    if (i) {
      for (int j = dep; j < header.depth; ++j)
        errf("|");
      errf(" ");
    }
    traceDfs(dep + 1, node.chd[i]);
  }
}

void trace() {
  Node root;
  WITH_ETR(header.root, root, );
  errf("(dep %d root.size %lu)\n", header.depth, root.size);
  errf("(trace ");
  traceDfs(0, header.root);
  errf(")\n");
  errf("(top ");
  for (int i = 0; i + 1 < root.size; ++i)
    errf("%s ", root.key[i]);
  errf(")\n");
  if (header.depth >= 2) {
    for (int i = 0; i < root.size; ++i) {
      Node ch;
      WITH_ETR(root.chd[i], ch, );
      errf(" (sub ");
      for (int j = 0; j + 1 < ch.size; ++j)
        errf("%s ", ch.key[j]);
      errf(")\n");
    }
  }
  errf("(nd.end %ld)\n", nd.end);
}

void my_insert(std::string key, int val) {
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
    Data dhd{};
    NWITH_ETR(nd, node.chd[k], dhd, );
    Cell cell{val, false, ++header.timestamp};
    bf.putT(0, header);
    push_back(dhd, cell);
    NWITH_ETW(nd, node.chd[k], dhd, );
  } else if (node.size + 1 < child_cnt) {
    for (int i = node.size; i > k; --i) {
      memcpy(&node.key[i], &node.key[i - 1], sizeof(node.key[i]));
      memcpy(&node.chd[i], &node.chd[i - 1], sizeof(node.chd[i]));
    }
    // memcpy(&node.key[k], key.c_str(), sizeof(Key));
    strcpy(node.key[k], key.c_str());
    Data dch{0, 0, 0};
    Cell cell{val, false, ++header.timestamp};
    bf.putT(0, header);
    push_back(dch, cell);
    node.chd[k] = nd.allocT(dch);
    ++node.size;
    WITH_ETW(now, node, );
  } else {
    assert(node.size == child_cnt - 1);
    Data dch{0, 0, 0};
    Cell cell{val, false, ++header.timestamp};
    bf.putT(0, header);
    push_back(dch, cell);
    pos_t dhd_pos = nd.allocT(dch);

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

int main() {
  // freopen("../test.in", "r", stdin);
  // freopen("/dev/null", "w", stdout);
  // errf("%lu\n", sizeof(Key));
  // errf("%lu\n", sizeof(BHeader));

  std::cin.tie(nullptr);
  std::ios::sync_with_stdio(false);

  bf.getHeaderT(0, header);
  if (header.root == 0) {
    errf("initializing tree\n");
    Node node{{}, 0, nullpos, {}};
    header.root = bf.allocT(node);
    header.depth = 0;
    bf.putT(0, header);
  }

  int T;
  std::cin >> T;
  while (T--) {
    std::string op, key;
    std::cin >> op >> key;
    if (key == "zliepws")
      errf("DEBUG ME");
    if (op == "insert") {
      int val;
      std::cin >> val;
      my_insert(key, val);
    } else if (op == "find") {
      int depth = 0;
      pos_t pos = header.root;
      Node node{};
      WITH_ETR(pos, node, );
      pos_t k{};
      while (depth <= header.depth) {
        WITH_ETR(pos, node, );
        for (k = 0; k + 1 < node.size; ++k)
          if (!KeyCmp(node.key[k], key))
            break;
        pos = node.chd[k];
        ++depth;
      }

      std::vector<Cell> res;
      if (k < child_cnt - 1 && KeyEq(node.key[k], key)) {
        Data dch{};
        NWITH_ETR(nd, pos, dch, );
        res.resize(dch.size);
        lm.get(dch.beg, reinterpret_cast<char*>(&res[0]), sizeof(Cell) * dch.size);
      }

      std::sort(res.begin(), res.end(), [&](Cell &u, Cell &v) {
        return u.val != v.val ? u.val < v.val : u.timestamp < v.timestamp;
      });
      // errf("find ");
      // for (int i = 0; i < (int)res.size(); ++i)
      //   errf("(%d %d) ", res[i].val, res[i].erase);
      // errf("\n");
      std::vector<int> ans;
      for (int i = 0, i_; i < (int)res.size(); i = i_) {
        bool exist = false;
        for (i_ = i; i_ < (int)res.size() && res[i].val == res[i_].val; ++i_)
          if (res[i_].erase)
            exist = false;
          else
            exist = true;
        if (exist)
          ans.push_back(res[i].val);
      }

      if (ans.empty()) {
        puts("null");
        continue;
      }
      for (const int &v: ans)
        printf("%d ", v);
      puts("");

      if (k < child_cnt - 1 && KeyEq(node.key[k], key)) {
        Data dch{};
        NWITH_ETR(nd, pos, dch, );
        dch.size = ans.size();
        assert(res.size() >= ans.size());
        // res.resize(ans.size());
        for (int i = 0; i < ans.size(); ++i)
          res[i] = Cell{ans[i], false, 0};
        lm.put(dch.beg, reinterpret_cast<char*>(&res[0]), sizeof(Cell) * dch.size);
        NWITH_ETW(nd, pos, dch, );
      }
    } else if (op == "delete") {
      int val;
      std::cin >> val;
      int depth = 0;
      pos_t pos = header.root;
      Node node{};
      WITH_ETR(pos, node, );
      pos_t k{};
      while (depth <= header.depth) {
        WITH_ETR(pos, node, );
        for (k = 0; k + 1 < node.size; ++k)
          if (!KeyCmp(node.key[k], key))
            break;
        pos = node.chd[k];
        ++depth;
      }
      if (k < child_cnt - 1 && KeyEq(node.key[k], key)) {
        // errf("(deleting)\n");
        Data dch{};
        NWITH_ETR(nd, pos, dch, );
        Cell cell{val, true, ++header.timestamp};
        push_back(dch, cell);
        bf.putT(0, header);
        NWITH_ETW(nd, pos, dch, );
      }
    } else
      assert(false);

    // trace();
  }
  return 0;
}
