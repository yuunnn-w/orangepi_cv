# CMakeList.txt: pi_cv_server 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("pi_cv_server")

# 将源代码添加到此项目的可执行文件。
add_executable (pi_cv_server "pi_cv_server.cpp" "pi_cv_server.h" "server.h" "server.cpp" "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET pi_cv_server PROPERTY CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# TODO: 如有需要，请添加测试并安装目标。

# 添加uWebSockets头文件路径
include_directories(/usr/local/include/uWebSockets)

target_link_libraries(pi_cv_server
    uSockets
    pthread
    z
    crypto
    ssl
)