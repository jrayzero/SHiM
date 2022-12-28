#pragma once

#include <iostream>
#include <cstring>
#include <array>
#include "common/loop_type.h"
#include "ref_count.h"

using namespace std;

namespace cola {

template <typename Elem>
struct HeapArray;

template <typename Elem>
struct BaseHeapArray : public RefCounted {
  friend struct HeapArray<Elem>;

  Elem &operator[](loop_type lidx) const {
    return data[lidx];
  }

  void write(loop_type lidx, Elem e) {
    data[lidx] = e;
  }

  Elem *data;
};

template <typename Elem>
struct HeapArray {

  HeapArray(loop_type sz) : base(new BaseHeapArray<Elem>()), sz(sz) {
    base->data = new Elem[sz];
    memset(base->data, 0, sizeof(Elem) * sz);
    base->incr();
  }

  ~HeapArray() {
    base->decr();
    if (base->can_free()) {
      delete[] base->data;
      delete base;
      base = nullptr;
    }
  }

  // Copy constructor
  HeapArray(const HeapArray<Elem> &other) : base(other.base), sz(other.sz) { base->incr(); }

  // Copy assignment
  HeapArray<Elem> &operator=(const HeapArray<Elem> &other) {
    if (this == &other) {
      return *this;
    }
    base = other.base;
    sz = other.sz;
    return *this;
  }

  // Move constructors don't update the reference count, so don't need to manually specify them

  Elem &operator[](loop_type lidx) const {
#ifdef BOUNDS_CHECK
    if (lidx >= sz) {
      std::cerr << "Out of bounds! Got " << lidx << " but data size is " << sz << std::endl;
      exit(-1);
    }
#endif
    return base->operator[](lidx);
  }

  void write(loop_type lidx, Elem e) {
#ifdef BOUNDS_CHECK
    if (lidx >= sz) {
      std::cerr << "Out of bounds! Got " << lidx << " but data size is " << sz << std::endl;
      exit(-1);
    }
#endif
    base->write(lidx, e);
  }
  
  BaseHeapArray<Elem> *base;
  loop_type sz;

};

}
