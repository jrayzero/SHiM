// -*-c++-*-

#pragma once

#include <array>
#include <sstream>
#include <iostream>
#include "heaparray.h"

namespace shim {
namespace unstaged {

#define T std::tuple

template <typename Elem, unsigned long Rank>
struct Block;

template <typename Elem, unsigned long Rank>
struct View;

/*template <int I, typename...TupTypes>
void join(std::stringstream &ss, std::tuple<TupTypes...> &tup) {
  if constexpr (I < sizeof...(TupTypes)) {
    if (I > 0) ss << ", ";
    ss << std::get<I>(tup);
    join<I+1>(ss,tup);
  } else { }
}

template <typename...TupTypes>
std::string join(std::tuple<TupTypes...> &tup) {
  std::stringstream ss;
  join<0>(ss, tup);
  return ss.str();
}
*/

/*template <int I, int Rank>
void join(std::stringstream &ss, std::array<int,Rank> &tup) {
  if constexpr (I < Rank) {
    if (I > 0) ss << ", ";
    ss << std::get<I>(tup);
    join<I+1>(ss,tup);
  } else { }
}*/

template <unsigned long Rank>
std::string join(const std::array<int,Rank> &tup) {
  std::stringstream ss;
  for (int i = 0; i < Rank; i++) {
    if (i > 0) ss << ", ";
    ss << tup[i];
  }
  return ss.str();
}

template <unsigned long Rank>
using Property_T = std::array<int,Rank>;

template <unsigned long Rank>
struct BlockedCS {

  BlockedCS(Property_T<Rank> strides, Property_T<Rank> refinement,
	    Property_T<Rank> permutations, Property_T<Rank> origin) :
  strides(std::move(strides)), refinement(std::move(refinement)),
  permutations(std::move(permutations)), origin(std::move(origin)) { }

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping(const Property_T<Rank> &other, int pad_val) const {
    if constexpr (Rank == ToRank) {
      return other;
    } else {
      Property_T<ToRank> rank_mapped;
      rank_mapping<ToRank>(other, pad_val, rank_mapped);
      return rank_mapped;
    }
  }

  template <unsigned long ToRank>
  void rank_mapping(const Property_T<Rank> &other, int pad_val,
		    Property_T<Rank> &rank_mapped) const {
    if constexpr (Rank == ToRank) {
      for (int i = 0; i < Rank; i++) {
	rank_mapped[i] = other[i];
      }
    } else {
      if constexpr (ToRank < Rank) {
	// drop
	for (int i = (Rank-ToRank); i < ToRank; i++) {
	  rank_mapped[i - (Rank-ToRank)] = other[i];
	}
      } else {
	// pad
	for (int i = 0; i < ToRank-Rank; i++) {
	  rank_mapped[i] = pad_val;
	}
	for (int i = 0; i < Rank; i++) {
	  rank_mapped[i+ToRank-Rank] = other[i];
	}
      }
    } 
  }

/*  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_strides() const {
    return rank_mapping<ToRank>(strides, 1);
 } 

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_refinement() const {
    return rank_mapping<ToRank>(refinement, 1);
  }

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_permutation() const {
    if constexpr (ToRank == Rank) {
      return permutations;
    } else {
      // PADDING
      // This applies padding before the 0 index and then adjusts all the others accordingly
      // ex: perm = 2,0,1 and you need to pad by 1 -> perm = 3,0,1,2
      // DROPPING
      // This drops the perms in order
      // ex: perm = 3,0,1,2 and you need to drop 2 -> perm = 1,0 (dropped 0, 1)    
      Property_T<ToRank> rank_mapped;
      if constexpr (ToRank < Rank) {
	constexpr int to_drop = ToRank - Rank;
	int i = 0;
	for (int j = 0; j < Rank; j++) {
	  if (permutations[j] >= to_drop) {
	    rank_mapped[i++] = permutations[j] - to_drop;
	  }
	}
      } else {
	constexpr int to_pad = Rank - ToRank;
	int i = 0;
	for (int j = 0; j < Rank; j++) {
	  if (permutations[j] == 0) {
	    // pad to the left of this
	    for (int k = 0; k < to_pad; k++) {
	      rank_mapped[i+k] = k;
	    }
	    i += to_pad;
	    rank_mapped[i++] += to_pad;
	  } else {
	    rank_mapped[i++] = permutations[j] + to_pad;
	  }
	}
      }
      return rank_mapped;
    }    
  }

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_origin() const {
    return rank_mapping<ToRank>(origin, 0);
  }*/
 
