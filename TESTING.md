# C2Core Testing

## Scope

`C2Core` is shared by multiple parent projects across Windows and Linux. The test strategy separates:

- unit and local integration tests that must be CI-safe and autonomous
- functional tests that exercise environment-dependent features without polluting standard CI

This file documents the current test logic, the relevant CMake flags, and the commands used in practice on Windows x64 and x86.

## CMake Flags

- `BUILD_TEAMSERVER`
  Enables teamserver-side parsing/help paths inside modules. This is for production builds that need CLI parsing and help text on the server side.

- `C2CORE_BUILD_TESTS`
  Enables the standard C2Core test targets used by CI. These tests must stay local, deterministic, and not depend on lab infrastructure.

- `C2CORE_BUILD_FUNCTIONAL_TESTS`
  Enables manual or env-driven functional tests. These tests are intended for operator-driven validation of modules that need credentials, network reachability, remote hosts, Kerberos tickets, or specific Windows services.

`enable_testing()` is active when either `C2CORE_BUILD_TESTS` or `C2CORE_BUILD_FUNCTIONAL_TESTS` is enabled.

## Test Categories

### Standard tests

These are the `tests<Module>` targets.

- compiled with `C2CORE_BUILD_TESTS=ON`
- expected to run in CI
- should complete quickly
- should not require manual input
- should not require external infrastructure

For modules that are hard to execute for real in CI, the unit tests should still validate:

- command-line parsing
- packing and unpacking of `C2Message`
- error mapping through `errorCodeToMsg()`
- local guard rails before any network or remote execution is attempted

### Functional tests

These are the `tests<Module>Functional` targets.

- compiled with `C2CORE_BUILD_FUNCTIONAL_TESTS=ON`
- intended for manual runs, lab validation, or env-driven automation
- should skip cleanly when configuration is missing
- should not run remote actions unless `--execute` is provided

Current functional tests are registered with `SKIP_RETURN_CODE 77`, so an unconfigured lab does not make CTest fail.

## Functional Test Helper Contract

`core/modules/tests/FunctionalTestHelpers.hpp` provides the shared behavior.

- `--help`
  Prints accepted options and environment variables.

- `--interactive`
  Prompts for missing values on stdin.

- `--execute`
  Actually calls the module `process()` path.

Without `--execute`, a functional test validates only:

- input collection
- command construction
- `init()`
- `C2Message` packing

With `--execute`, the helper treats the run as failed only when `result.errorCode() > 0`.

This is important because many modules keep the default `errorCode == -1` on success.

## Current Functional Test Modules

Current module-level functional tests:

- `testsAssemblyExecFunctional`
- `testsCimExecFunctional`
- `testsDcomExecFunctional`
- `testsDotnetExecFunctional`
- `testsEvasionFunctional`
- `testsEnumerateRdpSessionsFunctional`
- `testsInjectFunctional`
- `testsPsExecFunctional`
- `testsPwShFunctional`
- `testsSpawnAsFunctional`
- `testsSshExecFunctional`
- `testsTaskSchedulerFunctional`
- `testsWinRMFunctional`
- `testsWmiExecFunctional`

Current transport-level functional tests:

- listener functional tests
- beacon functional tests

Use `ctest -L functional` to discover them from a configured build tree.

## Windows Build Commands

### Configure x64 functional build

From PowerShell:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' `
  -S 'E:\Dev\C2Implant' `
  -B 'E:\Dev\C2Implant\buildx64-functional' `
  -G 'Visual Studio 17 2022' `
  -A x64 `
  -DC2CORE_BUILD_TESTS=ON `
  -DC2CORE_BUILD_FUNCTIONAL_TESTS=ON `
  -DLibssh2_DIR='E:/Dev/C2Implant/buildx64/conan/build/generators' `
  -Dnlohmann_json_DIR='E:/Dev/C2Implant/buildx64/conan/build/generators'
```

### Configure x86 functional build

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' `
  -S 'E:\Dev\C2Implant' `
  -B 'E:\Dev\C2Implant\buildx86-functional' `
  -G 'Visual Studio 17 2022' `
  -A Win32 `
  -DC2CORE_BUILD_TESTS=ON `
  -DC2CORE_BUILD_FUNCTIONAL_TESTS=ON `
  -DLibssh2_DIR='E:/Dev/C2Implant/buildx86/conan/build/generators' `
  -Dnlohmann_json_DIR='E:/Dev/C2Implant/buildx86/conan/build/generators'
```

