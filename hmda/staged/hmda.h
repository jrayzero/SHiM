#pragma once

#include "builder/dyn_var.h"
#include "builder/static_var.h"
#include "builder/builder.h"
#include "builder/builder_context.h"

#include "common/functors.h"

#include "common/format.h"
#include "traits.h"
#include "format.h"
#include "utils.h"
#include "fwd.h"
#include "exprs.h"

/*
 * The Hierarchical Data Types
 */

namespace hmda {

  namespace staged {

    // A staged representation of a hierarchical Block
    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    struct Block {

      using self = Block<Elem, Rank, BExtents, BStrides, BOrigin>;
      static const int rank = Rank;

      Block(const BExtents &bextents, const BStrides &bstrides, const BOrigin &borigin,
            const builder::dyn_var<Elem *> &data)
        : bextents(bextents), bstrides(std::move(bstrides)), borigin(std::move(borigin)),
          primary_extents(std::move(bextents)), data(data) {
        static_assert(is_tuple<BExtents>(), "Extents must be tuple");
        static_assert(is_tuple<BStrides>(), "Strides must be tuple");
        static_assert(is_tuple<BOrigin>(), "Origin must be tuple");
        static_assert(std::tuple_size<BStrides>() == Rank, "Rank/stride mismatch");
        static_assert(std::tuple_size<BOrigin>() == Rank, "Rank/origin mismatch");
      }

      // Return a representation of the block.
      std::string dump() const;

      // Just returns Block<Elem,Rank>
      std::string inline_dump() const;

      // Create a Ref that can be used for lazy computation
      template<typename Idx>
      auto operator[](const Idx idx);

      // "Updates" data
      template<typename Rhs, typename...Iterators>
      void assign(const Rhs &rhs, const std::tuple<Iterators...> &idxs);

      // Extents of this block
      const BExtents bextents;
      // Strides of this block
      const BStrides bstrides;
      // Origin of this block
      const BOrigin borigin;
      // Extents to use when determining the lhs loop size
      const BExtents primary_extents;
      // This is essentially a place holder for the data and is the primary different object between the runtime
      // and staged views
      builder::dyn_var<Elem *> data;

    };

    // A wrapper for a Block/View that can be a part of a lazy computation
    template<typename BlockLike, typename Idxs>
    struct Ref : public Expr<Ref<BlockLike, Idxs>> {

      Ref(BlockLike &block_like) : block_like(block_like) {}

      // Chain together lazy computations
      template<typename Idx>
      auto operator[](const Idx idx);

      // Trigger stage generation
      template<typename Rhs>
      void operator=(const Rhs rhs) {
#ifdef HMDA_VERBOSE
        std::cout << block_like.inline_dump() << "[" << idx_tuple_to_str(idxs) << "]";
        if constexpr (std::is_arithmetic<Rhs>()) {
           std::cout << " = " << rhs << std::endl;
        } else {
          std::cout << " = " << rhs.inline_dump() << std::endl;
        }
#endif
        basic_loop<0>(rhs, std::tuple{} /* no iterators at the top level yet!*/);
      }

      // The underlying Block/View
      BlockLike block_like;
      // The indices used to access this Ref
      Idxs idxs;

    private:

      // A simple fully-nested loop
      template<int Depth, typename Rhs, typename...Iterators>
      void basic_loop(Rhs rhs, const std::tuple<Iterators...> &iters) {
        if constexpr (Depth == BlockLike::rank) {
          // do the assignment
          block_like.assign(realize(rhs, idxs, iters), iters);
        } else {
          auto this_idx = std::get<Depth>(idxs);
          if constexpr (std::is_integral<decltype(this_idx)>()) {
            // scalar value at this dimension, only a single iteration
            auto new_iters = std::tuple_cat(iters, std::tuple{builder::static_var<int>(this_idx)});
            basic_loop<Depth + 1>(rhs, new_iters);
          } else {
            // this is an Iter. The extent is the extent of the block_like
            auto extent = std::get<Depth>(block_like.primary_extents);
            for (builder::dyn_var<int> i = 0; i < extent; i = i + 1) {
              auto new_iters = std::tuple_cat(iters, std::tuple{builder::dyn_var<int>(i)});
              basic_loop<Depth + 1>(rhs, new_iters);
            }
          }
        }
      }

    };

    /*
     * Block impls
     */

    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    std::string Block<Elem, Rank, BExtents, BStrides, BOrigin>::dump() const {
      std::stringstream ss;
      ss << "Block<" << type_to_str<Elem>() << "," << Rank << ">" << std::endl;
      ss << "  Extents: {" << tuple_to_str(bextents) << "}" << std::endl;
      ss << "  Strides: {" << tuple_to_str(bstrides) << "}" << std::endl;
      ss << "  Origin: {" << tuple_to_str(borigin) << "}" << std::endl;
      return ss.str();
    }

    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    std::string Block<Elem, Rank, BExtents, BStrides, BOrigin>::inline_dump() const {
      std::stringstream ss;
      ss << "Block<" << type_to_str<Elem>() << "," << Rank << ">";
      return ss.str();
    }

    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    template<typename Idx>
    auto Block<Elem, Rank, BExtents, BStrides, BOrigin>::operator[](const Idx idx) {
      Ref<self, std::tuple<Idx>> ref(*this);
      ref.idxs = std::tuple{idx};
      return ref;
    }

    template<typename Elem, int Rank, typename BExtents, typename BStrides, typename BOrigin>
    template<typename Rhs, typename...Iterators>
    void Block<Elem, Rank, BExtents, BStrides, BOrigin>::assign(const Rhs &rhs,
                                                                const std::tuple<Iterators...> &idxs) {
      auto lidx = linearize<0>(this->bextents, idxs);
      data[lidx] = rhs;
    }

    /*
     * Ref impls
     */

    template<typename BlockLike, typename Idxs>
    auto ref_factory(BlockLike &block_like, const Idxs &idxs) {
      auto ref = Ref<BlockLike, Idxs>(block_like);
      ref.idxs = move(idxs);
      return ref;
    }

    template<typename BlockLike, typename Idxs>
    template<typename Idx>
    auto Ref<BlockLike, Idxs>::operator[](const Idx idx) {
      auto merged_indices = std::tuple_cat(this->idxs, std::tuple{idx});
      auto ref = ref_factory(this->block_like, merged_indices);
//      static_assert(is_iter<decltype(std::get<1>(ref.idxs))>());
      return ref;
    }

  }

}
