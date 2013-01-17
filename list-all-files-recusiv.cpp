
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fts.h>


std::vector<std::string> getFilesList(const std::string &path)
{
    int saved_errno = 0;
    char *paths[] = { (char*)path.c_str(), 0 };
    std::vector<std::string> result;
    FTS *tree = fts_open(paths, FTS_NOCHDIR|FTS_NOSTAT, 0);
    saved_errno = errno;
    if (!tree)
        throw std::runtime_error("getFileList("+path+")"+strerror(saved_errno));

    FTSENT *node;
    while ((node = fts_read(tree))) {
//      if (node->fts_level > 0 && node->fts_name[0] == '.')
//          fts_set(tree, node, FTS_SKIP);
        if (node->fts_info & FTS_F)
            result.push_back(node->fts_accpath);
    }
    saved_errno = errno;
    fts_close(tree);
    if (saved_errno)
        throw std::runtime_error("getFileList("+path+")"+strerror(saved_errno));
    return result;
}

int main(int argc, char **argv)
{
    if(argc<2)
        return -1;
    const std::vector<std::string> result = getFilesList(argv[1]);
    for(size_t i(0); i < result.size(); ++i)
        std::cout<<result[i]<<std::endl;
    return 0;
}
