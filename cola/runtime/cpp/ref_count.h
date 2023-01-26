// -*-c++-*-

#pragma once

struct RefCounted {

  RefCounted() : count(0) { }

  int incr() { return ++count; }

  int decr() { return --count; }

  bool can_free() const { return count == 0; }

private:

  mutable int count;

};

