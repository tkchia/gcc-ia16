// PR c++/69631
// { dg-options -fwrapv }

#if __SIZEOF_INT__ == 2
#define N 32767L
#else
#define N 32767
#endif

struct C {
  static const unsigned short max = static_cast<unsigned short>((N * 2 + 1));
};
