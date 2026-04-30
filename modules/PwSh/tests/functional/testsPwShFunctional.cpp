#include "../../PwSh.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace functional_tests;

namespace
{
std::string defaultRunnerPath()
{
    return (std::filesystem::path(__FILE__).parent_path().parent_path() / "rdm.dll").string();
}
}

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--mode", "C2_FUNC_PWSH_MODE", "Execution mode: run, import, or script", false, false, "run"},
        {"--runner", "C2_FUNC_PWSH_RUNNER", "PowerShell runner dll path", false, false, defaultRunnerPath()},
        {"--type", "C2_FUNC_PWSH_TYPE", "Fully-qualified runner type", false, false, "rdm.rdm"},
        {"--command", "C2_FUNC_PWSH_COMMAND", "PowerShell command to run", false, false, "Write-Output \"c2core-pwsh-functional\""},
        {"--import", "C2_FUNC_PWSH_IMPORT", "PowerShell script path to import as a module", false},
        {"--script", "C2_FUNC_PWSH_SCRIPT", "PowerShell script path to execute", false},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsPwShFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

#ifndef _WIN32
    if (execute)
    {
        std::cout << "testsPwShFunctional skipped: execution is only supported on Windows.\n";
        return skipReturnCode;
    }
#endif

    const std::string mode = readInput(inputs[0], argc, argv, interactive);
    const std::string runner = readInput(inputs[1], argc, argv, interactive);
    const std::string type = readInput(inputs[2], argc, argv, interactive);
    const std::string commandText = readInput(inputs[3], argc, argv, interactive);
    const std::string importPath = readInput(inputs[4], argc, argv, interactive);
    const std::string scriptPath = readInput(inputs[5], argc, argv, interactive);

    std::vector<Input> missing;
    if (runner.empty())
    {
        missing.push_back(inputs[1]);
    }
    if (mode == "import" && importPath.empty())
    {
        missing.push_back(inputs[4]);
    }
    if (mode == "script" && scriptPath.empty())
    {
        missing.push_back(inputs[5]);
    }
    if (mode == "run" && commandText.empty())
    {
        missing.push_back(inputs[3]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsPwShFunctional", missing);
    }

    PwSh module;
    std::vector<std::string> loadCommand = {"pwSh", "init", runner, type};
    C2Message loadMessage;
    if (module.init(loadCommand, loadMessage) != 0)
    {
        std::cerr << "testsPwShFunctional init failed:\n" << loadMessage.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsPwShFunctional runner init OK\n";

    std::vector<std::string> actionCommand = {"pwSh"};
    if (mode == "import")
    {
        actionCommand.insert(actionCommand.end(), {"import", importPath});
    }
    else if (mode == "script")
    {
        actionCommand.insert(actionCommand.end(), {"script", scriptPath});
    }
    else
    {
        actionCommand.insert(actionCommand.end(), {"run", commandText});
    }

    C2Message actionMessage;
    if (module.init(actionCommand, actionMessage) != 0)
    {
        std::cerr << "testsPwShFunctional action init failed:\n" << actionMessage.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsPwShFunctional action init OK\n";

    if (!execute)
    {
        std::cout << "execution skipped; pass --execute to run the module process path.\n";
        return 0;
    }

    C2Message loadResult;
    module.process(loadMessage, loadResult);
    if (loadResult.errorCode() > 0)
    {
        std::string error;
        module.errorCodeToMsg(loadResult, error);
        std::cerr << "testsPwShFunctional load failed: " << error << '\n';
        return 1;
    }

    C2Message actionResult;
    module.process(actionMessage, actionResult);
    if (actionResult.errorCode() > 0)
    {
        std::string error;
        module.errorCodeToMsg(actionResult, error);
        std::cerr << "testsPwShFunctional action failed: " << error << '\n';
        return 1;
    }

    std::cout << actionResult.returnvalue() << '\n';
    return 0;
}
