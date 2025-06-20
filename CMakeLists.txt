cmake_minimum_required(VERSION 3.15)
project(LibraryServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 寻找并链接 vcpkg 管理的依赖项
# vcpkg 会自动处理 OpenSSL, httplib, json, 和 mysql connector
find_package(httplib REQUIRED)
find_package(nlohmann_json REQUIRED)

# 根据不同平台寻找正确的 MySQL 包
if(WIN32)
  find_package(unofficial-mysql REQUIRED)
else()
  find_package(unofficial-libmysql REQUIRED)
endif()

add_executable(server server.cpp)

# 链接所有库
target_link_libraries(server PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
)

# 根据平台链接正确的 MySQL 库
if(WIN32)
  target_link_libraries(server PRIVATE unofficial::mysql::client)
else()
  target_link_libraries(server PRIVATE unofficial::libmysql::libmysql)
endif() 