include_directories(../)
add_library(DcomExec SHARED DcomExec.cpp)
set_property(TARGET DcomExec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
if (WIN32)
    target_compile_definitions(DcomExec PRIVATE _WIN32_WINNT=0x0601)
    target_link_libraries(DcomExec PRIVATE ole32 oleaut32)
endif()
add_custom_command(TARGET DcomExec POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
$<TARGET_FILE:DcomExec> "${C2_RUNTIME_MODULE_OUTPUT_DIR}/$<TARGET_FILE_NAME:DcomExec>")

if(C2CORE_BUILD_TESTS)
    add_executable(testsDcomExec tests/testsDcomExec.cpp DcomExec.cpp)
    if (WIN32)
        target_compile_definitions(testsDcomExec PRIVATE _WIN32_WINNT=0x0601)
        target_link_libraries(testsDcomExec PRIVATE ole32 oleaut32)
    endif()
    add_custom_command(TARGET testsDcomExec POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:testsDcomExec> "${C2_TEST_BIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:testsDcomExec>")

    add_test(NAME testsDcomExec COMMAND "${C2_TEST_BIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:testsDcomExec>")
endif()

if(C2CORE_BUILD_FUNCTIONAL_TESTS)
    add_executable(testsDcomExecFunctional tests/functional/testsDcomExecFunctional.cpp DcomExec.cpp)
    target_include_directories(testsDcomExecFunctional PRIVATE ../tests)
    if (WIN32)
        target_compile_definitions(testsDcomExecFunctional PRIVATE _WIN32_WINNT=0x0601)
        target_link_libraries(testsDcomExecFunctional PRIVATE ole32 oleaut32)
    endif()
    add_custom_command(TARGET testsDcomExecFunctional POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:testsDcomExecFunctional> "${C2_TEST_BIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:testsDcomExecFunctional>")

    add_test(NAME testsDcomExecFunctional COMMAND "${C2_TEST_BIN_OUTPUT_DIR}/$<TARGET_FILE_NAME:testsDcomExecFunctional>")
    set_tests_properties(testsDcomExecFunctional PROPERTIES LABELS "functional;dcom;windows-network" SKIP_RETURN_CODE 77)
endif()