  Property_T<Rank> permutation_mapping(const Property_T<Rank> &other, const Property_T<Rank> &perm_idxs) const {
    Property_T<Rank> permuted;
    int i = 0;
    for (auto p : perm_idxs) {
      
      permuted[i++] = other[p];
    }
    return permuted;
  } 

  void permutation_mapping(const Property_T<Rank> &other, const Property_T<Rank> &perm_idxs,
			   Property_T<Rank> &permuted) const {
    int i = 0;
    for (auto p : perm_idxs) {
      permuted[i++] = other[p];
    }
  }

/*  Property_T<Rank> permutation_mapping_strides(const Property_T<Rank> &perm_idxs) const {
    return permutation_mapping(strides, perm_idxs);
  } 

  Property_T<Rank> permutation_mapping_refinement(const Property_T<Rank> &perm_idxs) const {
    return permutation_mapping(refinement, perm_idxs);
  } 

  Property_T<Rank> permutation_mapping_permutation(const Property_T<Rank> &perm_idxs) const {
    return permutation_mapping(permutations, perm_idxs);
  } 

  Property_T<Rank> permutation_mapping_origin(const Property_T<Rank> &perm_idxs) const {
    return permutation_mapping(origin, perm_idxs);
  } */
  
  template <unsigned long ToRank, typename...Point>
  Property_T<ToRank> point_mapping(const BlockedCS<ToRank> &to, Property_T<Rank> &point) const {
    Property_T<Rank> &Bi = point;
    Property_T<Rank> perms;
    for (int i = 0; i < Rank; i++) perms[i] = i;
    // Bi => Ri => Ci
    for (int i = 0; i < Rank; i++) {
      Bi[i] *= strides[i];
      Bi[i] += origin[i];
      Bi[i] /= refinement[i];
    }
    Property_T<Rank> Ri = permutation_mapping(Bi, perms);
    // Ci => Cj
    rank_mapping<ToRank>(Ri, 0, Ri);
    // Cj => Rj
    Property_T<ToRank> Rj = permutation_mapping(Ri, to.permutations);
    for (int i = 0; i < ToRank; i++) {
      Rj[i] *= to.refinement[i];
    }
    // Rj => Bj
    for (int i = 0; i < ToRank; i++) {
      Rj[i] -= to.origin[i];
      Rj[i] /= to.strides[i];
    }
    return Rj;
  }


  std::string dump() const {
    std::stringstream ss;   
    ss << "strides: " << join(strides) << std::endl; 
    ss << "refinement: " << join(refinement) << std::endl; 
    ss << "permutations: " << join(permutations) << std::endl; 
    ss << "origin: " << join(origin) << std::endl; 
    return ss.str();
  }


  Property_T<Rank> strides;
  Property_T<Rank> refinement;
  Property_T<Rank> permutations;
  Property_T<Rank> origin;
  
  template <unsigned long Rank2, int I, typename V, typename...Vs>
  void multiply(Property_T<Rank2> &out, const Property_T<Rank2> &obj, V v, Vs...vs) const {
    out[I] = v * std::get<I>(obj);
    if constexpr (sizeof...(Vs) > 0) {
      multiply<Rank2,I+1>(out, obj, vs...);
    }
  }

};

// Mutable builder
template <unsigned long Rank>
struct BlockedCSBuilder {

  BlockedCSBuilder() { 
    for (int i = 0; i < Rank; i++) {
      strides[i] = 1;
      refinement[i] = 1;
      permutations[i] = i;
      origin[i] = 0;
    }
  }

/*  template <typename...Strides>
  BlockedCSBuilder &with_strides(Strides...strides) {
    fill<0>(this->strides, strides...);
    return *this;
  };

  template <typename...Refinement>
  BlockedCSBuilder &with_refinement(Refinement...refinement) {
    fill<0>(this->refinement, refinement...);
    return *this;
  };

  template <typename...Permutations>
  BlockedCSBuilder &with_permutations(Permutations...permutations) {
    fill<0>(this->permutation, permutations...);
    return *this;
  };

  template <typename...Origin>
  BlockedCSBuilder &with_origin(Origin...origin) {
    fill<0>(this->origin, origin...);
    return *this;
  };
*/
  BlockedCSBuilder with_strides(Property_T<Rank> strides) {
    this->strides = std::move(strides);
    return *this;
  }

