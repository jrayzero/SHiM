// -*-c++-*-
//
#pragma once

#define DYN_VAR_PTR(type)				\
  template <>						\
  class dyn_var<type*> : public dyn_var_impl<type*> {	\
  public:						\
  typedef dyn_var_impl<type*> super;			\
  using super::super;					\
  using super::operator=;				\
  builder operator= (const dyn_var<type*> &t) {		\
    return (*this) = (builder)t;			\
  }							\
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
  dyn_var(): dyn_var_impl<type*>() {}			\
  dyn_var<type> operator *() {				\
    return (cast)this->operator[](0);			\
  }							\
  dyn_var<type> _p = as_member_of(this, "_p");		\
  dyn_var<type>* operator->() {				\
    _p = (cast)this->operator[](0);			\
    return _p.addr();					\
  }							\
  };

#define DYN_VAR_BP(type)				\
  typedef dyn_var_impl<type> super;			\
  using super::super;					\
  using super::operator=;				\
  builder operator= (const dyn_var<type> &t) {		\
    return (*this) = (builder)t;			\
  }							\
  dyn_var(const dyn_var& t): dyn_var_impl((builder)t){}	\
  dyn_var(): dyn_var_impl<type>() {}					
