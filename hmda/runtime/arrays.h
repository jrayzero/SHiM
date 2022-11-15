#pragma once

#include <array>
#include "ref_count.h"

using namespace std;

template <typename Elem>
struct HeapArray;

template <typename Elem>
struct BaseHeapArray : public RefCounted {
  friend struct HeapArray<Elem>;

  Elem &operator[](int lidx) const {
    return data[lidx];
  }

  void write(int lidx, Elem e) {
    data[lidx] = e;
  }

private:

  Elem *data;
};

template <typename Elem>
struct HeapArray {

  HeapArray(uint64_t sz) : base(new BaseHeapArray<Elem>()){
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
  HeapArray(const HeapArray<Elem> &other) : base(other.base) { base->incr(); }

  // Copy assignment
  HeapArray<Elem> &operator=(const HeapArray<Elem> &other) {
    if (this == &other) {
      return *this;
    }
    base->decr(); // since about to overwrite the prior base, make sure to decr (b/c the BaseHeapArray destructor does not do any decrement things)
    base = other.base;
    base->incr();
    return *this;
  }

  // Move constructors don't update the reference count, so don't need to manually specify them

  Elem &operator[](int lidx) const {
    return base->operator[](lidx);
  }

  void write(int lidx, Elem e) {
    base->write(lidx, e);
  }
  
private:

  BaseHeapArray<Elem> *base;

};
