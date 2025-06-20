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
    ./vcpkg install cpp-httplib[openssl] nlohmann-json mysql-connector-c

    # Windows (CMD) - 注意目标平台说明符
    .\vcpkg.exe install cpp-httplib[openssl]:x64-windows nlohmann-json:x64-windows mysql-connector-c:x64-windows
    ```

    *这个过程可能需要一些时间，vcpkg 会自动下载和编译所有内容。*

### 第3步：配置并编译本项目

1. **设置数据库**:
    - 登录到您的 MySQL 服务器。
    - 执行项目中的 `init.sql` 脚本来创建数据库和表。
    - 打开 `server.cpp` 文件，找到 `const string DB_PASS = "your_password";` 这一行，并修改为您的 MySQL 密码。

2. **编译项目 (全平台通用)**:
    在 **本项目的根目录** (不是vcpkg目录) 打开终端，执行以下命令。

    ```sh
    # 1. 创建构建目录
    mkdir -p build && cd build

    # 2. 运行 CMake (关键步骤)
    #    将 [vcpkg-path] 替换为您vcpkg的实际路径
    #    例如: ~/dev/vcpkg 或 C:/Users/YourUser/dev/vcpkg
    cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-path]/scripts/buildsystems/vcpkg.cmake

    # 3. 编译
    cmake --build .
    ```

### 第4步：运行服务器

- 编译成功后，可执行文件位于 `build` 目录下。
- 运行它：

  ```sh
  # Windows
  .\Debug\server.exe

  # macOS / Linux
  ./server
  ```

- 服务器将启动在 `http://localhost:8080`。打开浏览器访问即可。

---

### 第5步：【推荐】使用 GitHub Actions 自动编译 (一键获取所有平台版本)

如果您不想在自己的电脑上安装复杂的编译环境，或者想轻松地为不同操作系统（Windows, macOS）生成可执行文件，那么使用 GitHub Actions 是最佳选择。

**它是如何工作的？**

当您把代码推送到 GitHub 仓库时，GitHub 会自动启动云端服务器（一台 Windows 和一台 macOS），并按照我们项目中的 `.github/workflows/build.yml` 文件里的指令，完成所有编译步骤，然后将最终的程序打包供您下载。

**作为用户，您需要做什么？**

您只需要将您的项目链接到一个 GitHub 仓库，然后按照以下步骤操作即可。

1. **将代码推送到 GitHub**:
    - 将本项目的代码上传到您的 GitHub 仓库。
    - 每当您向 `main` 分支推送新的代码时，构建流程就会自动开始。

2. **查看构建进度**:
    - 在您的 GitHub 仓库页面上，点击顶部的 **"Actions"** 标签页。
    - 您会看到一个名为 "Build Project" 的工作流正在运行（黄色图标）或已完成（绿色/红色图标）。

3. **下载编译好的程序**:
    - 点击进入一个已成功完成（绿色对勾）的工作流。
    - 在 "Summary" 页面的底部，您会找到一个名为 **"Artifacts"** (构建产物) 的区域。
    - 这里会列出两个可下载的压缩包：
      - `build-windows-latest`: **里面包含了 Windows 版本的 `server.exe`**。
      - `build-macos-latest`: 里面包含了 macOS 版本 `server`。
    - 点击即可下载，解压后就能直接运行。

通过这个流程，您再也无需关心本地编译环境的差异，可以专注于代码编写，让 GitHub 为您完成所有平台的编译工作。
