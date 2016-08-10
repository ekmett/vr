#pragma once

// proxy<owner,T> is a form of T which can be modified only by owner.

template <typename owner, typename T> struct proxy {
public:
  proxy() : x() {}
  
  // copy
  template <typename other> proxy(const proxy<other, T> & o) : x(o.x) {}
  proxy(const T & o) : x(o.x) {}

  // move
  proxy(T && o) noexcept : x(std::move(o.x)) {}
  proxy(proxy<owner, T> && o) noexcept : x(std::move(o.x)) {}

  operator const T&() const { return data; }

  // TODO: add the rest
  template<typename U> inline bool operator==(const U& y) const { return x == y; }
  template<typename U> inline U operator+ (const U& y) const { return x + y; }
  template<typename U> inline U operator- (const U& y) const { return x - y; }
  template<typename U> inline U operator* (const U& y) const { return x * y; }
  template<typename U> inline U operator/ (const U& y) const { return x / y; }
  template<typename U> inline U operator<<(const U& y) const { return x << y; }
  template<typename U> inline U operator >> (const U& y) const { return x >> y; }
  template<typename U> inline U operator^ (const U& y) const { return x ^ y; }
  template<typename U> inline U operator| (const U& y) const { return x | y; }
  template<typename U> inline U operator& (const U& y) const { return x & y; }
  template<typename U> inline U operator&&(const U& y) const { return x &&y; }
  template<typename U> inline U operator||(const U& y) const { return x || y; }
  template<typename U> inline U operator~() const { return ~x; }

private:
  template<typename U> inline U operator= (const U& y) { return x = y; }
  template<typename U> inline U operator+=(const U& y) { return x += y; }
  template<typename U> inline U operator-=(const U& y) { return x -= y; }
  template<typename U> inline U operator*=(const U& y) { return x *= y; }
  template<typename U> inline U operator/=(const U& y) { return x /= y; }
  template<typename U> inline U operator&=(const U& y) { return x &= y; }
  template<typename U> inline U operator|=(const U& y) { return x |= y; }

  T & operator++() { return ++x; }
  T operator++(int) {
    T y(x);
    ++x;
    return y;    
  }

  T x;
  T & operator=(const T& arg) { x = y; return x; }

  friend class owner;
};