/*
 * filesystem.hpp
 *
 *  Created on: Mar 6, 2012
 *      Author: wikaka
 */

#ifndef FILESYSTEM_HPP_
#define FILESYSTEM_HPP_
#ifdef __cplusplus

#include <stdlib.h>
#include <fstream>
#include <string>

std::string cross_tmpstream(std::ofstream &stream,const std::string &tmpdir="")
{
	int fd(-1);
	char *name(0);
	std::string filename;
	if (tmpdir.empty())
		fd=cross_tmpfile(&name,DEFAULT_TMPDIR);
	else
		fd=cross_tmpfile(&name,tmpdir.c_str());

	if ( fd!=-1)
	{
		stream.open(name);
		filename.assign(name);
		free(name);
		close(fd);
	}
	return filename;
}



#endif
#endif /* FILESYSTEM_HPP_ */
