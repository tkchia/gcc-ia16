/* PR c/68657 */
/* { dg-do compile } */
/* { dg-require-effective-target ptr32plus } */

#pragma GCC diagnostic error "-Wlarger-than=8192"
int a[16384/sizeof (int)];	/* { dg-error "size of 'a' is \[1-9]\[0-9]* bytes" } */
int b[1024];	/* { dg-bogus "size of 'b' is \[1-9]\[0-9]* bytes" } */
#pragma GCC diagnostic ignored "-Wlarger-than=8192"
int c[16384/sizeof (int)];	/* { dg-bogus "size of 'c' is \[1-9]\[0-9]* bytes" } */
int d[1024];	/* { dg-bogus "size of 'd' is \[1-9]\[0-9]* bytes" } */
#pragma GCC diagnostic warning "-Wlarger-than=8192"
int e[16384/sizeof (int)];	/* { dg-warning "size of 'e' is \[1-9]\[0-9]* bytes" } */
int f[1024];	/* { dg-bogus "size of 'f' is \[1-9]\[0-9]* bytes" } */
/* { dg-prune-output "treated as errors" } */
