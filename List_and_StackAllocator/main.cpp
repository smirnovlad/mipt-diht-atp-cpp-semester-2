#include <iostream>
#include "stackallocator.cpp"
#include <vector>


int main() {
  StackStorage<100> storage;
  StackAllocator<int, 100> alloc(storage);
  std::vector<int, StackAllocator<int, 100>> v(alloc);



  return 0;
}
