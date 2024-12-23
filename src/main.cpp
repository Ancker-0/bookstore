#include "ci.h"
#include "error.h"
#include "database_more.h"
#include "util.h"
#include "config.h"

#include <iostream>
#include <cstdio>
#include <cstring>
#include <functional>

using namespace ci;

int main() {
#if TESTFILE
  freopen(FILENAME, "r", stdin);
#endif
  errf("%d\n", valid_bookname(std::string{"Hello"}));

  Bfsp bf("test.db");
  DBMore<int, int, int> dbm{bf};
	int error_cnt = 0;
  while (true) {
    Ci &ci = Ci::getInstance();
    try {
#if 1
#if PROMPT
      std::cout << "> ";
#endif
      ci.process_one();
#else
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
#endif
    } catch (const Error &e) {
#if VERBOSE
      errf("error - %s %d\n", e.msg.c_str(), ++error_cnt);
#endif
      std::cout << "Invalid" << std::endl;
    }
    errf("");
  }
  return 0;
}
