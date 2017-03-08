#include "hll.hpp"

asm (".symver memcpy, memcpy@GLIBC_2.2.5");

auto format = Format::COMPACT_6BITS;

extern "C" void init(char* arr) {
  Hll<uint64_t> hll(12);
  hll.serialize(arr, format);
}

extern "C" void add(char* arr, long i) {
  Hll<uint64_t> hll(12);
  hll.deserialize(arr, format);
  hll.add(i);
  hll.serialize(arr, format);
}

extern "C" void merge(char* arr, char* arrOther) {
  Hll<uint64_t> hll(12);
  Hll<uint64_t> hllOther(12);
  hll.deserialize(arr, format);
  hllOther.deserialize(arrOther, format);
  hll.add(hllOther);
  hll.serialize(arr, format);
}

extern "C" long count(char* arr) {
  Hll<uint64_t>hll(12);
  hll.deserialize(arr, format);
  return hll.approximateCountDistinct();
}
