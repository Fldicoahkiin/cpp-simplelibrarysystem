#include <fstream>
#include <httplib.h>
#include <iostream>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

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
const char *DB_HOST = "127.0.0.1";
const char *DB_USER = "root";
const char *DB_PASS = "PassW0rd";
const char *DB_SCHEMA = "library_db";
const unsigned int DB_PORT = 3306;

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
            cerr << "Critical: mysql_init failed." << endl;
            throw runtime_error("mysql_init failed");
        }
        if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_SCHEMA, DB_PORT, nullptr, 0))
        {
            string errMsg = "mysql_real_connect failed: " + string(mysql_error(conn));
            cerr << "Critical: " << errMsg << endl; // 将错误信息打印到 stderr
            mysql_close(conn);
            throw runtime_error(errMsg);
        }
        cout << "Successfully connected to database '" << DB_SCHEMA << "'." << endl;
        mysql_set_character_set(conn, "utf8mb4");
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

// 用于封装API响应的结构体
struct ApiResponse
{
    int status;
    json body;
};

// 服务层，封装所有业务逻辑
class LibraryService
{
private:
    // 辅助函数：安全地转义字符串，防止SQL注入
    string escape(MYSQL *conn, const string &s)
    {
        if (s.empty())
        {
            return "";
        }
        // 分配足够的内存
        char *to = new char[s.length() * 2 + 1];
        mysql_real_escape_string(conn, to, s.c_str(), s.length());
        string res(to);
        delete[] to;
        return res;
    }

    // 辅助函数：执行SQL查询 (现在需要传入数据库连接)
    json executeQuery(MYSQL *conn, const string &query)
    {
        if (mysql_query(conn, query.c_str()))
        {
            throw runtime_error("SQL query failed: " + string(mysql_error(conn)));
        }

        MYSQL_RES *result = mysql_store_result(conn);
        if (!result)
        {
            if (mysql_field_count(conn) == 0) // e.g., INSERT, UPDATE, DELETE
            {
                return {{"affected_rows", mysql_affected_rows(conn)}};
            }
            else
            {
                throw runtime_error("mysql_store_result failed: " + string(mysql_error(conn)));
            }
        }

        json j_res = json::array();
        MYSQL_ROW row;
        MYSQL_FIELD *fields = mysql_fetch_fields(result);
        unsigned int num_fields = mysql_num_fields(result);

        while ((row = mysql_fetch_row(result)))
        {
            json j_row;
            for (unsigned int i = 0; i < num_fields; i++)
            {
                const char *field_name = fields[i].name;
                if (row[i] == nullptr)
                {
                    // 关键修复: 正确处理数据库的NULL值
                    j_row[field_name] = nullptr;
                }
                else if (strcmp(field_name, "is_admin") == 0 || strcmp(field_name, "returned") == 0)
                {
                    // 关键修复: 将代表布尔值的"0"或"1"转换为真正的布尔类型
                    j_row[field_name] = (strcmp(row[i], "1") == 0);
                }
                else
                {
                    j_row[field_name] = row[i];
                }
            }
            j_res.push_back(j_row);
        }

        mysql_free_result(result);
        return j_res;
    }

