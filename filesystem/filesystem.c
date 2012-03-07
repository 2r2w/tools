/*
 * filesystem.cpp
 *
 *  Created on: Mar 6, 2012
 *      Author: wikaka
 */


#include "filesystem.h"

#ifdef WIN32

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <direct.h>
#include <string.h>

#elif defined(__unix__)

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

#endif


struct list_item
{
	char *path;
	struct list_item *next;
};

struct list_item *__add_to_list(struct list_item *i,char *path)
{
	struct list_item *ni=0;
	ni=calloc(1,sizeof(struct list_item));
	if (!ni)
		return 0;
	if (i)
		i->next=ni;
	ni->path=path;
	return ni;
}

void __free_list(struct list_item *i)
{
	while (i)
	{
		struct list_item *n=i;
		i=i->next;
		if ( n->path )
			free( n->path );
		free(n);
	}
}


struct list_item *__push(struct list_item *stack, char *path)
{
	struct list_item *ni=0;
	ni=calloc(sizeof(struct list_item),1);
	if (!ni)
		return 0;
	ni->next=stack;
	ni->path=path;
	return ni;
}

char *__pop(struct list_item **stack)
{
	char *res=0;
	if (*stack)
	{
		struct list_item *n = *stack;
		res=n->path;
		*stack = (*stack)->next;
		free(n);
	}
	return res;
}

struct masksearch_result_t
{
	struct list_item *files, *curfile;
	struct list_item *dirs, *curdir;
	struct list_item *errors, *curerror;
	size_t files_count, dirs_count, errors_count;
};

