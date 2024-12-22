#ifndef FINANCE_H
#define FINANCE_H

#include "fs.h"
#include "database.h"

// TODO: optimize using DBMore::vector

class Finance {
  Bfsp bf;
  Database<int, std::pair<bool, double>> db;
  int timestamp;
  Finance();

public:
  static Finance &getInstance() {
    static Finance r;
    return r;
  }

  void income(double c);
  void outcome(double c);
  void show(int c);
  void showAll();
};
// Finance &fnce = Finance::getInstance();
#define fnce Finance::getInstance()

#endif
