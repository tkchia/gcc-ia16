/* Offload image generation tool for Intel MIC devices.

   Copyright (C) 2014 Free Software Foundation, Inc.

   Contributed by Ilya Verbin <ilya.verbin@intel.com>.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <libgen.h>
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "collect-utils.h"
#include "diagnostic.h"
#include "obstack.h"

#define TARGET_DRIVER STANDARD_BINDIR_PREFIX DEFAULT_TARGET_MACHINE "-g++"
#define TARGET_EPATH  STANDARD_EXEC_PREFIX DEFAULT_TARGET_MACHINE "/" DEFAULT_TARGET_VERSION "/"
#define TARGET_CPATH  STANDARD_LIBEXEC_PREFIX DEFAULT_TARGET_MACHINE "/" DEFAULT_TARGET_VERSION "/:" LIBEXECSUBDIR "/"
#define TARGET_LPATH  STANDARD_EXEC_PREFIX DEFAULT_TARGET_MACHINE "/lib64/"

const char *target_compiler_driver = TARGET_DRIVER;
const char *target_compiler_epath = TARGET_EPATH;
const char *target_compiler_cpath = TARGET_CPATH;
const char *target_compiler_lpath = TARGET_LPATH;

const char tool_name[] = "mkoffload";
const char image_section_name[] = ".offload_image_section";
const char *symbols[3] = { "_offload_image_intelmic_start",
			   "_offload_image_intelmic_end",
			   "_offload_image_intelmic_size" };
const char *out_obj_filename = NULL;

int num_temps = 0;
const int MAX_NUM_TEMPS = 10;
const char *temp_files[MAX_NUM_TEMPS];


/* Delete tempfiles and exit function.  */
void
tool_cleanup (__attribute__((unused)) bool from_signal)
{
  for (int i = 0; i < num_temps; i++)
    maybe_unlink (temp_files[i]);
}

static void
mkoffload_atexit (void)
{
  tool_cleanup (false);
}

/* Unlink FILE unless we are debugging.  */
void
maybe_unlink (const char *file)
{
  if (debug)
    notice ("[Leaving %s]\n", file);
  else
    unlink_if_ordinary (file);
}

/* Add or change the value of an environment variable, outputting the
   change to standard error if in verbose mode.  */
static void
xputenv (const char *string)
{
  if (verbose)
    fprintf (stderr, "%s\n", string);
  putenv (CONST_CAST (char *, string));
}

static void
compile_for_target (struct obstack *argv_obstack)
{
  char **argv = XOBFINISH (argv_obstack, char **);

  /* Save environment variables.  */
  const char *epath = getenv ("GCC_EXEC_PREFIX");
  const char *cpath = getenv ("COMPILER_PATH");
  const char *lpath = getenv ("LIBRARY_PATH");
  const char *rpath = getenv ("LD_RUN_PATH");
  unsetenv ("GCC_EXEC_PREFIX");
  unsetenv ("COMPILER_PATH");
  unsetenv ("LIBRARY_PATH");
  unsetenv ("LD_RUN_PATH");

  xputenv (concat ("GCC_EXEC_PREFIX=", target_compiler_epath, NULL));
  xputenv (concat ("COMPILER_PATH=", target_compiler_cpath, NULL));
  xputenv (concat ("LIBRARY_PATH=", target_compiler_lpath, NULL));

  fork_execute (argv[0], argv, false);
  obstack_free (argv_obstack, NULL);

  /* Restore environment variables.  */
  unsetenv ("GCC_EXEC_PREFIX");
  unsetenv ("COMPILER_PATH");
  unsetenv ("LIBRARY_PATH");
  xputenv (concat ("GCC_EXEC_PREFIX=", epath, NULL));
  xputenv (concat ("COMPILER_PATH=", cpath, NULL));
  xputenv (concat ("LIBRARY_PATH=", lpath, NULL));
  xputenv (concat ("LD_RUN_PATH=", rpath, NULL));
}