    // 辅助函数：检查用户是否为管理员 (现在需要传入数据库连接)
    bool isAdmin(MYSQL *conn, int userId)
    {
        try
        {
            string query = "SELECT is_admin FROM users WHERE id = " + to_string(userId) + ";";
            json result = executeQuery(conn, query);
            if (result.is_array() && !result.empty())
            {
                // executeQuery现在返回真正的布尔值
                return result[0].at("is_admin").get<bool>();
            }
        }
        catch (const std::exception &e)
        {
            cerr << "isAdmin check failed: " << e.what() << endl;
        }
        return false;
    }

public:
    // API: 用户注册
    ApiResponse registerUser(const json &j)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        if (mysql_query(conn, "START TRANSACTION"))
            throw runtime_error("开启事务失败: " + string(mysql_error(conn)));
        try
        {
            string username = j.at("username").get<string>();
            string password = j.at("password").get<string>();

            // 关键修复: 使用转义防止SQL注入
            string safe_username = escape(conn, username);
            string safe_password = escape(conn, password);

            // 检查用户是否已存在
            string check_query = "SELECT id FROM users WHERE username = '" + safe_username + "';";
            if (!executeQuery(conn, check_query).empty())
            {
                mysql_query(conn, "ROLLBACK");
                return {409, {{"success", false}, {"message", "用户名已存在"}}};
            }

            // 插入新用户
            string insert_query = "INSERT INTO users (username, password) VALUES ('" + safe_username + "', '" + safe_password + "');";
            executeQuery(conn, insert_query);
            cout << "User registration query executed for: " << username << endl; // 增加日志

            if (mysql_query(conn, "COMMIT"))
                throw runtime_error("提交事务失败: " + string(mysql_error(conn)));

            cout << "Transaction committed for user: " << username << endl;
            return {201, {{"success", true}, {"message", "注册成功"}}};
        }
        catch (const json::exception &e)
        {
            mysql_query(conn, "ROLLBACK");
            return {400, {{"success", false}, {"message", "请求参数错误: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            mysql_query(conn, "ROLLBACK");
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 用户登录 (返回 isAdmin 状态)
    ApiResponse loginUser(const json &j)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        try
        {
            string username = j.at("username").get<string>();
            string password = j.at("password").get<string>();

            // 关键修复: 使用转义防止SQL注入
            string safe_username = escape(conn, username);
            string safe_password = escape(conn, password);

            string query = "SELECT id, is_admin FROM users WHERE username = '" + safe_username + "' AND password = '" + safe_password + "';";
            json result = executeQuery(conn, query);

            if (result.empty())
            {
                return {401, {{"success", false}, {"message", "用户名或密码错误"}}};
            }

            int userId = stoi(result[0].at("id").get<string>());
            bool isAdmin = result[0].at("is_admin").get<bool>(); // 直接获取布尔值

            return {200, {{"success", true}, {"token", "fake-jwt-token-for-" + to_string(userId)}, {"userId", userId}, {"username", username}, {"isAdmin", isAdmin}}};
        }
        catch (const json::exception &e)
        {
            return {400, {{"success", false}, {"message", "请求参数错误: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 获取/搜索图书
    ApiResponse searchBooks(const string &searchTerm)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        try
        {
            string query = "SELECT id, title, stock FROM books";
            if (!searchTerm.empty())
            {
                // 关键修复: 对搜索词进行转义
                query += " WHERE title LIKE '%" + escape(conn, searchTerm) + "%'";
            }
            query += ";";
            return {200, executeQuery(conn, query)};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API (Admin): 添加新书
    ApiResponse addBook(const json &j)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        try
        {
            int adminId = j.at("adminId").get<int>();
            if (!isAdmin(conn, adminId))
            {
                return {403, {{"success", false}, {"message", "无管理员权限"}}};
            }

            string title = j.at("title").get<string>();
            int stock = j.at("stock").get<int>();

            // 关键修复: 对书名进行转义
            string safe_title = escape(conn, title);

            string query = "INSERT INTO books (title, stock) VALUES ('" + safe_title + "', " + to_string(stock) + ");";
            executeQuery(conn, query);
            return {201, {{"success", true}, {"message", "图书添加成功"}}};
        }
        catch (const json::exception &e)
        {
            return {400, {{"success", false}, {"message", "请求参数错误: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 借阅图书
    ApiResponse borrowBook(const json &j_req)
    {
        try
        {
            DBConnection db;
            MYSQL *conn = db.get();

            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            // 开启事务
            if (mysql_query(conn, "START TRANSACTION"))
                throw runtime_error("开启事务失败: " + string(mysql_error(conn)));

            try
            {
                // 1. 检查用户是否已借阅此书且未归还 (并锁定相关行避免并发问题)
                string check_query = "SELECT id FROM borrow_records WHERE user_id = " + to_string(userId) + " AND book_id = " + to_string(bookId) + " AND returned = FALSE FOR UPDATE;";
                if (mysql_query(conn, check_query.c_str()))
                    throw runtime_error("检查重复借阅失败: " + string(mysql_error(conn)));

                MYSQL_RES *check_result = mysql_store_result(conn);
                if (check_result && mysql_num_rows(check_result) > 0)
                {
                    mysql_free_result(check_result);
                    mysql_query(conn, "ROLLBACK");
                    return {409, {{"success", false}, {"message", "您已借阅此书且尚未归还，不能重复借阅"}}};
                }
                if (check_result)
                    mysql_free_result(check_result);

                // 2. 检查库存并锁定行
                string stock_query = "SELECT stock FROM books WHERE id = " + to_string(bookId) + " FOR UPDATE;";
                if (mysql_query(conn, stock_query.c_str()))
                    throw runtime_error("检查库存失败: " + string(mysql_error(conn)));
                MYSQL_RES *result = mysql_store_result(conn);
                if (!result || mysql_num_rows(result) == 0)
                {
                    mysql_free_result(result);
                    throw runtime_error("未找到该图书");
                }
                MYSQL_ROW row = mysql_fetch_row(result);
                int stock = stoi(row[0]);
                mysql_free_result(result);

                if (stock <= 0)
                {
                    throw runtime_error("图书库存不足");
                }

                // 3. 扣减库存
                string update_query = "UPDATE books SET stock = stock - 1 WHERE id = " + to_string(bookId) + ";";
                if (mysql_query(conn, update_query.c_str()))
                    throw runtime_error("扣减库存失败: " + string(mysql_error(conn)));

                // 4. 添加借阅记录
                string insert_query = "INSERT INTO borrow_records (user_id, book_id) VALUES (" + to_string(userId) + ", " + to_string(bookId) + ");";
                if (mysql_query(conn, insert_query.c_str()))
                    throw runtime_error("创建借阅记录失败: " + string(mysql_error(conn)));

                // 5. 提交事务
                if (mysql_query(conn, "COMMIT"))
                    throw runtime_error("提交事务失败: " + string(mysql_error(conn)));

                return {200, {{"success", true}, {"message", "借阅成功"}}};
            }
            catch (const std::exception &e)
            {
                mysql_query(conn, "ROLLBACK"); // 回滚事务
                return {500, {{"success", false}, {"message", e.what()}}};
            }
        }
        catch (const json::exception &e)
        {
            return {400, {{"success", false}, {"message", "请求参数错误: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 归还图书
    ApiResponse returnBook(const json &j_req)
    {
        try
        {
            // 1. 解析请求 - 使用 userId 和 bookId
            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            // 2. 建立数据库连接
            DBConnection db;
            MYSQL *conn = db.get();

            // 3. 开始事务
            mysql_query(conn, "START TRANSACTION");

            try
            {
                // 4. 查找并更新该用户最旧的一条未归还的对应图书记录
                string update_borrow_query = "UPDATE borrow_records SET returned = TRUE, return_date = NOW() "
                                             "WHERE user_id = " +
                                             to_string(userId) + " AND book_id = " + to_string(bookId) +
                                             " AND returned = FALSE ORDER BY id ASC LIMIT 1;";

                if (mysql_query(conn, update_borrow_query.c_str()))
                {
                    throw runtime_error("更新借阅记录失败: " + string(mysql_error(conn)));
                }

                // 关键修复: 检查是否有记录被更新
                if (mysql_affected_rows(conn) == 0)
                {
                    throw runtime_error("您没有借阅该图书或该书已还");
                }

                // 5. 增加图书库存
                string update_book_query = "UPDATE books SET stock = stock + 1 WHERE id = " + to_string(bookId) + ";";
                if (mysql_query(conn, update_book_query.c_str()))
                {
                    throw runtime_error("更新图书库存失败: " + string(mysql_error(conn)));
                }

                // 6. 提交事务
                mysql_query(conn, "COMMIT");
                return {200, {{"success", true}, {"message", "归还成功"}}};
            }
            catch (const std::exception &e)
            {
                mysql_query(conn, "ROLLBACK");
                // 返回具体的数据库错误
                return {500, {{"success", false}, {"message", e.what()}}};
            }
        }
        catch (const json::exception &e)
        {
            // 返回JSON解析错误
            return {400, {{"success", false}, {"message", "请求参数错误: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            // 返回其他服务器内部错误
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 获取用户借阅的图书
    ApiResponse getUserBorrows(int userId)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        try
        {
            string query = "SELECT br.id, b.id as book_id, b.title, br.borrow_date FROM books b "
                           "JOIN borrow_records br ON b.id = br.book_id "
                           "WHERE br.user_id = " +
                           to_string(userId) + " AND br.returned = FALSE;";
            return {200, executeQuery(conn, query)};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // ---- 新增管理员和推荐功能 ----

    // API (Admin): 获取所有用户
    ApiResponse getAllUsers(int adminId)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        if (!isAdmin(conn, adminId))
        {
            return {403, {{"success", false}, {"message", "无管理员权限"}}};
        }
        try
        {
            return {200, executeQuery(conn, "SELECT id, username, is_admin FROM users;")};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API (Admin): 获取指定用户的全部借阅历史
    ApiResponse getAllUserBorrows(int adminId, int userId)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        if (!isAdmin(conn, adminId))
        {
            return {403, {{"success", false}, {"message", "无管理员权限"}}};
        }
        try
        {
            string query = "SELECT br.id as borrow_id, b.id as book_id, b.title, "
                           "br.borrow_date, br.return_date, br.returned FROM books b "
                           "JOIN borrow_records br ON b.id = br.book_id "
                           "WHERE br.user_id = " +
                           to_string(userId) + " ORDER BY br.borrow_date DESC;";
            return {200, executeQuery(conn, query)};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }

    // API: 获取图书推荐
    ApiResponse getRecommendations(int userId)
    {
        DBConnection db;
        MYSQL *conn = db.get();
        try
        {
            // 推荐逻辑: 找出借阅次数最多的图书
            string query = "SELECT b.id, b.title, b.stock "
                           "FROM books b "
                           "LEFT JOIN (SELECT book_id, COUNT(*) as borrow_count FROM borrow_records GROUP BY book_id) as popular ON b.id = popular.book_id "
                           "ORDER BY popular.borrow_count DESC, b.id ASC LIMIT 10;";
            return {200, executeQuery(conn, query)};
        }
        catch (const std::exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
    }
};

int main(void)
{
#ifdef _WIN32
    // 在Windows上将控制台输出设置为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IONBF, 0);
#endif

    cout << "Attempting to connect to database..." << endl;
    cout << "  Host: " << DB_HOST << endl;
    cout << "  User: " << DB_USER << endl;
    cout << "  Database: " << DB_SCHEMA << endl;

    try
    {
        // 尝试建立一个初始连接以检查数据库是否可用
        DBConnection db;
        cout << "Initial database connection check successful." << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "Failed to connect to the database on startup: " << e.what() << endl;
        cerr << "Please ensure the database server is running and the connection details are correct." << endl;
        return 1; // 数据库连接失败则退出
    }

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
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        ApiResponse api_res;
        try
        {
            if (req.body.empty())
            {
                api_res = {400, {{"success", false}, {"message", "请求体为空"}}};
            }
            else
            {
                api_res = service_method(service, json::parse(req.body));
            }
        }
        catch (const json::parse_error &e)
        {
            api_res = {400, {{"success", false}, {"message", "无效的JSON格式: " + string(e.what())}}};
        }
        catch (const std::exception &e)
        {
            api_res = {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
    };

    svr.Options(R"((/.*))", [](const Request &req, Response &res)
                {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.status = 204; });

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
        res.set_header("Content-Type", "application/json; charset=utf-8");
        auto search_term = req.get_param_value("q");
        auto api_res = service.searchBooks(search_term);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取当前用户的借阅
    svr.Get(R"(/api/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        int userId = stoi(req.matches[1]);
        auto api_res = service.getUserBorrows(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // 获取推荐
    svr.Get(R"(/api/user/(\d+)/recommendations)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        int userId = stoi(req.matches[1]);
        auto api_res = service.getRecommendations(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // (Admin) 获取所有用户
    svr.Get("/api/admin/users", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        ApiResponse api_res;
        try
        {
            int adminId = req.has_param("adminId") ? stoi(req.get_param_value("adminId")) : -1;
            api_res = service.getAllUsers(adminId);
        }
        catch (const std::invalid_argument &ia)
        {
            api_res = {400, {{"success", false}, {"message", "无效的 adminId"}}};
        }
        catch (const std::exception &e)
        {
            api_res = {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    // (Admin) 获取指定用户所有借阅记录
    svr.Get(R"(/api/admin/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        ApiResponse api_res;
        try
        {
            int adminId = req.has_param("adminId") ? stoi(req.get_param_value("adminId")) : -1;
            int userId = stoi(req.matches[1]);
            api_res = service.getAllUserBorrows(adminId, userId);
        }
        catch (const std::invalid_argument &ia)
        {
            api_res = {400, {{"success", false}, {"message", "无效的 ID"}}};
        }
        catch (const std::exception &e)
        {
            api_res = {500, {{"success", false}, {"message", "服务器内部错误: " + string(e.what())}}};
        }
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