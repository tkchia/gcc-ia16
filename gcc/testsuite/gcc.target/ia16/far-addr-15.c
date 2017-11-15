/* { dg-do assemble } */
/* { dg-options "-O0 --save-temps" } */

/* Casting an array within a far structure to a pointer.  Bug was reported
   by Bart Oldeman --- https://github.com/tkchia/gcc-ia16/issues/3 .

   The bug was caused by c_build_qualified_type (...) (in gcc/c/c-typeck.c)
   failing to propagate the address space information of an array's element
   type to the array type itself.

   A similar routine exists on the C++ side, but (as of Nov 2017) the GNU
   C++ compiler does not yet support named address spaces, so will not
   currently trigger this bug.  */

struct string1 {
  char str[1];
};

void farstr(char __far *);

void call_farstr(struct string1 __far *p)
{
  farstr(p->str);
}
