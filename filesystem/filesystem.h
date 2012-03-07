/*
 * filesystem.h
 *
 *  Created on: Mar 6, 2012
 *      Author: wikaka
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#ifdef __WIN32__
#	ifndef WIN32
#		define WIN32
#	endif/*WIN32*/
#endif/*__WIN32__*/

#ifdef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

typedef struct __stat64 cross_stat_t;
#define cross_stat(__path,__stat)  _stat64(__path,__stat)
#define S_ISREG(m)  (m&_S_IFREG)
#define S_ISDIR(m)  (m&_S_IFDIR)
#define S_ISCHR(m)  0
#define S_ISBLK(m)  0
#define S_ISFIFO(m) 0
#define cross_mkdir(__path,__mode)  _mkdir(__path)
#define cross_unlink(__path) _unlink(__path)

#elif defined __unix__

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#elif _FILE_OFFSET_BITS < 64
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

typedef struct stat cross_stat_t;
#define cross_stat(__path,__stat)  stat(__path,__stat)
#define cross_mkdir(__path,__mode) mkdir(__path,__mode)
#define cross_unlink(__path)       unlink(__path)

#endif

#ifdef __cplusplus
extern "C" {
#endif

struct masksearch_result_t;
//!flags for cross_masksearch
//!search for all matches without any constraints
#define MASKSEARCH_DEFAULT   0
//!do not search in subdirectories also
#define MASKSEARCH_NOSUBDIRS 1
//!do not search for files
#define MASKSEARCH_NOFILES   2
//!do not search for directories
#define MASKSEARCH_NODIRS    4

/**
 * search for files and directories in @a path which match @a path_mask
 * @param path path on which search will be performed
 * @param path_mask mask, that will be applyed to paths. Could include ? and *
 * @param flags one of flags above: MASKSEARCH_DEFAULT,MASKSEARCH_NOSUBDIRS,
 * MASKSEARCH_NOFILES,MASKSEARCH_NODIRS
 * @return NULL if search failed, pointer to result in other cases
 */
struct masksearch_result_t *
cross_masksearch(const char *path, const char *path_mask, int flags);
size_t cross_filescount(struct masksearch_result_t *r);
size_t cross_dirscount(struct masksearch_result_t *r);
size_t cross_errorscount(struct masksearch_result_t *r);
const char *cross_nextfile(struct masksearch_result_t *r);
const char *cross_nextdir(struct masksearch_result_t *r);
const char *cross_nexterror(struct masksearch_result_t *r);
void cross_finish_masksearch(struct masksearch_result_t *r);

#define DEFAULT_TMPDIR 0
int cross_tmpfile(char **filename, const char *tmpdir);

#ifdef __cplusplus
}//extern "C"
#include "filesystem.hpp"
#endif

#endif /* FILESYSTEM_H_ */
