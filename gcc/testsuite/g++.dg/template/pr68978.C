// PR c++/68978

#ifdef __INT_LEAST32_TYPE__
#define INT_LEAST32_T __INT_LEAST32_TYPE__
#else
#define INT_LEAST32_T int
#endif

INT_LEAST32_T i = 0, c = 0, radix = 10, max = 0x7fffffff;

template <typename T> INT_LEAST32_T toi_1() {
  if (max < ((i *= radix) += c))
    return 0;
  return i;
}

template <typename T> INT_LEAST32_T toi_2() {
  if (max < ((i = radix) = c))
    return 0;
  return i;
}

template <typename T> INT_LEAST32_T toi_3() {
  if (max < ((i = radix) += c))
    return 0;
  return i;
}

template <typename T> INT_LEAST32_T toi_4() {
  if (max < ((i += radix) = c))
    return 0;
  return i;
}

template <typename T> INT_LEAST32_T toi_5() {
  if (max < (((i = radix) += (c += 5)) *= 30))
    return 0;
  return i;
}

INT_LEAST32_T x = toi_1<INT_LEAST32_T> ();
INT_LEAST32_T y = toi_2<INT_LEAST32_T> ();
INT_LEAST32_T z = toi_3<INT_LEAST32_T> ();
INT_LEAST32_T w = toi_4<INT_LEAST32_T> ();
INT_LEAST32_T r = toi_5<INT_LEAST32_T> ();