  BlockedCSBuilder with_refinement(Property_T<Rank> refinement) {
    this->refinement = std::move(refinement);
    return *this;
  }

  BlockedCSBuilder with_permutations(Property_T<Rank> permutations) {
    this->permutations = std::move(permutations);
    return *this;
  }

  BlockedCSBuilder with_origin(Property_T<Rank> origin) {
    this->origin = std::move(origin);
    return *this;
  }

  BlockedCS<Rank> to_blocked_cs() const {
    return {std::move(strides), std::move(refinement),
      std::move(permutations), std::move(origin)};
  }

  std::string dump() const {
    std::stringstream ss;   
    ss << "strides: " << join(strides) << std::endl; 
    ss << "refinement: " << join(refinement) << std::endl; 
    ss << "permutations: " << join(permutations) << std::endl; 
    ss << "origin: " << join(origin) << std::endl; 
    return ss.str();
  }

private:

  Property_T<Rank> strides;
  Property_T<Rank> refinement;
  Property_T<Rank> permutations;
  Property_T<Rank> origin;  

  template <int I, typename V, typename...Vs>
  void fill(Property_T<Rank> &to_fill, V v, Vs...vs) {
    to_fill[I] = v;
    if constexpr (sizeof...(Vs) > 0) {
      fill<I+1>(to_fill, vs...);
    }
  }

};

template <unsigned long Rank>
struct Tensor {

  Tensor(BlockedCS<Rank> blocked_cs, Property_T<Rank> extents) :
    blocked_cs(std::move(blocked_cs)),
    extents(std::move(extents)) { }

  template <int I=0>
  int length() const {
    if constexpr (I < Rank) {
      return std::get<I>(extents) * length<I+1>();
    } else {
      return 1;
    }
  }
  
  template <int I, typename Idxs>
  int linearize(const Idxs &idxs) const {
    if constexpr (I == Rank - 1) {
      return std::get<Rank-1-I>(idxs);
    } else {
      return std::get<Rank-1-I>(idxs) + 
	std::get<Rank-1-I>(extents) * 
	linearize<I+1>(idxs);      
    }
  }

/*  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_extents() const {
    return blocked_cs.template rank_mapping<ToRank>(extents, 1);
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_strides() const {
    return blocked_cs.template rank_mapping_strides<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_refinement() const {
    return blocked_cs.template rank_mapping_refinement<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_permutations() const {
    return blocked_cs.template rank_mapping_permutations<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_origin() const {
    return blocked_cs.template rank_mapping_origin<ToRank>();
  }  

  Property_T<Rank> permutation_mapping_extents(const Property_T<Rank> &perms) const {
    return blocked_cs.template permutation_mapping(extents, perms);
  }  

  Property_T<Rank> permutation_mapping_strides(const Property_T<Rank> &perms) const {
    return blocked_cs.template permutation_mapping_strides(perms);
  }  

  Property_T<Rank> permutation_mapping_refinement(const Property_T<Rank> &perms) const {
    return blocked_cs.template permutation_mapping_refinement(perms);
  }  

  Property_T<Rank> permutation_mapping_permutations(const Property_T<Rank> &perms) const {
    return blocked_cs.template permutation_mapping_permutations(perms);
  }  

  Property_T<Rank> permutation_mapping_origin(const Property_T<Rank> &perms) const {
    return blocked_cs.template permutation_mapping_origin(perms);
  }*/
  
  template <int Idx>
  int extent_at() const {
    return std::get<Idx>(extents);
  }

  std::string dump() const {
    std::stringstream ss;
    ss << blocked_cs.dump();
    ss << "extents: " << join(extents) << std::endl; 
    return ss.str();
  }

  BlockedCS<Rank> blocked_cs;

  Property_T<Rank> extents;

};

template <unsigned long Rank>
struct TensorBuilder {

  TensorBuilder() {
    for (int i = 0; i < Rank; i++) {
      extents[i] = 1;
    }
  }

