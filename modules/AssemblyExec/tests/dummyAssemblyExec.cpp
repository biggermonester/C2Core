#include <cstdio>

int main(int argc, char** argv)
{
    std::printf("assemblyexec-dummy");
    for (int i = 1; i < argc; ++i)
    {
        std::printf(" %s", argv[i]);
    }
    std::printf("\n");
    return 0;
}
