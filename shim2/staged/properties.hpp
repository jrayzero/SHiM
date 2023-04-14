// -*-c++-*-

#pragma once

#include <array>
#include "builder/dyn_var.h"
#include "builder/array.h"
#include "common/loop_type.hpp"
#include "fwrappers.hpp"
#include "utils.hpp"

using builder::dyn_var;
using builder::dyn_arr;
using builder::static_var;
using namespace std;

namespace shim {

struct Range {
  
  Range(dyn_var<loop_type> start, dyn_var<loop_type> stop, dyn_var<loop_type> stride) :
    _start(start), _stop(stop), _stride(stride) { }

  dyn_var<loop_type> start() { return _start; }
  dyn_var<loop_type> stop() { return _stop; }
  dyn_var<loop_type> stride() { return _stride; }

private:
  dyn_var<loop_type> _start;
  dyn_var<loop_type> _stop;
  dyn_var<loop_type> _stride;
};

template <unsigned long Rank>
struct Properties {

  Properties() {
    for (static_var<int> i = 0; i < Rank; i=i+1) {
      _extents[i] = 1;
      _origin[i] = 0;
      _strides[i] = 1;
      _refinement[i] = 1;
    }
    for (int i = 0; i < Rank; i++) {
      _permutations[i] = i;
    }
  }
  
  Properties(Property<Rank> extents,
	     Property<Rank> origin,
	     Property<Rank> strides,
	     Property<Rank> refinement,
	     array<int,Rank> permutations) : 
    _extents(extents), _origin(origin),
    _strides(strides), _refinement(refinement),
    _permutations(permutations) { 
  }
  
  void dump();

  Property<Rank> extents() { return _extents; }
  Property<Rank> origin() { return _origin; }
  Property<Rank> strides() { return _strides; }
  Property<Rank> refinement() { return _refinement; }
  array<int,Rank> permutations() { return _permutations; }

  Properties<Rank> with_extents(Property<Rank> extents);
  Properties<Rank> with_origin(Property<Rank> origin);
  Properties<Rank> with_strides(Property<Rank> strides);
  Properties<Rank> with_refinement(Property<Rank> refinement);
  Properties<Rank> with_permutations(array<int,Rank> permutations);

  template <typename...Vals>
  Properties<Rank> with_extents(Vals...vals);
  template <typename...Vals>
  Properties<Rank> with_origin(Vals...vals);
  template <typename...Vals>
  Properties<Rank> with_strides(Vals...vals);
  template <typename...Vals>
  Properties<Rank> with_refinement(Vals...vals);
  template <typename...Vals>
  Properties<Rank> with_permutations(Vals...vals);

  Properties<Rank> permute(array<int,Rank> permutations);

  Properties<Rank> refine(Property<Rank> refinement);

  template <typename...Ranges>
  Properties<Rank> slice(Ranges...ranges);

  template <unsigned long Rank2>
  Properties<Rank2> colocate(Properties<Rank2> other);

  template <unsigned long Rank2>
  Properties<Rank> hcolocate(Properties<Rank2> other);

  template <unsigned long Rank2>
  Property<Rank2> pointwise_mapping(Property<Rank> coordinate,
				    Properties<Rank2> into);

  template <unsigned long Rank2>
  Property<Rank2> extent_mapping(Properties<Rank2> into);

  dyn_var<loop_type> linearize(Property<Rank> coordinate);

private:

  Property<Rank> _extents;
  Property<Rank> _origin;
  Property<Rank> _strides;
  Property<Rank> _refinement;
  // This takes the place of static_arr, which isn't a thing.
  // But I don't want to store this in the template signature
  array<loop_type,Rank> _permutations;

};

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::with_extents(Property<Rank> extents) {
  return {extents, _origin, _strides, _refinement, _permutations};
}

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::with_origin(Property<Rank> origin) {
  return {_extents, origin, _strides, _refinement, _permutations};
}

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::with_strides(Property<Rank> strides) {
  return {_extents, _origin, strides, _refinement, _permutations};
}

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::with_refinement(Property<Rank> refinement) {
  return {_extents, _origin, _strides, refinement, _permutations};
}

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::with_permutations(array<loop_type,Rank> properties) {
  return {_extents, _origin, _strides, _refinement, properties};
}

template <unsigned long Rank>
template <typename...Vals>
Properties<Rank> Properties<Rank>::with_extents(Vals...vals) {  
  return {Property<Rank>{vals...}, _origin, _strides, _refinement, _permutations};
}

template <unsigned long Rank>
template <typename...Vals>
Properties<Rank> Properties<Rank>::with_origin(Vals...vals) {
  return {_extents, Property<Rank>{vals...}, _strides, _refinement, _permutations};
}

template <unsigned long Rank>
template <typename...Vals>
Properties<Rank> Properties<Rank>::with_strides(Vals...vals) {
  return {_extents, _origin, Property<Rank>{vals...}, _refinement, _permutations};
}

template <unsigned long Rank>
template <typename...Vals>
Properties<Rank> Properties<Rank>::with_refinement(Vals...vals) {
  return {_extents, _origin, _strides, Property<Rank>{vals...}, _permutations};
}

template <unsigned long Rank>
template <typename...Vals>
Properties<Rank> Properties<Rank>::with_permutations(Vals...vals) {
  return {_extents, _origin, _strides, _refinement, array<loop_type,Rank>{vals...}};
}

template <unsigned long Rank>
void Properties<Rank>::dump() {
  print("Properties");
  print_newline();
  std::string rank = std::to_string(Rank);
  print("  Rank: ");
  print(rank);
  print_newline();
  print("  Extents: ");
  for (static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(_extents[r]);    
  }  
  print_newline();
  print("  Origin: ");
  for (static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(_origin[r]);    
  }  
  print_newline();
  print("  Strides: ");
  for (static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(_strides[r]);
  }  
  print_newline();
  print("  Refinement: ");
  for (static_var<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(_refinement[r]);
  }  
  // This probably will print out a little bit hacky cause if it's within a buildit region,
  // then it might get replayed
  print_newline();
  print("  Permutations: ");
  for (int r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(_permutations[r]);
  }  
  print_newline();
}

// undoes the permutation associated with from
template <unsigned long Rank, int I>
void move_from_into_canonical_space(Property<Rank> &from, Property<Rank> &canon, array<int,Rank> fromp) {
  if constexpr (I < Rank) {
    canon[fromp[I]] = from[I];
    move_from_into_canonical_space<Rank,I+1>(from, canon, fromp);
  }
}

// moves the canonical space into the new permuted space
template <unsigned long Rank, int I>
void move_canonical_into_to_space(Property<Rank> &canon, Property<Rank> &to, array<int,Rank> top) {
  if constexpr (I < Rank) {
    to[I] = canon[top[I]];
    move_canonical_into_to_space<Rank,I+1>(canon, to, top);
  }
}

// Applies a full permutation, i.e. B_i -> B_j
template <unsigned long Rank, int I>
void apply_permutation(Property<Rank> &from, Property<Rank> &to, array<int,Rank> fromp, array<int,Rank> top) {
  Property<Rank> canon;
  move_from_into_canonical_space<Rank,0>(from, canon, fromp);
  move_canonical_into_to_space<Rank,0>(canon, to, top);
} 

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::permute(array<int,Rank> permutations) {
  // need to reorder everything
  Property<Rank> new_extents;
  Property<Rank> new_origin;
  Property<Rank> new_strides;
  Property<Rank> new_refinement;
  apply_permutation<Rank, 0>(_extents, new_extents, this->permutations(), permutations);
  apply_permutation<Rank, 0>(_origin, new_origin, this->permutations(), permutations);
  apply_permutation<Rank, 0>(_strides, new_strides, this->permutations(), permutations);
  apply_permutation<Rank, 0>(_refinement, new_refinement, this->permutations(), permutations);
  return {new_extents, new_origin, new_strides, new_refinement, permutations};
}

template <unsigned long Rank, int I, typename...Ranges>
void gather_one(Property<Rank> &rel_extents, Property<Rank> &rel_origin,
		Property<Rank> &rel_strides, Range range, Ranges...ranges) {
  rel_origin[I] = range.start();
  rel_strides[I] = range.stride();
  dyn_var<loop_type> x = (range.stop()-range.start()-1)/range.stride();
  rel_extents[I] = x + 1;
  if constexpr (sizeof...(Ranges) > 0) {
    gather_one<Rank, I+1>(rel_extents, rel_origin, rel_strides, ranges...);
  }
}

template <unsigned long Rank>
template <typename...Ranges>
Properties<Rank> Properties<Rank>::slice(Ranges...ranges) {
  static_assert(sizeof...(Ranges) == Rank, "Not enough slice parameters specified");
  Property<Rank> new_extents;
  Property<Rank> new_origin;
  Property<Rank> new_strides;
  gather_one<Rank,0>(new_extents, new_origin, new_strides, ranges...);
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    new_origin[i] = new_strides[i] * new_origin[i] + this->_origin[i];
    new_strides[i] = new_strides[i] * this->_strides[i];
  }
  return {new_extents, new_origin, new_strides, this->_refinement, this->_permutations};
}

