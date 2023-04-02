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
 
/*  Property_T<Rank> permutation_mapping(const Property_T<Rank> &other, const Property_T<Rank> &perm_idxs) const {
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
  }*/

  // Goes between two permutations, going through the canonical one first
  template <unsigned long Rank2>
  static Property_T<Rank2> permutation_mapping(const Property_T<Rank2> left_perms, 
					       const Property_T<Rank2> right_perms, 
					       const Property_T<Rank2> to_permute) {
    Property_T<Rank2> permuted;
    // first, move left_perms into the canonical space
    for (int i = 0; i < Rank2; i++) {
      permuted[left_perms[i]] = to_permute[i];
    }
    // now move into right_perms
    Property_T<Rank2> permuted2;
    for (int i = 0; i < Rank2; i++) {
      permuted2[i] = permuted[right_perms[i]];
    }
    return permuted2;
  }

  template <unsigned long ToRank, typename...Point>
  Property_T<ToRank> point_mapping(const BlockedCS<ToRank> &to, Property_T<Rank> &point) const {
     Property_T<Rank> &Bi = point;

    // Bi => Ri => Ci
    for (int i = 0; i < Rank; i++) {
      Bi[i] *= strides[i];
      Bi[i] += origin[i];
      Bi[i] /= refinement[i];
    }
    // This is a bit redundant because we are going to the canonical permutation twice here, but w/e
    Property_T<Rank> perms_canon_rank;
    for (int i = 0; i < Rank; i++) perms_canon_rank[i] = i;
    Property_T<Rank> Ri = permutation_mapping(this->permutations, perms_canon_rank, Bi);
    // Ci => Cj
    rank_mapping<ToRank>(Ri, 0, Ri);
    // Cj => Rj
    Property_T<ToRank> perms_canon_torank;
    for (int i = 0; i < ToRank; i++) perms_canon_torank[i] = i;
    // Also redundant cause already in the canon
    Property_T<ToRank> Rj = permutation_mapping(perms_canon_torank, to.permutations, Ri);
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

  Block(Tensor<Rank> tensor, HeapArray<Elem> heaparr) : 
    tensor(std::move(tensor)),
    heaparr(std::move(heaparr)) { }

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
    } else if constexpr (Rank == 3) {
      ss << std::endl;
      for (int i = 0; i < tensor.template extent_at<0>(); i++) {
	for (int j = 0; j < tensor.template extent_at<1>(); j++) {
	  for (int k = 0; k < tensor.template extent_at<2>(); k++) {	    
	    ss << (int)this->read(i,j,k) << " ";
	  }
	  ss << std::endl;
	}
	ss << std::endl;
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
    int idx = btensor.template linearize<0>(coord);
    return heaparr[idx];
  }

  void write(Property_T<Rank> idxs, Elem elem) {
    auto coord = tensor.blocked_cs.point_mapping(btensor.blocked_cs, idxs);
    int idx = btensor.template linearize<0>(coord);
    heaparr[idx] = elem;
  }

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

  template <typename...PVals>
  View<Elem,Rank> partition(std::tuple<int,int,int> pvals,
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
      this->btensor, this->heaparr};
  }


  std::string dump() const {
    std::stringstream ss;
    ss << "View[" << Rank << "]" << std::endl;
    ss << tensor.dump();    
    if constexpr (Rank == 2) {
      ss << std::endl;
      for (int i = 0; i < tensor.template extent_at<0>(); i++) {
	for (int j = 0; j < tensor.template extent_at<1>(); j++) {
	  ss << (int)this->read(i,j) << " ";
	}
	ss << std::endl;
      }
      ss << std::endl;
    } else if constexpr (Rank == 3) {
      ss << std::endl;
      for (int i = 0; i < tensor.template extent_at<0>(); i++) {
	for (int j = 0; j < tensor.template extent_at<1>(); j++) {
	  for (int k = 0; k < tensor.template extent_at<2>(); k++) {	    
	    ss << (int)this->read(i,j,k) << " ";
	  }
	  ss << std::endl;
	}
	ss << std::endl;
	ss << std::endl;
      }
      ss << std::endl;
    }
    return ss.str();
  }

  template <typename Elem2=Elem>
  Block<Elem2,Rank> ppermute(Property_T<Rank> pidxs) {
    auto extents = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.extents);
    auto strides = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.strides);
    auto refinement = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.refinement);
    auto permutations = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.permutations);
    auto origin = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.origin);
    return {TensorBuilder<Rank>().with_extents(std::move(extents)).
      with_strides(std::move(strides)).
      with_refinement(std::move(refinement)).
      with_permutations(std::move(permutations)).
      with_origin(std::move(origin)).template to_block<Elem2>()};
  }

  template <typename Elem2=Elem>
  Block<Elem2,Rank> ppermute(Property_T<Rank> pidxs, Elem2 *data) {
    auto extents = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.extents);
    auto strides = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.strides);
    auto refinement = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.refinement);
    auto permutations = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.permutations);
    auto origin = tensor.blocked_cs.permutation_mapping(this->tensor.blocked_cs.permutations, pidxs, tensor.blocked_cs.origin);
    return {TensorBuilder<Rank>().with_extents(std::move(extents)).
      with_strides(std::move(strides)).
      with_refinement(std::move(refinement)).
      with_permutations(std::move(permutations)).
      with_origin(std::move(origin)).to_tensor(), HeapArray<Elem2>(data)};
  }
  
  template <typename Elem2=Elem>
  Block<Elem2,Rank> to_block() {
    return {tensor};
  }

  template <typename Elem2=Elem>
  Block<Elem2,Rank> to_block(Elem2 *data) {
    return {tensor, HeapArray<Elem2>(data)};
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
