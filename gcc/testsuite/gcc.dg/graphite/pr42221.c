/* { dg-require-effective-target int32plus } */
/* { dg-options "-Os -fgraphite-identity" } */
/* { dg-require-effective-target stdint_types } */

#include <stdint.h>

static void b2w(uint_least32_t *out, const unsigned char *in, unsigned int len)
{
  const unsigned char *bpend = in + len;
  for (; in != bpend; in += 4, ++out)
  {
    *out = (uint_least32_t) (in[0] ) |
      (uint_least32_t) ((int_least32_t)in[3] << 24);
  }
}
static void md4step(uint_least32_t state[4], const unsigned char *data)
{
  uint_least32_t A, X[16];
  b2w(X, data, 64);
  state[0] += A;
}
void md4sum(void)
{
  unsigned char final[128];
  uint_least32_t state[4];
  md4step(state, final);
  md4step(state, final + 64);
}