template <unsigned long Rank>
Properties<Rank> Properties<Rank>::refine(Property<Rank> refinement) {
  Property<Rank> new_extents;
  Property<Rank> new_origin;
  Property<Rank> new_refinement;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    new_extents[i] = _extents[i] * refinement[i];
    new_origin[i] = _origin[i] * refinement[i];
    new_refinement[i] = _refinement[i] * refinement[i];
  }
  return {new_extents, new_origin, _strides, new_refinement, this->_permutations};
}

template <int I, unsigned long Rank>
dyn_var<loop_type> do_linearize(const Property<Rank> &extents, const Property<Rank> &coordinate) {
  dyn_var<loop_type> c = coordinate[Rank-1-I];
  if constexpr (I == Rank - 1) {
    return c;
  } else {
    return c + extents[Rank-1-I] * do_linearize<I+1,Rank>(extents, coordinate);
  }
}

template <unsigned long Rank>
dyn_var<loop_type> Properties<Rank>::linearize(Property<Rank> coordinate) {
  return do_linearize<0, Rank>(_extents, coordinate);
}

template <unsigned long Rank>
template <unsigned long Rank2>
Property<Rank2>
Properties<Rank>::pointwise_mapping(Property<Rank> coordinate,
						     Properties<Rank2> into) {
  // B_i = this, B_j = into
  // move into the refined canonical space of B_i
  // also apply part of the transform into the canonical space where we remove the refinement
  Property<Rank> ri_canon_coordinate;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    ri_canon_coordinate[i] = (coordinate[i] * this->_strides[i] + this->_origin[i]) / this->_refinement[i];
  }
  // move into the canonical space of B_i
  Property<Rank> i_canon_coordinate;
  move_from_into_canonical_space<Rank,0>(ri_canon_coordinate, i_canon_coordinate, this->_permutations);
  // move into the canonical space of B_j
  Property<Rank2> j_canon_coordinate;
  if constexpr (Rank2 >= Rank) {
    // add padding
    for (static_var<int> i = 0; i < Rank2-Rank; i=i+1) {
      j_canon_coordinate[i] = 0;
    }
    for (static_var<int> i = Rank2-Rank; i < Rank2; i=i+1) {
      j_canon_coordinate[i] = i_canon_coordinate[i-(Rank2-Rank)];
    }
  } else if (Rank2 < Rank) {
    // drop dimensions
    for (static_var<int> i = Rank-Rank2; i < Rank; i=i+1) {
      j_canon_coordinate[i-(Rank-Rank2)] = i_canon_coordinate[i];
    }
  }
  // move into the refinement space of B_j (permutations first, then refinement)
  Property<Rank2> j_rcanon_coordinate;
  move_canonical_into_to_space<Rank2,0>(j_canon_coordinate, j_rcanon_coordinate, into.permutations());
  for (static_var<int> i = 0; i < Rank2; i=i+1) {
    j_rcanon_coordinate[i] = j_rcanon_coordinate[i] * into.refinement()[i];
  }
  // move into B_j
  Property<Rank2> j_coordinate;
  for (static_var<int> i = 0; i < Rank2; i=i+1) {
    j_coordinate[i] = (j_rcanon_coordinate[i] - into.origin()[i]) / into.strides()[i];
  }
  return j_coordinate;
}

