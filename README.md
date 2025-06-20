# 现代C++图书管理系统 (全平台通用指南)

这是一个使用现代C++构建的Web版图书管理系统。本指南提供了在 Windows, macOS 和 Linux 上进行开发的一致性流程，核心是 **CMake + vcpkg**，这是现代C++跨平台开发的最佳实践。

## 功能

- 用户注册和登录
- 浏览所有馆藏图书
- 添加新图书
- 借阅图书
- 归还图书
- 查看个人当前借阅记录

## 技术栈

- **后端**: C++17
- **依赖库 (由vcpkg管理)**: `cpp-httplib[openssl]`, `nlohmann-json`, `mysql-connector-c`
- **前端**: HTML, JavaScript
- **数据库**: MySQL
- **构建系统**: CMake
- **包管理器**: vcpkg

---

## 环境设置与运行指南 (全平台一致)

### 第1步：安装基础工具

您需要在您的系统上安装以下软件：

1. **C++ 编译器**:
    - **Windows**: 安装 [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)，并勾选 **"使用C++的桌面开发"** 工作负载。
    - **macOS**: 在终端运行 `xcode-select --install` 安装 Xcode Command Line Tools。(**注意**: 此命令仅安装轻量级的命令行编译工具，无需下载完整的 Xcode 应用)。
    - **Linux (Ubuntu/Debian)**: 运行 `sudo apt update && sudo apt install build-essential g++`。
2. **Git**: 从 [Git官网](https://git-scm.com/downloads) 下载并安装。
3. **CMake**: 从 [CMake官网](https://cmake.org/download/) 下载并安装 (>= 3.15)。
4. **MySQL 服务器**: 从 [MySQL官网](https://dev.mysql.com/downloads/mysql/) 下载并安装。

### 第2步：安装 vcpkg 并配置依赖库 (一次性)

vcpkg 将为我们自动处理所有C++库，极大简化了环境搭建。

1. **下载 vcpkg**:
    选择一个位置 (例如用户主目录下的 `dev` 文件夹)，然后用 Git 克隆 vcpkg。

    ```sh
    # Unix-like (macOS, Linux)
    mkdir -p ~/dev && cd ~/dev
    git clone https://github.com/microsoft/vcpkg.git

    # Windows (CMD)
    cd %USERPROFILE% && mkdir dev && cd dev
    git clone https://github.com/microsoft/vcpkg.git
    ```

2. **安装 vcpkg**:
    运行其 bootstrap 脚本。

    ```sh
    # Unix-like (macOS, Linux)
    cd vcpkg && ./bootstrap-vcpkg.sh

    # Windows (CMD)
    cd vcpkg && .\bootstrap-vcpkg.bat
    ```

3. **安装项目依赖库**:
    在 vcpkg 目录下，用 **一条命令** 安装本项目需要的所有库。

    ```sh
    # Unix-like (macOS, Linux)
    ./vcpkg install cpp-httplib[openssl] nlohmann-json libmysql

    # Windows (CMD) - 注意目标平台说明符
    .\vcpkg.exe install cpp-httplib[openssl]:x64-windows nlohmann-json:x64-windows mysql-client:x64-windows
    ```