/* Generates object file with the descriptor for the target library.  */
static const char *
generate_target_descr_file (void)
{
  const char *src_filename = make_temp_file ("_target_descr.c");
  const char *obj_filename = make_temp_file ("_target_descr.o");
  temp_files[num_temps++] = src_filename;
  temp_files[num_temps++] = obj_filename;
  FILE *src_file = fopen (src_filename, "w");

  if (!src_file)
    fatal_error ("cannot open '%s'", src_filename);

  fprintf (src_file,
	   "extern void *_offload_funcs_end[];\n"
	   "extern void *_offload_vars_end[];\n\n"

	   "void *_offload_func_table[0]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"__gnu_offload_funcs\"))) = { };\n\n"

	   "void *_offload_var_table[0]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"__gnu_offload_vars\"))) = { };\n\n"

	   "void *__OPENMP_TARGET_TABLE__[]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"%s\"))) = {\n"
	   "  &_offload_func_table, &_offload_funcs_end,\n"
	   "  &_offload_var_table, &_offload_vars_end\n"
	   "};\n\n", image_section_name);

  fprintf (src_file,
	   "#ifdef __cplusplus\n"
	   "extern \"C\"\n"
	   "#endif\n"
	   "void target_register_lib (const void *);\n\n"

	   "__attribute__((constructor))\n"
	   "static void\n"
	   "init (void)\n"
	   "{\n"
	   "  target_register_lib (__OPENMP_TARGET_TABLE__);\n"
	   "}\n");
  fclose (src_file);

  struct obstack argv_obstack;
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, target_compiler_driver);
  obstack_ptr_grow (&argv_obstack, "-c");
  obstack_ptr_grow (&argv_obstack, "-shared");
  obstack_ptr_grow (&argv_obstack, "-fPIC");
  obstack_ptr_grow (&argv_obstack, src_filename);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, obj_filename);
  obstack_ptr_grow (&argv_obstack, NULL);
  compile_for_target (&argv_obstack);

  return obj_filename;
}

/* Generates object file with _offload_*_end symbols for the target library.  */
static const char *
generate_target_offloadend_file (void)
{
  const char *src_filename = make_temp_file ("_target_offloadend.c");
  const char *obj_filename = make_temp_file ("_target_offloadend.o");
  temp_files[num_temps++] = src_filename;
  temp_files[num_temps++] = obj_filename;
  FILE *src_file = fopen (src_filename, "w");

  if (!src_file)
    fatal_error ("cannot open '%s'", src_filename);

  fprintf (src_file,
	   "void *_offload_funcs_end[0]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"__gnu_offload_funcs\"))) = { };\n\n"

	   "void *_offload_vars_end[0]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"__gnu_offload_vars\"))) = { };\n");
  fclose (src_file);

  struct obstack argv_obstack;
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, target_compiler_driver);
  obstack_ptr_grow (&argv_obstack, "-c");
  obstack_ptr_grow (&argv_obstack, "-shared");
  obstack_ptr_grow (&argv_obstack, "-fPIC");
  obstack_ptr_grow (&argv_obstack, src_filename);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, obj_filename);
  obstack_ptr_grow (&argv_obstack, NULL);
  compile_for_target (&argv_obstack);

  return obj_filename;
}

/* Generates object file with the host side descriptor.  */
static const char *
generate_host_descr_file (const char *host_compiler)
{
  const char *src_filename = make_temp_file ("_host_descr.c");
  const char *obj_filename = make_temp_file ("_host_descr.o");
  temp_files[num_temps++] = src_filename;
  temp_files[num_temps++] = obj_filename;
  FILE *src_file = fopen (src_filename, "w");

  if (!src_file)
    fatal_error ("cannot open '%s'", src_filename);

  fprintf (src_file,
	   "extern void *__OPENMP_TARGET__;\n"
	   "extern void *_offload_image_intelmic_start;\n"
	   "extern void *_offload_image_intelmic_end;\n\n"

	   "void *__OPENMP_TARGET_DATA__[]\n"
	   "__attribute__ ((__used__, visibility (\"hidden\"),\n"
	   "section (\"%s\"))) = {\n"
	   "  &_offload_image_intelmic_start, &_offload_image_intelmic_end\n"
	   "};\n\n", image_section_name);

  fprintf (src_file,
	   "#ifdef __cplusplus\n"
	   "extern \"C\"\n"
	   "#endif\n"
	   "void GOMP_offload_register (void *, int, void *);\n\n"

	   "__attribute__((constructor))\n"
	   "static void\n"
	   "init (void)\n"
	   "{\n"
	   "  GOMP_offload_register (&__OPENMP_TARGET__, 1, __OPENMP_TARGET_DATA__);\n"
	   "}\n");
  fclose (src_file);

  struct obstack argv_obstack;
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, host_compiler);
  obstack_ptr_grow (&argv_obstack, "-c");
  obstack_ptr_grow (&argv_obstack, "-shared");
  obstack_ptr_grow (&argv_obstack, "-fPIC");
  obstack_ptr_grow (&argv_obstack, src_filename);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, obj_filename);
  obstack_ptr_grow (&argv_obstack, NULL);

  char **argv = XOBFINISH (&argv_obstack, char **);
  fork_execute (argv[0], argv, false);
  obstack_free (&argv_obstack, NULL);

  return obj_filename;
}

