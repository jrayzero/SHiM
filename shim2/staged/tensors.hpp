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
#include "expr.hpp"
#include "fwddecls.hpp"
#include "ref.hpp"

using builder::dyn_var;
using builder::dyn_arr;
using builder::static_var;

namespace shim {

// TODO when the shared_ptr gets destroyed for the last time, can insert a delete for the heap allocator
template <typename Elem, int PhysicalRank>
using Allocation_T = std::shared_ptr<Allocation<Elem,PhysicalRank>>;

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
struct Block {

  template <typename BlockLike, typename Idxs>
  friend struct Ref;

  using Elem_T = Elem;
  static constexpr bool IsBlock_T = true;
  static constexpr unsigned long Rank_T = Rank;

  static Block<Elem,Rank,false,false> heap(Properties<Rank> location) { return Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>(location); }

  static Block<Elem,Rank,false,false> stack(Properties<Rank> location) { 
//    return Block<Elem,Rank,MultiDimRepr>(location); 
  }
  
  template <typename External>
  static Block<Elem,Rank,MultiDimRepr,false> external(Properties<Rank> location, dyn_var<External> external) {
    constexpr int nlevels = peel<External>();
    static_assert(nlevels == 1 || nlevels == Rank);
    constexpr bool is_multi = nlevels == Rank;
    if constexpr (is_multi) {
      auto allocator = std::make_shared<UserAllocation<Elem,External,Rank>>(external);
      return Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>(location, allocator);
    } else {
      auto allocator = std::make_shared<UserAllocation<Elem,Elem*,1>>(external);
      return Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>(location, std::move(allocator));
    }
  }

  template <typename External>
  static Block<Elem,Rank,MultiDimRepr,true> non_standard(Properties<Rank> location, dyn_var<External> external, 
							   Property<physical<Rank,peel<External>()>()> non_standard_extents) {
    constexpr int nlevels = peel<External>();
    static_assert(nlevels == 1 || nlevels == Rank);
    constexpr bool is_multi = nlevels == Rank;
    if constexpr (is_multi) {
      auto allocator = std::make_shared<NonStandardAllocation<Elem,External,Rank>>(external);
      return Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>(location, allocator, non_standard_extents);
    } else {
      auto allocator = std::make_shared<NonStandardAllocation<Elem,Elem*,1>>(external);
      return Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>(location, std::move(allocator), non_standard_extents);
    }
  }

