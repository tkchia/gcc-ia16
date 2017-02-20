/* { dg-require-effective-target fpic } */
/* { dg-require-effective-target shared } */
/* { dg-lto-do link } */
/* { dg-lto-options { { -fPIC -flto -flto-partition=1to1 } } } */
/* { dg-extra-ld-options { -shared } } */
/* { dg-skip-if "-shared not supported" { "ia16-*-*" } }  */

static void *master;
void *foo () { return master; }
