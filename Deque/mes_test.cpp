#include <type_traits>
#include <unordered_set>
#include <iostream>
#include <cassert>
#include <deque>

#include "deque.h"

//template <typename T>
//using Deque = std::deque<T>;

void test1() {
  Deque<int> d(10, 3);

  d[3] = 5;

  d[7] = 8;

  d[9] = 10;

  std::string s = "33353338310";
  std::string ss;
  Deque<int> dd;

  {
    Deque<int> d2 = d;

    dd = d2;
  }

  d[1] = 2;

  d.at(2) = 1;

  try {
    d.at(10) = 0;
    assert(false);
  } catch (std::out_of_range&) {}

  const Deque<int>& ddd = dd;
  for (size_t i = 0; i < ddd.size(); ++i) {
    ss += std::to_string(ddd[i]);
  }

  assert(s == ss);
}

void test2() {
  Deque<int> d(1);

  d[0] = 0;

  for (int i = 0; i < 8; ++i) {
    d.push_back(i);
    d.push_front(i);
  }

  for (int i = 0; i < 12; ++i) {
    d.pop_front();
  }

  d.pop_back();
  assert(d.size() == 4);

  std::string ss;

  for (size_t i = 0; i < d.size(); ++i) {
    ss += std::to_string(d[i]);
  }

  assert(ss == "3456");
}

void test3() {
  Deque<int> d;

  for (int i = 0; i < 1000; ++i) {
    for (int j = 0; j < 1000; ++j) {

      if (j % 3 == 2) {
        d.pop_back();
      } else {
        d.push_front(i*j);
      }

    }
  }

  assert(d.size() == 334'000);

  Deque<int>::iterator left = d.begin() + 100'000;
  Deque<int>::iterator right = d.end() - 233'990;
  while (d.begin() != left) d.pop_front();
  while (d.end() != right) d.pop_back();

  assert(d.size() == 10);

  assert(right - left == 10);

  std::string s;
  for (auto it = left; it != right; ++it) {
    ++*it;
  }
  for (auto it = right - 1; it >= left; --it) {
    s += std::to_string(*it);
  }

  assert(s == "51001518515355154401561015695158651595016120162051");
}

int main() {
  test1();
  test2();
  test3();
}