#define __add_smth(__type,__result,__path) \
do { \
	if(!(__result)) \
		break; \
	(__result)->cur##__type=__add_to_list((__result)->cur##__type,__path); \
	if(!(__result)->cur##__type) \
		break; \
	if(!(__result)->__type##s) \
		(__result)->__type##s=(__result)->cur##__type; \
	__result->__type##s_count++; \
} while(0)


/**
 * проверить соответствие строки маске.
 * проверяемые wildcard- это * и ?
 * \param wild - маска
 * \param string - строка
 */
int wildcmp(const char *wild, const char *string)
{
  // Written by Jack Handy - jakkhandy@hotmail.com

  const char *cp = NULL, *mp = NULL;

  while ((*string) && (*wild != '*')) {
    if ((*wild != *string) && (*wild != '?')) {
      return 0;
    }
    wild++;
    string++;
  }

  while (*string) {
    if (*wild == '*') {
      if (!*++wild) {
        return 1;
      }
      mp = wild;
      cp = string+1;
    } else if ((*wild == *string) || (*wild == '?')) {
      wild++;
      string++;
    } else {
      wild = mp;
      string = cp++;
    }
  }

  while (*wild == '*') {
    wild++;
  }
  return !*wild;
}

#ifdef __unix__

/**
EACCES Search permission is denied for one of the components of path.  (See also path_resolution(7).)
EACCES Permission denied.
EBADF  fd is not a valid file descriptor opened for reading.
EMFILE Too many file descriptors in use by process.
ENFILE Too many files are currently open in the system.
ENOENT Directory does not exist, or name is an empty string.
ENOMEM Insufficient memory to complete the operation.
ENOTDIR name is not a directory.
EFAULT path points outside your accessible address space.
EIO    An I/O error occurred.
ELOOP  Too many symbolic links were encountered in resolving path.
ENAMETOOLONG path is too long.
ENOMEM Insufficient kernel memory was available.
ENOTDIR A component of path is not a directory.The general errors for fchdir() are listed below:

 * @param path
 * @param path_mask
 * @param flags
 * @return
 */
struct masksearch_result_t *__linux_masksearch(const char *path, const char *path_mask, int flags)
{
	int err=0, merrno=0;
	DIR *dir=NULL;
	struct dirent *de=NULL;
	struct list_item *mstack=0;
	struct masksearch_result_t *r=NULL;
	char *curpath;

	r=calloc(1, sizeof(struct masksearch_result_t));
	if(!r)
	{
		errno=ENOMEM;
		return 0;
	}

	mstack=__push( mstack, strdup(path) );

	while( (curpath=__pop(&mstack)) )
	{
		//fprintf(stdout,"searching dir: [%s]\n",curpath);
		dir=opendir(curpath);
		if(!dir)
		{
			__add_smth( error, r, curpath);
			if (!r->curerror)
			{
				free(curpath);
				cross_finish_masksearch(r);
				r=0;
				break;
			}
			else
			{
				continue;
			}
		}


		while ((de = readdir(dir)))
		{
			cross_stat_t st;
			char *item_path=0;
			size_t pathlen=0;

			/*skip . and .. */
			if( de->d_name[0] == '.' )
				if(	(de->d_name[1]==0)||(de->d_name[1]=='.'&& de->d_name[2] == 0) )
					continue;

			pathlen=strlen(curpath)+strlen(de->d_name)+2;
			item_path=calloc(pathlen,1);

			if(!item_path)
			{
				cross_finish_masksearch(r);
				r=0;
				break;
			}

			snprintf(item_path,pathlen,"%s/%s",curpath,de->d_name);

			err = cross_stat(item_path,&st);
			if (err)
			{
				__add_smth( error, r,strdup(item_path));
				free(item_path);
				if(!r->curerror)
				{
					cross_finish_masksearch(r);
					r=0;
					break;
				}
				else
				{
					continue;
				}
			}
			//fprintf(stdout,"stat ok: [%s]\n",item_path);

			if ( S_ISDIR(st.st_mode) )
			{
				//fprintf(stdout,"directory: [%s]\n",item_path);
				if(!(flags&MASKSEARCH_NODIRS) && wildcmp(path_mask,de->d_name) )
				{
					__add_smth( dir, r, strdup(item_path));
					if(!r->curdir)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
					//fprintf(stdout,"dir added: [%s]\n",item_path);
				}

				if (!(flags&MASKSEARCH_NOSUBDIRS))
				{
					//fprintf(stdout,"adding subdir: [%s]\n",item_path);
					mstack=__push(mstack,strdup(item_path));
					if(!mstack)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
					//fprintf(stdout,"subdir added: [%s]\n",item_path);
				}
			}
			else if ( S_ISREG(st.st_mode) )
			{
				//fprintf(stdout,"file: [%s]\n",item_path);
				if(!(flags&MASKSEARCH_NOFILES) && wildcmp(path_mask,de->d_name) )
				{
					__add_smth( file, r,strdup(item_path));
					if(!r->curfile)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
					//fprintf(stdout,"file added: [%s]\n",item_path);
				}
			}
			free(item_path);
		}

		//fprintf(stdout,"%p===============================\n",r);
		closedir(dir);
		free(curpath);
		if(!r)
			break;
	}

	if(!r)
	{
			errno=ENOMEM;
	}
	else
	{
		r->curdir=NULL;
		r->curfile=NULL;
		r->curerror=NULL;
	}
	return r;
}

#elif defined(WIN32)

struct masksearch_result_t *__windows_masksearch(const char *path, const char *path_mask, int flags)
{
	WIN32_FIND_DATAA buf;
	HANDLE h=0;
	char *search_path=0;
	struct masksearch_result_t *r=NULL;
	struct list_item *mstack=0;
	char *curpath;

	r=calloc(1, sizeof(struct masksearch_result_t));
	if(!r)
	{
		errno=ENOMEM;
		return 0;
	}

	mstack=__push( mstack, _strdup(path) );

	while( (curpath=__pop(&mstack)) )
	{
		search_path=calloc(strlen(curpath)+3,1);
		if(!search_path)
		{
				free(curpath);
				cross_finish_masksearch(r);
				r=0;
				break;
		}

		snprintf(search_path,strlen(path)+3,"%s\\*",curpath);

		h = FindFirstFileA(search_path, &buf );
		if( h == INVALID_HANDLE_VALUE )
		{
			__add_smth( error, r, _strdup(curpath));
			if(!r->curdir)
			{
				free(curpath);
				free(search_path);
				cross_finish_masksearch(r);
				r=0;
				break;
			}
		}

		do
		{
			char *item_path=0;
			size_t pathlen=0;

			/*пропускать . и .. */
			if( buf.cFileName[0] == '.' )
				if(	(buf.cFileName[1]==0)||(buf.cFileName[1]=='.'&& buf.cFileName[2]== 0))
					continue;

			pathlen=strlen(curpath)+strlen(buf.cFileName)+2;
			item_path=calloc(pathlen,1);
			if(!item_path)
			{
				cross_finish_masksearch(r);
				r=0;
				break;
			}

			snprintf(item_path,pathlen,"%s\\%s",curpath,buf.cFileName);

			if ( buf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if(!(flags&MASKSEARCH_NODIRS) && wildcmp(path_mask,buf.cFileName) )
				{
					__add_smth( dir, r, _strdup(item_path));
					if(!r->curdir)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
					//fprintf(stdout,"dir added: [%s]\n",item_path);
				}

				if (!(flags&MASKSEARCH_NOSUBDIRS))
				{
					//fprintf(stdout,"adding subdir: [%s]\n",item_path);
					mstack=__push(mstack,_strdup(item_path));
					if(!mstack)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
				}
			}
			else
			{
				//fprintf(stdout,"file: [%s]\n",item_path);
				if(!(flags&MASKSEARCH_NOFILES) && wildcmp(path_mask,buf.cFileName) )
				{
					__add_smth( file, r,_strdup(item_path));
					if(!r->curfile)
					{
						free(item_path);
						cross_finish_masksearch(r);
						r=0;
						break;
					}
					//fprintf(stdout,"file added: [%s]\n",item_path);
				}
			}
			free(item_path);
		}
		while( FindNextFileA( h, &buf ) );

		FindClose( h );
		free(curpath);
		free(search_path);
	}

	if ( ERROR_NO_MORE_FILES != GetLastError() )
	{
		cross_finish_masksearch(r);
		r=0;
	}

	if(!r)
	{
			errno=ENOMEM;
	}
	else
	{
		r->curdir=NULL;
		r->curfile=NULL;
		r->curerror=NULL;
	}

	return r;
}
#endif

struct masksearch_result_t *cross_masksearch(const char *path, const char *path_mask, int flags)
{
	if( !path || !path_mask )
		return 0;
#ifdef __unix__
	return __linux_masksearch(path,path_mask,flags);
#else
	return __windows_masksearch(path,path_mask,flags);
#endif
}

size_t cross_filescount(struct masksearch_result_t *r)
{
		return (r)?r->files_count:0;
}
size_t cross_dirscount(struct masksearch_result_t *r)
{
		return (r)?r->dirs_count:0;
}
size_t cross_errorscount(struct masksearch_result_t *r)
{
		return (r)?r->errors_count:0;
}

const char *cross_nextfile(struct masksearch_result_t *r)
{
	if(!r)
		return 0;
	r->curfile=(!r->curfile)?r->files:r->curfile->next;
	return ( r->curfile && r->curfile->path )?
												r->curfile->path:
												0;
}
const char *cross_nextdir(struct masksearch_result_t *r)
{
	if(!r)
		return 0;
	r->curdir=(!r->curdir)?r->dirs:r->curdir->next;
	return ( r->curdir && r->curdir->path )?
												r->curdir->path:
												0;
}
const char *cross_nexterror(struct masksearch_result_t *r)
{
	if(!r)
		return 0;
	r->curerror=(!r->curerror)?r->errors:r->curerror->next;
	return ( r->curerror && r->curerror->path )?
												r->curerror->path:
												0;
}

void cross_finish_masksearch(struct masksearch_result_t *r)
{
	if (!r)
		return;
	__free_list(r->files);
	__free_list(r->dirs);
	__free_list(r->errors);
	free(r);
}

int cross_tmpfile(char **filename, const char *tmpdir)
{
	int res=-1;
	if(tmpdir==DEFAULT_TMPDIR)
	{
		(*filename)=malloc(12);
		if( !(*filename))
			return 0;
    snprintf(*filename,12,"/tmp/XXXXXX");
	}
	else
	{
		size_t len=strlen(tmpdir)+8;
		*filename=malloc(len);
		if( !(*filename) )
			return 0;
		snprintf((*filename),len,"%s/XXXXXX",tmpdir);
	}

	res=mkstemp(*filename);
	if(res==-1)
	{
		free((*filename));
		(*filename)=0;
	}
	return res;
}