static const char *
prepare_target_image (int argc, char **argv)
{
  const char *target_descr_filename = generate_target_descr_file ();
  const char *target_offloadend_filename = generate_target_offloadend_file ();

  char *opt1 = (char *) xmalloc (strlen (target_descr_filename)
				 + strlen ("-Wl,") + 1);
  char *opt2 = (char *) xmalloc (strlen (target_offloadend_filename)
				 + strlen ("-Wl,") + 1);
  if (!opt1 || !opt2)
    fatal_error ("cannot allocate memory.");
  sprintf (opt1, "-Wl,%s", target_descr_filename);
  sprintf (opt2, "-Wl,%s", target_offloadend_filename);

  const char *target_so_filename = make_temp_file ("_offload_intelmic.so");
  temp_files[num_temps++] = target_so_filename;
  struct obstack argv_obstack;
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, target_compiler_driver);
  obstack_ptr_grow (&argv_obstack, "-xlto");
  obstack_ptr_grow (&argv_obstack, "-fopenmp");
  obstack_ptr_grow (&argv_obstack, "-shared");
  obstack_ptr_grow (&argv_obstack, "-fPIC");
  obstack_ptr_grow (&argv_obstack, opt1);
  for (int i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-o") && i + 1 != argc)
	out_obj_filename = argv[++i];
      else
	obstack_ptr_grow (&argv_obstack, argv[i]);
    }
  if (!out_obj_filename)
    fatal_error ("output file not specified.");
  obstack_ptr_grow (&argv_obstack, opt2);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, target_so_filename);
  obstack_ptr_grow (&argv_obstack, NULL);
  compile_for_target (&argv_obstack);
  free (opt1);
  free (opt2);

  /* Run objcopy.  */
  opt1 = (char *) xmalloc (strlen (".data=") + strlen (image_section_name) + 1);
  if (!opt1)
    fatal_error ("cannot allocate memory.");
  sprintf (opt1, ".data=%s", image_section_name);
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, "objcopy");
  obstack_ptr_grow (&argv_obstack, "-B");
  obstack_ptr_grow (&argv_obstack, "i386");
  obstack_ptr_grow (&argv_obstack, "-I");
  obstack_ptr_grow (&argv_obstack, "binary");
  obstack_ptr_grow (&argv_obstack, "-O");
  obstack_ptr_grow (&argv_obstack, "elf64-x86-64");
  obstack_ptr_grow (&argv_obstack, target_so_filename);
  obstack_ptr_grow (&argv_obstack, "--rename-section");
  obstack_ptr_grow (&argv_obstack, opt1);
  obstack_ptr_grow (&argv_obstack, NULL);
  char **new_argv = XOBFINISH (&argv_obstack, char **);
  fork_execute (new_argv[0], new_argv, false);
  obstack_free (&argv_obstack, NULL);
  free (opt1);

  /* Objcopy has created symbols, containing the input file name with
     special characters replaced with '_'.  We are going to rename these
     new symbols.  */
  char *symbol_name = (char *) xmalloc (strlen (target_so_filename));
  int i = 0;
  while (target_so_filename[i])
    {
      char c = target_so_filename[i];
      if ((c == '/') || (c == '.'))
	c = '_';
      symbol_name[i] = c;
      i++;
    }
  symbol_name[i] = 0;

  char *opt_for_objcopy[3];
  opt_for_objcopy[0]
    = (char *) xmalloc (strlen ("_binary__start=")
			+ strlen (symbol_name) + strlen (symbols[0]) + 1);
  opt_for_objcopy[1]
    = (char *) xmalloc (strlen ("_binary__end=")
			+ strlen (symbol_name) + strlen (symbols[1]) + 1);
  opt_for_objcopy[2]
    = (char *) xmalloc (strlen ("_binary__size=")
			+ strlen (symbol_name) + strlen (symbols[2]) + 1);
  sprintf (opt_for_objcopy[0], "_binary_%s_start=%s", symbol_name, symbols[0]);
  sprintf (opt_for_objcopy[1], "_binary_%s_end=%s", symbol_name, symbols[1]);
  sprintf (opt_for_objcopy[2], "_binary_%s_size=%s", symbol_name, symbols[2]);

  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, "objcopy");
  obstack_ptr_grow (&argv_obstack, target_so_filename);
  obstack_ptr_grow (&argv_obstack, "--redefine-sym");
  obstack_ptr_grow (&argv_obstack, opt_for_objcopy[0]);
  obstack_ptr_grow (&argv_obstack, "--redefine-sym");
  obstack_ptr_grow (&argv_obstack, opt_for_objcopy[1]);
  obstack_ptr_grow (&argv_obstack, "--redefine-sym");
  obstack_ptr_grow (&argv_obstack, opt_for_objcopy[2]);
  obstack_ptr_grow (&argv_obstack, NULL);
  new_argv = XOBFINISH (&argv_obstack, char **);
  fork_execute (new_argv[0], new_argv, false);
  obstack_free (&argv_obstack, NULL);
  free (symbol_name);
  for (i = 0; i < 3; i++)
    free (opt_for_objcopy[i]);

  return target_so_filename;
}

