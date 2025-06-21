#include <fstream>
#include <httplib.h>
#include <iostream>
// #include <mysql/mysql.h> // 暂时禁用
// #include <mysql/mysqld_error.h> // 暂时禁用
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

// 在编译时有条件地包含嵌入的HTML头文件
#if __has_include("index_html.h")
#include "index_html.h"
#endif

// 使用 nlohmann/json 命名空间
using json = nlohmann::json;
using namespace std;
using namespace httplib;

// MySQL 连接信息
const string DB_HOST = "127.0.0.1";
const string DB_USER = "root";
const string DB_PASS = "123456"; // ！！！ 请务必修改为您的数据库密码 ！！！
const string DB_SCHEMA = "library_db";
const unsigned int DB_PORT = 3306;

/* 暂时禁用数据库功能
// RAII 包装器，用于自动管理 MYSQL 连接
class DBConnection
{
private:
    MYSQL *conn;

public:
    DBConnection()
    {
        conn = mysql_init(nullptr);
        if (!conn)
        {
            throw runtime_error("mysql_init failed");
        }
        if (!mysql_real_connect(conn, DB_HOST.c_str(), DB_USER.c_str(), DB_PASS.c_str(), DB_SCHEMA.c_str(), DB_PORT, nullptr, 0))
        {
            string errMsg = "mysql_real_connect failed: " + string(mysql_error(conn));
            mysql_close(conn);
            throw runtime_error(errMsg);
        }
    }

    ~DBConnection()
    {
        if (conn)
        {
            mysql_close(conn);
        }
    }

    MYSQL *get()
    {
        return conn;
    }
};
*/

// 用于封装API响应的结构体
struct ApiResponse
{
    int status;
    json body;
};

// 服务层，封装所有业务逻辑
class LibraryService
{
public:
    // API: 用户注册
    ApiResponse registerUser(const json &j)
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API: 用户登录 (返回 isAdmin 状态)
    ApiResponse loginUser(const json &j)
    {
        // 为了方便测试，返回一个临时的管理员用户
        string username = j.at("username").get<string>();
        return {200, {{"success", true}, {"token", "fake-jwt-token"}, {"userId", 1}, {"username", username}, {"isAdmin", true}}};
    }

    // API: 获取/搜索图书
    ApiResponse searchBooks(const string &searchTerm)
    {
        // 返回一些假的图书数据
        json fake_books = json::array();
        fake_books.push_back({{"id", 1}, {"title", "C++ Primer (数据库已禁用)"}, {"stock", 10}});
        fake_books.push_back({{"id", 2}, {"title", "三体 (数据库已禁用)"}, {"stock", 5}});
        return {200, fake_books};
    }

    // API: 添加新书
    ApiResponse addBook(const json &j)
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API: 借阅图书
    ApiResponse borrowBook(const json &j_req)
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API: 归还图书
    ApiResponse returnBook(const json &j_req)
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API: 获取用户借阅的图书
    ApiResponse getUserBorrows(int userId)
    {
        return {200, json::array()}; // 返回空列表
    }

    // ---- 新增管理员和推荐功能 ----

    // API (Admin): 获取所有用户
    ApiResponse getAllUsers()
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API (Admin): 获取指定用户的全部借阅历史
    ApiResponse getAllUserBorrows(int userId)
    {
        return {501, {{"success", false}, {"message", "数据库功能已禁用"}}};
    }

    // API: 获取图书推荐
    ApiResponse getRecommendations(int userId)
    {
        return {200, json::array()}; // 返回空列表
    }
};

int main(void)
{
#ifdef _WIN32
    // 在Windows上设置控制台输出编码为UTF-8，以防止中文乱码
    SetConsoleOutputCP(CP_UTF8);
    // 将标准输出的模式设置为UTF-16，以支持宽字符打印
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif

    Server svr;
    LibraryService service;

    // 提供静态文件服务
    svr.Get("/", [](const Request &req, Response &res)
            {
#ifdef index_html_content
                // 如果HTML内容已嵌入，则直接从内存提供
                res.set_content(index_html_content, "text/html; charset=utf-8");
#else
                // 否则，作为备选，从文件系统读取 (主要用于本地开发)
                std::ifstream file("index.html");
                if (file)
                {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    res.set_content(buffer.str(), "text/html; charset=utf-8");
                }
                else
                {
                    res.set_content("<h1>Error 404</h1><p>index.html not found.</p>", "text/html; charset=utf-8");
                    res.status = 404;
                }
#endif
            });

    auto handle_request = [&](const Request &req, Response &res, auto service_method)
    {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        ApiResponse api_res;
        try
        {
            api_res = service_method(service, json::parse(req.body));
        }
        catch (const json::parse_error &e)
        {
            api_res = {400, {{"success", false}, {"message", "无效的JSON格式"}}};
        }
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
    };

    svr.Post("/api/auth/register", [&](const Request &req, Response &res)
             { handle_request(req, res, [](LibraryService &s, const json &j)
                              { return s.registerUser(j); }); });

    svr.Post("/api/auth/login", [&](const Request &req, Response &res)
             { handle_request(req, res, [](LibraryService &s, const json &j)
                              { return s.loginUser(j); }); });

    svr.Post("/api/books", [&](const Request &req, Response &res)
             { handle_request(req, res, [](LibraryService &s, const json &j)
                              { return s.addBook(j); }); });

    svr.Post("/api/borrow", [&](const Request &req, Response &res)
             { handle_request(req, res, [](LibraryService &s, const json &j)
                              { return s.borrowBook(j); }); });

    svr.Post("/api/return", [&](const Request &req, Response &res)
             { handle_request(req, res, [](LibraryService &s, const json &j)
                              { return s.returnBook(j); }); });

    // 获取/搜索图书
    svr.Get("/api/books", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        auto search_term = req.get_param_value("q");
        auto api_res = service.searchBooks(search_term);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取当前用户的借阅
    svr.Get(R"(/api/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        int userId = stoi(req.matches[1]);
        auto api_res = service.getUserBorrows(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取推荐
    svr.Get(R"(/api/user/(\d+)/recommendations)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        int userId = stoi(req.matches[1]);
        auto api_res = service.getRecommendations(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取所有用户
    svr.Get("/api/admin/users", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        auto api_res = service.getAllUsers();
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取指定用户所有借阅记录
    svr.Get(R"(/api/admin/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json");
        int userId = stoi(req.matches[1]);
        auto api_res = service.getAllUserBorrows(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

#ifdef _WIN32
    wprintf(L"服务器已启动于 http://localhost:8080\n");
#else
    cout << "服务器已启动于 http://localhost:8080" << endl;
#endif
    svr.listen("0.0.0.0", 8080);

    return 0;
}