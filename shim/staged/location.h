// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "traits.h"
#include "staged_utils.h"

namespace shim {

/// An immutable representation of a physical location.
/// Represents the base object for use within Blocks and Views.
template <unsigned long Rank>
struct MeshLocation {
  using SLoc_T = Loc_T<Rank>;

  MeshLocation(SLoc_T extents, SLoc_T strides, SLoc_T origin,
	       SLoc_T refinement_factors, SLoc_T coarsening_factors);

  void dump_location();

  MeshLocation<Rank> compute_base_mesh_location();

  SLoc_T get_extents();
  SLoc_T get_strides();
  SLoc_T get_origin();
  SLoc_T get_refinement_factors();
  SLoc_T get_coarsening_factors(); 
  
private:

  SLoc_T extents;
  SLoc_T strides;
  SLoc_T origin;
  SLoc_T refinement_factors;
  SLoc_T coarsening_factors; 

};

/// An intermediate object that can be used to buildup a location piecewise.
/// Can be converted into an MeshLocation once complete
template <unsigned long Rank>
struct LocationBuilder {
  using SLoc_T = Loc_T<Rank>;
};

template <unsigned long Rank>
MeshLocation::MeshLocation(SLoc_T extents, SLoc_T strides, SLoc_T origin,
					     SLoc_T refinement_factors, SLoc_T coarsening_factors) :
  extents(std::move(extents)), strides(std::move(strides)), origin(std::move(origin)),
  refinement_factors(std::move(refinement_factors)), coarsening_factors(std::move(coarsening_factors))
{ }

template <unsigned long Rank>
void MeshLocation<Rank>::dump_location() {
  print("Mesh space location info");
  print_newline();
  print("  Extents: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(extents[r]);    
  }  
  print("  Origin: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(origin[r]);    
  }  
  print("  Strides: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(strides[r]);    
  }  
}

template <unsigned long Rank>
MeshLocation<Rank> MeshLocation<Rank>::compute_base_mesh_location() {
  SLoc_T base_extents;
  SLoc_T base_origin;
  SLoc_T base_strides;
  SLoc_T base_refinement;
  SLoc_T base_coarsening;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    base_extents[i] = this->bextents[i] * this->bcoarsening_factors[i] / this->brefinement_factors[i];
    base_origin[i] = this->borigin[i] * this->bcoarsening_factors[i] / this->brefinement_factors[i];
    base_strides[i] = this->bstrides[i] * this->bcoarsening_factors[i] / this->brefinement_factors[i];
    base_refinement[i] = 1;
    base_coarsening[i] = 1;
  }
  return {std::move(base_extents), std::move(base_origin), std::move(base_strides),
    std::move(base_refinement), std::move(base_coarsening)};
}

}
