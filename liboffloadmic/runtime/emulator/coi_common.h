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

#ifndef COI_COMMON_H_INCLUDED
#define COI_COMMON_H_INCLUDED

#include <common/COIMacros_common.h>
#include <common/COIPerf_common.h>
#include <source/COIEngine_source.h>
#include <source/COIProcess_source.h>
#include <source/COIPipeline_source.h>
#include <source/COIBuffer_source.h>
#include <source/COIEvent_source.h>

#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* Environment variable for path to 'target' files.  */
#define MIC_DIR_ENV	      "OFFLOAD_MIC_DIR"

/* Environment variable for engine index.  */
#define MIC_INDEX_ENV	      "OFFLOAD_MIC_INDEX"

/* Environment variable for SDE run command.  */
#define OFFLOAD_RUN_SDE_ENV   "OFFLOAD_RUN_SDE"

/* Relative path to directory with pipes.  */
#define PIPES_PATH	      "/pipes"
/* Relative path to pipe (target -> host).  */
#define PIPE_HOST_PATH	      PIPES_PATH "/host"
/* Relative path to pipe (host -> target).  */
#define PIPE_TARGET_PATH      PIPES_PATH "/target"

#define NAME_MAX	      256
#define PATH_MAX	      256

/* Command codes enum.  */
typedef enum
{
  CMD_BUFFER_COPY,
  CMD_BUFFER_MAP,
  CMD_BUFFER_UNMAP,
  CMD_GET_FUNCTION_HANDLE,
  CMD_OPEN_LIBRARY,
  CMD_RUN_FUNCTION,
  CMD_SHUTDOWN
} cmd_t;

#endif // COI_COMMON_H_INCLUDED
