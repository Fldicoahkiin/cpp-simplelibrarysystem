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

## 🚀 本地开发与构建

本项目使用 **CMake Presets** 和 **vcpkg** 来实现极简的本地构建体验。

### 首次环境配置

1. **安装构建工具**:
    - **CMake**: 版本3.15或更高。
    - **Ninja**: (推荐) 一个快速的构建系统。
    - **C++编译器**:
      - Windows: Visual Studio 2019 或更高版本 (需安装 "使用C++的桌面开发" 工作负载)。
      - macOS: Xcode Command Line Tools。

2. **获取 vcpkg**:
    如果您本地没有vcpkg，请克隆它。本项目推荐将其放置在一个统一的位置 (例如用户主目录)。

    ```bash
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh  # macOS/Linux
    ./vcpkg/bootstrap-vcpkg.bat # Windows
    ```

3. **配置CMake预设文件**:
    本项目已包含 `CMakePresets.json` 文件。它被配置为：
    - **优先使用** `VCPKG_ROOT` 环境变量（如果已设置）。
    - 如果`VCPKG_ROOT`未设置，则**自动回退**到您本地的vcpkg路径。**请根据您的实际情况修改 `CMakePresets.json` 中第13行的回退路径 `/Users/mac/vcpkg`**。

### 编译项目

完成上述配置后，只需在项目根目录运行以下两行命令：

```bash
# 1. 配置项目 (此步骤会自动通过vcpkg安装所有依赖)
cmake --preset default

# 2. 编译项目
cmake --build build
```

编译成功后，可执行文件 `server` (或 `server.exe`) 和 `index.html` 将位于 `build` 目录下。

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
