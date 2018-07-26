#include "bptree.hpp"
// #include <queue>
#include <string>
#include <vector>

#include "String.h"
#include <iostream>

class database
{
private:
  bptree<int, svalue> *dataRecord;
  vector<pair<int, svalue>> memory;
  // queue<int, svalue> cache;

public:
  database();
  ~database();

  void open(const string file);
  void close();
  svalue getValue(const int key);
  void getRange(const int key1, const int key2);
  void insert(const int key, const string value);
  void set(const int key, const string value);
  void remove(const int key);
  void printAll();
};