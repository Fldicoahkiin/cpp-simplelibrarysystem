#include <fstream>
#include <httplib.h>
#include <iostream>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

// 使用 nlohmann/json 命名空间
using json = nlohmann::json;
using namespace std;
using namespace httplib;

// MySQL 连接信息
const string DB_HOST = "127.0.0.1";
const string DB_USER = "root";
const string DB_PASS = "PassW0rd"; // ！！！ 请务必修改为您的数据库密码 ！！！
const string DB_SCHEMA = "library_db";
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
        try
        {
            DBConnection db;
            string username = j.at("username").get<string>();
            string password = j.at("password").get<string>();

            const char *query = "INSERT INTO users (username, password, is_admin) VALUES (?, ?, FALSE)";
            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query, strlen(query));

            MYSQL_BIND params[2];
            memset(params, 0, sizeof(params));
            params[0].buffer_type = MYSQL_TYPE_STRING;
            params[0].buffer = (char *)username.c_str();
            params[0].buffer_length = username.length();
            params[1].buffer_type = MYSQL_TYPE_STRING;
            params[1].buffer = (char *)password.c_str();
            params[1].buffer_length = password.length();

            mysql_stmt_bind_param(stmt.get(), params);

            if (mysql_stmt_execute(stmt.get()))
            {
                if (mysql_stmt_errno(stmt.get()) == ER_DUP_ENTRY)
                {
                    return {409, {{"success", false}, {"message", "用户名已存在"}}};
                }
                throw runtime_error("mysql_stmt_execute failed: " + string(mysql_stmt_error(stmt.get())));
            }

            return {201, {{"success", true}, {"message", "用户注册成功"}}};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }

    // API: 用户登录 (返回 isAdmin 状态)
    ApiResponse loginUser(const json &j)
    {
        try
        {
            DBConnection db;
            string username = j.at("username").get<string>();
            string password = j.at("password").get<string>();

            const char *query = "SELECT id, username, is_admin FROM users WHERE username = ? AND password = ?";
            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query, strlen(query));

            MYSQL_BIND params[2];
            memset(params, 0, sizeof(params));
            params[0].buffer_type = MYSQL_TYPE_STRING;
            params[0].buffer = (char *)username.c_str();
            params[0].buffer_length = username.length();
            params[1].buffer_type = MYSQL_TYPE_STRING;
            params[1].buffer = (char *)password.c_str();
            params[1].buffer_length = password.length();
            mysql_stmt_bind_param(stmt.get(), params);

            mysql_stmt_execute(stmt.get());
            mysql_stmt_store_result(stmt.get());

            if (mysql_stmt_num_rows(stmt.get()) == 1)
            {
                long long userId;
                char dbUsername[256];
                bool isAdmin = false;

                MYSQL_BIND result[3];
                memset(result, 0, sizeof(result));
                result[0].buffer_type = MYSQL_TYPE_LONGLONG;
                result[0].buffer = &userId;
                result[1].buffer_type = MYSQL_TYPE_STRING;
                result[1].buffer = dbUsername;
                result[1].buffer_length = sizeof(dbUsername);
                result[2].buffer_type = MYSQL_TYPE_TINY;
                result[2].buffer = &isAdmin;
                mysql_stmt_bind_result(stmt.get(), result);
                mysql_stmt_fetch(stmt.get());

                json response_json = {
                    {"success", true},
                    {"token", "fake-jwt-token"}, // 实际项目中应使用真实的JWT
                    {"userId", userId},
                    {"username", dbUsername},
                    {"isAdmin", isAdmin}};
                return {200, response_json};
            }
            else
            {
                return {401, {{"success", false}, {"message", "无效的用户名或密码"}}};
            }
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }

    // API: 获取/搜索图书
    ApiResponse searchBooks(const string &searchTerm)
    {
        try
        {
            DBConnection db;
            json books_array = json::array();
            string query_str = "SELECT id, title, stock FROM books";
            if (!searchTerm.empty())
            {
                query_str += " WHERE title LIKE ? OR id = ?";
            }

            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query_str.c_str(), query_str.length());

            if (!searchTerm.empty())
            {
                MYSQL_BIND params[2];
                memset(params, 0, sizeof(params));
                string like_term = "%" + searchTerm + "%";
                params[0].buffer_type = MYSQL_TYPE_STRING;
                params[0].buffer = (char *)like_term.c_str();
                params[0].buffer_length = like_term.length();
                long long bookId_term = atoll(searchTerm.c_str());
                params[1].buffer_type = MYSQL_TYPE_LONGLONG;
                params[1].buffer = &bookId_term;
                mysql_stmt_bind_param(stmt.get(), params);
            }

            mysql_stmt_execute(stmt.get());
            mysql_stmt_store_result(stmt.get());

            long long bookId;
            char title[256];
            int stock;
            MYSQL_BIND result[3];
            memset(result, 0, sizeof(result));
            result[0].buffer_type = MYSQL_TYPE_LONGLONG;
            result[0].buffer = &bookId;
            result[1].buffer_type = MYSQL_TYPE_STRING;
            result[1].buffer = title;
            result[1].buffer_length = sizeof(title);
            result[2].buffer_type = MYSQL_TYPE_LONG;
            result[2].buffer = &stock;
            mysql_stmt_bind_result(stmt.get(), result);

            while (mysql_stmt_fetch(stmt.get()) == 0)
            {
                books_array.push_back({{"id", bookId},
                                       {"title", title},
                                       {"stock", stock}});
            }
            return {200, books_array};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }

    // API: 添加新书
    ApiResponse addBook(const json &j)
    {
        try
        {
            DBConnection db;
            string title = j.at("title").get<string>();
            int stock = j.at("stock").get<int>();

            const char *query = "INSERT INTO books (title, stock) VALUES (?, ?)";
            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query, strlen(query));

            MYSQL_BIND params[2];
            memset(params, 0, sizeof(params));
            params[0].buffer_type = MYSQL_TYPE_STRING;
            params[0].buffer = (char *)title.c_str();
            params[0].buffer_length = title.length();
            params[1].buffer_type = MYSQL_TYPE_LONG;
            params[1].buffer = &stock;

            mysql_stmt_bind_param(stmt.get(), params);
            mysql_stmt_execute(stmt.get());

            long long new_id = mysql_stmt_insert_id(stmt.get());

            json response_json = {
                {"success", true},
                {"message", "图书添加成功"},
                {"book", {{"id", new_id}, {"title", title}, {"stock", stock}}}};
            return {201, response_json};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }

    // API: 借阅图书
    ApiResponse borrowBook(const json &j_req)
    {
        DBConnection db;
        try
        {
            mysql_query(db.get(), "START TRANSACTION");

            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            // 检查是否已借阅且未归还
            {
                const char *q_check = "SELECT id FROM borrow_records WHERE user_id = ? AND book_id = ? AND returned = FALSE";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q_check, strlen(q_check));
                MYSQL_BIND p_check[2];
                memset(p_check, 0, sizeof(p_check));
                p_check[0].buffer_type = MYSQL_TYPE_LONG;
                p_check[0].buffer = &userId;
                p_check[1].buffer_type = MYSQL_TYPE_LONG;
                p_check[1].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p_check);
                mysql_stmt_execute(stmt.get());
                mysql_stmt_store_result(stmt.get());
                if (mysql_stmt_num_rows(stmt.get()) > 0)
                {
                    throw runtime_error("您已借阅此书且尚未归还");
                }
            }

            // 检查库存
            int stock = -1;
            {
                const char *q1 = "SELECT stock FROM books WHERE id = ? FOR UPDATE";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q1, strlen(q1));
                MYSQL_BIND p1[1];
                memset(p1, 0, sizeof(p1));
                p1[0].buffer_type = MYSQL_TYPE_LONG;
                p1[0].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p1);
                mysql_stmt_execute(stmt.get());
                mysql_stmt_store_result(stmt.get());
                if (mysql_stmt_num_rows(stmt.get()) == 0)
                {
                    throw runtime_error("未找到该图书");
                }
                MYSQL_BIND r1[1];
                memset(r1, 0, sizeof(r1));
                r1[0].buffer_type = MYSQL_TYPE_LONG;
                r1[0].buffer = &stock;
                mysql_stmt_bind_result(stmt.get(), r1);
                mysql_stmt_fetch(stmt.get());
            }

            if (stock <= 0)
            {
                throw runtime_error("该图书库存不足");
            }

            // 减库存
            {
                const char *q2 = "UPDATE books SET stock = stock - 1 WHERE id = ?";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q2, strlen(q2));
                MYSQL_BIND p2[1];
                memset(p2, 0, sizeof(p2));
                p2[0].buffer_type = MYSQL_TYPE_LONG;
                p2[0].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p2);
                mysql_stmt_execute(stmt.get());
            }

            // 添加借阅记录
            {
                const char *q3 = "INSERT INTO borrow_records (user_id, book_id) VALUES (?, ?)";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q3, strlen(q3));
                MYSQL_BIND p3[2];
                memset(p3, 0, sizeof(p3));
                p3[0].buffer_type = MYSQL_TYPE_LONG;
                p3[0].buffer = &userId;
                p3[1].buffer_type = MYSQL_TYPE_LONG;
                p3[1].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p3);
                mysql_stmt_execute(stmt.get());
            }

            mysql_query(db.get(), "COMMIT");
            return {200, {{"success", true}, {"message", "借书成功"}}};
        }
        catch (const exception &e)
        {
            mysql_query(db.get(), "ROLLBACK");
            return {400, {{"success", false}, {"message", e.what()}}};
        }
    }

    // API: 归还图书
    ApiResponse returnBook(const json &j_req)
    {
        DBConnection db;
        try
        {
            mysql_query(db.get(), "START TRANSACTION");

            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            // 更新借阅记录
            long long affected_rows = 0;
            {
                const char *q1 = "UPDATE borrow_records SET returned = TRUE WHERE user_id = ? AND book_id = ? AND returned = FALSE";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q1, strlen(q1));
                MYSQL_BIND p1[2];
                memset(p1, 0, sizeof(p1));
                p1[0].buffer_type = MYSQL_TYPE_LONG;
                p1[0].buffer = &userId;
                p1[1].buffer_type = MYSQL_TYPE_LONG;
                p1[1].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p1);
                mysql_stmt_execute(stmt.get());
                affected_rows = mysql_stmt_affected_rows(stmt.get());
            }

            if (affected_rows == 0)
            {
                throw runtime_error("未找到此借阅记录");
            }

            // 增加库存
            {
                const char *q2 = "UPDATE books SET stock = stock + 1 WHERE id = ?";
                unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
                mysql_stmt_prepare(stmt.get(), q2, strlen(q2));
                MYSQL_BIND p2[1];
                memset(p2, 0, sizeof(p2));
                p2[0].buffer_type = MYSQL_TYPE_LONG;
                p2[0].buffer = &bookId;
                mysql_stmt_bind_param(stmt.get(), p2);
                mysql_stmt_execute(stmt.get());
            }

            mysql_query(db.get(), "COMMIT");
            return {200, {{"success", true}, {"message", "还书成功"}}};
        }
        catch (const exception &e)
        {
            mysql_query(db.get(), "ROLLBACK");
            return {400, {{"success", false}, {"message", e.what()}}};
        }
    }

    // API: 获取用户借阅的图书
    ApiResponse getUserBorrows(int userId)
    {
        try
        {
            DBConnection db;
            const char *query = "SELECT b.id, b.title FROM borrow_records br JOIN books b ON br.book_id = b.id WHERE br.user_id = ? AND br.returned = FALSE";
            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query, strlen(query));

            MYSQL_BIND params[1];
            memset(params, 0, sizeof(params));
            params[0].buffer_type = MYSQL_TYPE_LONG;
            params[0].buffer = &userId;
            mysql_stmt_bind_param(stmt.get(), params);

            mysql_stmt_execute(stmt.get());
            mysql_stmt_store_result(stmt.get());

            json books_array = json::array();
            if (mysql_stmt_num_rows(stmt.get()) > 0)
            {
                long long bookId;
                char title[256];
                MYSQL_BIND result[2];
                memset(result, 0, sizeof(result));
                result[0].buffer_type = MYSQL_TYPE_LONGLONG;
                result[0].buffer = &bookId;
                result[1].buffer_type = MYSQL_TYPE_STRING;
                result[1].buffer = title;
                result[1].buffer_length = sizeof(title);
                mysql_stmt_bind_result(stmt.get(), result);
                while (mysql_stmt_fetch(stmt.get()) == 0)
                {
                    books_array.push_back({{"id", bookId}, {"title", title}});
                }
            }
            return {200, books_array};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }

    // ---- 新增管理员和推荐功能 ----

    // API (Admin): 获取所有用户
    ApiResponse getAllUsers()
    {
        try
        {
            DBConnection db;
            if (mysql_query(db.get(), "SELECT id, username, is_admin FROM users"))
            {
                throw runtime_error("mysql_query failed: " + string(mysql_error(db.get())));
            }
            unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(mysql_store_result(db.get()), &mysql_free_result);
            if (!result)
            {
                throw runtime_error("mysql_store_result failed: " + string(mysql_error(db.get())));
            }
            json users_array = json::array();
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(result.get())))
            {
                users_array.push_back({{"id", stoll(row[0])},
                                       {"username", row[1]},
                                       {"isAdmin", (bool)atoi(row[2])}});
            }
            return {200, users_array};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", e.what()}}};
        }
    }

    // API (Admin): 获取指定用户的全部借阅历史
    ApiResponse getAllUserBorrows(int userId)
    {
        try
        {
            DBConnection db;
            const char *query = "SELECT b.id, b.title, br.borrow_date, br.returned FROM borrow_records br JOIN books b ON br.book_id = b.id WHERE br.user_id = ? ORDER BY br.borrow_date DESC";
            unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(db.get()), &mysql_stmt_close);
            mysql_stmt_prepare(stmt.get(), query, strlen(query));
            MYSQL_BIND params[1];
            memset(params, 0, sizeof(params));
            params[0].buffer_type = MYSQL_TYPE_LONG;
            params[0].buffer = &userId;
            mysql_stmt_bind_param(stmt.get(), params);
            mysql_stmt_execute(stmt.get());
            mysql_stmt_store_result(stmt.get());
            json records_array = json::array();
            if (mysql_stmt_num_rows(stmt.get()) > 0)
            {
                long long bookId;
                char title[256];
                MYSQL_TIME borrow_date;
                bool returned;
                MYSQL_BIND result[4];
                memset(result, 0, sizeof(result));
                result[0].buffer_type = MYSQL_TYPE_LONGLONG;
                result[0].buffer = &bookId;
                result[1].buffer_type = MYSQL_TYPE_STRING;
                result[1].buffer = title;
                result[1].buffer_length = sizeof(title);
                result[2].buffer_type = MYSQL_TYPE_TIMESTAMP;
                result[2].buffer = &borrow_date;
                result[3].buffer_type = MYSQL_TYPE_TINY;
                result[3].buffer = &returned;
                mysql_stmt_bind_result(stmt.get(), result);
                while (mysql_stmt_fetch(stmt.get()) == 0)
                {
                    char date_str[20];
                    snprintf(date_str, 20, "%04d-%02d-%02d", borrow_date.year, borrow_date.month, borrow_date.day);
                    records_array.push_back({{"bookId", bookId},
                                             {"title", title},
                                             {"borrowDate", date_str},
                                             {"returned", (bool)returned}});
                }
            }
            return {200, records_array};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", e.what()}}};
        }
    }

    // API: 获取图书推荐
    ApiResponse getRecommendations(int userId)
    {
        try
        {
            DBConnection db;
            // 复杂的推荐逻辑，分步执行
            // 1. 找到该用户借过的所有书
            string q1 = "SELECT book_id FROM borrow_records WHERE user_id=" + to_string(userId);
            if (mysql_query(db.get(), q1.c_str()))
                throw runtime_error("Query failed: " + string(mysql_error(db.get())));
            unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res1(mysql_store_result(db.get()), &mysql_free_result);
            string user_books = "";
            MYSQL_ROW row1;
            while ((row1 = mysql_fetch_row(res1.get())))
            {
                user_books += string(row1[0]) + ",";
            }
            if (user_books.empty())
                return {200, json::array()}; // 没有借阅记录，无法推荐
            user_books.pop_back();

            // 2. 找到借过这些书的其他用户
            string q2 = "SELECT DISTINCT user_id FROM borrow_records WHERE book_id IN (" + user_books + ") AND user_id != " + to_string(userId);
            if (mysql_query(db.get(), q2.c_str()))
                throw runtime_error("Query failed: " + string(mysql_error(db.get())));
            unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res2(mysql_store_result(db.get()), &mysql_free_result);
            string other_users = "";
            MYSQL_ROW row2;
            while ((row2 = mysql_fetch_row(res2.get())))
            {
                other_users += string(row2[0]) + ",";
            }
            if (other_users.empty())
                return {200, json::array()}; // 没有其他相似用户
            other_users.pop_back();

            // 3. 找到这些用户借阅的、但当前用户没借过的书，并按频率排序
            string q3 = "SELECT b.id, b.title, b.stock, COUNT(b.id) as freq "
                        "FROM borrow_records br JOIN books b ON br.book_id = b.id "
                        "WHERE br.user_id IN (" +
                        other_users + ") AND br.book_id NOT IN (" + user_books + ") "
                                                                                 "GROUP BY b.id, b.title, b.stock ORDER BY freq DESC LIMIT 5";
            if (mysql_query(db.get(), q3.c_str()))
                throw runtime_error("Query failed: " + string(mysql_error(db.get())));
            unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res3(mysql_store_result(db.get()), &mysql_free_result);

            json recs = json::array();
            MYSQL_ROW row3;
            while ((row3 = mysql_fetch_row(res3.get())))
            {
                recs.push_back({{"id", stoll(row3[0])},
                                {"title", row3[1]},
                                {"stock", stoi(row3[2])}});
            }
            return {200, recs};
        }
        catch (const exception &e)
        {
            return {500, {{"success", false}, {"message", "服务器错误: " + string(e.what())}}};
        }
    }
};

int main(void)
{
    Server svr;
    LibraryService service;

    // 提供静态文件服务
    svr.set_base_dir(".");
    svr.Get("/", [](const Request &req, Response &res)
            {
        std::ifstream file("index.html");
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            res.set_content(buffer.str(), "text/html; charset=utf-8");
        } else {
            res.status = 404;
            res.set_content("Not Found", "text/plain");
        } });

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

    cout << "服务器已启动于 http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}