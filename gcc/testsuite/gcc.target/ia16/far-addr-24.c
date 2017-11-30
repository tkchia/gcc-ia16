/* { dg-do assemble } */
/* { dg-options "-Os -fcall-used-es --save-temps" } */

/* First test for a far pointer processing bug reported by Bart Oldeman ---
   see https://github.com/tkchia/gcc-ia16/issues/14 .

      "	tst14.c: In function ‘bar’:
	tst14.c:11:1: internal compiler error: in reload_combine_note_use, at
	postreload.c:1536
	 }
	 ^
	0x84eac98 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1536
	0x84eaa47 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1603
	0x84eaa82 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1598
	0x84eaa82 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1598
	0x84eaa82 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1598
	0x84eaa82 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1598
	0x84eaa47 reload_combine_note_use
		../../gcc-ia16/gcc/postreload.c:1603
	0x84ec61a reload_combine
		../../gcc-ia16/gcc/postreload.c:1386
	0x84ed58f reload_cse_regs
		../../gcc-ia16/gcc/postreload.c:68
	0x84ed58f execute
		../../gcc-ia16/gcc/postreload.c:2343
	Please submit a full bug report,
	with preprocessed source if appropriate.
	Please include the complete backtrace with any bug report.
	See <http://gcc.gnu.org/bugs.html> for instructions. "  */

extern int foo(int, int);

struct ab {
  int a, b;
};

int bar(struct ab __far * x)
{
  x->a += foo(2, 16);
  return 16 % x->b;
}
