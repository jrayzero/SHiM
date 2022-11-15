#pragma once

#include <sstream>

#include "fwd.h"
#include "common/functors.h"
#include "utils.h"

namespace hmda {

  namespace staged {

    // Figure out where an Iter on the RHS corresponds to on the LHS
    template <char Ident, int Depth, typename LhsIdx, typename...LhsIdxs>
    struct FindIter {
      template <typename...Iterations>
      auto operator()(const std::tuple<Iterations...> &iters) const {
        if constexpr (sizeof...(LhsIdxs) == Depth) {
          HMDA_FAIL(SS << "Could not find LHS index for RHS Iter " << Ident << ".");
          return -1; // stub
        } else {
          return FindIter<Ident, Depth+1, LhsIdxs...>()(iters);
        }
      }
    };

    template <char Ident, int Depth, typename...LhsIdxs>
    struct FindIter<Ident, Depth, Iter<Ident>, LhsIdxs...> {
      template <typename...Iterations>
      auto operator()(const std::tuple<Iterations...> &iters) const {
        return std::get<Depth>(iters);
      }
    };

    template <char Ident, typename...LhsIdxs, typename...Iterations>
    int find_iter(const std::tuple<LhsIdxs...> &lhs_idxs, const std::tuple<Iterations...> &iters) {
      return FindIter<Ident, 0, LhsIdxs...>()(iters);
    }


    // A lazy expression
    template<typename Derived>
    struct Expr {

      template<typename Rhs>
      Binary<AddFunctor, Derived, Rhs> operator+(Rhs rhs) {
        return Binary<AddFunctor, Derived, Rhs>(*static_cast<Derived*>(this), std::move(rhs));
      }

    };

    // A lazy unary computation
    template <typename Functor, typename Operand>
    struct Unary : public Expr<Unary<Functor, Operand>> {

      Unary(Operand operand) : operand(move(operand)) { }

      template<typename...Iterations, typename...LhsIdxs>
      auto realize(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs) {
        auto op = operand.realize(iters, lhs_idxs);
        return Functor()(op);
      }

      // Get a string representation of this operator
      std::string inline_dump() const {
        std::stringstream ss;
        if constexpr (std::is_arithmetic<Operand>()) {
          ss << Functor::op << "(" << operand << ")";
        } else {
          ss << Functor::op << "(" << operand.inline_dump() << ")";
        }
        return ss.str();
      }

      Operand operand;

    };

    // A lazy binary computation
    template<typename Functor, typename Operand0, typename Operand1>
    struct Binary : public Expr<Binary<Functor, Operand0, Operand1>> {

      Binary(Operand0 operand0, Operand1 operand1) :
        operand0(std::move(operand0)), operand1(std::move(operand1)) {}

      template<typename...Iterations, typename...LhsIdxs>
      auto realize(const std::tuple<Iterations...> &iters, const std::tuple<LhsIdxs...> &lhs_idxs) {
        auto op0 = operand0.realize(iters, lhs_idxs);
        auto op1 = operand1.realize(iters, lhs_idxs);
        return Functor()(op0, op1);
      }

      // Get a string representation of this operator
      std::string inline_dump() const {
        std::stringstream ss;
        if constexpr (Functor::is_func_like) {
          ss << Functor::op << "(";
        }
        if constexpr (std::is_arithmetic<Operand0>()) {
          ss << "(" << operand0 << ")";
        } else {
          ss << "(" << operand0.inline_dump() << ")";
        }
        if constexpr (!Functor::is_func_like) {
          ss << " " << Functor::op << " ";
        }
        if constexpr (std::is_arithmetic<Operand0>()) {
          ss << "(" << operand1<< ")";
        } else {
          ss << "(" << operand1.inline_dump() << ")";
        }
        if constexpr (Functor::is_func_like) {
          ss << ")";
        }
        return ss.str();
      }
      
      Operand0 operand0;
      Operand1 operand1;

    };

    // A placeholder for elementwise operations that takes on the entire domain of the LHS
    template<char Ident>
    struct Iter : public Expr<Iter<Ident>> {

      static constexpr char ident = Ident;

      // Get the loop iteration value corresponding to this Iter based on the lhs
      template< typename...LhsIdxs, typename...Iterations>
      int realize(const std::tuple<LhsIdxs...> &lhs_idxs, const std::tuple<Iterations...> &iters) {
        return find_iter<Ident>(lhs_idxs, iters);
      }

      // Just return ident
      std::string inline_dump() const { return std::string(1, Ident); }

    };

    // Unwind a lazy expression and ultimately return a dyn_var
    template<typename Rhs, typename LhsIdxs, typename...Iterators>
    auto realize(Rhs rhs, const LhsIdxs &idxs, const std::tuple<Iterators...> &iters) {
      if constexpr (std::is_arithmetic<Rhs>()) {
        return rhs; // just a scalar, no unwinding needed
      } else {
	// TODO
	return rhs.realize(idxs, iters);
      }
    }

  }

}
