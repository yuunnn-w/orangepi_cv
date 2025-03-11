cmake_minimum_required(VERSION 3.8)

# 项目名称
project(orangepi_cv)

# 启用MSVC编译器的热重载（如果支持）
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
find_package(OpenCV REQUIRED) #很奇怪opencv只能这样找，pkg-config找不太行
find_package(PkgConfig REQUIRED)
# find_package(nlohmann_json CONFIG REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(nlohmann_json REQUIRED)
find_library(RGA_LIBRARY NAMES rga)


# sudo apt-get install libeigen3-dev
# sudo apt-get install nlohmann-json3-dev

#pkg_check_modules(OpenCV REQUIRED opencv) # 查找OpenCV库
pkg_check_modules(LIBUVC REQUIRED libuvc) # 查找libuvc库
pkg_check_modules(LIBUSB REQUIRED libusb-1.0) # 查找libusb库

# 将src目录下的所有.cpp文件和main.cpp加入到项目中
file(GLOB SOURCES "src/*.cpp" "main.cpp")

# 添加可执行文件目标
add_executable(orangepi_cv ${SOURCES})

# 添加头文件路径
include_directories(
    #/usr/include/libuvc
    include
    #/usr/local/include/uWebSockets
    #/usr/include/opencv4
    ${CMAKE_SOURCE_DIR}/include
    ${LIBUVC_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${LIBUSB_1_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIRS}
    ${nlohmann_json_INCLUDE_DIRS}
    /usr/include/rga
)

# 设置C++标准
if(CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET orangepi_cv PROPERTY CXX_STANDARD 20)
else()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

find_library(RKNN_API_LIBRARY NAMES rknnrt PATHS /usr/lib)
if(RKNN_API_LIBRARY)
    target_link_libraries(orangepi_cv ${RKNN_API_LIBRARY})
else()
    message(FATAL_ERROR "Could not find librknnrt.so")
endif()

# 链接库
target_link_libraries(orangepi_cv
    #uSockets
    #pthread
    #z
    #crypto
    #ssl # 往上都是uWebSockets必须的链接库
    tbb
    nlohmann_json::nlohmann_json
    #nlohmann_json
    ${LIBUVC_LIBRARIES}
    ${OpenCV_LIBS}
    ${LIBUSB_1_LIBRARIES}
    ${RGA_LIBRARY}
    ${EIGEN3_LIBRARIES}
    #${nlohmann_json_LIBRARIES}

)

# 设置 C++ 编译选项以启用 O3 优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# sudo apt-get install dos2unix
# dos2unix install_rknpu.sh