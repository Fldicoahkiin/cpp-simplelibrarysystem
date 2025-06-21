cmake_minimum_required(VERSION 3.15)

project(LibraryServer LANGUAGES CXX)

#-----------------------------------------------------------------------------
# C++ 标准和编译选项
#-----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE ON) # 在构建时显示详细的编译命令

#-----------------------------------------------------------------------------
# 依赖项查找 (由 vcpkg 自动处理)
#-----------------------------------------------------------------------------
# 注意：find_package 的包名不一定与 vcpkg.json 中的完全一致。
# 应使用库安装后提供的CMake包名 (例如 vcpkg 包 'cpp-httplib' 提供的是 'httplib')。
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-libmysql CONFIG REQUIRED)

#-----------------------------------------------------------------------------
# 构建目标定义
#-----------------------------------------------------------------------------
# 定义最终生成的可执行文件 "server"，源文件为 "server.cpp"
add_executable(server server.cpp)

# 将构建目录添加到头文件搜索路径，以便找到生成的 index_html.h
target_include_directories(server PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

#-----------------------------------------------------------------------------
# 链接库
#-----------------------------------------------------------------------------
# 将所有第三方库链接到我们的 "server" 可执行文件。
# 这些目标 (如 httplib::httplib) 是由 vcpkg 的包配置文件定义的。
target_link_libraries(server PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    unofficial::libmysql::libmysql
) 