  template <int I, typename V, typename...Vs>
  void fill(Property_T<Rank> &to_fill, V v, Vs...vs) {
    to_fill[I] = v;
    if constexpr (sizeof...(Vs) > 0) {
      fill<I+1>(to_fill, vs...);
    }
  }

/*  template <typename...Strides>
  TensorBuilder &with_strides(Strides...strides) {
    bcsb.with_strides(strides...);
    return *this;
  };

  template <typename...Refinement>
  TensorBuilder &with_refinement(Refinement...refinement) {
    bcsb.with_refinement(refinement...);
    return *this;
  };

  template <typename...Permutations>
  TensorBuilder &with_permutations(Permutations...permutations) {
    bcsb.with_permutations(permutations...);
    return *this;
  };

  template <typename...Origin>
  TensorBuilder &with_origin(Origin...origin) {
    bcsb.with_origin(origin...);
    return *this;
  };

  template <typename...Extents>
  TensorBuilder &with_extents(Extents...extents) {
    fill<0>(this->extents, extents...);
    return *this;
  };*/
  
  TensorBuilder &with_strides(Property_T<Rank> strides) {
    bcsb.with_strides(std::move(strides));
    return *this;
  };

  TensorBuilder &with_refinement(Property_T<Rank> refinement) {
    bcsb.with_refinement(std::move(refinement));
    return *this;
  };

  TensorBuilder &with_permutations(Property_T<Rank> permutations) {
    bcsb.with_permutations(std::move(permutations));
    return *this;
  };

  TensorBuilder &with_origin(Property_T<Rank> origin) {
    bcsb.with_origin(std::move(origin));
    return *this;
  };

  TensorBuilder &with_extents(Property_T<Rank> extents) {
    this->extents = std::move(extents);
    return *this;
  };

  Tensor<Rank> to_tensor() const {
    return {bcsb.to_blocked_cs(), std::move(extents)};
  }

  template <typename Elem>
  Block<Elem,Rank> to_block() const; 

  std::string dump() const {
    std::stringstream ss;
    ss << bcsb.dump();
    ss << "extents: " << join(extents) << std::endl; 
    return ss.str();
  }

  BlockedCSBuilder<Rank> bcsb;

  Property_T<Rank> extents;

};

template <typename Elem, unsigned long Rank>
struct Block {

  Block(Tensor<Rank> tensor) : 
    tensor(std::move(tensor)),
    heaparr(this->tensor.length()) { }

  std::string dump() const {
    std::stringstream ss;
    ss << "Block[" << Rank << "]" << std::endl;
    ss << tensor.dump();
    if constexpr (Rank == 2) {
      ss << std::endl;
      for (int i = 0; i < tensor.template extent_at<0>(); i++) {
	for (int j = 0; j < tensor.template extent_at<1>(); j++) {
	  ss << this->read(i,j) << " ";
	}
	ss << std::endl;
      }
      ss << std::endl;
    }
    return ss.str();
  }
  
  template <typename...Idxs>
  Elem read(Idxs...idxs) const {
    Property_T<Rank> t{idxs...};
    int idx = tensor.template linearize<0>(t);
    return heaparr[idx];
  }

  Elem read(Property_T<Rank> idxs) const {
    int idx = tensor.template linearize<0>(idxs);
    return heaparr[idx];
  }

  void write(Property_T<Rank> idxs, Elem elem) {
    int idx = tensor.template linearize<0>(idxs);
    heaparr[idx] = elem;
  }

  template <typename...PVals>
  View<Elem,Rank> partition(std::tuple<int,int,int> pvals, PVals...p);

