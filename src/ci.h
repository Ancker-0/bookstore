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
    std::string raw;
    std::vector<std::string> splited;
    std::vector<std::string> command;
    std::map<std::string, std::string> param;
    bool cmdB4param, fail_param;
  };

  Tokenized tokenize(std::string);

}

using namespace ci;

#endif //CI_H
