#include "../../Evasion.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

using namespace functional_tests;

namespace
{
bool requiresValue(const std::string& action)
{
    return action == "Introspection" || action == "ReadMemory" || action == "PatchMemory";
}

bool requiresExtra(const std::string& action)
{
    return action == "ReadMemory" || action == "PatchMemory";
}
}

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--action", "C2_FUNC_EVASION_ACTION", "Evasion action, e.g. CheckHooks, DisableETW, Unhook, AmsiBypass, Introspection, ReadMemory, PatchMemory, RemotePatch", false, false, "CheckHooks"},
        {"--value", "C2_FUNC_EVASION_VALUE", "Primary value for actions such as Introspection, ReadMemory, or PatchMemory", false},
        {"--extra", "C2_FUNC_EVASION_EXTRA", "Secondary value for actions such as ReadMemory size or PatchMemory bytes", false},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsEvasionFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

#ifndef _WIN32
    if (execute)
    {
        std::cout << "testsEvasionFunctional skipped: execution is only supported on Windows.\n";
        return skipReturnCode;
    }
#endif

    const std::string action = readInput(inputs[0], argc, argv, interactive);
    const std::string value = readInput(inputs[1], argc, argv, interactive);
    const std::string extra = readInput(inputs[2], argc, argv, interactive);

    std::vector<Input> missing;
    if (requiresValue(action) && value.empty())
    {
        missing.push_back(inputs[1]);
    }
    if (requiresExtra(action) && extra.empty())
    {
        missing.push_back(inputs[2]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsEvasionFunctional", missing);
    }

    std::vector<std::string> command = {"evasion", action};
    if (!value.empty())
    {
        command.push_back(value);
    }
    if (!extra.empty())
    {
        command.push_back(extra);
    }

    Evasion module;
    return runModuleScenario("testsEvasionFunctional", module, command, execute);
}
