#include "../AssemblyExec.hpp"
#include "../../tests/TestHelpers.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace test_helpers;

namespace
{
std::string dummyExePath()
{
#ifdef C2_ASSEMBLYEXEC_DUMMY_EXE
    return C2_ASSEMBLYEXEC_DUMMY_EXE;
#else
    return {};
#endif
}

bool expectAssemblyMessage(const C2Message& message,
                           const std::filesystem::path& expectedInput,
                           const std::string& expectedMode,
                           const std::string& expectedCmdPart,
                           const std::string& label)
{
    bool ok = true;
    ok &= expect(message.instruction() == "assemblyExec", label + ": instruction should be set");
    ok &= expect(message.inputfile() == expectedInput.string(), label + ": input file should be packed");
    ok &= expect(message.args() == expectedMode, label + ": execution mode should be packed");
    ok &= expect(message.pid() == -1, label + ": pid should default to -1");
    ok &= expect(message.cmd().find(expectedCmdPart) != std::string::npos, label + ": original command tail should be packed");
    ok &= expect(!message.data().empty(), label + ": payload should be non-empty");
    return ok;
}
}

int main()
{
    bool ok = true;
    const std::string dummyExe = dummyExePath();

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "thread"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "thread mode should be handled locally");
        ok &= expect(message.returnvalue() == "thread mode.\n", "thread mode should report selected mode");
    }

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "process"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "process mode should be handled locally");
        ok &= expect(message.returnvalue() == "process mode.\n", "process mode should report selected mode");
    }

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "processWithSpoofedParent"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "spoofed-parent mode should be handled locally");
        ok &= expect(message.returnvalue() == "process mode with parent spoofing.\n", "spoofed-parent mode should report selected mode");
    }

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "-r", "missing.bin"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "missing raw shellcode file should be rejected");
        ok &= expect(message.returnvalue().find("Couldn't open file") != std::string::npos, "missing file error should mention open failure");
    }

    {
        const auto raw = writeTempFile("c2core_assembly_raw.bin", "raw-bytes");
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "-r", raw.string()};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "existing raw shellcode file should be accepted");
        ok &= expectAssemblyMessage(message, raw, "1", "-r", "raw process mode");
        ok &= expect(message.data() == "raw-bytes", "raw bytes should be packed");
        std::filesystem::remove(raw);
    }

    {
        const auto raw = writeTempFile("c2core_assembly_raw_thread.bin", "thread-bytes");
        AssemblyExec module;
        C2Message modeMessage;
        std::vector<std::string> modeCmd = {"assemblyExec", "thread"};
        ok &= expect(module.init(modeCmd, modeMessage) == -1, "thread mode selection should be accepted before payload init");

        std::vector<std::string> cmd = {"assemblyExec", "-r", raw.string()};
        C2Message message;
        ok &= expect(module.init(cmd, message) == 0, "raw shellcode should honor thread mode");
        ok &= expectAssemblyMessage(message, raw, "0", "-r", "raw thread mode");
        ok &= expect(message.data() == "thread-bytes", "thread raw bytes should be packed");
        std::filesystem::remove(raw);
    }

    {
        const auto raw = writeTempFile("c2core_assembly_raw_spoof.bin", "spoof-bytes");
        AssemblyExec module;
        C2Message modeMessage;
        std::vector<std::string> modeCmd = {"assemblyExec", "processWithSpoofedParent"};
        ok &= expect(module.init(modeCmd, modeMessage) == -1, "spoofed-parent mode selection should be accepted before payload init");

        std::vector<std::string> cmd = {"assemblyExec", "-r", raw.string()};
        C2Message message;
        ok &= expect(module.init(cmd, message) == 0, "raw shellcode should honor spoofed-parent mode");
        ok &= expectAssemblyMessage(message, raw, "2", "-r", "raw spoofed-parent mode");
        ok &= expect(message.data() == "spoof-bytes", "spoof raw bytes should be packed");
        std::filesystem::remove(raw);
    }

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "-x", "payload.bin"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "unknown payload option should be rejected");
        ok &= expect(message.returnvalue().find("One of the tags") != std::string::npos, "unknown payload option should explain accepted tags");
    }

    {
        AssemblyExec module;
        std::vector<std::string> cmd = {"assemblyExec", "-d", "payload.dll"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "DLL mode without method should be rejected");
        ok &= expect(message.returnvalue().find("Method is mandatory") != std::string::npos, "DLL mode should explain missing method");
    }

    if (dummyExe.empty())
    {
#ifdef _WIN32
        ok &= expect(false, "dummy exe path should be provided by CMake");
#endif
    }
    else
    {
        const std::filesystem::path dummyPath(dummyExe);
        ok &= expect(std::filesystem::exists(dummyPath), "dummy exe should exist before Donut tests");

        {
            AssemblyExec module;
            std::vector<std::string> cmd = {"assemblyExec", "-e", dummyPath.string()};
            C2Message message;

            ok &= expect(module.init(cmd, message) == 0, "dummy exe should be accepted by Donut EXE mode");
            ok &= expectAssemblyMessage(message, dummyPath, "1", "-e", "dummy exe default process mode");
        }

        {
            AssemblyExec module;
            C2Message modeMessage;
            std::vector<std::string> modeCmd = {"assemblyExec", "thread"};
            ok &= expect(module.init(modeCmd, modeMessage) == -1, "thread mode should be configurable before Donut EXE mode");

            std::vector<std::string> cmd = {"assemblyExec", "-e", dummyPath.string(), "alpha", "beta gamma"};
            C2Message message;

            ok &= expect(module.init(cmd, message) == 0, "dummy exe with arguments should be accepted by Donut EXE mode");
            ok &= expectAssemblyMessage(message, dummyPath, "0", "alpha beta gamma", "dummy exe thread mode with args");
            ok &= expect(message.cmd().find(dummyPath.string()) != std::string::npos, "dummy exe command should include input path");
        }

        {
            AssemblyExec module;
            C2Message modeMessage;
            std::vector<std::string> modeCmd = {"assemblyExec", "processWithSpoofedParent"};
            ok &= expect(module.init(modeCmd, modeMessage) == -1, "spoofed-parent mode should be configurable before Donut EXE mode");

            std::vector<std::string> cmd = {"assemblyExec", "-e", dummyPath.string(), "--flag"};
            C2Message message;

            ok &= expect(module.init(cmd, message) == 0, "dummy exe should be accepted in spoofed-parent mode");
            ok &= expectAssemblyMessage(message, dummyPath, "2", "--flag", "dummy exe spoofed-parent mode");
        }
    }

    return ok ? 0 : 1;
}