### Build selected test targets

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' `
  --build 'E:\Dev\C2Implant\buildx64-functional' `
  --config Release `
  --target testsSpawnAs testsSshExec testsWmiExec testsSpawnAsFunctional testsSshExecFunctional testsWmiExecFunctional
```

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' `
  --build 'E:\Dev\C2Implant\buildx86-functional' `
  --config Release `
  --target testsSpawnAs testsSshExec testsWmiExec testsSpawnAsFunctional testsSshExecFunctional testsWmiExecFunctional
```

## Standard CTest Commands

### Run one unit test

```powershell
Set-Location 'E:\Dev\C2Implant\buildx64'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' `
  -C Release `
  -R testsSshExec `
  --timeout 10 `
  --output-on-failure
```

### Run a small group of unit tests

```powershell
Set-Location 'E:\Dev\C2Implant\buildx64'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' `
  -C Release `
  -R 'tests(SpawnAs|SshExec|WmiExec)' `
  --timeout 10 `
  --output-on-failure
```

### Run all functional tests

```powershell
Set-Location 'E:\Dev\C2Implant\buildx64-functional'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' `
  -C Release `
  -L functional `
  --timeout 10 `
  --output-on-failure
```

### Run only functional tests for remote execution modules

```powershell
Set-Location 'E:\Dev\C2Implant\buildx64-functional'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' `
  -C Release `
  -R 'tests(SpawnAs|SshExec|WmiExec)Functional' `
  --timeout 10 `
  --output-on-failure
```

If no environment is configured, these tests should be reported as skipped rather than failed.

## Direct Functional Test Invocation

The binaries are copied under `${build}/tests`.

Examples on x64:

```powershell
E:\Dev\C2Implant\buildx64-functional\tests\testsSshExecFunctional.exe --help
E:\Dev\C2Implant\buildx64-functional\tests\testsWmiExecFunctional.exe --help
E:\Dev\C2Implant\buildx64-functional\tests\testsSpawnAsFunctional.exe --help
```

### Dry-run packing validation

This validates parsing and `init()` only:

```powershell
E:\Dev\C2Implant\buildx64-functional\tests\testsSshExecFunctional.exe `
  --host 10.10.10.20 `
  --port 22 `
  --user alice `
  --password secret `
  --command "whoami"
```

### Interactive mode

```powershell
E:\Dev\C2Implant\buildx64-functional\tests\testsWmiExecFunctional.exe --interactive
```

### Real execution

Only this form runs the remote or privileged action:

```powershell
E:\Dev\C2Implant\buildx64-functional\tests\testsSshExecFunctional.exe `
  --execute `
  --host 10.10.10.20 `
  --user alice `
  --password secret `
  --command "whoami"
```

## Environment Variables For Functional Tests

### SpawnAs

- `C2_FUNC_SPAWNAS_USER`
- `C2_FUNC_SPAWNAS_PASSWORD`
- `C2_FUNC_SPAWNAS_COMMAND`
- `C2_FUNC_SPAWNAS_DOMAIN`
- `C2_FUNC_SPAWNAS_LOGON_TYPE`
- `C2_FUNC_SPAWNAS_PROFILE`
- `C2_FUNC_SPAWNAS_SHOW_WINDOW`

Example:

```powershell
$env:C2_FUNC_SPAWNAS_USER='DOMAIN\alice'
$env:C2_FUNC_SPAWNAS_PASSWORD='secret'
$env:C2_FUNC_SPAWNAS_COMMAND='cmd.exe /c whoami'
E:\Dev\C2Implant\buildx64-functional\tests\testsSpawnAsFunctional.exe --execute
```

Notes:

- logon type `2` is the default interactive mode
- logon type `9` maps to `--netonly`
- `with-profile` and `no-profile` control profile loading
- this test is Windows-only for real execution

### SshExec

- `C2_FUNC_SSH_HOST`
- `C2_FUNC_SSH_PORT`
- `C2_FUNC_SSH_USER`
- `C2_FUNC_SSH_PASSWORD`
- `C2_FUNC_SSH_COMMAND`

Example:

```powershell
$env:C2_FUNC_SSH_HOST='10.10.10.20'
$env:C2_FUNC_SSH_PORT='22'
$env:C2_FUNC_SSH_USER='alice'
$env:C2_FUNC_SSH_PASSWORD='secret'
$env:C2_FUNC_SSH_COMMAND='whoami'
E:\Dev\C2Implant\buildx64-functional\tests\testsSshExecFunctional.exe --execute
```

### WmiExec

