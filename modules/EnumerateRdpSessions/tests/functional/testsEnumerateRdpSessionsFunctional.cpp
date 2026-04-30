#include "../../EnumerateRdpSessions.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

using namespace functional_tests;

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--server", "C2_FUNC_RDP_SERVER", "Remote RDP server; omit for local session enumeration", false},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsEnumerateRdpSessionsFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");
    const std::string server = readInput(inputs[0], argc, argv, interactive);

    std::vector<std::string> command = {"enumerateRdpSessions"};
    if (!server.empty())
    {
        command.insert(command.end(), {"-s", server});
    }

    EnumerateRdpSessions module;
    return runModuleScenario("testsEnumerateRdpSessionsFunctional", module, command, execute);
}
