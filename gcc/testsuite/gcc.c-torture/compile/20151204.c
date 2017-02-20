/* { dg-skip-if "Array too big" { "avr-*-*" } } */

typedef __SIZE_TYPE__ size_t;

#ifdef __ia16__
#define BUFFER_LENGTH 3275
#else
#define BUFFER_LENGTH 32753
#endif

int strcmp (const char*, const char*);
void *memchr (const void *, int, size_t);
char* strncpy (char *, const char *, size_t);

int
main (int argc, char** argv)
{
  char target[BUFFER_LENGTH] = "A";
  char buffer[BUFFER_LENGTH];
  char *x;
  x = buffer;

  if (strcmp (target, "A")
      || memchr (target, 'A', 0) != ((void *) 0))
    if (strncpy (x, "", 4) != x);
  return 0;
}
