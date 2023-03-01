// -*-c++-*-

#pragma once

#include "builder/dyn_var.h"
#include "builder/array.h"
#include "traits.h"
#include "fwddecls.h"
#include "staged_utils.h"
#include "fwrappers.h"

namespace shim {

/// An immutable representation of a physical location.
/// Represents the base object for use within Blocks and Views.
template <unsigned long Rank>
struct MeshLocation {
  using SLoc_T = Loc_T<Rank>;

  MeshLocation();
  
  MeshLocation(SLoc_T extents, SLoc_T strides, SLoc_T origin,
	       SLoc_T refinement_factors, SLoc_T coarsening_factors);
  
  void dump_location();
  
  MeshLocation<Rank> compute_base_mesh_location();
  MeshLocation<Rank> into(MeshLocation<Rank> &other);
  MeshLocation<Rank> refine(SLoc_T refinement_factors);
  MeshLocation<Rank> coarsen(SLoc_T coarsening_factors);
  

  SLoc_T get_extents() { return extents; }
  SLoc_T get_strides() { return strides; }
  SLoc_T get_origin() { return origin; }
  SLoc_T get_refinement_factors() { return refinement_factors; }
  SLoc_T get_coarsening_factors() { return coarsening_factors; }
  
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

  LocationBuilder();

  LocationBuilder<Rank> &with_extents(SLoc_T extents);
  LocationBuilder<Rank> &with_strides(SLoc_T strides);
  LocationBuilder<Rank> &with_origin(SLoc_T origin);
  LocationBuilder<Rank> &with_refinement(SLoc_T refinement);
  LocationBuilder<Rank> &with_coarsening(SLoc_T coarsening);

  MeshLocation<Rank> to_loc();

  operator MeshLocation<Rank>() { return to_loc(); }

private:
  SLoc_T extents;
  SLoc_T strides;
  SLoc_T origin;
  SLoc_T refinement;
  SLoc_T coarsening; 
};

template <unsigned long Rank>
LocationBuilder<Rank>::LocationBuilder() {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    extents[i] = 1;
    strides[i] = 1;
    origin[i] = 0;
    refinement[i] = 1;
    coarsening[i] = 1;
  }
}

template <unsigned long Rank>
LocationBuilder<Rank> &LocationBuilder<Rank>::with_extents(SLoc_T extents) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    this->extents[i] = extents[i];
  }
  return *this;
}

template <unsigned long Rank>
LocationBuilder<Rank> &LocationBuilder<Rank>::with_strides(SLoc_T strides) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    this->strides[i] = strides[i];
  }
  return *this;
}

template <unsigned long Rank>
LocationBuilder<Rank> &LocationBuilder<Rank>::with_origin(SLoc_T origin) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    this->origin[i] = origin[i];
  }
  return *this;
}

template <unsigned long Rank>
LocationBuilder<Rank> &LocationBuilder<Rank>::with_refinement(SLoc_T refinement) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    this->refinement[i] = refinement[i];
  }
  return *this;
}

template <unsigned long Rank>
LocationBuilder<Rank> &LocationBuilder<Rank>::with_coarsening(SLoc_T coarsening) {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    this->coarsening[i] = coarsening[i];
  }
  return *this;
}

template <unsigned long Rank>
MeshLocation<Rank> LocationBuilder<Rank>::to_loc() {
  return {extents, strides, origin, refinement, coarsening};
}

template <unsigned long Rank>
MeshLocation<Rank>::MeshLocation() {
  for (svar<int> i = 0; i < Rank; i=i+1) {
    extents[i] = 1;
    strides[i] = 1;
    origin[i] = 0;
    refinement_factors[i] = 1;
    coarsening_factors[i] = 1;
  }
}

template <unsigned long Rank>
MeshLocation<Rank>::MeshLocation(SLoc_T extents, SLoc_T strides, SLoc_T origin,
				 SLoc_T refinement_factors, SLoc_T coarsening_factors) :
  extents(std::move(extents)), strides(std::move(strides)), origin(std::move(origin)),
  refinement_factors(std::move(refinement_factors)), coarsening_factors(std::move(coarsening_factors))
{ }

