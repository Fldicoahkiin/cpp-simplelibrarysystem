# 简易图书管理系统

这是一个基于 C++ 和 Web 技术的简易图书管理系统。后端使用 `httplib` 构建轻量级HTTP服务器，通过 `nlohmann/json` 处理JSON数据，并连接到`MySQL`数据库。前端是一个单页应用(SPA)，使用 `Tailwind CSS` 构建现代化界面。

## ✨ 主要功能

### 普通用户

- [x] 用户注册与登录
- [x] 浏览所有馆藏图书
- [x] 按书名搜索图书
- [x] 借阅图书
- [x] 归还图书
- [x] 查看自己当前的借阅列表
- [x] 查看热门图书推荐

### 管理员

- [x] 拥有普通用户的所有权限
- [x] 添加新图书到图书馆
- [x] 查看所有注册用户列表
- [x] 查看任一用户的完整借阅历史（包括已归还和未归还的）

## 🛠️ 技术栈

- **后端**: C++17
  - **Web框架**: `cpp-httplib`
  - **JSON处理**: `nlohmann/json`
  - **数据库驱动**: `mysql-connector-c`
- **前端**: HTML5, JavaScript (ES6), `Tailwind CSS`, `Font Awesome`
- **数据库**: `MySQL` 或 `MariaDB`
- **构建系统**: `CMake`
- **包管理器**: `vcpkg`

## 🚀 环境搭建与运行指南

### 1. 先决条件

- **C++编译器**: 支持 C++17 的编译器 (如 GCC, Clang, MSVC)。
- **CMake**: 版本 3.15 或更高。
- **构建工具**: Ninja 或 Make。
- **vcpkg**: 用于管理C++依赖项。
- **MySQL/MariaDB**: 数据库服务器。

### 2. 数据库设置

1. 确保您的MySQL服务器正在运行。
2. 使用数据库管理工具（如 MySQL Workbench, Navicat, DBeaver 等）连接到您的数据库。
3. 打开 `init.sql` 文件，将其内容复制并执行。这将创建 `library_db` 数据库、所有需要的表，并插入一些初始数据。
4. **重要**: 根据您自己的MySQL配置，修改 `server.cpp` 文件中第31行的数据库连接密码 `DB_PASS`。

    ```cpp
    // server.cpp: Line 29
    const char *DB_PASS = "PassW0rd"; // ！！！ 请务必修改为您的数据库密码 ！！！
    ```

### 3. 后端编译与运行

1. **安装依赖**: 在项目根目录下，使用vcpkg安装所有必需的库。

    ```bash
    vcpkg install
    ```

2. **配置CMake**: 创建一个构建目录，并运行CMake进行配置。

    ```bash
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=[你的vcpkg路径]/scripts/buildsystems/vcpkg.cmake
    ```

    *请将 `[你的vcpkg路径]` 替换为您本地vcpkg的实际安装路径。*

3. **编译项目**: 使用CMake编译项目。

    ```bash
    cmake --build build
    ```

4. **运行服务器**: 执行编译生成的可执行文件。

    ```bash
    # 在 Windows 上
    .\build\server.exe

    # 在 Linux 或 macOS 上
    ./build/server
    ```

    服务器启动后，会在 `http://localhost:8080` 上监听请求。

### 4. 访问系统

- 打开您的现代浏览器（如 Chrome, Firefox, Edge），访问 `http://localhost:8080`。
- **默认登录凭据** (来自 `init.sql`):
  - **管理员**:
    - 用户名: `admin`
    - 密码: `123456`
  - **普通用户**:
    - 用户名: `test`
    - 密码: `password`
