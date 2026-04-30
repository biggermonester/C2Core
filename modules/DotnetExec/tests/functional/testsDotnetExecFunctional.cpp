#include "../../DotnetExec.hpp"
#include "../../../tests/FunctionalTestHelpers.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace functional_tests;

namespace
{
std::string defaultAssemblyPath()
{
#ifdef C2_DOTNETEXEC_DUMMY_EXE
    return C2_DOTNETEXEC_DUMMY_EXE;
#else
    return {};
#endif
}

std::string normalizeKind(const std::string& kind)
{
    return kind == "dll" ? "dll" : "exe";
}
}

int main(int argc, char** argv)
{
    const std::vector<Input> inputs = {
        {"--assembly", "C2_FUNC_DOTNETEXEC_ASSEMBLY", "Managed assembly path", false, false, defaultAssemblyPath()},
        {"--kind", "C2_FUNC_DOTNETEXEC_KIND", "Assembly kind: exe or dll", false, false, "exe"},
        {"--name", "C2_FUNC_DOTNETEXEC_NAME", "Short in-memory module name", false, false, "dummy"},
        {"--type", "C2_FUNC_DOTNETEXEC_TYPE", "Fully-qualified type name for dll mode", false},
        {"--method", "C2_FUNC_DOTNETEXEC_METHOD", "Method to invoke for dll mode", false},
        {"--args", "C2_FUNC_DOTNETEXEC_ARGS", "Arguments passed to the managed entrypoint", false, false, "alpha beta"},
    };

    if (hasFlag(argc, argv, "--help"))
    {
        printUsage("testsDotnetExecFunctional", inputs);
        return 0;
    }

    const bool interactive = hasFlag(argc, argv, "--interactive");
    const bool execute = hasFlag(argc, argv, "--execute");

#ifndef _WIN32
    if (execute)
    {
        std::cout << "testsDotnetExecFunctional skipped: execution is only supported on Windows.\n";
        return skipReturnCode;
    }
#endif

    const std::string assembly = readInput(inputs[0], argc, argv, interactive);
    const std::string kind = normalizeKind(readInput(inputs[1], argc, argv, interactive));
    const std::string name = readInput(inputs[2], argc, argv, interactive);
    const std::string type = readInput(inputs[3], argc, argv, interactive);
    const std::string method = readInput(inputs[4], argc, argv, interactive);
    const std::string arguments = readInput(inputs[5], argc, argv, interactive);

    std::vector<Input> missing;
    if (assembly.empty())
    {
        missing.push_back(inputs[0]);
    }
    if (kind == "dll" && type.empty())
    {
        missing.push_back(inputs[3]);
    }
    if (kind == "dll" && method.empty())
    {
        missing.push_back(inputs[4]);
    }
    if (!missing.empty())
    {
        return skipMissing("testsDotnetExecFunctional", missing);
    }

    DotnetExec module;
    std::vector<std::string> loadCommand = {"dotnetExec", "load", name, assembly};
    if (kind == "dll")
    {
        loadCommand.push_back(type);
    }

    C2Message loadMessage;
    if (module.init(loadCommand, loadMessage) != 0)
    {
        std::cerr << "testsDotnetExecFunctional load init failed:\n" << loadMessage.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsDotnetExecFunctional load init OK\n";
    std::cout << "instruction: " << loadMessage.instruction() << '\n';

    std::vector<std::string> runCommand = {"dotnetExec", kind == "dll" ? "runDll" : "runExe", name};
    if (kind == "dll")
    {
        runCommand.push_back(method);
    }
    if (!arguments.empty())
    {
        runCommand.push_back(arguments);
    }

    C2Message runMessage;
    if (module.init(runCommand, runMessage) != 0)
    {
        std::cerr << "testsDotnetExecFunctional run init failed:\n" << runMessage.returnvalue() << '\n';
        return 1;
    }

    std::cout << "testsDotnetExecFunctional run init OK\n";

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
        std::cerr << "testsDotnetExecFunctional load failed: " << error << '\n';
        return 1;
    }

    C2Message runResult;
    module.process(runMessage, runResult);
    if (runResult.errorCode() > 0)
    {
        std::string error;
        module.errorCodeToMsg(runResult, error);
        std::cerr << "testsDotnetExecFunctional run failed: " << error << '\n';
        return 1;
    }

    std::cout << runResult.returnvalue() << '\n';
    return 0;
}
