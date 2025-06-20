# 现代C++图书管理系统 (Windows + Visual Studio 指南)

这是一个使用现代C++构建的Web版图书管理系统。本指南专门针对在 Windows 上使用 Visual Studio 和 vcpkg 进行开发的流程，这是目前最简便的方式。

## 功能

- 用户注册和登录
- 浏览所有馆藏图书
- 添加新图书
- 借阅图书
- 归还图书
- 查看个人当前借阅记录

## 技术栈

- **后端**: C++17
- **依赖库**: `cpp-httplib[openssl]`, `nlohmann-json`, `mysql-connector-c`
- **前端**: HTML, Tailwind CSS, JavaScript
- **数据库**: MySQL
- **开发环境**: Visual Studio 2022
- **包管理器**: vcpkg

---

## 环境设置与运行指南 (一次性设置)

### 1. 安装核心工具

1. **Visual Studio 2022**:
    - 从 [Visual Studio官网](https://visualstudio.microsoft.com/downloads/) 下载并安装 Community 版本。
    - 在安装时，请务必勾选 **"使用C++的桌面开发"** 工作负载。

2. **Git**:
    - 从 [Git官网](https://git-scm.com/downloads) 下载并安装。

3. **MySQL 服务器**:
    - 从 [MySQL Community Server官网](https://dev.mysql.com/downloads/mysql/) 下载并安装。

### 2. 设置 vcpkg 并安装依赖库

vcpkg 将自动处理所有C++库的下载和配置，极大简化了环境搭建。

1. **下载 vcpkg**:
    打开 "命令提示符" (cmd) 或者 "PowerShell"，执行以下命令将 vcpkg 克隆到您选择的目录，例如 `C:\dev`。

    ```cmd
    cd C:\
    mkdir dev
    cd dev
    git clone https://github.com/microsoft/vcpkg.git
    ```

2. **安装 vcpkg**:
    进入 vcpkg 目录并运行其安装脚本。

    ```cmd
    cd vcpkg
    .\bootstrap-vcpkg.bat
    ```

3. **安装项目依赖库**:
    在 vcpkg 目录下，用一条命令安装本项目需要的所有库。

    ```cmd
    .\vcpkg.exe install cpp-httplib[openssl]:x64-windows nlohmann-json:x64-windows mysql-connector-c:x64-windows
    ```

    *(这个过程可能需要一些时间，因为它会自动编译 OpenSSL 等库。)*

4. **集成 vcpkg 到 Visual Studio (重要)**:
    运行以下命令，让 Visual Studio 能自动找到 vcpkg 安装的库。

    ```cmd
    .\vcpkg.exe integrate install
    ```

    *看到 `Applied user-wide integration for this vcpkg root.` 的提示即表示成功。*

### 3. 设置数据库

1. **初始化数据表**:
    - 登录到您的 MySQL 服务器 (例如通过 `mysql.exe -u root -p`)。
    - 执行项目中的 `init.sql` 脚本来创建数据库和表：

      ```sql
      source <项目路径>/init.sql;
      ```

2. **修改代码中的密码**:
    - 打开 `server.cpp` 文件。
    - 找到 `const string DB_PASS = "your_password";` 这一行，将其中的 `your_password` 修改为您自己的 MySQL 密码。

### 4. 在 Visual Studio 中设置并运行项目

1. **创建项目**:
    - 打开 Visual Studio 2022。
    - 选择 "创建新项目"。
    - 选择 **"控制台应用" (Console App)** 模板，点击 "下一步"。
    - 命名您的项目（例如 `LibraryServer`），选择项目存放位置，然后点击 "创建"。

2. **添加代码**:
    - Visual Studio 会生成一个包含 `main` 函数的 `.cpp` 文件。
    - 将我们项目中 `server.cpp` 的 **全部内容** 复制并粘贴到这个文件中，覆盖掉模板代码。
    - 在 "解决方案资源管理器" 中，右键点击项目名称 -> "在文件资源管理器中打开文件夹"。
    - 将 `index.html` 文件复制到这个打开的文件夹中。

3. **配置项目属性**:
    - 在 Visual Studio 顶部，将解决方案平台从 `x86` 改为 `x64`。
    - 右键点击项目名称 -> "属性"。
    - 在属性页中，选择 "调试" -> "工作目录"。
    - 将其值设置为 `$(ProjectDir)`。这能确保服务器运行时可以找到 `index.html`。
    - 点击 "确定" 保存。

4. **运行项目**:
    - 按下 `F5` 键或点击顶部绿色的 "▶" 按钮来编译和运行项目。
    - 如果一切顺利，一个控制台窗口会出现，显示 `服务器已启动于 http://localhost:8080`。
    - 打开浏览器访问 [http://localhost:8080](http://localhost:8080) 即可使用本系统。
