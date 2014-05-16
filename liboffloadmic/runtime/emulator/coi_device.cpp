/*
    Copyright (c) 2014 Intel Corporation.  All Rights Reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of Intel Corporation nor the names of its
        contributors may be used to endorse or promote products derived
        from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "coi_device.h"

#include "coi_version_asm.h"

#define CYCLE_FREQUENCY     1000000000


static uint32_t engine_index;


extern "C"
{

COIRESULT
SYMBOL_VERSION (COIBufferAddRef, 1) (void *ptr)
{
  COITRACE ("COIBufferAddRef");

  /* Looks like we have nothing to do here.  */

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferReleaseRef, 1) (void *ptr)
{
  COITRACE ("COIBufferReleaseRef");

  /* Looks like we have nothing to do here.  */

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIEngineGetIndex, 1) (COI_ISA_TYPE *type,
				       uint32_t *index)
{
  COITRACE ("COIEngineGetIndex");

  /* type is not used in liboffload.  */
  *index = engine_index;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIPipelineStartExecutingRunFunctions, 1) ()
{
  COITRACE ("COIPipelineStartExecutingRunFunctions");

  /* Looks like we have nothing to do here.  */

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIProcessWaitForShutdown, 1) ()
{
  COITRACE ("COIProcessWaitForShutdown");

  char *mic_dir = getenv (MIC_DIR_ENV);
  char *mic_index = getenv (MIC_INDEX_ENV);
  char pipe_host_path[PATH_MAX], pipe_target_path[PATH_MAX];
  int pipe_host, pipe_target;
  pid_t ppid = getppid ();
  cmd_t cmd;

  /* Get engine index.  */
  engine_index = atoi (mic_index);

  /* Open pipes.  */
  strcpy (pipe_host_path, mic_dir);
  strcat (pipe_host_path, PIPE_HOST_PATH);
  strcpy (pipe_target_path, mic_dir);
  strcat (pipe_target_path, PIPE_TARGET_PATH);
  pipe_host = open (pipe_host_path, O_WRONLY);
  pipe_target = open (pipe_target_path, O_RDONLY);
  if (pipe_host < 0 || pipe_target < 0)
    COIERROR ("Cannot open pipes.");

  /* Handler.  */
  while (1)
    {
      /* Read and execute command.  */
      cmd = CMD_SHUTDOWN;
      read (pipe_target, &cmd, sizeof (cmd_t));

      switch (cmd)
	{
	case CMD_BUFFER_COPY:
	  {
	    uint64_t len;
	    void *dest, *source;

	    /* Receive data from host.  */
	    read (pipe_target, &dest, sizeof (void *));
	    read (pipe_target, &source, sizeof (void *));
	    read (pipe_target, &len, sizeof (uint64_t));

	    /* Copy.  */
	    memcpy (dest, source, len);

	    /* Notify host about completion.  */
	    write (pipe_host, &cmd, sizeof (cmd_t));

	    break;
	  }
	case CMD_BUFFER_MAP:
	  {
	    char name[NAME_MAX];
	    int fd;
	    size_t len;
	    uint64_t buffer_len;
	    void *buffer;

	    /* Receive data from host.  */
	    read (pipe_target, &len, sizeof (size_t));
	    read (pipe_target, name, len);
	    read (pipe_target, &buffer_len, sizeof (uint64_t));

	    /* Open shared memory.  */
	    fd = shm_open (name, O_RDWR, S_IRUSR | S_IWUSR);
	    if (fd < 0)
	      COIERROR ("Cannot open shared memory.");

	    /* Map shared memory.  */
	    buffer = mmap (NULL, buffer_len, PROT_READ | PROT_WRITE,
			   MAP_SHARED, fd, 0);
	    if (buffer == NULL)
	      COIERROR ("Cannot map shared memory.");

	    /* Send data to host.  */
	    write (pipe_host, &fd, sizeof (int));
	    write (pipe_host, &buffer, sizeof (void *));

	    break;
	  }
	case CMD_BUFFER_UNMAP:
	  {
	    int fd;
	    uint64_t buffer_len;
	    void *buffer;

	    /* Receive data from host.  */
	    read (pipe_target, &fd, sizeof (int));
	    read (pipe_target, &buffer, sizeof (void *));
	    read (pipe_target, &buffer_len, sizeof (uint64_t));

	    /* Unmap buffer.  */
	    if (munmap (buffer, buffer_len) < 0)
	      COIERROR ("Cannot unmap shared memory.");

	    /* Close shared memory.  */
	    close (fd);

	    /* Notify host about completion.  */
	    write (pipe_host, &cmd, sizeof (cmd_t));

	    break;
	  }
	case CMD_GET_FUNCTION_HANDLE:
	  {
	    char fname[NAME_MAX];
	    size_t len;
	    void *ptr;

	    /* Receive data from host.  */
	    read (pipe_target, &len, sizeof (size_t));
	    read (pipe_target, fname, len);

	    /* Find function.  */
	    ptr = dlsym (RTLD_DEFAULT, fname);
	    if (ptr == NULL)
	      COIERROR ("Cannot find symbol %s.", fname);

	    /* Send data to host.  */
	    write (pipe_host, &ptr, sizeof (void *));

	    break;
	  }
	case CMD_OPEN_LIBRARY:
	  {
	    char lib_path[PATH_MAX];
	    size_t len;

	    /* Receive data from host.  */
	    read (pipe_target, &len, sizeof (size_t));
	    read (pipe_target, lib_path, len);

	    /* Open library.  */
	    if (dlopen (lib_path, RTLD_LAZY | RTLD_GLOBAL) == 0)
	      COIERROR ("Cannot load %s: %s", lib_path, dlerror ());

	    break;
	  }
	case CMD_RUN_FUNCTION:
	  {
	    char name[NAME_MAX];
	    size_t len;
	    uint16_t misc_data_len, return_data_len;
	    uint32_t buffer_count, i;
	    uint64_t *buffers_len, size;
	    void *ptr;
	    void **buffers, *misc_data, *return_data;

	    void (*func) (uint32_t, void **, uint64_t *,
			  void *, uint16_t, void*, uint16_t);

	    /* Receive data from host.  */
	    read (pipe_target, &func, sizeof (void *));
	    read (pipe_target, &buffer_count, sizeof (uint32_t));
	    buffers = (void **) malloc (buffer_count * sizeof (void *));
	    buffers_len = (uint64_t *) malloc (buffer_count
					       * sizeof (uint64_t));
	    for (i = 0; i < buffer_count; i++)
	      {
		read (pipe_target, &len, sizeof (size_t));
		read (pipe_target, name, len);
		read (pipe_target, &(buffers_len[i]), sizeof (uint64_t));
		read (pipe_target, &(buffers[i]), sizeof (void *));
	      }
	    read (pipe_target, &misc_data_len, sizeof (uint16_t));
	    if (misc_data_len > 0)
	      {
		misc_data = malloc (misc_data_len);
		read (pipe_target, misc_data, misc_data_len);
	      }
	    read (pipe_target, &return_data_len, sizeof (uint16_t));
	    if (return_data_len > 0)
	      return_data = malloc (return_data_len);

	    /* Run function.  */
	    func (buffer_count, buffers, buffers_len, misc_data,
		  misc_data_len, return_data, return_data_len);

	    /* Send data to host if any or just send notification.  */
	    if (return_data_len > 0)
	      write (pipe_host, return_data, return_data_len);
	    else
	      write (pipe_host, &cmd, sizeof (cmd_t));

	    /* Clean up.  */
	    free (buffers);
	    free (buffers_len);
	    if (misc_data_len > 0)
	      free (misc_data);
	    if (return_data_len > 0)
	      free (return_data);

	    break;
	  }
	case CMD_SHUTDOWN:
	  close (pipe_host);
	  close (pipe_target);
	  return COI_SUCCESS;
	default:
	  COIERROR ("Unrecognizable command from host.");
	}
    }

  return COI_ERROR;
}



uint64_t
SYMBOL_VERSION (COIPerfGetCycleFrequency, 1) ()
{
  COITRACE ("COIPerfGetCycleFrequency");

  return (uint64_t) CYCLE_FREQUENCY;
}

} // extern "C"

