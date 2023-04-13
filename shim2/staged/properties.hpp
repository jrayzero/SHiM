// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "common/loop_type.hpp"
#include "fwrappers.hpp"
#include "utils.hpp"

using builder::dyn_var;
using builder::dyn_arr;
using builder::static_var;

namespace shim {

template <unsigned long Rank, int...Permutations>
struct Properties {

  Properties() {
    for (static_var<int> i = 0; i < Rank; i=i+1) {
      _extents[i] = 1;
      _origin[i] = 0;
      _strides[i] = 1;
      _refinement[i] = 1;
    }
  }
  
  Properties(Property<Rank> extents,
	     Property<Rank> origin,
	     Property<Rank> strides,
	     Property<Rank> refinement) : 
    _extents(extents), _origin(origin),
    _strides(strides), _refinement(refinement) { 
    static_assert(sizeof...(Permutations) == Rank);
  }
  
  void dump();

  Property<Rank> extents() { return _extents; }
  Property<Rank> origin() { return _origin; }
  Property<Rank> strides() { return _strides; }
  Property<Rank> refinement() { return _refinement; }

  Properties<Rank, Permutations...> with_extents(Property<Rank> extents);
  Properties<Rank, Permutations...> with_origin(Property<Rank> origin);
  Properties<Rank, Permutations...> with_strides(Property<Rank> strides);
  Properties<Rank, Permutations...> with_refinement(Property<Rank> refinement);

  template <int...NewPermutations>
  Properties<Rank, NewPermutations...> permute();

  Properties<Rank, Permutations...> refine(Property<Rank> refinement);

  dyn_var<loop_type> linearize(Property<Rank> coordinate);

  template <unsigned long Rank2, int...NewPermutations>
  Property<Rank2> pointwise_mapping(Property<Rank> coordinate,
				    Properties<Rank2, NewPermutations...> into);

private:

  template <int Perm, int...ThesePermutations>
  void print_permutations();
  
  Property<Rank> _extents;
  Property<Rank> _origin;
  Property<Rank> _strides;
  Property<Rank> _refinement;

};

template <unsigned long Rank, int...Permutations>
Properties<Rank, Permutations...> Properties<Rank, Permutations...>::with_extents(Property<Rank> extents) {
  return {extents, _origin, _strides, _refinement};
}

template <unsigned long Rank, int...Permutations>
Properties<Rank, Permutations...> Properties<Rank, Permutations...>::with_origin(Property<Rank> origin) {
  return {_extents, origin, _strides, _refinement};
}

template <unsigned long Rank, int...Permutations>
Properties<Rank, Permutations...> Properties<Rank, Permutations...>::with_strides(Property<Rank> strides) {
  return {_extents, _origin, strides, _refinement};
}

template <unsigned long Rank, int...Permutations>
Properties<Rank, Permutations...> Properties<Rank, Permutations...>::with_refinement(Property<Rank> refinement) {
  return {_extents, _origin, _strides, refinement};
}

template <unsigned long Rank, int...Permutations>
template <int Perm, int...ThesePermutations>
void Properties<Rank, Permutations...>::print_permutations() {
  print(" ");
  dispatch_print_elem<int>(Perm);
  if constexpr (sizeof...(ThesePermutations) > 0) {
    print_permutations<ThesePermutations...>();
  }
}

template <unsigned long Rank, int...Permutations>
void Properties<Rank, Permutations...>::dump() {
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
  print_newline();
  print("  Permutations: ");
  print_permutations<Permutations...>();
  print_newline();
}

template <int...Perms>
struct PermWrapper { 

  template <int At>
  static constexpr int get() {
    static_assert(At < sizeof...(Perms));
    return get<At,0,Perms...>();
  }

private:

