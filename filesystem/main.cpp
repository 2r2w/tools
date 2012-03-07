/*
 * mian.c
 *
 *  Created on: Mar 6, 2012
 *      Author: wikaka
 */


#include "filesystem.h"
#include <stdio.h>

int main(int argc, char**argv)
{
	struct masksearch_result_t *r;
	if(argc<3)
	{
		fprintf(stderr,"need 2 parameters: path and mask\n");
		return -3;
	}

	r=cross_masksearch(argv[1],argv[2],MASKSEARCH_DEFAULT);
	if (!r)
	{
		fprintf(stderr,"failed to perform search\n");
		return -1;
	}

	const char *path;
	fprintf(stdout,"found: %zi %zi %zi\n",
			cross_filescount(r),
			cross_dirscount(r),
			cross_errorscount(r));

	while ( (path=cross_nextfile(r)) )
		fprintf(stdout,"FILE: [%s]\n",path);

	while ( (path=cross_nextdir(r)) )
		fprintf(stdout,"DIR : [%s]\n",path);

	while ( (path=cross_nexterror(r)) )
		fprintf(stdout,"ERR : [%s]\n",path);

	cross_finish_masksearch(r);


	std::ofstream ofile;
	std::string filename(cross_tmpstream(ofile));
	if(!filename.empty())
	{
		fprintf(stdout,"opened file: %s\n",filename.c_str());
		ofile<<"Hello, world\n";
		ofile.close();
	}
	return 0;
}

