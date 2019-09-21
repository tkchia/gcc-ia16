/* { dg-options "-std=gnu11 -Wall --save-temps" } */
/* { dg-do assemble } */

/* Test that the `near_section' and `far_section' attributes are ignored for
   non-far function declarations.  */

int (*p1) (int)
  __attribute__ ((near_section));  /* { dg-warning "directive ignored" } */
int (*p2) (int)
  __attribute__ ((far_section));   /* { dg-warning "directive ignored" } */
int __far (*p3) (int)
  __attribute__ ((near_section));  /* { dg-bogus "directive ignored" } */
int __far (*p4) (int)
  __attribute__ ((far_section));   /* { dg-bogus "directive ignored" } */

void
f1 (int (*p) (int))
{
  p1 = p;  /* { dg-bogus "assignment from incompatible pointer type" } */
}

void
f2 (int (*p) (int))
{
  p2 = p;  /* { dg-bogus "assignment from incompatible pointer type" } */
}
void
f3 (int (*p) (int)
    __attribute__ ((far_section)))  /* { dg-warning "directive ignored" } */
{
  p1 = p;  /* { dg-bogus "assignment from incompatible pointer type" } */
}

void
f4 (int (*p) (int)
    __attribute__ ((near_section)))  /* { dg-warning "directive ignored" } */
{
  p2 = p;  /* { dg-bogus "assignment from incompatible pointer type" } */
}
