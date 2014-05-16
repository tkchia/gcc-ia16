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

#include "coi_host.h"

#include "coi_version_asm.h"

#define CYCLE_FREQUENCY     1000000000

#define NUM_KNC_ENGINES	    1


extern char **environ;
pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;


typedef enum
{
  BUFFER_NORMAL,
  BUFFER_MEMORY
} buffer_t;

typedef struct
{
  COI_ISA_TYPE type;
  uint32_t index;
  char dir[PATH_MAX];
} Engine;

typedef struct
{
  char name[NAME_MAX];
  void *ptr;
} Function;

typedef struct
{
  int pipe_host;
  int pipe_target;
} Pipeline;

typedef struct
{
  pid_t pid;
  Engine *engine;
  Function **functions;
  Pipeline *pipeline;
} Process;

typedef struct
{
  buffer_t type;
  char name[NAME_MAX];
  int fd;
  int fd_target;
  uint64_t size;
  void *data;
  void *data_target;
  Process *process;
} Buffer;


extern "C"
{

COIRESULT
SYMBOL_VERSION (COIBufferCopy, 1) (COIBUFFER dest_buffer,
				   COIBUFFER source_buffer,
				   uint64_t dest_offset,
				   uint64_t source_offset,
				   uint64_t length,
				   COI_COPY_TYPE type,
				   uint32_t dependencies_num,
				   const COIEVENT *dependencies,
				   COIEVENT *completion)
{
  COITRACE ("COIBufferCopy");

  /* Convert input arguments.  */
  Buffer *dest = (Buffer *) dest_buffer;
  Buffer *source = (Buffer *) source_buffer;

  /* Features of liboffload.  */
  assert (type == COI_COPY_UNSPECIFIED);

  /* FIXME: Handle dependencies.  */

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Map buffers if needed.  */
  if (dest->data == 0 && dest->type == BUFFER_NORMAL)
    if (COIBufferMap (dest_buffer, 0, dest->size, (COI_MAP_TYPE) 0,
		      0, 0, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;
  if (source->data == 0 && source->type == BUFFER_NORMAL)
    if (COIBufferMap (source_buffer, 0, source->size, (COI_MAP_TYPE) 0,
		      0, 0, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Copy data.  */
  if (source->data != 0 && dest->data != 0)
    memcpy ((void *) ((uintptr_t) dest->data+dest_offset),
	    (void *) ((uintptr_t) source->data+source_offset), length);
  else
    {
      assert (dest->process == source->process);

      Buffer *buffer;
      cmd_t cmd = CMD_BUFFER_COPY;
      Pipeline *pipeline = dest->process->pipeline;

      /* Create intermediary buffer.  */
      if (COIBufferCreate (length, COI_BUFFER_NORMAL, 0, 0, 1,
			   (COIPROCESS*) &dest->process,
			   (COIBUFFER *) &buffer) == COI_ERROR)
	return COI_ERROR;

      /* Copy from source to intermediary buffer.  */
      if (source->data == 0)
	{
	  assert (source->data_target != 0);

	  /* Send data to target.  */
	  write (pipeline->pipe_target, &cmd, sizeof (cmd_t));
	  write (pipeline->pipe_target, &(buffer->data_target), sizeof (void *));
	  write (pipeline->pipe_target, &(source->data_target), sizeof (void *));
	  write (pipeline->pipe_target, &(buffer->size), sizeof (uint64_t));

	  /* Receive data from  target.  */
	  read (pipeline->pipe_host, &cmd, sizeof (cmd_t));
	}
      else
	{
	  if (COIBufferCopy ((COIBUFFER) buffer, source_buffer, 0, source_offset,
			     length, type, 0, 0, 0) == COI_ERROR)
	    return COI_ERROR;
	}

      /* Copy from intermediary buffer to dest. */
      if (dest->data == 0)
	{
	  assert (dest->data_target != 0);

	  /* Send data to target.  */
	  write (pipeline->pipe_target, &cmd, sizeof (cmd_t));
	  write (pipeline->pipe_target, &(dest->data_target), sizeof (void *));
	  write (pipeline->pipe_target, &(buffer->data_target), sizeof (void *));
	  write (pipeline->pipe_target, &(buffer->size), sizeof (uint64_t));

	  /* Receive data from  target.  */
	  read (pipeline->pipe_host, &cmd, sizeof (cmd_t));
	}
      else
	{
	  if (COIBufferCopy (dest_buffer, (COIBUFFER) buffer, dest_offset,
			     0, length, type, 0, 0, 0) == COI_ERROR)
	    return COI_ERROR;
	}

      /* Unmap on target and destroy intermediary buffer.  */
      if (COIBufferDestroy ((COIBUFFER) buffer) == COI_ERROR)
	return COI_ERROR;
    }

  /* Unmap buffers if needed.  */
  if (dest->type == BUFFER_NORMAL)
    if (COIBufferUnmap ((COIMAPINSTANCE) dest, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;
  if (source->type == BUFFER_NORMAL)
    if (COIBufferUnmap ((COIMAPINSTANCE) source, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  /* Prepare out arguments.  */
  /* FIXME: Here are random values.  */
  if (completion != 0)
    {
      completion->opaque[0] = 1;
      completion->opaque[1] = 1;
    }

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferCreate, 1) (uint64_t size,
				     COI_BUFFER_TYPE type,
				     uint32_t flags,
				     const void *init_data,
				     uint32_t processes_num,
				     const COIPROCESS *processes,
				     COIBUFFER *buffer)
{
  COITRACE ("COIBufferCreate");

  char shm_name[NAME_MAX], temp[NAME_MAX];
  cmd_t cmd = CMD_BUFFER_MAP;
  int shm_fd;
  size_t len;
  unsigned long long i;

  Buffer *buf;
  Pipeline *pipeline;

  /* Features of liboffload.  */
  assert (type == COI_BUFFER_NORMAL);
  assert ((flags & COI_SINK_MEMORY) == 0);
  assert ((flags & COI_SAME_ADDRESS_SINKS) == 0);
  assert ((flags & COI_SAME_ADDRESS_SINKS_AND_SOURCE) == 0);
  assert (init_data == 0);
  assert (processes_num == 1);

  /* Create shared memory with an unique name.  */
  for (i = 0; i >= 0; i++)
    {
      strcpy (shm_name, "/knc_offload_shm_");
      sprintf (temp, "%lu", i);
      strcat (shm_name, temp);
      shm_fd = shm_open (shm_name, O_CREAT | O_EXCL | O_RDWR,
			 S_IRUSR | S_IWUSR);
      if (shm_fd > 0)
	break;
    }
  ftruncate (shm_fd, size);

  /* Create buffer.  */
  buf = (Buffer *) malloc (sizeof (Buffer));
  buf->data = 0;
  buf->fd = shm_fd;
  buf->process = (Process *) processes[0];
  buf->size = size;
  buf->type = BUFFER_NORMAL;
  strcpy (buf->name, shm_name);

  /* Map buffer on target.  */
  len = strlen (buf->name) + 1;
  pipeline = buf->process->pipeline;

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Send data to target.  */
  write (pipeline->pipe_target, &cmd, sizeof (cmd_t));
  write (pipeline->pipe_target, &len, sizeof (size_t));
  write (pipeline->pipe_target, buf->name, len);
  write (pipeline->pipe_target, &(buf->size), sizeof (uint64_t));

  /* Receive data from  target.  */
  read (pipeline->pipe_host, &(buf->fd_target), sizeof (int));
  read (pipeline->pipe_host, &(buf->data_target), sizeof (void *));

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  /* Prepare output arguments.  */
  *buffer = (COIBUFFER) buf;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferCreateFromMemory, 1) (uint64_t size,
					       COI_BUFFER_TYPE type,
					       uint32_t flags,
					       void *memory,
					       uint32_t processes_num,
					       const COIPROCESS *processes,
					       COIBUFFER *buffer)
{
  COITRACE ("COIBufferCreateFromMemory");

  Buffer *buf;

  /* Features of liboffload.  */
  assert (type == COI_BUFFER_NORMAL);
  assert ((flags & COI_SAME_ADDRESS_SINKS) == 0);
  assert ((flags & COI_SAME_ADDRESS_SINKS_AND_SOURCE) == 0);
  assert (processes_num == 1);

  /* Create buffer.  */
  buf = (Buffer *) malloc (sizeof (Buffer));
  buf->data = (flags & COI_SINK_MEMORY) == 0 ? memory : 0;
  buf->data_target = (flags & COI_SINK_MEMORY) != 0 ? memory : 0;
  buf->process = (Process *) processes[0];
  buf->size = size;
  buf->type = BUFFER_MEMORY;

  /* Prepare output argument.  */
  *buffer = (COIBUFFER) buf;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferDestroy, 1) (COIBUFFER buffer)
{
  COITRACE ("COIBufferDestroy");

  cmd_t cmd = CMD_BUFFER_UNMAP;

  /* Convert input arguments.  */
  Buffer *buf = (Buffer *) buffer;
  Pipeline *pipeline = buf->process->pipeline;

  /* Unmap buffer on host.  */
  if (buf->data != 0 && buf->type == BUFFER_NORMAL)
    if (COIBufferUnmap ((COIMAPINSTANCE) buffer, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Unmap buffer on target.  */
  if (buf->data_target != 0)
    {
      /* Start critical section.  */
      pthread_mutex_lock (&mutex);

      /* Send data to target.  */
      write (pipeline->pipe_target, &cmd, sizeof (cmd_t));
      write (pipeline->pipe_target, &(buf->fd_target), sizeof (int));
      write (pipeline->pipe_target, &(buf->data_target), sizeof (void *));
      write (pipeline->pipe_target, &(buf->size), sizeof (uint64_t));

      /* Receive data from  target.  */
      read (pipeline->pipe_host, &cmd, sizeof (cmd_t));

      /* Finish critical section.  */
      pthread_mutex_unlock (&mutex);
    }

  /* Unlink shared memory.  */
  if (buf->type == BUFFER_NORMAL)
    {
      close (buf->fd);
      shm_unlink (buf->name);
    }

  /* Free data.  */
  free (buf);

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferGetSinkAddress, 1) (COIBUFFER buffer,
					     uint64_t *data)
{
  COITRACE ("COIBufferGetSinkAddress");

  /* Convert input arguments.  */
  Buffer *buf = (Buffer *) buffer;

  /* Here should come BUFFER_NORMAL buffer.  */
  assert (buf->type == BUFFER_NORMAL);

  /* Prepare output argument.  */
  *data = (uint64_t) buf->data_target;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferMap, 1) (COIBUFFER buffer,
				  uint64_t offset,
				  uint64_t length,
				  COI_MAP_TYPE type,		    // Ignored
				  uint32_t dependencies_num,
				  const COIEVENT *dependencies,
				  COIEVENT *completion,
				  COIMAPINSTANCE *map_instance,
				  void **data)
{
  COITRACE ("COIBufferMap");

  /* Features of liboffload.  */
  assert (offset == 0);
  assert (dependencies_num == 0);
  assert (dependencies == 0);
  assert (completion == 0);

  /* Convert input arguments.  */
  Buffer *buf = (Buffer *) buffer;

  /* Only BUFFER_NORMAL buffers should come here.  */
  assert (buf->type == BUFFER_NORMAL);

  /* Map shared memory.  */
  buf->data = mmap (NULL, buf->size, PROT_READ | PROT_WRITE,
		    MAP_SHARED, buf->fd, 0);
  if (buf->data == NULL)
    COIERROR ("Cannot map shared memory.");

  /* Prepare output arguments.  */
  if (map_instance != 0)
    *map_instance = (COIMAPINSTANCE) buf;
  if (data != 0)
    *data = buf->data;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferRead, 1) (COIBUFFER buffer,
				   uint64_t offset,
				   void *data,
				   uint64_t length,
				   COI_COPY_TYPE type,
				   uint32_t dependencies_num,
				   const COIEVENT *dependencies,
				   COIEVENT *completion)
{
  COITRACE ("COIBufferRead");

  /* Convert input arguments.  */
  Buffer *buf = (Buffer *) buffer;

  /* Features of liboffload.  */
  assert (type == COI_COPY_UNSPECIFIED);

  /* FIXME: Handle dependencies.  */

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Map buffers if needed.  */
  if (buf->data == 0 && buf->type == BUFFER_NORMAL)
    if (COIBufferMap (buffer, 0, buf->size, (COI_MAP_TYPE) 0,
		      0, 0, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Copy data.  */
  memcpy (data, (void *) ((uintptr_t) buf->data+offset), length);

  /* Unmap buffers if needed.  */
  if (buf->type == BUFFER_NORMAL)
    if (COIBufferUnmap ((COIMAPINSTANCE) buf, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  /* Prepare out arguments.  */
  /* FIXME: Here are random values.  */
  if (completion != 0)
    {
      completion->opaque[0] = 3;
      completion->opaque[1] = 3;
    }

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferSetState, 1) (COIBUFFER buffer,
				       COIPROCESS process,
				       COI_BUFFER_STATE state,
				       COI_BUFFER_MOVE_FLAG flag,
				       uint32_t dependencies_num,
				       const COIEVENT *dependencies,
				       COIEVENT *completion)
{
  COITRACE ("COIBufferSetState");

  /* Features of liboffload.  */
  assert (flag == COI_BUFFER_NO_MOVE);
  assert (dependencies_num == 0);
  assert (dependencies == 0);
  assert (completion == 0);

  /* Looks like we have nothing to do here.  */

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferUnmap, 1) (COIMAPINSTANCE map_instance,
				    uint32_t dependencies_num,
				    const COIEVENT *dependencies,
				    COIEVENT *completion)
{
  COITRACE ("COIBufferUnmap");

  /* Features of liboffload.  */
  assert (dependencies_num == 0);
  assert (dependencies == 0);
  assert (completion == 0);

  /* Convert input arguments.  */
  Buffer *buffer = (Buffer *) map_instance;

  /* Only BUFFER_NORMAL buffers should come here.  */
  assert (buffer->type == BUFFER_NORMAL);

  /* Unmap shared memory.  */
  if (munmap (buffer->data, buffer->size) < 0)
    COIERROR ("Cannot unmap shared memory.");

  buffer->data = 0;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIBufferWrite, 1) (COIBUFFER buffer,
				    uint64_t offset,
				    const void *data,
				    uint64_t length,
				    COI_COPY_TYPE type,
				    uint32_t dependencies_num,
				    const COIEVENT *dependencies,
				    COIEVENT *completion)
{
  COITRACE ("COIBufferWrite");

  /* Convert input arguments.  */
  Buffer *buf = (Buffer *) buffer;

  /* Features of liboffload.  */
  assert (type == COI_COPY_UNSPECIFIED);
  assert (dependencies_num == 0);
  assert (dependencies == 0);

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Map buffers if needed.  */
  if (buf->data == 0 && buf->type == BUFFER_NORMAL)
    if (COIBufferMap (buffer, 0, buf->size, (COI_MAP_TYPE) 0,
		      0, 0, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Copy data.  */
  memcpy ((void *) ((uintptr_t) buf->data+offset), data, length);

  /* Unmap buffers if needed.  */
  if (buf->type == BUFFER_NORMAL)
    if (COIBufferUnmap ((COIMAPINSTANCE) buf, 0, 0, 0) == COI_ERROR)
      return COI_ERROR;

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  /* Prepare out arguments.  */
  /* FIXME: Here are random values.  */
  if (completion != 0)
    {
      completion->opaque[0] = 2;
      completion->opaque[1] = 2;
    }

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIEngineGetCount, 1) (COI_ISA_TYPE isa,
				       uint32_t *count)
{
  COITRACE ("COIEngineGetCount");

  /* Features of liboffload.  */
  assert (isa == COI_ISA_KNC);

  /* Prepare output arguments.  */
  *count = NUM_KNC_ENGINES;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIEngineGetHandle, 1) (COI_ISA_TYPE isa,
					uint32_t index,
					COIENGINE *handle)
{
  COITRACE ("COIEngineGetHandle");

  Engine *engine;

  /* Features of liboffload.  */
  assert (isa == COI_ISA_KNC);

  /* Check engine index.  */
  if (index >= NUM_KNC_ENGINES)
    COIERROR ("Wrong engine index.");

  /* Create engine handle.  */
  engine = (Engine *) malloc (sizeof (Engine));
  engine->type = isa;
  engine->index = index;
  strcpy (engine->dir, "/tmp/knc_offload_XXXXXX");

  /* Prepare output argument.  */
  *handle = (COIENGINE) engine;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIEventWait, 1) (uint16_t events_num,
				  const COIEVENT *events,
				  int32_t timeout,
				  uint8_t wait_all,
				  uint32_t *signaled_num,
				  uint32_t *signaled_indices)
{
  COITRACE ("COIEventWait");

  /* Features of liboffload.  */
  assert (wait_all == 1);
  assert (signaled_num == 0);
  assert (signaled_indices == 0);

  /* FIXME: Handle dependencies.  */

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIPipelineCreate, 1) (COIPROCESS process,
				       COI_CPU_MASK mask,
				       uint32_t stack_size,       // Ignored
				       COIPIPELINE *pipeline)
{
  COITRACE ("COIPipelineCreate");

  /* Features of liboffload.  */
  assert (mask == 0);

  /* Prepare output arguments.  */
  *pipeline = (COIPIPELINE) ((Process *) process)->pipeline;

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIPipelineDestroy, 1) (COIPIPELINE pipeline)
{
  COITRACE ("COIPipelineDestroy");

  assert (false);

  return COI_ERROR;
}


COIRESULT
SYMBOL_VERSION (COIPipelineRunFunction, 1) (COIPIPELINE pipeline,
					    COIFUNCTION function,
					    uint32_t buffers_num,
					    const COIBUFFER *buffers,
					    const COI_ACCESS_FLAGS *access_flags,  // Ignored
					    uint32_t dependencies_num,
					    const COIEVENT *dependencies,
					    const void *misc_data,
					    uint16_t misc_data_len,
					    void *return_data,
					    uint16_t return_data_len,
					    COIEVENT *completion)
{
  COITRACE ("COIPipelineRunFunction");

  cmd_t cmd = CMD_RUN_FUNCTION;
  size_t len;
  uint32_t i;
  uint64_t size;
  void *ptr;

  /* Convert input arguments.  */
  Buffer **bufs = (Buffer **) buffers;
  Function *func = (Function *) function;
  Pipeline *pipe = (Pipeline *) pipeline;

  /* FIXME: Handle dependencies.  */

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Send data to target.  */
  write (pipe->pipe_target, &cmd, sizeof (cmd_t));
  write (pipe->pipe_target, &(func->ptr), sizeof (void *));
  write (pipe->pipe_target, &buffers_num, sizeof (uint32_t));
  for (i = 0; i < buffers_num; i++)
    {
      len = strlen (bufs[i]->name) + 1;
      write (pipe->pipe_target, &len, sizeof (size_t));
      write (pipe->pipe_target, bufs[i]->name, len);
      write (pipe->pipe_target, &(bufs[i]->size), sizeof (uint64_t));
      write (pipe->pipe_target, &(bufs[i]->data_target), sizeof (void *));
    }
  write (pipe->pipe_target, &misc_data_len, sizeof (uint16_t));
  if (misc_data_len > 0)
    write (pipe->pipe_target, misc_data, misc_data_len);
  write (pipe->pipe_target, &return_data_len, sizeof (uint16_t));

  /* Receive data from target.  In emulator we don't need any asynchronous data
     transfer, so we wait for target process whether it has any data or not.  */
  if (return_data_len > 0)
    read (pipe->pipe_host, return_data, return_data_len);
  else
    read (pipe->pipe_host, &cmd, sizeof (cmd_t));

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  /* Prepare out arguments.  */
  /* FIXME: Here are random values.  */
  if (completion != 0)
    {
      completion->opaque[0] = 0;
      completion->opaque[1] = 0;
    }

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIProcessCreateFromMemory, 1) (COIENGINE engine,
						const char *bin_name,
						const void *bin_buffer,
						uint64_t bin_buffer_len,
						int argc,
						const char **argv,
						uint8_t inherit_env,
						const char **additional_env,
						uint8_t proxy_active,		  // Ignored
						const char *proxyfs_root,	  // Ignored
						uint64_t buffer_space,		  // Ignored
						const char *lib_search_path,
						const char *file_of_origin,       // Ignored
						uint64_t file_of_origin_offset,   // Ignored
						COIPROCESS *process)
{
  COITRACE ("COIProcessCreateFromMemory");

  char **envp;
  char *run_argv[1024];
  char *run_sde = getenv (OFFLOAD_RUN_SDE_ENV);
  char *tok;
  char idx_str[10];
  char pipes_path[PATH_MAX], target_exe[PATH_MAX];
  char pipe_host_path[PATH_MAX], pipe_target_path[PATH_MAX];
  FILE *file;
  int i, env_i, env_num;
  int pipe_host, pipe_target;
  pid_t pid;
  Pipeline *pipeline;
  Process *proc;

  /* Features of liboffload.  */
  assert (argc == 0);
  assert (argv == 0);

  /* Convert input arguments.  */
  Engine *eng = (Engine *) engine;

  /* Create temporary directory for engine files.  */
  if (mkdtemp (eng->dir) == NULL)
    COIERROR ("Cannot create temporary directory %s.", eng->dir);

  /* Create target executable file.  */
  strcpy (target_exe, eng->dir);
  strcat (target_exe, "/");
  strcat (target_exe, bin_name);
  file = fopen (target_exe, "wb");
  if (file == NULL)
    COIERROR ("Cannot create file %s.", target_exe);
  if (fwrite (bin_buffer, 1, bin_buffer_len, file) != bin_buffer_len)
    COIERROR ("Cannot write in file %s.", target_exe);
  fclose (file);

  /* Fix file permissions.  */
  if (chmod (target_exe, S_IRWXU) < 0)
    COIERROR ("Cannot change permissions for file %s.", target_exe);

  /* Create directory for pipes to prevent names collision.  */
  strcpy (pipes_path, eng->dir);
  strcat (pipes_path, PIPES_PATH);
  if (mkdir (pipes_path, S_IRWXU) < 0)
    COIERROR ("Cannot create folder %s.", pipes_path);

  /* Create pipes.  */
  strcpy (pipe_host_path, eng->dir);
  strcat (pipe_host_path, PIPE_HOST_PATH);
  strcpy (pipe_target_path, eng->dir);
  strcat (pipe_target_path, PIPE_TARGET_PATH);
  if (mkfifo (pipe_host_path, S_IRUSR | S_IWUSR) < 0)
    COIERROR ("Cannot create pipe %s.", pipe_host_path);
  if (mkfifo (pipe_target_path, S_IRUSR | S_IWUSR) < 0)
    COIERROR ("Cannot create pipe %s.", pipe_target_path);

  /* Prepare argv.  */
  if (run_sde == NULL || strcmp (run_sde, "") == 0)
    {
      run_argv[0] = strdup (target_exe);
      run_argv[1] = (char *) NULL;
    }
  else
    {
      i = 0;
      tok = strtok (run_sde, " ");
      while (tok != NULL)
	{
	  run_argv[i++] = strdup (tok);
	  tok = strtok (NULL, " ");
	}
      run_argv[i] = strdup (target_exe);
      run_argv[i+1] = (char *) NULL;
    }

  /* Prepare envp.  */
  /* FIXME: take into account additional_env.  */
  assert (additional_env == NULL);

  env_num = 0;
  if (inherit_env == true)
    while (environ[env_num++]);
  env_num += 4; // LD_LIBRARY_PATH, MIC_DIR, MIC_INDEX, NULL

  envp = (char **) malloc (env_num * sizeof (char *));

  env_i = 0;
  if (inherit_env == true)
    for (i = 0; environ[i] != NULL; i++)
      {
	char *name = strdup (environ[i]);
	int j;
	for (j = 0; name[j] != '=' && name[j] != '\0';j++);
	name[j] = '\0';
	if (strcmp (name, "LD_LIBRARY_PATH") != 0
	    && strcmp (name, MIC_DIR_ENV) != 0
	    && strcmp (name, MIC_INDEX_ENV) != 0)
	  envp[env_i++] = strdup (environ[i]);
	free (name);
      }

  envp[env_i] = (char *) malloc (strlen (MIC_DIR_ENV) + strlen (eng->dir) + 2);
  strcpy (envp[env_i], MIC_DIR_ENV);
  strcat (envp[env_i], "=");
  strcat (envp[env_i], eng->dir);

  sprintf (idx_str, "%u", eng->index);
  envp[env_i+1] = (char *) malloc (strlen (MIC_INDEX_ENV) + strlen (idx_str) + 2);
  strcpy (envp[env_i+1], MIC_INDEX_ENV);
  strcat (envp[env_i+1], "=");
  strcat (envp[env_i+1], idx_str);

  envp[env_i+2] = (char *) malloc (strlen ("LD_LIBRARY_PATH=")
			     + strlen (lib_search_path) + 1);
  strcpy (envp[env_i+2], "LD_LIBRARY_PATH=");
  strcat (envp[env_i+2], lib_search_path);

  envp[env_i+3] = (char *) NULL;

  /* Create target process.  */
  pid = vfork ();
  if (pid < 0)
    COIERROR ("Cannot create child process.");

  if (pid == 0)
    {
      /* Run target executable.  */
      if (execvpe (run_argv[0], run_argv, envp) == -1)
	COIERROR ("Cannot execute file %s.", target_exe);
    }

  /* Open pipes.  */
  pipe_host = open (pipe_host_path, O_RDONLY);
  pipe_target = open (pipe_target_path, O_WRONLY);
  if (pipe_host < 0 || pipe_target < 0)
    COIERROR ("Cannot open pipes.");

  /* Create pipeline handle.  */
  pipeline = (Pipeline *) malloc (sizeof (Pipeline));
  pipeline->pipe_host = pipe_host;
  pipeline->pipe_target = pipe_target;

  /* Create process handle.  */
  proc = (Process *) malloc (sizeof (Process));
  proc->pid = pid;
  proc->engine = eng;
  proc->functions = 0;
  proc->pipeline = pipeline;

  /* Prepare output arguments.  */
  *process = (COIPROCESS) proc;

  /* Clean up.  */
  for (i = 0; run_argv[i] != NULL; i++)
    free (run_argv[i]);
  for (i = 0; envp[i] != NULL; i++)
    free (envp[i]);
  free (envp);

  return COI_SUCCESS;
}


/* Helper function for directory removing.  */
static void remove_directory (char *path)
{
  char *file;
  DIR *dir = opendir (path);
  struct dirent *entry;
  struct stat statfile;

  while (entry = readdir (dir))
    {
      if (!strcmp (entry->d_name, ".") || !strcmp (entry->d_name, ".."))
	continue;

      file = (char *) malloc (strlen (path) + strlen (entry->d_name) + 2);

      strcpy (file, path);
      strcat (file, "/");
      strcat (file, entry->d_name);

      stat (file, &statfile);

      if (S_ISDIR (statfile.st_mode))
	remove_directory (file);
      else
	unlink (file);

      free (file);
    }

  closedir (dir);
  rmdir (path);
}

COIRESULT
SYMBOL_VERSION (COIProcessDestroy, 1) (COIPROCESS process,
				       int32_t wait_timeout,      // Ignored
				       uint8_t force,
				       int8_t *proc_return,
				       uint32_t *reason)
{
  COITRACE ("COIProcessDestroy");

  int i;

  /* Convert input arguments.  */
  Process *proc = (Process *) process;

  /* Close pipes.  */
  /* FIXME: Shouldn't we call here PipelineDestroy?  */
  close (proc->pipeline->pipe_host);
  close (proc->pipeline->pipe_target);
  free (proc->pipeline);

  /* Closing pipes should be enough to finish target process.
     But if 'force' > 0 we will do it by force.  */
  if (force)
    kill (proc->pid, SIGTERM);

  /* Remove directory with engine files.  */
  remove_directory (proc->engine->dir);

  /* Free data.  */
  for (i = 0; proc->functions[i] != 0; i++)
    free (proc->functions[i]);
  free (proc->engine);
  free (proc->functions);
  free (proc);

  /* Prepare output arguments.  */
  *proc_return = 0;
  *reason = 0;

  return COI_ERROR;
}


COIRESULT
SYMBOL_VERSION (COIProcessGetFunctionHandles, 1) (COIPROCESS process,
						  uint32_t functions_num,
						  const char **function_names,
						  COIFUNCTION *function_handles)
{
  COITRACE ("COIProcessGetFunctionHandles");

  cmd_t cmd = CMD_GET_FUNCTION_HANDLE;
  size_t len;
  void *ptr;
  uint32_t i;

  /* Convert input arguments.  */
  Process *proc = (Process *) process;

  /* This function should be called once for the process.  */
  assert (proc->functions == 0);

  /* Create array of function pointers.  Last element is 0, what shows
     the end of the array.  This array is used to free memory when process
     is destroyed.  */
  proc->functions = (Function **) calloc (functions_num + 1,
					  sizeof (Function *));

  /* Get handles for functions.  */
  for (i = 0; i < functions_num; i++)
    {
      Function *function = (Function *) malloc (sizeof (Function));
      len = strlen (function_names[i]) + 1;

      /* Start critical section.  */
      pthread_mutex_lock (&mutex);

      /* Send data to target.  */
      write (proc->pipeline->pipe_target, &cmd, sizeof (cmd_t));
      write (proc->pipeline->pipe_target, &len, sizeof (size_t));
      write (proc->pipeline->pipe_target, function_names[i], len);

      /* Receive data from  target.  */
      read (proc->pipeline->pipe_host, &ptr, sizeof (void *));

      /* Finish critical section.  */
      pthread_mutex_unlock (&mutex);

      /* Prepare output arguments.  */
      strcpy (function->name, function_names[i]);
      function->ptr = ptr;
      function_handles[i] = (COIFUNCTION) function;

      /* Save function pointer.  */
      proc->functions[i] = function;
    }

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIProcessLoadLibraryFromMemory, 2) (COIPROCESS process,
						     const void *lib_buffer,
						     uint64_t lib_buffer_len,
						     const char *lib_name,
						     const char *lib_search_path,
						     const char *file_of_origin,	// Ignored
						     uint64_t file_from_origin_offset,  // Ignored
						     uint32_t flags,			// Ignored
						     COILIBRARY *library)		// Ignored
{
  COITRACE ("COIProcessLoadLibraryFromMemory");

  char lib_path[PATH_MAX];
  cmd_t cmd = CMD_OPEN_LIBRARY;
  FILE *file;
  size_t len;

  /* Convert input arguments.  */
  Process *proc = (Process *) process;

  /* Create target library file.  */
  strcpy (lib_path, proc->engine->dir);
  strcat (lib_path, "/");
  strcat (lib_path, lib_name);
  file = fopen (lib_path, "wb");
  if (file == NULL)
    COIERROR ("Cannot create file %s.", lib_path);
  if (fwrite (lib_buffer, 1, lib_buffer_len, file) != lib_buffer_len)
    COIERROR ("Cannot write in file %s.", lib_path);
  fclose (file);

  len = strlen (lib_path) + 1;

  /* Start critical section.  */
  pthread_mutex_lock (&mutex);

  /* Make target open library.  */
  write (proc->pipeline->pipe_target, &cmd, sizeof (cmd_t));
  write (proc->pipeline->pipe_target, &len, sizeof (size_t));
  write (proc->pipeline->pipe_target, lib_path, len);

  /* Finish critical section.  */
  pthread_mutex_unlock (&mutex);

  return COI_SUCCESS;
}


COIRESULT
SYMBOL_VERSION (COIProcessRegisterLibraries, 1) (uint32_t libraries_num,
						 const void **libraries,
						 const uint64_t *library_sizes,
						 const char **files_of_origin,
						 const uint64_t *file_of_origin_offsets)
{
  COITRACE ("COIProcessRegisterLibraries");

  /* Looks like we have nothing to do here.  */

  return COI_SUCCESS;
}


uint64_t
SYMBOL_VERSION (COIPerfGetCycleFrequency, 1) ()
{
  COITRACE ("COIPerfGetCycleFrequency");

  return (uint64_t) CYCLE_FREQUENCY;
}

} // extern "C"