- `C2_FUNC_WMI_AUTH`
- `C2_FUNC_WMI_TARGET`
- `C2_FUNC_WMI_COMMAND`
- `C2_FUNC_WMI_USER`
- `C2_FUNC_WMI_PASSWORD`
- `C2_FUNC_WMI_DC`

Example NTLM or user/password flow:

```powershell
$env:C2_FUNC_WMI_AUTH='userpass'
$env:C2_FUNC_WMI_TARGET='server01'
$env:C2_FUNC_WMI_COMMAND='cmd.exe /c whoami'
$env:C2_FUNC_WMI_USER='DOMAIN\alice'
$env:C2_FUNC_WMI_PASSWORD='secret'
E:\Dev\C2Implant\buildx64-functional\tests\testsWmiExecFunctional.exe --execute
```

Example Kerberos flow:

```powershell
$env:C2_FUNC_WMI_AUTH='kerberos'
$env:C2_FUNC_WMI_TARGET='server01'
$env:C2_FUNC_WMI_COMMAND='cmd.exe /c hostname'
$env:C2_FUNC_WMI_DC='DOMAIN\dc01'
E:\Dev\C2Implant\buildx64-functional\tests\testsWmiExecFunctional.exe --execute
```

Notes:

- `no-cred` maps to `wmiExec -n`
- `userpass` maps to `wmiExec -u`
- `kerberos` maps to `wmiExec -k`
- this test is Windows-only for real execution

### AssemblyExec

- `C2_FUNC_ASSEMBLYEXEC_PAYLOAD`
- `C2_FUNC_ASSEMBLYEXEC_KIND`
- `C2_FUNC_ASSEMBLYEXEC_MODE`
- `C2_FUNC_ASSEMBLYEXEC_METHOD`
- `C2_FUNC_ASSEMBLYEXEC_ARGS`
- `C2_FUNC_ASSEMBLYEXEC_PROCESS`
- `C2_FUNC_ASSEMBLYEXEC_SPOOFED_PARENT`

### DotnetExec

- `C2_FUNC_DOTNETEXEC_ASSEMBLY`
- `C2_FUNC_DOTNETEXEC_KIND`
- `C2_FUNC_DOTNETEXEC_NAME`
- `C2_FUNC_DOTNETEXEC_TYPE`
- `C2_FUNC_DOTNETEXEC_METHOD`
- `C2_FUNC_DOTNETEXEC_ARGS`

### Evasion

- `C2_FUNC_EVASION_ACTION`
- `C2_FUNC_EVASION_VALUE`
- `C2_FUNC_EVASION_EXTRA`

### Inject

- `C2_FUNC_INJECT_PAYLOAD`
- `C2_FUNC_INJECT_KIND`
- `C2_FUNC_INJECT_PID`
- `C2_FUNC_INJECT_METHOD`
- `C2_FUNC_INJECT_ARGS`
- `C2_FUNC_INJECT_PROCESS`
- `C2_FUNC_INJECT_SYSCALL`

### PsExec

- `C2_FUNC_PSEXEC_AUTH`
- `C2_FUNC_PSEXEC_TARGET`
- `C2_FUNC_PSEXEC_SERVICE`
- `C2_FUNC_PSEXEC_USER`
- `C2_FUNC_PSEXEC_PASSWORD`

### PwSh

- `C2_FUNC_PWSH_MODE`
- `C2_FUNC_PWSH_RUNNER`
- `C2_FUNC_PWSH_TYPE`
- `C2_FUNC_PWSH_COMMAND`
- `C2_FUNC_PWSH_IMPORT`
- `C2_FUNC_PWSH_SCRIPT`

## Module-Specific Intent

### SpawnAs

The unit test should stay focused on parsing and packing. The functional test is where real credentialed process creation is exercised because it depends on:

- local Windows privileges
- local or domain credentials
- profile loading behavior
- logon type selection

### SshExec

The unit test should validate local parameter handling and local failure paths. The functional test is the place for real SSH coverage because it depends on:

- reachable host
- valid credentials
- server-side shell behavior

### WmiExec

The unit test should validate packing and error mapping. The functional test is required for meaningful execution coverage because it depends on:

- COM and WMI availability
- remote host state
- firewall and service exposure
- credential mode or Kerberos ticket state

## CI Guidance

Standard CI should keep using `C2CORE_BUILD_TESTS=ON` only.

`C2CORE_BUILD_FUNCTIONAL_TESTS=ON` is appropriate for:

- operator-driven validation
- dedicated lab jobs
- manual pre-release verification

Do not make the default CI pipeline depend on environment-specific functional tests.
