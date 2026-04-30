#include "../../Inject.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace functional_tests;

namespace
{
std::string defaultPayloadPath()
{
#ifdef C2_INJECT_DUMMY_EXE
    return C2_INJECT_DUMMY_EXE;
#else
    return {};
#endif
}

std::string defaultSpawnProcess()
{
#ifdef C2_INJECT_DUMMY_EXE
    return C2_INJECT_DUMMY_EXE;
#else
    return {};
#endif
}

std::string normalizeKind(const std::string& kind)
{
    if (kind == "dll")
    {
        return "dll";
    }
    if (kind == "raw")
    {
        return "raw";
    }
    return "exe";
}

bool isTruthy(const std::string& value)
{
    return value == "1" || value == "true" || value == "TRUE" || value == "yes" || value == "YES";
}
}

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--payload", "C2_FUNC_INJECT_PAYLOAD", "Payload path", false, false, defaultPayloadPath()},
        {"--kind", "C2_FUNC_INJECT_KIND", "Payload kind: exe, dll, or raw", false, false, "exe"},
        {"--pid", "C2_FUNC_INJECT_PID", "Target pid, or -1 to spawn a new process", false, false, "-1"},
        {"--method", "C2_FUNC_INJECT_METHOD", "DLL method name for dll mode", false},
        {"--args", "C2_FUNC_INJECT_ARGS", "Arguments passed to the payload", false},
        {"--spawn-process", "C2_FUNC_INJECT_PROCESS", "Process path to spawn when pid is negative", false, false, defaultSpawnProcess()},
        {"--syscall", "C2_FUNC_INJECT_SYSCALL", "Enable syscall path (1/0)", false, false, "0"},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsInjectFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

#ifndef _WIN32
    if (execute)
    {
        std::cout << "testsInjectFunctional skipped: execution is only supported on Windows in this harness.\n";
        return skipReturnCode;
    }
#endif

    const std::string payload = readInput(inputs[0], argc, argv, interactive);
    const std::string kind = normalizeKind(readInput(inputs[1], argc, argv, interactive));
    const std::string pid = readInput(inputs[2], argc, argv, interactive);
    const std::string method = readInput(inputs[3], argc, argv, interactive);
    const std::string arguments = readInput(inputs[4], argc, argv, interactive);
    const std::string processToSpawn = readInput(inputs[5], argc, argv, interactive);
    const std::string useSyscall = readInput(inputs[6], argc, argv, interactive);

    std::vector<Input> missing;
    if (payload.empty())
    {
        missing.push_back(inputs[0]);
    }
    if (kind == "dll" && method.empty())
    {
        missing.push_back(inputs[3]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsInjectFunctional", missing);
    }

    std::vector<std::string> command = {"inject"};
    if (kind == "raw")
    {
        command.push_back("-r");
        command.push_back(payload);
    }
    else if (kind == "dll")
    {
        command.push_back("-d");
        command.push_back(payload);
        command.push_back(pid);
        command.push_back(method);
        if (!arguments.empty())
        {
            command.push_back(arguments);
        }
    }
    else
    {
        command.push_back("-e");
        command.push_back(payload);
        command.push_back(pid);
        if (!arguments.empty())
        {
            command.push_back(arguments);
        }
    }

    if (kind == "raw")
    {
        command.push_back(pid);
    }

    Inject module;
    C2Message message;
    if (module.init(command, message) != 0)
    {
        std::cerr << "testsInjectFunctional init failed:\n" << message.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsInjectFunctional init OK\n";
    std::cout << "instruction: " << message.instruction() << '\n';
    std::cout << "pid: " << message.pid() << '\n';

    if (!execute)
    {
        std::cout << "execution skipped; pass --execute to run the module process path.\n";
        return 0;
    }

    nlohmann::json config;
    if (!processToSpawn.empty())
    {
        config["process"] = processToSpawn;
    }
    if (isTruthy(useSyscall))
    {
        config["syscall"] = true;
    }
    if (!config.empty())
    {
        module.initConfig(config);
    }

    C2Message result;
    module.process(message, result);
    std::cout << result.returnvalue() << '\n';
    return 0;
}