template <unsigned long Rank>
void MeshLocation<Rank>::dump_location() {
  std::string rank = std::to_string(Rank);
  print("  Rank: ");
  print(rank);
  print_newline();
  print("  Extents: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(extents[r]);    
  }  
  print_newline();
  print("  Origin: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(origin[r]);    
  }  
  print_newline();
  print("  Strides: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(strides[r]);    
  }  
  print_newline();
  print("  Refinement: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(refinement_factors[r]);    
  }  
  print_newline();
  print("  Coarsening: ");
  for (svar<int> r = 0; r < Rank; r=r+1) {
    print(" ");
    dispatch_print_elem<int>(coarsening_factors[r]);    
  }  
  print_newline();
}

template <unsigned long Rank>
MeshLocation<Rank> MeshLocation<Rank>::compute_base_mesh_location() {
  SLoc_T base_extents;
  SLoc_T base_origin;
  SLoc_T base_strides;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    base_extents[i] = extents[i] * coarsening_factors[i] / refinement_factors[i];
    base_origin[i] = origin[i] * coarsening_factors[i] / refinement_factors[i];
    base_strides[i] = strides[i] * coarsening_factors[i] / refinement_factors[i];
  }
  return {std::move(base_extents), std::move(base_strides), std::move(base_origin),
    refinement_factors, coarsening_factors};
}
  
template <unsigned long Rank>
MeshLocation<Rank> MeshLocation<Rank>::into(MeshLocation<Rank> &other) {
  SLoc_T mesh_extents;
  SLoc_T mesh_strides;
  SLoc_T mesh_origin;
  for (svar<int> i = 0; i < Rank; i=i+1) {
    mesh_extents[i] = get_extents()[i] * other.get_refinement_factors()[i] / other.get_coarsening_factors()[i];
    // this needs to be at least 1!
    // ceil(stride*ref/coarse)
    mesh_strides[i] = ((get_strides()[i] * other.get_refinement_factors()[i]) - 1) / other.get_coarsening_factors()[i] + 1;
    mesh_origin[i] = get_origin()[i] * other.get_refinement_factors()[i] / other.get_coarsening_factors()[i];
  }  
  return {mesh_extents, mesh_strides, mesh_origin, 
    other.get_refinement_factors(), other.get_coarsening_factors()};
}

template <unsigned long Rank>
MeshLocation<Rank> MeshLocation<Rank>::refine(SLoc_T refinement_factors) {
  SLoc_T rextents;
  SLoc_T rorigin;
  SLoc_T rrefinement;
  for (svar<int> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (strides[i] != 1 && refinement_factors[i] > 1) {
      print("Cannot refine block with non-unit stride %d in dimension %d.\\n", strides[i], i);
      hexit(-1);
    }
#endif
    rextents[i] = extents[i] * refinement_factors[i];
    rorigin[i] = origin[i] * refinement_factors[i];
    rrefinement[i] = this->refinement_factors[i] * refinement_factors[i];    
  }
  return LocationBuilder<Rank>().
    with_extents(std::move(rextents)).
    with_origin(std::move(rorigin)).
    with_refinement(std::move(rrefinement)).
    with_coarsening(coarsening_factors).to_loc();
}

template <unsigned long Rank>
MeshLocation<Rank> MeshLocation<Rank>::coarsen(SLoc_T coarsening_factors) {
  SLoc_T cextents;
  SLoc_T corigin;
  SLoc_T ccoarsening;
  for (svar<loop_type> i = 0; i < Rank; i=i+1) {
#ifdef SHIM_USER_DEBUG 
    if (strides[i] != 1 && coarsening_factors[i] > 1) {
      print("Cannot coarsen block with non-unit stride %d in dimension %d.\\n", strides[i], i);
      hexit(-1);
    }
#endif
    cextents[i] = extents[i] /  coarsening_factors[i];
    corigin[i] = origin[i] / coarsening_factors[i];
    ccoarsening[i] = coarsening_factors[i] * coarsening_factors[i];
  }  
  return LocationBuilder<Rank>().
    with_extents(std::move(cextents)).
    with_origin(std::move(corigin)).
    with_refinement(refinement_factors).
    with_coarsening(std::move(ccoarsening)).to_loc();
}
  
}
