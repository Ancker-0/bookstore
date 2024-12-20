#include "ci.h"
#include "error.h"
#include "database_more.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <functional>

using namespace ci;

int main() {
  errf("%lu\n", sizeof(double));
  double x = 1.0;
  memset(&x, 0, sizeof(x));
  printf("%Lf\n", x);
  Bfsp bf("test.db");
  DBMore<int, int, int> dbm{bf};
  while (true) {
    Ci &ci = Ci::getInstance();
    try {
      // ci.process_one();
      std::string s;
      std::getline(std::cin, s);
      Tokenized tk = tokenize(s);
      if (tk.command.empty())
        continue;
      if (tk.command.at(0) == "insert") {
        int k = std::atoi(tk.command.at(1).c_str());
        int v = std::atoi(tk.command.at(2).c_str());
        dbm.insert(k, v, v);
      } else if (tk.command.at(0) == "find") {
        auto vs = dbm.get(std::atoi(tk.command.at(1).c_str()));
        for (auto x : vs)
          printf("%d ", x);
        puts("");
      } else if (tk.command.at(0) == "exit")
        exit(0);
    } catch (const Error &e) {
      errf("error - %s\n", e.msg.c_str());
    }
    errf("");
  }
  return 0;
}
