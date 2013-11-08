/* Copyright (C) 2013-2014 Free Software Foundation, Inc.
   Contributed by Jakub Jelinek <jakub@redhat.com>.

   This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

/* This file handles the maintainence of threads in response to team
   creation and termination.  */

#include "libgomp.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef PLUGIN_SUPPORT
# include <dlfcn.h>
# include <dirent.h>
#endif

static void gomp_target_init (void);

static pthread_once_t gomp_is_initialized = PTHREAD_ONCE_INIT;

/* Forward declaration for a node in the tree.  */
typedef struct splay_tree_node_s *splay_tree_node;
typedef struct splay_tree_s *splay_tree;
typedef struct splay_tree_key_s *splay_tree_key;

struct target_mem_desc {
  /* Reference count.  */
  uintptr_t refcount;
  /* All the splay nodes allocated together.  */
  splay_tree_node array;
  /* Start of the target region.  */
  uintptr_t tgt_start;
  /* End of the targer region.  */
  uintptr_t tgt_end;
  /* Handle to free.  */
  void *to_free;
  /* Previous target_mem_desc.  */
  struct target_mem_desc *prev;
  /* Number of items in following list.  */
  size_t list_count;

  /* Corresponding target device descriptor.  */
  struct gomp_device_descr *device_descr;

  /* List of splay keys to remove (or decrease refcount)
     at the end of region.  */
  splay_tree_key list[];
};

struct splay_tree_key_s {
  /* Address of the host object.  */
  uintptr_t host_start;
  /* Address immediately after the host object.  */
  uintptr_t host_end;
  /* Descriptor of the target memory.  */
  struct target_mem_desc *tgt;
  /* Offset from tgt->tgt_start to the start of the target object.  */
  uintptr_t tgt_offset;
  /* Reference count.  */
  uintptr_t refcount;
  /* True if data should be copied from device to host at the end.  */
  bool copy_from;
};

/* Array of descriptors of all available devices.  */
static struct gomp_device_descr *devices;

/* Total number of available devices.  */
static int num_devices;

/* The comparison function.  */

static int
splay_compare (splay_tree_key x, splay_tree_key y)
{
  if (x->host_start == x->host_end
      && y->host_start == y->host_end)
    return 0;
  if (x->host_end <= y->host_start)
    return -1;
  if (x->host_start >= y->host_end)
    return 1;
  return 0;
}

#include "splay-tree.h"

/* This structure describes accelerator device.
   It contains name of the corresponding libgomp plugin, function handlers for
   interaction with the device, ID-number of the device, and information about
   mapped memory.  */
struct gomp_device_descr
{
  /* This is the ID number of device.  It could be specified in DEVICE-clause of
     TARGET construct.  */
  int id;

  /* Plugin file handler.  */
  void *plugin_handle;

  /* Function handlers.  */
  bool (*device_available_func) (void);

  /* Splay tree containing information about mapped memory regions.  */
  struct splay_tree_s dev_splay_tree;

  /* Mutex for operating with the splay tree and other shared structures.  */
  gomp_mutex_t dev_env_lock;
};

attribute_hidden int
gomp_get_num_devices (void)
{
  (void) pthread_once (&gomp_is_initialized, gomp_target_init);
  return num_devices;
}

static struct gomp_device_descr *
resolve_device (int device_id)
{
  if (device_id == -1)
    {
      struct gomp_task_icv *icv = gomp_icv (false);
      device_id = icv->default_device_var;
    }
  if (device_id < 0
      || (device_id >= gomp_get_num_devices ()
	  && device_id != 257))
    return NULL;

  /* FIXME: Temporary hack for testing non-shared address spaces on host.  */
  if (device_id == 257)
    return &devices[0];

  return &devices[device_id];
}


/* Handle the case where splay_tree_lookup found oldn for newn.
   Helper function of gomp_map_vars.  */

static inline void
gomp_map_vars_existing (splay_tree_key oldn, splay_tree_key newn,
			unsigned char kind)
{
  if (oldn->host_start > newn->host_start
      || oldn->host_end < newn->host_end)
    gomp_fatal ("Trying to map into device [%p..%p) object when"
		"[%p..%p) is already mapped",
		(void *) newn->host_start, (void *) newn->host_end,
		(void *) oldn->host_start, (void *) oldn->host_end);
  if (((kind & 7) == 2 || (kind & 7) == 3)
      && !oldn->copy_from
      && oldn->host_start == newn->host_start
      && oldn->host_end == newn->host_end)
    oldn->copy_from = true;
  oldn->refcount++;
}

