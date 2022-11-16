#pragma once

#include "builder/dyn_var.h"

#include "base.h"

// Unstaged version
template <typename Elem, int Rank>
struct Block<Elem,Rank,false> : public BaseBlockLike<Elem,Rank> {

  static constexpr bool staged = false;

  Block(const std::array<loop_type, Rank> &bextents) :
    BaseBlockLike<Elem,Rank>(bextents, bextents) {
    // TODO ref counted buffer
    data = new Elem[100];
  }

  Elem *data;

  // Use these when you just want a single loop
  template <typename...Iters>
  Elem &operator()(const std::tuple<Iters...> &);

  template <typename...Iters>
  Elem &operator()(Iters...iters);

  // Take an unstaged block and create one with Staged = true, as well
  // as a dyn_var buffer. This must only be called within the staging context!
  auto stage(builder::dyn_var<Elem*> data);

};

template <typename Elem, int Rank>
auto Block<Elem,Rank,false>::stage(builder::dyn_var<Elem*> data) {
  return Block<Elem, Rank, true>(this->bextents, data);
}


// for staging, this returns a dyn_var
template <typename Elem, int Rank>
template <typename...Iters>
Elem &Block<Elem,Rank,false>::operator()(const std::tuple<Iters...> &iters) {
  // TODO verify that no Iters in here
  auto lidx = this->template linearize<0>(iters);
  return data[lidx];
}

template <typename Elem, int Rank>
template <typename...Iters>
Elem &Block<Elem,Rank,false>::operator()(Iters...iters) {
  // TODO verify that no Iters in here
  return this->operator()(std::tuple{iters...});
}

