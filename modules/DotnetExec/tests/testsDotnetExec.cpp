#include "../DotnetExec.hpp"
#include "../../tests/TestHelpers.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace test_helpers;

namespace
{
constexpr const char* kLoadModule = "00001";
constexpr const char* kRunExe = "00002";
constexpr const char* kRunDll = "00003";

std::string dummyExePath()
{
#ifdef C2_DOTNETEXEC_DUMMY_EXE
    return C2_DOTNETEXEC_DUMMY_EXE;
#else
    return {};
#endif
}

std::filesystem::path writeTempFileWithExtension(const std::string& stem,
                                                 const std::string& extension,
                                                 const std::string& content)
{
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
        + "_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    const auto path = std::filesystem::temp_directory_path() / (stem + "_" + suffix + extension);
    std::ofstream out(path, std::ios::binary);
    out << content;
    return path;
}

bool expectLoadMessage(const C2Message& message,
                       const std::filesystem::path& expectedInput,
                       const std::string& expectedName,
                       const std::string& expectedType,
                       const std::string& expectedData,
                       const std::string& label)
{
    bool ok = true;
    ok &= expect(message.instruction() == "dotnetExec", label + ": instruction should be set");
    ok &= expect(message.cmd() == kLoadModule, label + ": load command id should be packed");
    ok &= expect(message.inputfile() == expectedInput.string(), label + ": input file should be packed");
    ok &= expect(message.args() == expectedName, label + ": module short name should be packed");
    ok &= expect(message.returnvalue() == expectedType, label + ": DLL type should be packed");
    ok &= expect(message.data() == expectedData, label + ": assembly bytes should be packed");
    return ok;
}

bool expectErrorMessage(DotnetExec& module, int errorCode, const std::string& expected, const std::string& label)
{
    C2Message message;
    message.set_errorCode(errorCode);
    std::string error;

    bool ok = true;
    ok &= expect(module.errorCodeToMsg(message, error) == 0, label + ": errorCodeToMsg should return 0");
    ok &= expect(error == expected, label + ": error text should match");
    return ok;
}
}

