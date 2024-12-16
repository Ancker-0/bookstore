#ifndef CI_H
#define CI_H

#include <iostream>
#include <map>
#include <vector>

namespace ci {
  struct Tokenized;

  class Ci {
  public:
    Ci() = default;
    Ci(const Ci &) = delete;
    Ci(Ci &&) = delete;

    void process_one();
    static Ci &getInstance() {
      static Ci instance;
      return instance;
    }

  private:
    std::istream &is = std::cin;
    std::ostream &os = std::cout;
  };

  struct Tokenized {
    std::vector<std::string> command;
    std::map<std::string, std::string> param;
  };

  Tokenized tokenize(std::string);

}

#endif //CI_H
