// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "common/loop_type.hpp"
#include "fwrappers.hpp"
#include "utils.hpp"
#include "properties.hpp"
#include "allocators.hpp"
#include "functors.hpp"

using builder::dyn_var;
using builder::dyn_arr;
using builder::static_var;

namespace shim {

// TODO when the shared_ptr gets destroyed for the last time, can insert a delete for the heap allocator
template <typename Elem, int PhysicalRank>
using Allocation_T = std::shared_ptr<Allocation<Elem,PhysicalRank>>;

template <typename Elem, unsigned long Rank, bool MultiDimRepr=false>
struct View;

template <typename Elem, unsigned long Rank, bool MultiDimRepr=false>
struct Block {

  static Block<Elem,Rank,false> heap(Properties<Rank> location) { return Block<Elem,Rank,MultiDimRepr>(location); }

  static Block<Elem,Rank,false> stack(Properties<Rank> location) { 
//    return Block<Elem,Rank,MultiDimRepr>(location); 
  }
  
  template <typename External>
  static Block<Elem,Rank,peel<External>()!=1> external(Properties<Rank> location, External external) {
    constexpr int nlevels = peel<External>();
    static_assert(nlevels == 1 || nlevels == Rank);
    constexpr bool is_multi = nlevels == Rank;
    if constexpr (is_multi) {
      auto allocator = std::make_shared<UserAllocation<Elem,External,Rank>>(external);
      return Block<Elem,Rank,MultiDimRepr>(location, allocator);
    } else {
      auto allocator = std::make_shared<UserAllocation<Elem,Elem*,1>>(external);
      return Block<Elem,Rank,MultiDimRepr>(location, std::move(allocator));
    }
  }

  Block(Properties<Rank> location,
	Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
    _location(location), _allocator(allocator) { }

  Block(Properties<Rank> location) : 
    _location(location) { 
    dyn_var<loop_type> sz = 1;
    for (static_var<int> i = 0; i < Rank; i=i+1) {
      sz *= location.extents()[i];
    }
    this->_allocator = std::make_shared<HeapAllocation<Elem>>(sz);
  }

  Properties<Rank> location() { return _location; }
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator() { return _allocator; }

  template <typename...Ranges>
  View<Elem,Rank> slice(Ranges...ranges);

  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank2> colocate(Block<Elem2,Rank2> &idx);
  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank2> colocate(View<Elem2,Rank2> &idx);

  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank> hcolocate(Block<Elem2,Rank2> &idx);
  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank> hcolocate(View<Elem2,Rank2> &idx);

  Block<Elem,Rank> ppermute(array<loop_type,Rank> perms);
  View<Elem,Rank> vpermute(array<loop_type,Rank> perms);

  Block<Elem,Rank> prefine(Property<Rank> refine);
  View<Elem,Rank> vrefine(Property<Rank> refine);

  template <typename...Idxs>
  dyn_var<Elem> operator()(Idxs...idxs);

  template <unsigned long Rank2>
  void write(Property<Rank2> idxs, dyn_var<Elem> val);

  View<Elem,Rank> view();

  void dump();

private:
  
  Properties<Rank> _location;
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> _allocator;

};

template <typename Elem, unsigned long Rank>
Block<Elem,Rank,false> heap(Properties<Rank> prop) {
  return Block<Elem,Rank,false>::heap(prop);
}

template <typename Elem, unsigned long Rank>
Block<Elem,Rank,false> stack(Properties<Rank> prop) {

}

template <typename Elem, unsigned long Rank, typename External>
Block<Elem,Rank,peel<External>()!=1> external(Properties<Rank> prop, External external) {
  return Block<Elem,Rank,peel<External>()!=1>::external(prop, external);
}


template <typename Elem, unsigned long Rank, bool MultiDimRepr>
struct View {

