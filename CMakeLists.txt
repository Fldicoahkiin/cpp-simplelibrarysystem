cmake_minimum_required(VERSION 3.15)

# --- 智能寻找并加载 vcpkg 工具链 (用于改善 IDE/Linter 体验) ---
# 此代码块解决在某些IDE中直接打开项目时，Linter无法找到vcpkg库的问题。
# 它检查 CMAKE_TOOLCHAIN_FILE 是否未被定义，如果是，则直接包含本地的vcpkg.cmake脚本，
# 强制IDE的CMake Linter加载vcpkg。
# 此操作不会影响CI或任何使用预设(Presets)的正确命令行构建流程。
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "/Users/mac/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "IDE/Linter Helper: No toolchain file specified. Forcing inclusion of local vcpkg toolchain.")
    include("/Users/mac/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

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

# 在编译时，自动将前端静态文件 (index.html) 复制到构建输出目录，
# 这样可执行文件才能在运行时找到它。
file(COPY index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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