int
main (int argc, char **argv)
{
  if (atexit (mkoffload_atexit) != 0)
    fatal_error ("atexit failed");

  const char *host_compiler = getenv ("COLLECT_GCC");
  if (!host_compiler)
    fatal_error ("COLLECT_GCC must be set.");

  /* We may be called with all the arguments stored in some file and
     passed with @file.  Expand them into argv before processing.  */
  expandargv (&argc, &argv);

  const char *target_so_filename = prepare_target_image (argc, argv);

  const char *host_descr_filename = generate_host_descr_file (host_compiler);

  /* Perform partial linking for the target image and host side descriptor.
     As a result we'll get a finalized object file with all offload data.  */
  struct obstack argv_obstack;
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, "ld");
  obstack_ptr_grow (&argv_obstack, "-r");
  obstack_ptr_grow (&argv_obstack, host_descr_filename);
  obstack_ptr_grow (&argv_obstack, target_so_filename);
  obstack_ptr_grow (&argv_obstack, "-o");
  obstack_ptr_grow (&argv_obstack, out_obj_filename);
  obstack_ptr_grow (&argv_obstack, NULL);
  char **new_argv = XOBFINISH (&argv_obstack, char **);
  fork_execute (new_argv[0], new_argv, false);
  obstack_free (&argv_obstack, NULL);

  /* Run objcopy on the resultant object file to localize generated symbols
     to avoid conflicting between different DSO and an executable.  */
  obstack_init (&argv_obstack);
  obstack_ptr_grow (&argv_obstack, "objcopy");
  obstack_ptr_grow (&argv_obstack, "-L");
  obstack_ptr_grow (&argv_obstack, symbols[0]);
  obstack_ptr_grow (&argv_obstack, "-L");
  obstack_ptr_grow (&argv_obstack, symbols[1]);
  obstack_ptr_grow (&argv_obstack, "-L");
  obstack_ptr_grow (&argv_obstack, symbols[2]);
  obstack_ptr_grow (&argv_obstack, out_obj_filename);
  obstack_ptr_grow (&argv_obstack, NULL);
  new_argv = XOBFINISH (&argv_obstack, char **);
  fork_execute (new_argv[0], new_argv, false);
  obstack_free (&argv_obstack, NULL);

  return 0;
}
