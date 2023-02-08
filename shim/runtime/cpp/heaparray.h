// -*-c++-*-

#pragma once

#include <iostream>
#include <cstring>
#include <array>
#include "common/loop_type.h"
#include "ref_count.h"

using namespace std;

namespace shim {

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

  HeapArray(loop_type sz) : base(new BaseHeapArray<Elem>()) {
    base->data = new Elem[sz];
    memset(base->data, 0, sizeof(Elem) * sz);
    base->incr();
  }

  HeapArray(Elem *data) : base(new BaseHeapArray<Elem>()) {
    base->data = data;
    base->incr(); 
    base->incr(); // increment twice so that it is never actually deleted by us. The user made it, so they must delete it.
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
  HeapArray(const HeapArray<Elem> &other) : base(other.base) { base->incr(); }

  // Copy assignment
  HeapArray<Elem> &operator=(const HeapArray<Elem> &other) {
    if (this == &other) {
      return *this;
    }
    base = other.base;
    return *this;
  }

  // Move constructors don't update the reference count, so don't need to manually specify them

  Elem &operator[](loop_type lidx) const {
    return base->operator[](lidx);
  }

  void write(loop_type lidx, Elem e) {
    base->write(lidx, e);
  }
  
  BaseHeapArray<Elem> *base;

};

}
