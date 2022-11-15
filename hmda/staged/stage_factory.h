#pragma once

#include "hmda.h"

namespace hmda {

  namespace staged {

    // Wrapper for building a Block from a set of extents
    template<typename Elem, int Rank, typename...BExtents>
    auto factory(const builder::dyn_var<Elem *> &buffer, BExtents...bextents) {
      auto bstrides = make_tuple<1, Rank>();
      auto borigin = make_tuple<0, Rank>();
      auto tbextents = std::tuple<BExtents...>(bextents...);
      return Block<Elem, Rank, decltype(tbextents), decltype(bstrides), decltype(borigin)>(tbextents,
                                                                                           bstrides,
                                                                                           borigin,
                                                                                           buffer);
    }

  }

}