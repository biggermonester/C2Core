#include "../../WinRM.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

using namespace functional_tests;

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--target", "C2_FUNC_WINRM_TARGET", "WinRM endpoint, e.g. http://host:5985/wsman", true},
        {"--command", "C2_FUNC_WINRM_COMMAND", "Remote command line to execute", true},
        {"--auth", "C2_FUNC_WINRM_AUTH", "Authentication mode: no-cred, kerberos, or userpass", false, false, "no-cred"},
        {"--user", "C2_FUNC_WINRM_USER", "Username for userpass mode, optionally DOMAIN\\user", false},
        {"--password", "C2_FUNC_WINRM_PASSWORD", "Password for userpass mode", false, true},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsWinRMFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

    const std::string target = readInput(inputs[0], argc, argv, interactive);
    const std::string commandLine = readInput(inputs[1], argc, argv, interactive);
    const std::string auth = readInput(inputs[2], argc, argv, interactive);
    const std::string user = readInput(inputs[3], argc, argv, interactive);
    const std::string password = readInput(inputs[4], argc, argv, interactive);

    std::vector<Input> missing;
    if (target.empty())
    {
        missing.push_back(inputs[0]);
    }
    if (commandLine.empty())
    {
        missing.push_back(inputs[1]);
    }
    if (auth == "userpass" && user.empty())
    {
        missing.push_back(inputs[3]);
    }
    if (auth == "userpass" && password.empty())
    {
        missing.push_back(inputs[4]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsWinRMFunctional", missing);
    }

    std::vector<std::string> command = {"winRm"};
    if (auth == "userpass")
    {
        command.insert(command.end(), {"-u", user, password, target, commandLine});
    }
    else if (auth == "kerberos")
    {
        command.insert(command.end(), {"-k", target, commandLine});
    }
    else
    {
        command.insert(command.end(), {"-n", target, commandLine});
    }

    WinRM module;
    return runModuleScenario("testsWinRMFunctional", module, command, execute);
}
