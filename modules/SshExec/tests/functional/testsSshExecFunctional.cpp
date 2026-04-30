#include "../../SshExec.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

using namespace functional_tests;

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--host", "C2_FUNC_SSH_HOST", "SSH hostname or IP", true},
        {"--port", "C2_FUNC_SSH_PORT", "SSH port", false, false, "22"},
        {"--user", "C2_FUNC_SSH_USER", "SSH username", true},
        {"--password", "C2_FUNC_SSH_PASSWORD", "SSH password", true, true},
        {"--command", "C2_FUNC_SSH_COMMAND", "Remote command to execute", true},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsSshExecFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

    const std::string host = readInput(inputs[0], argc, argv, interactive);
    const std::string port = readInput(inputs[1], argc, argv, interactive);
    const std::string user = readInput(inputs[2], argc, argv, interactive);
    const std::string password = readInput(inputs[3], argc, argv, interactive);
    const std::string commandLine = readInput(inputs[4], argc, argv, interactive);

    std::vector<Input> missing;
    if (host.empty())
    {
        missing.push_back(inputs[0]);
    }
    if (user.empty())
    {
        missing.push_back(inputs[2]);
    }
    if (password.empty())
    {
        missing.push_back(inputs[3]);
    }
    if (commandLine.empty())
    {
        missing.push_back(inputs[4]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsSshExecFunctional", missing);
    }

    std::vector<std::string> command = {
        "sshExec",
        "-h", host,
        "-P", port,
        "-u", user,
        "-p", password,
        "--", commandLine
    };

    SshExec module;
    return runModuleScenario("testsSshExecFunctional", module, command, execute);
}
