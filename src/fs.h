#ifndef FS_H
#define FS_H
#include <string>
#include <cstdint>
#include <mutex>
#include <fstream>

typedef int64_t pos_t;

constexpr pos_t nullpos = -1;
constexpr size_t header_size = 56;

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

#endif //FS_H
