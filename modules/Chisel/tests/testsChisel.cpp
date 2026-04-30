#include "../Chisel.hpp"
#include "../../tests/TestHelpers.hpp"

#include <iostream>
#include <vector>

using namespace test_helpers;

int main()
{
    bool ok = true;

    {
        Chisel module;
        std::vector<std::string> cmd = {"chisel", "status"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "status should be handled on the teamserver side");
        ok &= expect(message.returnvalue().empty(), "status should return an empty instance list by default");
    }

    {
        Chisel module;
        std::vector<std::string> cmd = {"chisel", "stop", "1234"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "stop should pack a pid");
        ok &= expect(message.instruction() == "chisel", "instruction should be set");
        ok &= expect(message.cmd() == "stop", "stop command should be packed");
        ok &= expect(message.pid() == 1234, "pid should be packed");
    }

    {
        Chisel module;
        std::vector<std::string> cmd = {"chisel", "stop", "not-a-pid"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == 0, "atoi-compatible parser currently accepts non-numeric pids as zero");
        ok &= expect(message.pid() == 0, "non-numeric pid should map to zero with current parser");
    }

    {
        Chisel module;
        C2Message message;
        message.set_cmd("stop");
        message.set_pid(2147483647);
        C2Message ret;

#ifdef __linux__
#elif _WIN32
        ok &= expect(module.process(message, ret) == 0, "missing chisel pid should return through C2Message");
        ok &= expect(ret.errorCode() > 0, "missing chisel pid should set an error code");
        ok &= expect(ret.returnvalue().find("OpenProcess failed") != std::string::npos, "missing chisel pid should explain the failure");
        std::string errorMsg;
        ok &= expect(module.errorCodeToMsg(ret, errorMsg) == 0, "missing chisel pid should map error text");
        ok &= expect(errorMsg == ret.returnvalue(), "missing chisel pid error text should come from returnvalue");
#endif
    }

    {
        Chisel module;
        std::vector<std::string> cmd = {"chisel", "missing.exe", "client", "host:8000", "R:socks"};
        C2Message message;

        ok &= expect(module.init(cmd, message) == -1, "missing payload file should be rejected");
        ok &= expect(!message.returnvalue().empty(), "missing payload file should explain the error");
    }

    return ok ? 0 : 1;
}
