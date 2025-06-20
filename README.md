# C++ 图书管理系统

这是一个使用现代C++ (C++17) 构建的全功能Web应用，从一个本地命令行程序重构而来。它拥有前后端、数据库、跨平台构建系统和自动化CI/CD流程。

## 主要技术栈

* **后端**: C++17, `httplib` (Web服务), `nlohmann-json` (JSON处理), `mysql-connector-c` (数据库连接)
* **前端**: `HTML`, `JavaScript`, `Tailwind CSS` (UI)
* **数据库**: `MySQL`
* **构建系统**: `CMake`
* **包管理**: `vcpkg`
* **持续集成**: `GitHub Actions`

## 核心功能

* **用户认证**: 支持用户注册和登录。
* **图书管理**:
  * 展示所有馆藏图书。
  * 支持按 **书名** 或 **ID** 进行实时搜索。
  * 添加新书到库存。
* **借阅管理**:
  * 用户可以借阅和归还图书。
  * 同一用户不能重复借阅尚未归还的图书。
  * 用户可以查看自己当前借阅的所有书籍。
* **智能推荐**:
  * 在"为你推荐"页面，系统会根据用户的借阅历史，推荐其他相似用户也喜欢的书籍。
* **管理员系统**:
  * **初始管理员账号**: `admin` / **密码**: `123456`。
  * 管理员登录后，侧边栏会出现"管理员面板"。
  * **用户管理**: 管理员可以查看系统内所有用户的列表。
  * **历史查询**: 管理员可以查看任意一个用户的完整借阅历史（包括已归还的）。

---

## 如何在本地运行

### 第1步：准备环境

1. **C++ 编译器**:
    * **Windows**: 安装 Visual Studio 2022 (或更高版本)，并确保已安装 "使用 C++ 的桌面开发" 工作负载。
    * **macOS**: 运行 `xcode-select --install` 来安装 Xcode Command Line Tools。
    * **Linux**: 安装 `g++`, `cmake`, `git` (例如 `sudo apt update && sudo apt install build-essential cmake git`)。
2. **MySQL 服务器**:
    * 确保您已安装并运行了 MySQL 服务器。您可以使用 [XAMPP](https://www.apachefriends.org/index.html), [WampServer](https://www.wampserver.com/en/) 或官方安装程序。

### 第2步：安装 vcpkg 并配置依赖库

1. **克隆 vcpkg**:
    选择一个合适的目录 (如 `~/dev` 或 `C:\dev`)，然后运行:

    ```sh
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ```

2. **引导 vcpkg**:

    ```sh
    # Windows
    .\bootstrap-vcpkg.bat
    # macOS / Linux
    ./bootstrap-vcpkg.sh
    ```

3. **安装依赖**: (关键步骤)
    * 这一步会下载和编译所有需要的库，可能需要较长时间。

    ```sh
    # Windows (使用 x64-windows triplet)
    .\vcpkg.exe install cpp-httplib[openssl]:x64-windows nlohmann-json:x64-windows mysql-client:x64-windows

    # macOS / Linux
    ./vcpkg install 'cpp-httplib[openssl]' nlohmann-json libmysql
    ```

    *如果安装失败，请先尝试运行 `git pull` 和 `./bootstrap-vcpkg.sh` 更新vcpkg自身再重试。*

### 第3步：配置并编译本项目

1. **设置数据库**:
    * 登录到您的 MySQL 服务器。
    * 执行项目根目录中的 `init.sql` 脚本来创建数据库、表和初始管理员账户。
    * **重要**: 打开 `server.cpp` 文件，找到 `const string DB_PASS = "your_password";` 这一行，并修改为您的 MySQL **root** 用户的密码。

2. **编译项目**:
    在 **本项目的根目录** (不是vcpkg目录) 打开终端，执行以下命令。

    ```sh
    # 1. 如果存在旧的 build 目录，先删除它
    rm -rf build
    
    # 2. 创建一个干净的构建目录
    mkdir build && cd build

    # 3. 运行 CMake (最关键的步骤)
    #    将 [vcpkg-path] 替换为您vcpkg的实际路径
    #    例如: C:/dev/vcpkg or /Users/yourname/dev/vcpkg
    cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-path]/scripts/buildsystems/vcpkg.cmake

    # 4. 编译
    cmake --build .
    ```

### 第4步：运行服务器

* 编译成功后，可执行文件位于 `build` 目录下。
* 在 `build` 目录中运行它：

  ```sh
  # Windows
  .\Debug\server.exe
  # macOS / Linux
  ./server
  ```

* 服务器将启动在 `http://localhost:8080`。打开浏览器访问即可。

---

### 第5步：【推荐】使用 GitHub Actions 自动编译

如果您不想在本地配置环境，或者想轻松地为 Windows 和 macOS 同时生成可执行文件，可以使用 GitHub Actions。

1. **推送到 GitHub**: 将项目代码推送到您的 GitHub 仓库的 `main` 分支。
2. **查看进度**: 在仓库页面点击 "Actions" 标签页，等待 "Build Project" 工作流完成。
3. **下载产物**: 在工作流的 "Summary" 页面底部，找到 "Artifacts"，您可以在这里下载为 Windows 和 macOS 分别编译好的程序压缩包。