static struct target_mem_desc *
gomp_map_vars (struct gomp_device_descr *devicep, size_t mapnum,
	       void **hostaddrs, size_t *sizes, unsigned char *kinds,
	       bool is_target)
{
  size_t i, tgt_align, tgt_size, not_found_cnt = 0;
  struct splay_tree_key_s cur_node;
  struct target_mem_desc *tgt
    = gomp_malloc (sizeof (*tgt) + sizeof (tgt->list[0]) * mapnum);
  tgt->list_count = mapnum;
  tgt->refcount = 1;
  tgt->device_descr = devicep;

  if (mapnum == 0)
    return tgt;

  tgt_align = sizeof (void *);
  tgt_size = 0;
  if (is_target)
    {
      size_t align = 4 * sizeof (void *);
      tgt_align = align;
      tgt_size = mapnum * sizeof (void *);
    }

  gomp_mutex_lock (&devicep->dev_env_lock);
  for (i = 0; i < mapnum; i++)
    {
      cur_node.host_start = (uintptr_t) hostaddrs[i];
      if ((kinds[i] & 7) != 4)
	cur_node.host_end = cur_node.host_start + sizes[i];
      else
	cur_node.host_end = cur_node.host_start + sizeof (void *);
      splay_tree_key n = splay_tree_lookup (&devicep->dev_splay_tree,
					    &cur_node);
      if (n)
	{
	  tgt->list[i] = n;
	  gomp_map_vars_existing (n, &cur_node, kinds[i]);
	}
      else
	{
	  size_t align = (size_t) 1 << (kinds[i] >> 3);
	  tgt->list[i] = NULL;
	  not_found_cnt++;
	  if (tgt_align < align)
	    tgt_align = align;
	  tgt_size = (tgt_size + align - 1) & ~(align - 1);
	  tgt_size += cur_node.host_end - cur_node.host_start;
	}
    }

  if (not_found_cnt || is_target)
    {
      /* FIXME: This would be accelerator memory allocation, not
	 host, and should allocate tgt_align aligned tgt_size block
	 of memory.  */
      tgt->to_free = gomp_malloc (tgt_size + tgt_align - 1);
      tgt->tgt_start = (uintptr_t) tgt->to_free;
      tgt->tgt_start = (tgt->tgt_start + tgt_align - 1) & ~(tgt_align - 1);
      tgt->tgt_end = tgt->tgt_start + tgt_size;
    }
  else
    {
      tgt->to_free = NULL;
      tgt->tgt_start = 0;
      tgt->tgt_end = 0;
    }

  tgt_size = 0;
  if (is_target)
    tgt_size = mapnum * sizeof (void *);

  tgt->array = NULL;
  if (not_found_cnt)
    {
      tgt->array = gomp_malloc (not_found_cnt * sizeof (*tgt->array));
      splay_tree_node array = tgt->array;

      for (i = 0; i < mapnum; i++)
	if (tgt->list[i] == NULL)
	  {
	    splay_tree_key k = &array->key;
	    k->host_start = (uintptr_t) hostaddrs[i];
	    if ((kinds[i] & 7) != 4)
	      k->host_end = k->host_start + sizes[i];
	    else
	      k->host_end = k->host_start + sizeof (void *);
	    splay_tree_key n
	      = splay_tree_lookup (&devicep->dev_splay_tree, k);
	    if (n)
	      {
		tgt->list[i] = n;
		gomp_map_vars_existing (n, k, kinds[i]);
	      }
	    else
	      {
		size_t align = (size_t) 1 << (kinds[i] >> 3);
		tgt->list[i] = k;
		tgt_size = (tgt_size + align - 1) & ~(align - 1);
		k->tgt = tgt;
		k->tgt_offset = tgt_size;
		tgt_size += k->host_end - k->host_start;
		k->copy_from = false;
		if ((kinds[i] & 7) == 2 || (kinds[i] & 7) == 3)
		  k->copy_from = true;
		k->refcount = 1;
		tgt->refcount++;
		array->left = NULL;
		array->right = NULL;
		splay_tree_insert (&devicep->dev_splay_tree, array);
		switch (kinds[i] & 7)
		  {
		  case 0: /* ALLOC */
		  case 2: /* FROM */
		    break;
		  case 1: /* TO */
		  case 3: /* TOFROM */
		    /* FIXME: This is supposed to be copy from host to device
		       memory.  Perhaps add some smarts, like if copying
		       several adjacent fields from host to target, use some
		       host buffer to avoid sending each var individually.  */
		    memcpy ((void *) (tgt->tgt_start + k->tgt_offset),
			    (void *) k->host_start,
			    k->host_end - k->host_start);
		    break;
		  case 4: /* POINTER */
		    cur_node.host_start
		      = (uintptr_t) *(void **) k->host_start;
		    /* Add bias to the pointer value.  */
		    cur_node.host_start += sizes[i];
		    cur_node.host_end = cur_node.host_start + 1;
		    n = splay_tree_lookup (&devicep->dev_splay_tree,
					   &cur_node);
		    if (n == NULL)
		      {
			/* Could be possibly zero size array section.  */
			cur_node.host_end--;
			n = splay_tree_lookup (&devicep->dev_splay_tree,
					       &cur_node);
			if (n == NULL)
			  {
			    cur_node.host_start--;
			    n = splay_tree_lookup (&devicep->dev_splay_tree,
						   &cur_node);
			    cur_node.host_start++;
			  }
		      }
		    if (n == NULL)
		      gomp_fatal ("Pointer target of array section "
				  "wasn't mapped");
		    cur_node.host_start -= n->host_start;
		    cur_node.tgt_offset = n->tgt->tgt_start + n->tgt_offset
					  + cur_node.host_start;
		    /* At this point tgt_offset is target address of the
		       array section.  Now subtract bias to get what we want
		       to initialize the pointer with.  */
		    cur_node.tgt_offset -= sizes[i];
		    /* FIXME: host to device copy, see above FIXME comment.  */
		    memcpy ((void *) (tgt->tgt_start + k->tgt_offset),
			    (void *) &cur_node.tgt_offset,
			    sizeof (void *));
		    break;
		  }
		array++;
	      }
	  }
    }
  if (is_target)
    {
      for (i = 0; i < mapnum; i++)
	{
	  cur_node.tgt_offset = tgt->list[i]->tgt->tgt_start
				+ tgt->list[i]->tgt_offset;
	  /* FIXME: host to device copy, see above FIXME comment.  */
	  memcpy ((void *) (tgt->tgt_start + i * sizeof (void *)),
		  (void *) &cur_node.tgt_offset,
		  sizeof (void *));
	}
    }

  gomp_mutex_unlock (&devicep->dev_env_lock);
  return tgt;
}

