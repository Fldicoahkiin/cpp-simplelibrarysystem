# C++图书管理系统

这是一个基于现代C++技术栈构建的、跨平台的Web图书管理系统。它旨在演示如何结合使用C++后端、Web前端和自动化CI/CD流程，来创建一个健壮且易于部署的应用程序。

## ✨ 功能特性

- **跨平台支持**: 一份代码库，可在Windows和macOS上无缝编译和运行。
- **Web界面**: 使用HTML, CSS和JavaScript构建现代化、用户友好的前端界面。
- **RESTful API**: C++后端通过RESTful API提供图书的增、删、改、查服务。
- **数据库集成**: 使用MySQL作为数据存储，并已在代码中集成。
- **自动化构建与发布**: 集成GitHub Actions，每次代码推送到`main`分支时，都会自动：
  - 编译出Windows和macOS的可执行文件。
  - 创建一个带有北京时间戳的GitHub Release，并附上构建产物供下载。

## 🛠️ 技术栈

- **后端**: C++17, [cpp-httplib](https://github.com/yhirose/cpp-httplib) (HTTP服务器), [nlohmann-json](https://github.com/nlohmann/json) (JSON处理), MySQL C API
- **数据库**: MySQL
- **前端**: HTML, JavaScript, [Tailwind CSS](https://tailwindcss.com/)
- **构建系统**: CMake
- **依赖管理**: [vcpkg](https://github.com/microsoft/vcpkg)
- **CI/CD**: GitHub Actions

---

## 🚀 CI/CD 自动化流程

本项目采用了双CI/CD流水线（GitHub Actions 和 CircleCI），以确保构建的灵活性和冗余。当代码被推送到 `main` 分支时，两个平台会同时被触发，自动完成所有构建、测试和发布任务。

**最终产物**: 每次成功的CI运行，都会生成两个独立的、完全自包含的可执行文件 (`server-windows.exe` 和 `server-macos`)，并发布到GitHub Release页面，用户下载后可直接运行，无需安装任何额外依赖。

### 方案一: GitHub Actions (配合自托管Runner)

- **配置文件**: `.github/workflows/build.yml`
- **核心优势**: 解决了GitHub对macOS虚拟机的高额计费问题，通过在您自己的Mac上运行一个`self-hosted runner`来免费执行macOS的构建任务。
- **流程简介**:
  1. **Windows构建**: 在GitHub提供的云端Windows虚拟机上，通过vcpkg进行静态编译。
  2. **macOS构建**: 任务被分发到您自己的、已注册的自托管Runner上执行静态编译。
  3. **资源嵌入**: 在编译前，自动将`index.html`文件的内容嵌入到C++代码中。
  4. **发布**: 汇总两个平台的可执行文件，创建一个新的GitHub Release。

### 方案二: CircleCI

- **配置文件**: `.circleci/config.yml`
- **核心优势**: 提供了一个完全独立的第三方构建环境，作为GitHub Actions的备份或替代方案。拥有慷慨的免费额度，界面简洁高效。
- **流程简介**:
  1. **并行构建**: 在CircleCI的云端Windows和macOS虚拟机上同时开始构建。
  2. **静态链接与资源嵌入**: 执行与GitHub Actions完全相同的逻辑，确保产物一致。
  3. **发布**: 使用GitHub CLI工具，将构建好的两个可执行文件发布到项目的GitHub Release页面。

---

## 🚀 本地开发与构建

本项目使用 **CMake Presets** 和 **vcpkg** 来实现极简的本地构建体验。

### 环境要求

1. **安装构建工具**:
    - **CMake**: 版本3.15或更高。
    - **Ninja**: (推荐) 一个快速的构建系统。
    - **C++编译器**:
      - Windows: Visual Studio 2022 或更高版本 (需安装 "使用C++的桌面开发" 工作负载)。
      - macOS: Xcode Command Line Tools。

2. **获取 vcpkg**:
    如果您本地没有vcpkg，请克隆它。本项目推荐将其放置在一个统一的位置 (例如用户主目录)。

    ```bash
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh  # macOS/Linux
    ./vcpkg/bootstrap-vcpkg.bat # Windows
    ```

3. **配置CMake预设文件**:
    本项目包含的 `CMakePresets.json` 文件已将vcpkg工具链的路径硬编码为 `/Users/mac/vcpkg`。如果您的vcpkg安装在不同位置，请相应修改此文件。

### 编译项目

完成上述配置后，只需在项目根目录运行以下两行命令：

```bash
# 1. 配置项目 (此步骤会自动通过vcpkg安装所有依赖)
cmake --preset default

# 2. 编译项目
cmake --build build
```

编译成功后，可执行文件 `server` (或 `server.exe`) 将位于 `build` 目录下。由于我们采用了资源嵌入，本地编译时，程序会从文件系统读取 `index.html` 以方便您修改前端；而通过CI发布的版本则直接包含了HTML内容。

---

## ⚙️ 部署与运行

1. **启动数据库**: 确保您的MySQL服务器正在运行。
2. **初始化数据库**: 使用项目中的 `init.sql` 脚本创建所需的数据库和表。

    ```sql
    -- 登录MySQL
    mysql -u root -p
    -- 执行脚本
    source /path/to/your/project/init.sql;
    ```

3. **启动后端服务器**:
    进入 `build` 目录，直接运行可执行文件。

    ```bash
    cd build
    ./server  # macOS/Linux
    ./server.exe # Windows
    ```

4. **访问前端**:
    服务器启动后，在浏览器中打开 `http://localhost:8080` 即可访问图书管理系统。