  View(Properties<Rank> vlocation, Properties<Rank> blocation,
       Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
    _vlocation(vlocation), _blocation(blocation), _allocator(allocator) { }

  Properties<Rank> vlocation() { return _vlocation; }
  Properties<Rank> blocation() { return _blocation; }
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator() { return _allocator; }

  template <typename...Ranges>
  View<Elem,Rank> slice(Ranges...ranges);

  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank2> colocate(Block<Elem2,Rank2> &idx);
  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank2> colocate(View<Elem2,Rank2> &idx);

  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank> hcolocate(Block<Elem2,Rank2> &idx);
  template <unsigned long Rank2, typename Elem2>
  View<Elem,Rank> hcolocate(View<Elem2,Rank2> &idx);

  Block<Elem,Rank> ppermute(array<loop_type,Rank> perms);
  View<Elem,Rank> vpermute(array<loop_type,Rank> perms);

  Block<Elem,Rank> prefine(Property<Rank> refine);
  View<Elem,Rank> vrefine(Property<Rank> refine);

  template <typename...Idxs>
  dyn_var<Elem> operator()(Idxs...idxs);

  template <unsigned long Rank2>
  void write(Property<Rank2> idxs, dyn_var<Elem> val);
  
  template <typename Elem2=Elem>
  Block<Elem2,Rank> block();

  void dump();

private:
  
  Properties<Rank> _vlocation;
  Properties<Rank> _blocation;
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> _allocator;

};

////////////////
// BLOCK
////////////////

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Ranges>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::slice(Ranges...ranges) {
  return {this->location().slice(ranges...), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank2> Block<Elem,Rank,MultiDimRepr>::colocate(Block<Elem2,Rank2> &idx) {
  return {this->location().colocate(idx.location()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank2> Block<Elem,Rank,MultiDimRepr>::colocate(View<Elem2,Rank2> &idx) {
  return {this->location().colocate(idx.vlocation()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::hcolocate(Block<Elem2,Rank2> &idx) {
  return {this->location().hcolocate(idx.location()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::hcolocate(View<Elem2,Rank2> &idx) {
  return {this->location().hcolocate(idx.vlocation()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::ppermute(array<loop_type,Rank> perms) {
  return {this->location().permute(perms)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::vpermute(array<loop_type,Rank> perms) {
  return {this->location().permute(perms), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::prefine(Property<Rank> refine) {
  return {this->location().refine(refine)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::vrefine(Property<Rank> refine) {
  return {this->location().refine(refine), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Idxs>
dyn_var<Elem> Block<Elem,Rank,MultiDimRepr>::operator()(Idxs...idxs) {
  constexpr unsigned long s = sizeof...(Idxs);
  static_assert(s <= Rank, "Too many indices specified for read!"); // I can remove this if I drop dimensions. Just too lazy right now.
  Property<s> tmp_idxs{idxs...};
  Property<Rank> padded;
  if constexpr (s < Rank) {
    for (static_var<int> i = 0; i < Rank-s; i=i+1) {
      padded[i] = 0;
    }
  }
  for (static_var<int> i = Rank-s; i < Rank; i=i+1) {
    padded[i] = tmp_idxs[i-(Rank-s)];
  }
  if constexpr (MultiDimRepr) {
    return allocator()->read(padded);
  } else {
    dyn_var<loop_type> lidx = this->location().linearize(padded);
    Property<1> tlidx{lidx};
    return allocator()->read(tlidx);
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2>
void Block<Elem,Rank,MultiDimRepr>::write(Property<Rank2> idxs, dyn_var<Elem> val) {
  constexpr unsigned long s = Rank2;
  static_assert(s <= Rank, "Too many indices specified for write!"); // I can remove this if I drop dimensions. Just too lazy right now.
  Property<Rank> padded;
  if constexpr (s < Rank) {
    for (static_var<int> i = 0; i < Rank-s; i=i+1) {
      padded[i] = 0;
    }
  }
  for (static_var<int> i = Rank-s; i < Rank; i=i+1) {
    padded[i] = idxs[i-(Rank-s)];
  }
  dyn_var<loop_type> lidx = this->location().linearize(padded);
  if constexpr (MultiDimRepr) {
    allocator()->write(val, padded);
  } else {
    dyn_var<loop_type> lidx = this->location().linearize(padded);
    Property<1> tlidx{lidx};
    allocator()->write(val, tlidx);
  } 
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank> Block<Elem,Rank,MultiDimRepr>::view() {
  return {this->location(), this->location()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void Block<Elem,Rank,MultiDimRepr>::dump() {
  print("Block");
  print_newline();
  location().dump();
}

////////////////
// VIEW
////////////////

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Ranges>
View<Elem,Rank> View<Elem,Rank,MultiDimRepr>::slice(Ranges...ranges) {
  return {this->vlocation().slice(ranges...), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank2> View<Elem,Rank,MultiDimRepr>::colocate(Block<Elem2,Rank2> &idx) {
  return {this->vlocation().colocate(idx.location()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank2> View<Elem,Rank,MultiDimRepr>::colocate(View<Elem2,Rank2> &idx) {
  return {this->vlocation().colocate(idx.vlocation()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank> View<Elem,Rank,MultiDimRepr>::hcolocate(Block<Elem2,Rank2> &idx) {
  return {this->vlocation().hcolocate(idx.location()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2, typename Elem2>
View<Elem,Rank> View<Elem,Rank,MultiDimRepr>::hcolocate(View<Elem2,Rank2> &idx) {
  return {this->vlocation().hcolocate(idx.vlocation()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank> View<Elem,Rank,MultiDimRepr>::ppermute(array<loop_type,Rank> perms) {
  return {this->vlocation().permute(perms)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank> View<Elem,Rank,MultiDimRepr>::vpermute(array<loop_type,Rank> perms) {
  return {this->vlocation().permute(perms), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
Block<Elem,Rank> View<Elem,Rank,MultiDimRepr>::prefine(Property<Rank> refine) {
  return {this->vlocation().refine(refine)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
View<Elem,Rank> View<Elem,Rank,MultiDimRepr>::vrefine(Property<Rank> refine) {
  return {this->vlocation().refine(refine), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename...Idxs>
dyn_var<Elem> View<Elem,Rank,MultiDimRepr>::operator()(Idxs...idxs) {
  constexpr unsigned long s = sizeof...(Idxs);
  static_assert(s <= Rank, "Too many indices specified for read!"); // I can remove this if I drop dimensions. Just too lazy right now.
  Property<s> tmp_idxs{idxs...};
  Property<Rank> padded;
  if constexpr (s < Rank) {
    for (static_var<int> i = 0; i < Rank-s; i=i+1) {
      padded[i] = 0;
    }
  }
  for (static_var<int> i = Rank-s; i < Rank; i=i+1) {
    padded[i] = tmp_idxs[i-(Rank-s)];
  }
  if constexpr (MultiDimRepr) {
    Property<Rank> mapped = this->vlocation().
      pointwise_mapping(padded, this->blocation());
    return allocator()->read(mapped);
  } else {
    dyn_var<loop_type> lidx = this->vlocation().
      pointwise_mapping(padded, this->blocation()).
      linearize(padded);
    Property<1> tlidx{lidx};
    return allocator()->read(tlidx);
  }  
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <unsigned long Rank2>
void View<Elem,Rank,MultiDimRepr>::write(Property<Rank2> idxs, dyn_var<Elem> val) {
  constexpr unsigned long s = Rank2;
  static_assert(s <= Rank, "Too many indices specified for write!"); // I can remove this if I drop dimensions. Just too lazy right now.
  Property<Rank> padded;
  if constexpr (s < Rank) {
    for (static_var<int> i = 0; i < Rank-s; i=i+1) {
      padded[i] = 0;
    }
  }
  for (static_var<int> i = Rank-s; i < Rank; i=i+1) {
    padded[i] = idxs[i-(Rank-s)];
  }
  if constexpr (MultiDimRepr) {
    Property<Rank> mapped = this->vlocation().
      pointwise_mapping(padded, this->blocation());
    allocator()->write(val, mapped);
  } else {
    dyn_var<loop_type> lidx = this->vlocation().
      pointwise_mapping(padded, this->blocation()).
      linearize(padded);
    Property<1> tlidx{lidx};
    allocator()->write(val, tlidx);
  }  
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
template <typename Elem2>
Block<Elem2,Rank> View<Elem,Rank,MultiDimRepr>::block() {
  return {this->vlocation()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr>
void View<Elem,Rank,MultiDimRepr>::dump() {
  print("View");
  print_newline();
  vlocation().dump();
  blocation().dump();
}

}
