#include "fs.h"
#include "error.h"
#include "config.h"

#include <cassert>
#include <memory>
#include <cstring>

struct BfspHeader {
  char data[header_size];
  pos_t nxt;
};
static_assert(sizeof(BfspHeader) == 64);

template <class T>
inline std::pair<T, T> cross(T l1, T r1, T l2, T r2) {
  return { std::max(l1, l2), std::min(r1, r2) };
}

Bfsp::~Bfsp() {
  sync();
  delete[] cache;
}

Bfsp::Bfsp(std::string filename_, size_t cache_size_, pos_t cache_start_) : filename(filename_),
                                    fs(filename, std::fstream::in | std::fstream::out | std::fstream::binary),
                                    cache_size(cache_size_), cache(new char[cache_size]), cache_start(cache_start_) {
#if RESET_DB
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
  errf("constructing fs %s\n", filename.c_str());
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
  errf("constructed fs %s\n", filename.c_str());
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
  get(getHeaderPos(id), x, sz);
}

pos_t Bfsp::getHeaderPos(int id) {
  assert(0 <= id);
  pos_t now = 0;
  int cnt = 0;
  std::unique_ptr<BfspHeader> hd(new BfspHeader);
  while ((cnt++) < id) {
    getT(now, *hd);
    if (hd->nxt == nullpos) {
      BfspHeader nhd;
      nhd.nxt = nullpos;
      ::memset(nhd.data, 0, sizeof(nhd.data));
      hd->nxt = allocT(nhd);
      putT(now, *hd);
    }
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
