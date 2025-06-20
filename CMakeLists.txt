cmake_minimum_required(VERSION 3.15)
project(LibraryServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找并链接 vcpkg 管理的依赖项
# vcpkg 会自动处理 OpenSSL, httplib, json, 和 mysql connector
find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)

# 统一查找 libmysql（vcpkg 包名：libmysql），其会生成 unofficial::libmysql::libmysql 目标。
find_package(unofficial-libmysql REQUIRED)

add_executable(server server.cpp)

# 在编译时，自动将 index.html 复制到构建目录
file(COPY index.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# 链接所有库
target_link_libraries(server PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    unofficial::libmysql::libmysql
) 