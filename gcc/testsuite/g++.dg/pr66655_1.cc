#include "pr66655.h"

extern int_least32_t g (void);

int_least32_t S::i;

int_least32_t
f (void)
{
  int_least32_t ret = g ();

  S::set (ret);
  return ret;
}