  Block(Properties<Rank> location,
	Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
    _location(location), _allocator(allocator) { }

  Block(Properties<Rank> location) : _location(location) { 
    dyn_var<loop_type> sz = 1;
    for (static_var<int> i = 0; i < Rank; i=i+1) {
      sz *= location.extents()[i];
    }
    this->_allocator = std::make_shared<HeapAllocation<Elem>>(sz);
  }

  Block(Properties<Rank> location,
	Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator,
	Property<physical<Rank,MultiDimRepr>()> non_standard_extents) : 
    _location(location), _allocator(allocator),
    _non_standard_extents(non_standard_extents) { }

  Properties<Rank> location() { return _location; }

  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator() { return _allocator; }

  template <typename...Ranges>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> slice(Ranges...ranges);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> colocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> colocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> hcolocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> hcolocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> ppermute(array<loop_type,Rank> perms);

  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> vpermute(array<loop_type,Rank> perms);

  Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> prefine(Property<Rank> refine);

  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> vrefine(Property<Rank> refine);

  template <typename...Idxs>
  dyn_var<Elem> operator()(Idxs...idxs);

  template <unsigned long Rank2>
  void write(Property<Rank2> idxs, dyn_var<Elem> val);

  template <typename Idx>
  Ref<Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>,std::tuple<typename RefIdxType<Idx>::type>> operator[](Idx idx);

  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> view();

  void dump();

  template <typename T=Elem>
  void dump_data();

private:
  
  Properties<Rank> _location;

  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> _allocator;

  Property<physical<Rank,MultiDimRepr>()> _non_standard_extents;

};

template <typename Elem, unsigned long Rank>
Block<Elem,Rank,false,false> heap(Properties<Rank> prop) {
  return Block<Elem,Rank,false,false>::heap(prop);
}

template <typename Elem, unsigned long Rank>
Block<Elem,Rank,false,false> stack(Properties<Rank> prop) {

}

template <unsigned long Rank, typename External>
Block<typename GetCoreT<External>::Core_T,Rank,peel<External>()!=1,false> external(Properties<Rank> prop, 
										   dyn_var<External> external) {
  return Block<typename GetCoreT<External>::Core_T,Rank,peel<External>()!=1,false>::external(prop, external);
}

template <unsigned long Rank, typename External>
Block<typename GetCoreT<External>::Core_T,Rank,peel<External>()!=1,true> 
non_standard(Properties<Rank> prop, dyn_var<External> external, 
	     Property<physical<Rank,peel<External>()>()> non_standard_extents) {
  return Block<typename GetCoreT<External>::Core_T,Rank,peel<External>()!=1,true>::non_standard(prop, external, non_standard_extents);
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
struct View {

  template <typename BlockLike, typename Idxs>
  friend struct Ref;

  using Elem_T = Elem;
  static constexpr bool IsBlock_T = false;
  static constexpr unsigned long Rank_T = Rank;

  View(Properties<Rank> vlocation, Properties<Rank> blocation,
       Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator) : 
    _vlocation(vlocation), _blocation(blocation), _allocator(allocator) { }

  Properties<Rank> vlocation() { return _vlocation; }

  Properties<Rank> blocation() { return _blocation; }

  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> allocator() { return _allocator; }

  template <typename...Ranges>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> slice(Ranges...ranges);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> colocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> colocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> hcolocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> hcolocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx);

  Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> ppermute(array<loop_type,Rank> perms);

  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> vpermute(array<loop_type,Rank> perms);

  Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> prefine(Property<Rank> refine);
  View<Elem,Rank,MultiDimRepr,NonStandardAlloc> vrefine(Property<Rank> refine);

  template <typename...Idxs>
  dyn_var<Elem> operator()(Idxs...idxs);

  template <unsigned long Rank2>
  void write(Property<Rank2> idxs, dyn_var<Elem> val);

  template <typename Idx>
  auto operator[](Idx idx);
  
  template <typename Elem2=Elem>
  Block<Elem2,Rank,false,false> heap();

  void dump();

  template <typename T=Elem>
  void dump_data();

private:
  
  Properties<Rank> _vlocation;
  Properties<Rank> _blocation;
  Allocation_T<Elem,physical<Rank,MultiDimRepr>()> _allocator;

};

////////////////
// BLOCK
////////////////

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename...Ranges>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::slice(Ranges...ranges) {
  return {this->location().slice(ranges...), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> 
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::colocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->location().colocate(idx.location()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> 
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::colocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->location().colocate(idx.vlocation()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> 
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::hcolocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->location().hcolocate(idx.location()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> 
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::hcolocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->location().hcolocate(idx.vlocation()), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::ppermute(array<loop_type,Rank> perms) {
  return {this->location().permute(perms)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::vpermute(array<loop_type,Rank> perms) {
  return {this->location().permute(perms), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::prefine(Property<Rank> refine) {
  return {this->location().refine(refine)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::vrefine(Property<Rank> refine) {
  return {this->location().refine(refine), this->location(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename...Idxs>
dyn_var<Elem> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::operator()(Idxs...idxs) {
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2>
void Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::write(Property<Rank2> idxs, dyn_var<Elem> val) {
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename Idx>
Ref<Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>,std::tuple<typename RefIdxType<Idx>::type>> 
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    dyn_var<loop_type> didx = idx;
    return Ref<Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    return Ref<Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>,std::tuple<Idx>>(*this, std::tuple{idx});
  }
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::view() {
  return {this->location(), this->location()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
void Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::dump() {
  print("Block");
  print_newline();
  location().dump();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename T>
void Block<Elem,Rank,MultiDimRepr,NonStandardAlloc>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports ranks 1, 2, and 3");
  if constexpr (Rank == 1) {
    for (dyn_var<loop_type> i = 0; i < location().extents()[0]; i=i+1) {
      dispatch_print_elem<T>(cast<T>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (dyn_var<loop_type> i = 0; i < location().extents()[0]; i=i+1) {
      for (dyn_var<loop_type> j = 0; j < location().extents()[1]; j=j+1) {
	dispatch_print_elem<T>(cast<T>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (dyn_var<loop_type> i = 0; i < location().extents()[0]; i=i+1) {
      for (dyn_var<loop_type> j = 0; j < location().extents()[1]; j=j+1) {
	for (dyn_var<loop_type> k = 0; k < location().extents()[2]; k=k+1) {
	  dispatch_print_elem<T>(cast<T>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();
    }
  }
}

////////////////
// VIEW
////////////////

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename...Ranges>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::slice(Ranges...ranges) {
  return {this->vlocation().slice(ranges...), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> 
View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::colocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->vlocation().colocate(idx.location()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank2,MultiDimRepr,NonStandardAlloc> 
View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::colocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->vlocation().colocate(idx.vlocation()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> 
View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::hcolocate(Block<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->vlocation().hcolocate(idx.location()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2, typename Elem2, bool MultiDimRepr2, bool NonStandardAlloc2>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> 
View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::hcolocate(View<Elem2,Rank2,MultiDimRepr2,NonStandardAlloc2> &idx) {
  return {this->vlocation().hcolocate(idx.vlocation()), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::ppermute(array<loop_type,Rank> perms) {
  return {this->vlocation().permute(perms)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::vpermute(array<loop_type,Rank> perms) {
  return {this->vlocation().permute(perms), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
Block<Elem,Rank,MultiDimRepr,NonStandardAlloc> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::prefine(Property<Rank> refine) {
  return {this->vlocation().refine(refine)};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
View<Elem,Rank,MultiDimRepr,NonStandardAlloc> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::vrefine(Property<Rank> refine) {
  return {this->vlocation().refine(refine), this->blocation(), this->allocator()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename...Idxs>
dyn_var<Elem> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::operator()(Idxs...idxs) {
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <unsigned long Rank2>
void View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::write(Property<Rank2> idxs, dyn_var<Elem> val) {
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

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename Idx>
auto View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::operator[](Idx idx) {
  if constexpr (is_dyn_like<Idx>::value) {
    // potentially slice builder::builder to dyn_var 
    dyn_var<loop_type> didx = idx;
    // include any frozen dims here 
    return Ref<View<Elem,Rank,MultiDimRepr>,std::tuple<decltype(didx)>>(*this, std::tuple{didx});
  } else {
    // include any frozen dims here
    return Ref<View<Elem,Rank,MultiDimRepr>,std::tuple<decltype(idx)>>(*this, std::tuple{idx});
  }
}


template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename Elem2>
Block<Elem2,Rank,false,false> View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::heap() {
  return {this->vlocation()};
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
void View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::dump() {
  print("View");
  print_newline();
  vlocation().dump();
  blocation().dump();
}

template <typename Elem, unsigned long Rank, bool MultiDimRepr, bool NonStandardAlloc>
template <typename T>
void View<Elem,Rank,MultiDimRepr,NonStandardAlloc>::dump_data() {
  // TODO format nicely with the max string length thing
  static_assert(Rank<=3, "dump_data only supports up to 3 rank dimensions for printing.");
  if constexpr (Rank == 1) {
    for (dyn_var<loop_type> i = 0; i < vlocation().extents()[0]; i=i+1) {
      dispatch_print_elem<T>(cast<T>(this->operator()(i)));
    }
  } else if constexpr (Rank == 2) {
    for (dyn_var<loop_type> i = 0; i < vlocation().extents()[0]; i=i+1) {
      for (dyn_var<loop_type> j = 0; j < vlocation().extents()[1]; j=j+1) {
	dispatch_print_elem<T>(cast<T>(this->operator()(i,j)));
      }
      print_newline();
    }
  } else {
    for (dyn_var<loop_type> i = 0; i < vlocation().extents()[0]; i=i+1) {
      for (dyn_var<loop_type> j = 0; j < vlocation().extents()[1]; j=j+1) {
	for (dyn_var<loop_type> k = 0; k < vlocation().extents()[2]; k=k+1) {
	  dispatch_print_elem<T>(cast<T>(this->operator()(i,j,k)));
	}
	print_newline();
      }
      print_newline();
      print_newline();

    }
  }
}

}
