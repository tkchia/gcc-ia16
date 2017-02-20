// { dg-lto-do run }
// { dg-require-effective-target stdint_types }

#include <stdint.h>

const char *func(int_least32_t val) {
 switch (val) {
   case 2147483647: return "foo";
   default: return "";
 }
}

int main() {
 return 0;
}