template <unsigned long Rank>
template <unsigned long Rank2>
Properties<Rank2> 
Properties<Rank>::colocate(Properties<Rank2> other) {
  return {other.extents(), other.origin(), other.strides(), other.refinement(), other.permutations()};
}

template <unsigned long Rank>
template <unsigned long Rank2>
Properties<Rank> 
Properties<Rank>::hcolocate(Properties<Rank2> other) {
  Property<Rank2> zero_origin;
  for (static_var<int> i = 0; i < Rank2; i=i+1) {
    zero_origin[i] = 0;
  }
  Property<Rank> mapped_origin = other.pointwise_mapping(zero_origin, *this);
  for (static_var<int> i = 0; i < Rank; i=i+1) {  
    mapped_origin[i] = mapped_origin[i] * this->strides()[i] + this->origin()[i];
  }
  Property<Rank> mapped_extents = other.extent_mapping(*this);
  return {mapped_extents, mapped_origin, this->strides(), this->refinement(), this->permutations()};
}

template <unsigned long Rank>
template <unsigned long Rank2>
Property<Rank2>
Properties<Rank>::extent_mapping(Properties<Rank2> into) {
  Property<Rank> ri_canon_coordinate;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    ri_canon_coordinate[i] = _extents[i] * this->_strides[i] / this->_refinement[i];
  }
  // move into the canonical space of B_i
  Property<Rank> i_canon_coordinate;
  move_from_into_canonical_space<Rank,0>(ri_canon_coordinate, i_canon_coordinate, this->_permutations);
  // move into the canonical space of B_j
  Property<Rank2> j_canon_coordinate;
  if constexpr (Rank2 >= Rank) {
    // add padding
    for (static_var<int> i = 0; i < Rank2-Rank; i=i+1) {
      j_canon_coordinate[i] = 0;
    }
    for (static_var<int> i = Rank2-Rank; i < Rank2; i=i+1) {
      j_canon_coordinate[i] = i_canon_coordinate[i-(Rank2-Rank)];
    }
  } else if (Rank2 < Rank) {
    // drop dimensions
    for (static_var<int> i = Rank-Rank2; i < Rank; i=i+1) {
      j_canon_coordinate[i-(Rank-Rank2)] = i_canon_coordinate[i];
    }
  }
  // move into the refinement space of B_j (permutations first, then refinement)
  Property<Rank2> j_rcanon_coordinate;
  move_canonical_into_to_space<Rank2,0>(j_canon_coordinate, j_rcanon_coordinate, into.permutations());
  for (static_var<int> i = 0; i < Rank2; i=i+1) {
    j_rcanon_coordinate[i] = j_rcanon_coordinate[i] * into.refinement()[i];
  }
  // subtract twiddle factor and divide by stride, then add back twiddle
  Property<Rank2> j_coordinate;
  for (static_var<int> i = 0; i < Rank2; i=i+1) {
    j_coordinate[i] = (j_rcanon_coordinate[i] - 1) / into.strides()[i];
    j_coordinate[i] = j_coordinate[i] + 1;
  }
  return j_coordinate;
}

}