  template <int At, int I, int P, int...Ps> 
  static constexpr int get() {
    if constexpr (At == I) { 
      return P; 
    } else { 
      static_assert(sizeof...(Ps) > 0);
      return get<At,I+1,Ps...>(); 
    }
  }

};

// undoes the permutation associated with from
template <unsigned long Rank, int I, typename From>
void move_from_into_canonical_space(Property<Rank> &from, Property<Rank> &canon) {
  if constexpr (I < Rank) {
    canon[From::template get<I>()] = from[I];
    move_from_into_canonical_space<Rank,I+1,From>(from, canon);
  }
}

// moves the canonical space into the new permuted space
template <unsigned long Rank, int I, typename To>
void move_canonical_into_to_space(Property<Rank> &canon, Property<Rank> &to) {
  if constexpr (I < Rank) {
    to[I] = canon[To::template get<I>()];
    move_canonical_into_to_space<Rank,I+1,To>(canon, to);
  }
}

// Applies a full permutation, i.e. B_i -> B_j
template <unsigned long Rank, int I, typename From, typename To>
void apply_permutation(Property<Rank> &from, Property<Rank> &to) {
  Property<Rank> canon;
  move_from_into_canonical_space<Rank,0,From>(from, canon);
  move_canonical_into_to_space<Rank,0,To>(canon, to);
} 

template <unsigned long Rank, int...Permutations>
template <int...NewPermutations>
Properties<Rank, NewPermutations...> Properties<Rank, Permutations...>::permute() {
  // need to reorder everything
  Property<Rank> new_extents;
  Property<Rank> new_origin;
  Property<Rank> new_strides;
  Property<Rank> new_refinement;
  apply_permutation<Rank, 0, PermWrapper<Permutations...>, PermWrapper<NewPermutations...>>(_extents, new_extents);
  apply_permutation<Rank, 0, PermWrapper<Permutations...>, PermWrapper<NewPermutations...>>(_origin, new_origin);
  apply_permutation<Rank, 0, PermWrapper<Permutations...>, PermWrapper<NewPermutations...>>(_strides, new_strides);
  apply_permutation<Rank, 0, PermWrapper<Permutations...>, PermWrapper<NewPermutations...>>(_refinement, new_refinement);
  return {new_extents, new_origin, new_strides, new_refinement};
}

template <unsigned long Rank, int...Permutations>
Properties<Rank, Permutations...> Properties<Rank, Permutations...>::refine(Property<Rank> refinement) {
  Property<Rank> new_extents;
  Property<Rank> new_origin;
  Property<Rank> new_refinement;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    new_extents[i] = _extents[i] * refinement[i];
    new_origin[i] = _origin[i] * refinement[i];
    new_refinement[i] = _refinement[i] * refinement[i];
  }
  return {new_extents, new_origin, _strides, new_refinement};
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

template <unsigned long Rank, int...Permutations>
dyn_var<loop_type> Properties<Rank, Permutations...>::linearize(Property<Rank> coordinate) {
  return do_linearize<0, Rank>(_extents, coordinate);
}

template <unsigned long Rank, int...Permutations>
template <unsigned long Rank2, int...NewPermutations>
Property<Rank2>
Properties<Rank, Permutations...>::pointwise_mapping(Property<Rank> coordinate,
						     Properties<Rank2, NewPermutations...> into) {
  // B_i = this, B_j = into
  // move into the refined canonical space of B_i
  // also apply part of the transform into the canonical space where we remove the refinement
  Property<Rank> ri_canon_coordinate;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    ri_canon_coordinate[i] = (coordinate[i] * this->_strides[i] + this->_origin[i]) / this->_refinement[i];
  }
  // move into the canonical space of B_i
  Property<Rank> i_canon_coordinate;
  move_from_into_canonical_space<Rank,0,PermWrapper<Permutations...>>(ri_canon_coordinate, i_canon_coordinate);
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
  move_canonical_into_to_space<Rank2,0,PermWrapper<NewPermutations...>>(j_canon_coordinate, j_rcanon_coordinate);
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

// Need for colocation
/*
  // move into the refined canonical space of B_i
  Property<Rank> ri_canon_extents;
  Property<Rank> ri_canon_origin;
  Property<Rank> ri_canon_strides;
  Property<Rank> ri_canon_refinement;
  for (static_var<int> i = 0; i < Rank; i=i+1) {
    
  }
  // move into the canonical space of B_i
  Property<Rank> canon_extents;
  Property<Rank> canon_origin;
  Property<Rank> canon_strides;
  Property<Rank> canon_refinement;
  move_from_into_canonical_space<Rank,0,PermWrapper<Permutations...>>(ri_canon_extents, canon_extents);
  move_from_into_canonical_space<Rank,0,PermWrapper<Permutations...>>(ri_canon_origin, canon_origin);
  move_from_into_canonical_space<Rank,0,PermWrapper<Permutations...>>(ri_canon_strides, canon_strides);
  move_from_into_canonical_space<Rank,0,PermWrapper<Permutations...>>(ri_canon_refinement, canon_refinement);
*/

}
