#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

int main(int argc, char** argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--sleep") == 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        return 0;
    }

    std::cout << "inject-dummy";
    for (int i = 1; i < argc; ++i)
    {
        std::cout << ' ' << argv[i];
    }
    std::cout << std::endl;
    return 0;
}
