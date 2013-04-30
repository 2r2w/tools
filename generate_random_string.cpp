#include <stdexcept>
#include <iostream>
#include <string>
#include <cstdlib>


const std::string randomStr(const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string s(len + 1, 0);
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    return s;
}


int main(int argc, char **argv)
{
    srand(time(NULL));
    if(argc<3)
        return -1;
    int len = atoi(argv[1]);
    int count = atoi(argv[2]);
    for (int i=0; i < count; ++i) {
        std::cout << randomStr(len) << std::endl;
    }
    return 0;
}