  template <int I, typename...PVals>
  void partition(Property_T<Rank> &starts,
		 Property_T<Rank> &stops,
		 Property_T<Rank> &strides,
		 std::tuple<int,int,int> pvals, PVals...p) {
    starts[I] = std::get<0>(pvals);
    stops[I] = std::get<1>(pvals);
    strides[I] = std::get<2>(pvals);
    if constexpr (sizeof...(PVals) > 0) {
      partition<I+1>(starts, stops, strides, p...);
    }
  };

/*  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_extents() const {
    return tensor.template rank_mapping<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_strides() const {
    return tensor.template rank_mapping_strides<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_refinement() const {
    return tensor.template rank_mapping_refinement<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_permutations() const {
    return tensor.template rank_mapping_permutations<ToRank>();
  }  

  template <unsigned long ToRank>
  Property_T<ToRank> rank_mapping_origin() const {
    return tensor.template rank_mapping_origin<ToRank>();
  }  

  Property_T<Rank> permutation_mapping_extents(const Property_T<Rank> &perms) const {
    return tensor.template permutation_mapping_extents(perms);
  }  

  Property_T<Rank> permutation_mapping_strides(const Property_T<Rank> &perms) const {
    return tensor.template permutation_mapping_strides(perms);
  }  

  Property_T<Rank> permutation_mapping_refinement(const Property_T<Rank> &perms) const {
    return tensor.template permutation_mapping_refinement(perms);
  }  

  Property_T<Rank> permutation_mapping_permutations(const Property_T<Rank> &perms) const {
    return tensor.template permutation_mapping_permutations(perms);
  }  

  Property_T<Rank> permutation_mapping_origin(const Property_T<Rank> &perms) const {
    return tensor.template permutation_mapping_origin(perms);
  } */

  View<Elem,Rank> view();
    
  Tensor<Rank> tensor;

  HeapArray<Elem> heaparr;

};

template <typename Elem, unsigned long Rank>
struct View {

  View(Tensor<Rank> tensor, Tensor<Rank> btensor,
       HeapArray<Elem> heaparr) :
    tensor(std::move(tensor)), btensor(std::move(btensor)),
    heaparr(std::move(heaparr)) { }

  template <typename...Idxs>
  Elem read(Idxs...idxs) const {
    Property_T<Rank> pidxs{idxs...};
    auto coord = tensor.blocked_cs.point_mapping(btensor.blocked_cs, pidxs);    
    int idx = tensor.template linearize<0>(coord);
    return heaparr[idx];
  }

  void write(Property_T<Rank> idxs, Elem elem) {
    auto coord = tensor.blocked_cs.point_mapping(btensor.blocked_cs, idxs);
    int idx = tensor.template linearize<0>(coord);
    heaparr[idx] = elem;
  }

  std::string dump() const {
    std::stringstream ss;
    ss << "View[" << Rank << "]" << std::endl;
    ss << tensor.dump();
    if constexpr (Rank == 2) {
      ss << std::endl;
      for (int i = 0; i < tensor.template extent_at<0>(); i++) {
	for (int j = 0; j < tensor.template extent_at<1>(); j++) {
	  ss << this->read(i,j) << " ";
	}
	ss << std::endl;
      }
      ss << std::endl;
    }
    return ss.str();
  }

  // For the view
  Tensor<Rank> tensor;
  // For the backing block
  Tensor<Rank> btensor;
  HeapArray<Elem> heaparr;
};

template <unsigned long Rank>
template <typename Elem>
Block<Elem,Rank> TensorBuilder<Rank>::to_block() const {
  return {this->to_tensor()};
}

template <typename Elem, unsigned long Rank>
View<Elem,Rank> Block<Elem,Rank>::view() {
  return {tensor, tensor, heaparr};
}

template <typename Elem, unsigned long Rank>
template <typename...PVals>
View<Elem,Rank> Block<Elem,Rank>::partition(std::tuple<int,int,int> pvals,
					    PVals...p) {
  Property_T<Rank> starts;
  Property_T<Rank> stops;
  Property_T<Rank> strides;
  partition<0>(starts, stops, strides, pvals, p...);
  Property_T<Rank> adj_extents;
  Property_T<Rank> adj_origin;
  Property_T<Rank> adj_strides;
  for (int i = 0; i < Rank; i++) {
    // extent
    int t = (stops[i] - starts[i] - 1) / strides[i];
    t++;
    adj_extents[i] = t;
    // stride
    adj_strides[i] = this->tensor.blocked_cs.strides[i] * strides[i];
    // origin
    adj_origin[i] = this->tensor.blocked_cs.origin[i] + this->tensor.blocked_cs.strides[i] * starts[i];
  }
  return {TensorBuilder<Rank>().with_extents(std::move(adj_extents)).
    with_strides(std::move(adj_strides)).
    with_refinement(this->tensor.blocked_cs.refinement).
    with_permutations(this->tensor.blocked_cs.permutations).
    with_origin(std::move(adj_origin)).to_tensor(), 
    this->tensor, this->heaparr};
}

}
}
