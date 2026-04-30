#include "../../AssemblyExec.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace functional_tests;

namespace
{
std::string defaultPayloadPath()
{
#ifdef C2_ASSEMBLYEXEC_DUMMY_EXE
    return C2_ASSEMBLYEXEC_DUMMY_EXE;
#else
    return {};
#endif
}

std::string normalizeMode(const std::string& mode)
{
    if (mode == "thread")
    {
        return "thread";
    }
    if (mode == "process-spoofed" || mode == "processWithSpoofedParent")
    {
        return "processWithSpoofedParent";
    }
    return "process";
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
}

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--payload", "C2_FUNC_ASSEMBLYEXEC_PAYLOAD", "Assembly or shellcode path", false, false, defaultPayloadPath()},
        {"--kind", "C2_FUNC_ASSEMBLYEXEC_KIND", "Payload kind: exe, dll, or raw", false, false, "exe"},
        {"--mode", "C2_FUNC_ASSEMBLYEXEC_MODE", "Execution mode: thread, process, or process-spoofed", false, false, "process"},
        {"--method", "C2_FUNC_ASSEMBLYEXEC_METHOD", "DLL method name for dll mode", false},
        {"--args", "C2_FUNC_ASSEMBLYEXEC_ARGS", "Arguments passed to the assembly entrypoint", false},
        {"--process", "C2_FUNC_ASSEMBLYEXEC_PROCESS", "Optional process to spawn for process mode", false},
        {"--spoofed-parent", "C2_FUNC_ASSEMBLYEXEC_SPOOFED_PARENT", "Optional spoofed parent process path", false},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsAssemblyExecFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

#ifndef _WIN32
    if (execute)
    {
        std::cout << "testsAssemblyExecFunctional skipped: execution is only supported on Windows in this harness.\n";
        return skipReturnCode;
    }
#endif

    const std::string payload = readInput(inputs[0], argc, argv, interactive);
    const std::string kind = normalizeKind(readInput(inputs[1], argc, argv, interactive));
    const std::string mode = normalizeMode(readInput(inputs[2], argc, argv, interactive));
    const std::string method = readInput(inputs[3], argc, argv, interactive);
    const std::string arguments = readInput(inputs[4], argc, argv, interactive);
    const std::string processToSpawn = readInput(inputs[5], argc, argv, interactive);
    const std::string spoofedParent = readInput(inputs[6], argc, argv, interactive);

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
        return skipMissing("testsAssemblyExecFunctional", missing);
    }

    AssemblyExec module;

    C2Message modeMessage;
    std::vector<std::string> modeCommand = {"assemblyExec", mode};
    if (module.init(modeCommand, modeMessage) != -1)
    {
        std::cerr << "testsAssemblyExecFunctional mode selection failed\n";
        return 1;
    }

    std::vector<std::string> command = {"assemblyExec"};
    if (kind == "raw")
    {
        command.push_back("-r");
        command.push_back(payload);
    }
    else if (kind == "dll")
    {
        command.push_back("-d");
        command.push_back(payload);
        command.push_back(method);
    }
    else
    {
        command.push_back("-e");
        command.push_back(payload);
    }
    if (!arguments.empty())
    {
        command.push_back(arguments);
    }

    C2Message message;
    if (module.init(command, message) != 0)
    {
        std::cerr << "testsAssemblyExecFunctional init failed:\n" << message.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsAssemblyExecFunctional init OK\n";
    std::cout << "instruction: " << message.instruction() << '\n';
    std::cout << "input: " << message.inputfile() << '\n';

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
    if (!spoofedParent.empty())
    {
        config["spoofedParent"] = spoofedParent;
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