static void
gomp_unmap_tgt (struct target_mem_desc *tgt)
{
  /* FIXME: Deallocate on target the tgt->tgt_start .. tgt->tgt_end
     region.  */
  if (tgt->tgt_end)
    free (tgt->to_free);

  free (tgt->array);
  free (tgt);
}

static void
gomp_unmap_vars (struct target_mem_desc *tgt)
{
  struct gomp_device_descr *devicep = tgt->device_descr;

  if (tgt->list_count == 0)
    {
      free (tgt);
      return;
    }

  size_t i;
  gomp_mutex_lock (&devicep->dev_env_lock);
  for (i = 0; i < tgt->list_count; i++)
    if (tgt->list[i]->refcount > 1)
      tgt->list[i]->refcount--;
    else
      {
	splay_tree_key k = tgt->list[i];
	if (k->copy_from)
	  /* FIXME: device to host copy.  */
	  memcpy ((void *) k->host_start,
		  (void *) (k->tgt->tgt_start + k->tgt_offset),
		  k->host_end - k->host_start);
	splay_tree_remove (&devicep->dev_splay_tree, k);
	if (k->tgt->refcount > 1)
	  k->tgt->refcount--;
	else
	  gomp_unmap_tgt (k->tgt);
      }

  if (tgt->refcount > 1)
    tgt->refcount--;
  else
    gomp_unmap_tgt (tgt);
  gomp_mutex_unlock (&devicep->dev_env_lock);
}