int main()
{
    bool ok = true;
    const std::string dummyExe = dummyExePath();

    {
        const auto exe = writeTempFileWithExtension("c2core_dotnetexec_dummy", ".exe", "exe-bytes");
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "tool", exe.string()};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "load exe should be accepted without type");
        ok &= expectLoadMessage(message, exe, "tool", "", "exe-bytes", "load exe");
        std::filesystem::remove(exe);
    }

    {
        const auto dll = writeTempFileWithExtension("c2core_dotnetexec_dummy", ".dll", "dll-bytes");
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "library", dll.string(), "Dummy.Namespace.Type"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "load dll should be accepted with type");
        ok &= expectLoadMessage(message, dll, "library", "Dummy.Namespace.Type", "dll-bytes", "load dll");
        std::filesystem::remove(dll);
    }

    {
        const auto exe = writeTempFileWithExtension("c2core_dotnetexec_typed_exe", ".exe", "exe-bytes");
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "tool", exe.string(), "Unexpected.Type"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "load exe with type should be rejected");
        ok &= expect(message.returnvalue().find("For exe typeForDll need to be left empty") != std::string::npos,
                     "load exe with type should explain the type rule");
        std::filesystem::remove(exe);
    }

    {
        const auto dll = writeTempFileWithExtension("c2core_dotnetexec_untyped_dll", ".dll", "dll-bytes");
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "library", dll.string()};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "load dll without type should be rejected");
        ok &= expect(message.returnvalue().find("For dll typeForDll need to specify") != std::string::npos,
                     "load dll without type should explain the type rule");
        std::filesystem::remove(dll);
    }

    {
        const auto txt = writeTempFileWithExtension("c2core_dotnetexec_wrong_ext", ".txt", "bytes");
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "bad", txt.string()};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "load unsupported extension should be rejected");
        ok &= expect(message.returnvalue().find("For exe typeForDll need to be left empty") != std::string::npos,
                     "unsupported extension should explain accepted load forms");
        std::filesystem::remove(txt);
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "load", "missing", "missing.exe"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "load missing assembly should be rejected");
        ok &= expect(message.returnvalue().find("Couldn't open file") != std::string::npos,
                     "missing assembly should explain the file error");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "runExe", "tool"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "runExe without arguments should be accepted");
        ok &= expect(message.instruction() == "dotnetExec", "runExe: instruction should be set");
        ok &= expect(message.cmd() == kRunExe, "runExe: command id should be packed");
        ok &= expect(message.data() == "tool", "runExe: module short name should be packed");
        ok &= expect(message.args().empty(), "runExe: empty arguments should stay empty");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "runExe", "tool", "alpha", "beta gamma"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "runExe with arguments should be accepted");
        ok &= expect(message.cmd() == kRunExe, "runExe args: command id should be packed");
        ok &= expect(message.data() == "tool", "runExe args: module short name should be packed");
        ok &= expect(message.args() == "alpha beta gamma ", "runExe args: arguments should be packed with existing spacing behavior");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "runDll", "library", "Invoke"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "runDll without arguments should be accepted");
        ok &= expect(message.instruction() == "dotnetExec", "runDll: instruction should be set");
        ok &= expect(message.cmd() == kRunDll, "runDll: command id should be packed");
        ok &= expect(message.returnvalue() == "library", "runDll: module short name should be packed");
        ok &= expect(message.data() == "Invoke", "runDll: method should be packed");
        ok &= expect(message.args().empty(), "runDll: empty arguments should stay empty");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "runDll", "library", "Invoke", "echo", "test"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "runDll with arguments should be accepted");
        ok &= expect(message.cmd() == kRunDll, "runDll args: command id should be packed");
        ok &= expect(message.returnvalue() == "library", "runDll args: module short name should be packed");
        ok &= expect(message.data() == "Invoke", "runDll args: method should be packed");
        ok &= expect(message.args() == "echo test ", "runDll args: arguments should be packed with existing spacing behavior");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "runDll", "library"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "runDll without method should be rejected");
    }

    {
        DotnetExec module;
        std::vector<std::string> cmd = {"dotnetExec", "unknown"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "unknown command should be rejected");
    }

    {
        DotnetExec module;

        ok &= expectErrorMessage(module, 1, "Failed: CLRCreateInstance", "error code 1");
        ok &= expectErrorMessage(module, 2, "Failed: GetRuntime", "error code 2");
        ok &= expectErrorMessage(module, 3, "Failed: RuntimeInfo - IsLoadable", "error code 3");
        ok &= expectErrorMessage(module, 4, "Failed: RuntimeInfo - GetInterface CLRRuntimeHost", "error code 4");
        ok &= expectErrorMessage(module, 5, "Failed: ClrRuntimeHost - Start", "error code 5");
        ok &= expectErrorMessage(module, 6, "Failed: RuntimeInfo - GetInterface CorRuntimeHost", "error code 6");
        ok &= expectErrorMessage(module, 7, "Failed: CorHost - GetDefaultDomain", "error code 7");
        ok &= expectErrorMessage(module, 8, "Failed: AppDomainThunk - QueryInterface", "error code 8");
        ok &= expectErrorMessage(module, 11, "Failed: IdentityManagerProc", "error code 11");
        ok &= expectErrorMessage(module, 12, "Failed: IdentityMnaager - GetBindingIdentityFromStream", "error code 12");
        ok &= expectErrorMessage(module, 13, "Failed: DefaultAppDomain - Load_2", "error code 13");
        ok &= expectErrorMessage(module, 14, "Failed: DefaultAppDomain - Load_3", "error code 14");
        ok &= expectErrorMessage(module, 15, "Failed: No module loaded", "error code 15");
        ok &= expectErrorMessage(module, 21, "Failed: Assembly - GetType_2", "error code 21");
        ok &= expectErrorMessage(module, 22, "Failed: Type - InvokeMember_3", "error code 22");
        ok &= expectErrorMessage(module, 23, "Failed: InvokeMember_3 - COM exception", "error code 23");
        ok &= expectErrorMessage(module, 24, "Failed: InvokeMember_3 - unknown exception", "error code 24");
        ok &= expectErrorMessage(module, 31, "Failed: Assembly null", "error code 31");
        ok &= expectErrorMessage(module, 32, "Failed: Assembly - EntryPoint", "error code 32");
        ok &= expectErrorMessage(module, 33, "Failed: Invoke_3", "error code 33");
        ok &= expectErrorMessage(module, 34, "Failed: Invoke_3 - COM exception", "error code 34");
        ok &= expectErrorMessage(module, 35, "Failed: Invoke_3 - unknown exception", "error code 35");
        ok &= expectErrorMessage(module, 999, "Failed: Unknown error", "unknown error code");

        C2Message noError;
        noError.set_errorCode(0);
        std::string untouched = "unchanged";
        ok &= expect(module.errorCodeToMsg(noError, untouched) == 0, "zero error code should return 0");
        ok &= expect(untouched == "unchanged", "zero error code should leave existing error text unchanged");
    }

#if defined(_WIN32)
    if (dummyExe.empty())
    {
        ok &= expect(false, "dummy C# exe path should be provided by CMake");
    }
    else
    {
        const std::filesystem::path dummyPath(dummyExe);
        ok &= expect(std::filesystem::exists(dummyPath), "dummy C# exe should exist before execution test");

        if (std::filesystem::exists(dummyPath))
        {
            DotnetExec module;

            {
                std::vector<std::string> cmd = {"dotnetExec", "load", "dummy", dummyPath.string()};
                C2Message message;
                C2Message retMessage;

                ok &= expect(module.init(cmd, message) == 0, "dummy C# exe load init should be accepted");
                ok &= expect(module.process(message, retMessage) == 0, "dummy C# exe should load into CLR");
                ok &= expect(retMessage.errorCode() == -1, "dummy C# exe load should not set an error code");
                ok &= expect(retMessage.returnvalue() == "Success", "dummy C# exe load should report success");
            }

            {
                std::vector<std::string> cmd = {"dotnetExec", "runExe", "dummy", "alpha", "beta"};
                C2Message message;
                C2Message retMessage;

                ok &= expect(module.init(cmd, message) == 0, "dummy C# exe run init should be accepted");
                ok &= expect(module.process(message, retMessage) == 0, "dummy C# exe should execute");
                ok &= expect(retMessage.errorCode() == -1, "dummy C# exe run should not set an error code");
                ok &= expect(retMessage.returnvalue().find("dotnetexec-dummy alpha beta") != std::string::npos,
                             "dummy C# exe output should be captured");
            }
        }
    }
#endif

    return ok ? 0 : 1;
}
