// PR c++/34918
// { dg-do compile { target { stdint_types } } }

#include <stdint.h>

int32_t v __attribute ((vector_size (8)));
bool b = !(v - v);	// { dg-error "not convert .__vector.2. \(long \)\?int. to .bool. in initialization" }
