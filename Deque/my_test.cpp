#include "deque.h"
#include <deque>
#include <random>
#include <chrono>

std::mt19937 gen(std::chrono::high_resolution_clock::now().time_since_epoch().count());

Deque<int> my_deque;
std::deque<int> stl_deque;

void CHECK() {
  auto it1 = my_deque.begin();
  auto it2 = stl_deque.begin();
  for (size_t i = 0; i < 20; ++i) {
    std::cout << *(it1 + i) << " " << *(it2 + i) << " " << my_deque.size() << std::endl;
    if (*(it1 + i) != *(it2 + i)) {
      std::cout << "OOPS!\n";
      break;
    }
  }
}

void TEST1() {
  for (uint32_t i = 0; i < 1000; ++i) {
    bool b = gen() % 2;
    int val = gen() % 1000;
    std::cout << "i = " << i << '\n';
    if (b) {
      my_deque.push_back(val);
      stl_deque.push_back(val);
    } else {
      my_deque.push_front(val);
      stl_deque.push_front(val);
    }
  }
  CHECK();
}

void TEST2() {
  for (uint32_t i = 0; i < 600; ++i) {
    uint32_t index = gen() % my_deque.size();
    my_deque.erase(my_deque.begin() + index);
    stl_deque.erase(stl_deque.begin() + index);
  }
  CHECK();
}

void TEST3() {
  for (uint32_t i = 0; i < 200; ++i) {
    uint32_t index = gen() % my_deque.size();
    int val = gen() % 100;
    my_deque.insert(my_deque.begin() + index, val);
    stl_deque.insert(stl_deque.begin() + index, val);
  }
  CHECK()
}

int main() {
  TEST1();
  TEST2();

  return 0;
}