static void
gomp_update (struct gomp_device_descr *devicep, size_t mapnum,
	     void **hostaddrs, size_t *sizes, unsigned char *kinds)
{
  size_t i;
  struct splay_tree_key_s cur_node;

  if (!devicep)
    return;

  if (mapnum == 0)
    return;

  gomp_mutex_lock (&devicep->dev_env_lock);
  for (i = 0; i < mapnum; i++)
    if (sizes[i])
      {
	cur_node.host_start = (uintptr_t) hostaddrs[i];
	cur_node.host_end = cur_node.host_start + sizes[i];
	splay_tree_key n = splay_tree_lookup (&devicep->dev_splay_tree,
					      &cur_node);
	if (n)
	  {
	    if (n->host_start > cur_node.host_start
		|| n->host_end < cur_node.host_end)
	      gomp_fatal ("Trying to update [%p..%p) object when"
			  "only [%p..%p) is mapped",
			  (void *) cur_node.host_start,
			  (void *) cur_node.host_end,
			  (void *) n->host_start,
			  (void *) n->host_end);
	    if ((kinds[i] & 7) == 1)
	      /* FIXME: host to device copy.  */
	      memcpy ((void *) (n->tgt->tgt_start + n->tgt_offset
				+ cur_node.host_start - n->host_start),
		      (void *) cur_node.host_start,
		      cur_node.host_end - cur_node.host_start);
	    else if ((kinds[i] & 7) == 2)
	      /* FIXME: device to host copy.  */
	      memcpy ((void *) cur_node.host_start,
		      (void *) (n->tgt->tgt_start + n->tgt_offset
				+ cur_node.host_start - n->host_start),
		      cur_node.host_end - cur_node.host_start);
	  }
	else
	  gomp_fatal ("Trying to update [%p..%p) object that is not mapped",
		      (void *) cur_node.host_start,
		      (void *) cur_node.host_end);
      }
  gomp_mutex_unlock (&devicep->dev_env_lock);
}

/* Called when encountering a target directive.  If DEVICE
   is -1, it means use device-var ICV.  If it is -2 (or any other value
   larger than last available hw device, use host fallback.
   FN is address of host code, OPENMP_TARGET contains value of the
   __OPENMP_TARGET__ symbol in the shared library or binary that invokes
   GOMP_target.  HOSTADDRS, SIZES and KINDS are arrays
   with MAPNUM entries, with addresses of the host objects,
   sizes of the host objects (resp. for pointer kind pointer bias
   and assumed sizeof (void *) size) and kinds.  */

void
GOMP_target (int device, void (*fn) (void *), const void *openmp_target,
	     size_t mapnum, void **hostaddrs, size_t *sizes,
	     unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device);
  if (devicep == NULL)
    {
      /* Host fallback.  */
      struct gomp_thread old_thr, *thr = gomp_thread ();
      old_thr = *thr;
      memset (thr, '\0', sizeof (*thr));
      if (gomp_places_list)
	{
	  thr->place = old_thr.place;
	  thr->ts.place_partition_len = gomp_places_list_len;
	}
      fn (hostaddrs);
      gomp_free_thread (thr);
      *thr = old_thr;
      return;
    }

  struct target_mem_desc *tgt
    = gomp_map_vars (devicep, mapnum, hostaddrs, sizes, kinds, true);
  struct gomp_thread old_thr, *thr = gomp_thread ();
  old_thr = *thr;
  memset (thr, '\0', sizeof (*thr));
  if (gomp_places_list)
    {
      thr->place = old_thr.place;
      thr->ts.place_partition_len = gomp_places_list_len;
    }
  fn ((void *) tgt->tgt_start);
  gomp_free_thread (thr);
  *thr = old_thr;
  gomp_unmap_vars (tgt);
}

void
GOMP_target_data (int device, const void *openmp_target, size_t mapnum,
		  void **hostaddrs, size_t *sizes, unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device);
  if (devicep == NULL)
    {
      /* Host fallback.  */
      struct gomp_task_icv *icv = gomp_icv (false);
      if (icv->target_data)
	{
	  /* Even when doing a host fallback, if there are any active
	     #pragma omp target data constructs, need to remember the
	     new #pragma omp target data, otherwise GOMP_target_end_data
	     would get out of sync.  */
	  struct target_mem_desc *tgt
	    = gomp_map_vars (NULL, 0, NULL, NULL, NULL, false);
	  tgt->prev = icv->target_data;
	  icv->target_data = tgt;
	}
      return;
    }

  struct target_mem_desc *tgt
    = gomp_map_vars (devicep, mapnum, hostaddrs, sizes, kinds, false);
  struct gomp_task_icv *icv = gomp_icv (true);
  tgt->prev = icv->target_data;
  icv->target_data = tgt;
}

void
GOMP_target_end_data (void)
{
  struct gomp_task_icv *icv = gomp_icv (false);
  if (icv->target_data)
    {
      struct target_mem_desc *tgt = icv->target_data;
      icv->target_data = tgt->prev;
      gomp_unmap_vars (tgt);
    }
}

void
GOMP_target_update (int device, const void *openmp_target, size_t mapnum,
		    void **hostaddrs, size_t *sizes, unsigned char *kinds)
{
  struct gomp_device_descr *devicep = resolve_device (device);
  if (devicep == NULL)
    return;

  gomp_update (devicep, mapnum, hostaddrs, sizes, kinds);
}

