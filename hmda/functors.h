#pragma once

namespace hmda {

struct AddFunctor {
  template <typename L, typename R>
  auto operator()(const L &l, const R &r) {
    return l + r;
  }
};

}
