#pragma once

#include "builder/dyn_var.h"

#include "base.h"

namespace hmda {

// Unstaged version
template <typename Elem, int Rank>
struct Block<Elem,Rank,false> : public BaseBlockLike<Elem,Rank> {

  static constexpr bool staged = false;

  Block(const std::array<loop_type, Rank> &bextents) :
    BaseBlockLike<Elem,Rank>(bextents, make_array<loop_type,1,Rank>(),
			     make_array<loop_type,0,Rank>(), bextents) {
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

  std::string dump() const {
    std::stringstream ss;
    // TODO print out Elem type
    ss << "Block<" << Rank << ">" << std::endl;
    ss << "  Extents: " << join(this->bextents) << std::endl;
    ss << "  Strides: " << join(this->bstrides) << std::endl;
    ss << "  Origin:  " << join(this->borigin) << std::endl;
    return ss.str();
  }

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

namespace unstaged {

template <typename Elem, int Rank>
using Block = Block<Elem, Rank, false>;

}

}
