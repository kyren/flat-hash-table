#include <cassert>
#include <iostream>

#include "flat_hash_set.hpp"
#include "flat_hash_map.hpp"

using namespace flat_hash;

void test_hash_set() {
  hash_set<int> test_set = {42, 63};
  assert(test_set.find(41) == test_set.end());
  assert(*test_set.find(42) == 42);
  assert(*test_set.find(63) == 63);
  assert(test_set.find(64) == test_set.end());
  assert(test_set.size() == 2u);

  test_set.erase(test_set.find(42));
  assert(test_set.find(42) == test_set.end());
  assert(*test_set.find(63) == 63);
  assert(test_set.size() == 1u);

  test_set.erase(test_set.find(63));
  assert(test_set.find(42) == test_set.end());
  assert(test_set.find(63) == test_set.end());
  assert(test_set.size() == 0u);

  test_set.insert(12);
  test_set.insert(24);
  assert(*test_set.find(12) == 12);
  assert(test_set.size() == 2u);
  test_set.clear();

  assert(test_set.find(12) == test_set.end());
  assert(test_set.size() == 0u);

  assert(test_set.insert(7).second);
  assert(test_set.insert(11).second);
  assert(!test_set.insert(7).second);

  assert(test_set.size() == 2u);

  hash_set<int> test_set2(test_set.begin(), test_set.end());
  assert(test_set == test_set2);

  assert(test_set.erase(test_set.begin(), test_set.end()) == test_set.end());
  assert(test_set.size() == 0u);

  assert(test_set != test_set2);

  hash_set<int> test_set3(test_set.begin(), test_set.end());
  assert(test_set3.size() == 0u);

  test_set2 = test_set;
  assert(test_set == test_set2);
}

void test_hash_map() {
  hash_map<int, int> test_map = {{42, 42}, {63, 63}};
  assert(test_map.find(41) == test_map.end());
  assert(test_map.find(42)->second == 42);
  assert(test_map.find(63)->second == 63);
  assert(test_map.find(64) == test_map.end());
  assert(test_map.size() == 2u);

  test_map.erase(test_map.find(42));
  assert(test_map.find(42) == test_map.end());
  assert(test_map.find(63)->second == 63);
  assert(test_map.size() == 1u);

  test_map.erase(test_map.find(63));
  assert(test_map.find(42) == test_map.end());
  assert(test_map.find(63) == test_map.end());
  assert(test_map.size() == 0u);

  test_map.insert({12, 12});
  test_map.insert({24, 24});
  assert(test_map.find(12)->second == 12);
  assert(test_map.size() == 2u);
  test_map.clear();

  assert(test_map.find(12) == test_map.end());
  assert(test_map.size() == 0u);

  assert(test_map.insert({7, 7}).second);
  assert(test_map.insert({11, 11}).second);
  assert(!test_map.insert({7, 7}).second);

  assert(test_map.size() == 2u);

  hash_map<int, int> test_map2(test_map.begin(), test_map.end());
  assert(test_map == test_map2);

  assert(test_map.erase(test_map.begin(), test_map.end()) == test_map.end());
  assert(test_map.size() == 0u);

  assert(test_map != test_map2);

  hash_map<int, int> test_map3(test_map.begin(), test_map.end());
  assert(test_map3.size() == 0u);

  test_map2 = test_map;
  assert(test_map == test_map2);
}

int main(int argc, char** argv) {
    test_hash_set();
    test_hash_map();
    std::cout << "tests passed!" << std::endl;
    return 0;
}
