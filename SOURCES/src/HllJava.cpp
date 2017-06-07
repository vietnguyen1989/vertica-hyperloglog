#include "stdlib.h"
#include "hll.hpp"

auto format = Format::COMPACT_6BITS;
uint8_t precision = 12;
auto synopsisSize = Hll<uint64_t>::getDeserializedSynopsisSize(precision);

extern "C" void init(char* arr) {
  uint8_t* synopsis = (uint8_t*)malloc(synopsisSize);

  Hll<uint64_t> hll(12, synopsis);
  hll.serialize(arr, format);

  free(synopsis);
}

extern "C" void add(char* arr, long i) {
  uint8_t* synopsis = (uint8_t*)malloc(synopsisSize);

  Hll<uint64_t> hll(12, synopsis);
  hll.deserialize(arr, format);
  hll.add(i);
  hll.serialize(arr, format);

  free(synopsis);
}

extern "C" void merge(char* arr, char* arrOther) {
  uint8_t* synopsis = (uint8_t*)malloc(synopsisSize);
  uint8_t* synopsisOther = (uint8_t*)malloc(synopsisSize);

  Hll<uint64_t> hll(12, synopsis);
  Hll<uint64_t> hllOther(12, synopsisOther);
  hll.deserialize(arr, format);
  hllOther.deserialize(arrOther, format);
  hll.add(hllOther);
  hll.serialize(arr, format);

  free(synopsis);
  free(synopsisOther);
}

extern "C" long count(char* arr) {
  uint8_t* synopsis = (uint8_t*)malloc(synopsisSize);

  Hll<uint64_t>hll(12, synopsis);
  hll.deserialize(arr, format);
  return hll.approximateCountDistinct();

  free(synopsis);
}