void
GOMP_teams (unsigned int num_teams, unsigned int thread_limit)
{
  if (thread_limit)
    {
      struct gomp_task_icv *icv = gomp_icv (true);
      icv->thread_limit_var
	= thread_limit > INT_MAX ? UINT_MAX : thread_limit;
    }
  (void) num_teams;
}

#ifdef PLUGIN_SUPPORT

/* This function checks if the given string FNAME matches
   "libgomp-plugin-*.so.1".  */
static bool
gomp_check_plugin_file_name (const char *fname)
{
  const char *prefix = "libgomp-plugin-";
  const char *suffix = ".so.1";
  if (!fname)
    return false;
  if (strncmp (fname, prefix, strlen (prefix)) != 0)
    return false;
  if (strncmp (fname + strlen (fname) - strlen (suffix), suffix,
	       strlen (suffix)) != 0)
    return false;
  return true;
}

/* This function tries to load plugin for DEVICE.  Name of plugin is passed
   in PLUGIN_NAME.
   Plugin handle and handles of the found functions are stored in the
   corresponding fields of DEVICE.
   The function returns TRUE on success and FALSE otherwise.  */
static bool
gomp_load_plugin_for_device (struct gomp_device_descr *device,
			     const char *plugin_name)
{
  if (!device || !plugin_name)
    return false;

  device->plugin_handle = dlopen (plugin_name, RTLD_LAZY);
  if (!device->plugin_handle)
    return false;

  /* Clear any existing error.  */
  dlerror ();

  /* Check if all required functions are available in the plugin and store
     their handlers.
     TODO: check for other routines as well.  */
  device->device_available_func = dlsym (device->plugin_handle,
					 "device_available");
  if (dlerror () != NULL)
    {
      dlclose (device->plugin_handle);
      return false;
    }

  return true;
}

/* This functions scans folder, specified in environment variable
   LIBGOMP_PLUGIN_PATH, and loads all suitable libgomp plugins from this folder.
   For a plugin to be suitable, its name should be "libgomp-plugin-*.so.1" and
   it should implement a certain set of functions.
   Result of this function is properly initialized variable NUM_DEVICES and
   array DEVICES, containing all plugins and their callback handles.  */
static void
gomp_find_available_plugins (void)
{
  char *plugin_path = NULL;
  DIR *dir = NULL;
  struct dirent *ent;
  char plugin_name[PATH_MAX];

  num_devices = 0;
  devices = NULL;

  plugin_path = getenv ("LIBGOMP_PLUGIN_PATH");
  if (!plugin_path)
    goto out;

  dir = opendir (plugin_path);
  if (!dir)
    goto out;

  while ((ent = readdir (dir)) != NULL)
    {
      struct gomp_device_descr current_device;
      if (!gomp_check_plugin_file_name (ent->d_name))
	continue;
      if (strlen (plugin_path) + 1 + strlen (ent->d_name) >= PATH_MAX)
	continue;
      strcpy (plugin_name, plugin_path);
      strcat (plugin_name, "/");
      strcat (plugin_name, ent->d_name);
      if (!gomp_load_plugin_for_device (&current_device, plugin_name))
	continue;
      devices = realloc (devices, (num_devices + 1)
				  * sizeof (struct gomp_device_descr));
      if (devices == NULL)
	{
	  num_devices = 0;
	  closedir (dir);
	  goto out;
	}

      devices[num_devices] = current_device;
      devices[num_devices].id = num_devices + 1;
      devices[num_devices].dev_splay_tree.root = NULL;
      gomp_mutex_init (&devices[num_devices].dev_env_lock);
      num_devices++;
    }
  closedir (dir);

 out:
  /* FIXME: Temporary hack for testing non-shared address spaces on host.
     We create device 257 just to check memory mapping.  */
  if (num_devices == 0)
    {
      num_devices = 1;
      devices = malloc (sizeof (struct gomp_device_descr));
      if (devices == NULL)
	{
	  num_devices = 0;
	  return;
	}
      devices[0].plugin_handle = NULL;
      devices[0].device_available_func = NULL;
      devices[0].dev_splay_tree.root = NULL;
      gomp_mutex_init (&devices[0].dev_env_lock);
    }
  devices[0].id = 257;
}

/* This function initializes runtime needed for offloading.
   It loads plugins, sets up a connection with devices, etc.  */
static void
gomp_target_init (void)
{
  gomp_find_available_plugins ();
}

#else /* PLUGIN_SUPPORT */
/* If dlfcn.h is unavailable we always fallback to host execution.
   GOMP_target* routines are just stubs for this case.  */
static void
gomp_target_init (void)
{
}
#endif /* PLUGIN_SUPPORT */
