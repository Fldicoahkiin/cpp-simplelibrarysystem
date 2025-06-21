#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <httplib.h>
#include <mysql/mysql.h>
#include <mysql/mysqld_error.h>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#if __has_include("index_html.h")
#include "index_html.h"
#endif

using json = nlohmann::json;
using namespace httplib;

const char *DB_HOST = "127.0.0.1";
const char *DB_USER = "root";
const char *DB_PASS = "PassW0rd";
const char *DB_SCHEMA = "library_db";
const unsigned int DB_PORT = 3306;

class DBConnection
{
private:
    MYSQL *conn;
    bool connected;

public:
    DBConnection() : conn(nullptr), connected(false)
    {
        conn = mysql_init(nullptr);
        if (!conn)
        {
            fprintf(stderr, "Critical: mysql_init failed.\n");
            return;
        }
        if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_SCHEMA, DB_PORT, nullptr, 0))
        {
            fprintf(stderr, "Critical: mysql_real_connect failed: %s\n", mysql_error(conn));
            mysql_close(conn);
            conn = nullptr;
            return;
        }
        printf("Successfully connected to database '%s'.\n", DB_SCHEMA);
        mysql_set_character_set(conn, "utf8mb4");
        connected = true;
    }

    ~DBConnection()
    {
        if (conn)
        {
            mysql_close(conn);
        }
    }

    MYSQL *get() { return conn; }
    bool isConnected() const { return connected; }
};

struct ApiResponse
{
    int status;
    json body;
};

struct QueryResult
{
    json data;
    bool success;
    const char *error_message;
};

class LibraryService
{
private:
    char *escape(MYSQL *conn, const char *s)
    {
        if (s == nullptr)
            s = "";
        size_t len = strlen(s);
        char *to = new char[len * 2 + 1];
        mysql_real_escape_string(conn, to, s, len);
        return to;
    }

    QueryResult executeQuery(MYSQL *conn, const char *query)
    {
        if (mysql_query(conn, query))
        {
            return {json(), false, mysql_error(conn)};
        }

        MYSQL_RES *result = mysql_store_result(conn);
        if (!result)
        {
            if (mysql_field_count(conn) == 0)
            {
                return {{{"affected_rows", mysql_affected_rows(conn)}}, true, nullptr};
            }
            else
            {
                return {json(), false, mysql_error(conn)};
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
                    j_row[field_name] = nullptr;
                }
                else if (strcmp(field_name, "is_admin") == 0 || strcmp(field_name, "returned") == 0)
                {
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
        return {j_res, true, nullptr};
    }

    bool isAdmin(MYSQL *conn, int userId)
    {
        char query[256];
        snprintf(query, sizeof(query), "SELECT is_admin FROM users WHERE id = %d;", userId);
        QueryResult qr = executeQuery(conn, query);
        if (qr.success && qr.data.is_array() && !qr.data.empty())
        {
            return qr.data[0].at("is_admin").get<bool>();
        }
        if (!qr.success)
        {
            fprintf(stderr, "isAdmin check failed: %s\n", qr.error_message);
        }
        return false;
    }

public:
    ApiResponse registerUser(const json &j)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        if (mysql_query(conn, "START TRANSACTION"))
            return {500, {{"success", false}, {"message", "开启事务失败"}}};

        try
        {
            std::string username_s = j.at("username").get<std::string>();
            std::string password_s = j.at("password").get<std::string>();

            char *safe_username = escape(conn, username_s.c_str());
            char *safe_password = escape(conn, password_s.c_str());

            char *check_query = new char[strlen(safe_username) + 64];
            snprintf(check_query, strlen(safe_username) + 64, "SELECT id FROM users WHERE username = '%s';", safe_username);

            QueryResult qr_check = executeQuery(conn, check_query);
            delete[] check_query;

            if (!qr_check.success)
            {
                mysql_query(conn, "ROLLBACK");
                delete[] safe_username;
                delete[] safe_password;
                return {500, {{"success", false}, {"message", qr_check.error_message}}};
            }
            if (!qr_check.data.empty())
            {
                mysql_query(conn, "ROLLBACK");
                delete[] safe_username;
                delete[] safe_password;
                return {409, {{"success", false}, {"message", "用户名已存在"}}};
            }

            char *insert_query = new char[strlen(safe_username) + strlen(safe_password) + 64];
            snprintf(insert_query, strlen(safe_username) + strlen(safe_password) + 64, "INSERT INTO users (username, password) VALUES ('%s', '%s');", safe_username, safe_password);

            delete[] safe_username;
            delete[] safe_password;

            QueryResult qr_insert = executeQuery(conn, insert_query);
            delete[] insert_query;

            if (!qr_insert.success)
            {
                mysql_query(conn, "ROLLBACK");
                return {500, {{"success", false}, {"message", qr_insert.error_message}}};
            }

            printf("User registration query executed for: %s\n", username_s.c_str());

            if (mysql_query(conn, "COMMIT"))
                return {500, {{"success", false}, {"message", "提交事务失败"}}};

            printf("Transaction committed for user: %s\n", username_s.c_str());
            return {201, {{"success", true}, {"message", "注册成功"}}};
        }
        catch (const json::exception &e)
        {
            mysql_query(conn, "ROLLBACK");
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "请求参数错误: %s", e.what());
            return {400, {{"success", false}, {"message", error_msg}}};
        }
    }

    ApiResponse loginUser(const json &j)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        try
        {
            std::string username_s = j.at("username").get<std::string>();
            std::string password_s = j.at("password").get<std::string>();

            char *safe_username = escape(conn, username_s.c_str());
            char *safe_password = escape(conn, password_s.c_str());

            char *query = new char[strlen(safe_username) + strlen(safe_password) + 128];
            snprintf(query, strlen(safe_username) + strlen(safe_password) + 128, "SELECT id, is_admin FROM users WHERE username = '%s' AND password = '%s';", safe_username, safe_password);

            delete[] safe_username;
            delete[] safe_password;

            QueryResult qr = executeQuery(conn, query);
            delete[] query;

            if (!qr.success)
                return {500, {{"success", false}, {"message", qr.error_message}}};
            if (qr.data.empty())
                return {401, {{"success", false}, {"message", "用户名或密码错误"}}};

            int userId = atoi(qr.data[0].at("id").get<std::string>().c_str());
            bool isAdminFlag = qr.data[0].at("is_admin").get<bool>();

            char token[128];
            snprintf(token, sizeof(token), "fake-jwt-token-for-%d", userId);

            return {200, {{"success", true}, {"token", token}, {"userId", userId}, {"username", username_s}, {"isAdmin", isAdminFlag}}};
        }
        catch (const json::exception &e)
        {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "请求参数错误: %s", e.what());
            return {400, {{"success", false}, {"message", error_msg}}};
        }
    }

    ApiResponse searchBooks(const char *searchTerm)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();

        char *query = nullptr;
        if (searchTerm != nullptr && searchTerm[0] != '\0')
        {
            char *safe_term = escape(conn, searchTerm);
            query = new char[strlen(safe_term) + 128];
            snprintf(query, strlen(safe_term) + 128, "SELECT id, title, stock FROM books WHERE title LIKE '%%%s%%';", safe_term);
            delete[] safe_term;
        }
        else
        {
            const char *base_query = "SELECT id, title, stock FROM books;";
            query = new char[strlen(base_query) + 1];
            strcpy(query, base_query);
        }

        QueryResult qr = executeQuery(conn, query);
        delete[] query;

        if (!qr.success)
            return {500, {{"success", false}, {"message", qr.error_message}}};
        return {200, qr.data};
    }

    ApiResponse addBook(const json &j)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        try
        {
            int adminId = j.at("adminId").get<int>();
            if (!isAdmin(conn, adminId))
            {
                return {403, {{"success", false}, {"message", "无管理员权限"}}};
            }

            std::string title_s = j.at("title").get<std::string>();
            int stock = j.at("stock").get<int>();

            char *safe_title = escape(conn, title_s.c_str());
            char *query = new char[strlen(safe_title) + 128];
            snprintf(query, strlen(safe_title) + 128, "INSERT INTO books (title, stock) VALUES ('%s', %d);", safe_title, stock);
            delete[] safe_title;

            QueryResult qr = executeQuery(conn, query);
            delete[] query;
            if (!qr.success)
                return {500, {{"success", false}, {"message", qr.error_message}}};

            return {201, {{"success", true}, {"message", "图书添加成功"}}};
        }
        catch (const json::exception &e)
        {
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "请求参数错误: %s", e.what());
            return {400, {{"success", false}, {"message", error_msg}}};
        }
    }

    ApiResponse borrowBook(const json &j_req)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();

        try
        {
            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            if (mysql_query(conn, "START TRANSACTION"))
                return {500, {{"success", false}, {"message", "开启事务失败"}}};

            char query[256];
            snprintf(query, sizeof(query), "SELECT id FROM borrow_records WHERE user_id = %d AND book_id = %d AND returned = FALSE FOR UPDATE;", userId, bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "检查重复借阅失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }
            MYSQL_RES *check_result = mysql_store_result(conn);
            if (check_result && mysql_num_rows(check_result) > 0)
            {
                mysql_free_result(check_result);
                mysql_query(conn, "ROLLBACK");
                return {409, {{"success", false}, {"message", "您已借阅此书且尚未归还，不能重复借阅"}}};
            }
            if (check_result)
                mysql_free_result(check_result);

            snprintf(query, sizeof(query), "SELECT stock FROM books WHERE id = %d FOR UPDATE;", bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "检查库存失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }
            MYSQL_RES *result = mysql_store_result(conn);
            if (!result || mysql_num_rows(result) == 0)
            {
                if (result)
                    mysql_free_result(result);
                mysql_query(conn, "ROLLBACK");
                return {404, {{"success", false}, {"message", "未找到该图书"}}};
            }
            MYSQL_ROW row = mysql_fetch_row(result);
            int stock = atoi(row[0]);
            mysql_free_result(result);

            if (stock <= 0)
            {
                mysql_query(conn, "ROLLBACK");
                return {409, {{"success", false}, {"message", "图书库存不足"}}};
            }

            snprintf(query, sizeof(query), "UPDATE books SET stock = stock - 1 WHERE id = %d;", bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "扣减库存失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }

            snprintf(query, sizeof(query), "INSERT INTO borrow_records (user_id, book_id) VALUES (%d, %d);", userId, bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "创建借阅记录失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }
            if (mysql_query(conn, "COMMIT"))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "提交事务失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }

            return {200, {{"success", true}, {"message", "借阅成功"}}};
        }
        catch (const json::exception &e)
        {
            mysql_query(conn, "ROLLBACK");
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "请求参数错误: %s", e.what());
            return {400, {{"success", false}, {"message", error_msg}}};
        }
    }

    ApiResponse returnBook(const json &j_req)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();

        try
        {
            int userId = j_req.at("userId").get<int>();
            int bookId = j_req.at("bookId").get<int>();

            mysql_query(conn, "START TRANSACTION");

            char query[256];
            snprintf(query, sizeof(query), "UPDATE borrow_records SET returned = TRUE, return_date = NOW() WHERE user_id = %d AND book_id = %d AND returned = FALSE ORDER BY id ASC LIMIT 1;", userId, bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "更新借阅记录失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }
            if (mysql_affected_rows(conn) == 0)
            {
                mysql_query(conn, "ROLLBACK");
                return {404, {{"success", false}, {"message", "您没有借阅该图书或该书已还"}}};
            }

            snprintf(query, sizeof(query), "UPDATE books SET stock = stock + 1 WHERE id = %d;", bookId);
            if (mysql_query(conn, query))
            {
                mysql_query(conn, "ROLLBACK");
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "更新图书库存失败: %s", mysql_error(conn));
                return {500, {{"success", false}, {"message", error_msg}}};
            }

            mysql_query(conn, "COMMIT");
            return {200, {{"success", true}, {"message", "归还成功"}}};
        }
        catch (const json::exception &e)
        {
            mysql_query(conn, "ROLLBACK");
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "请求参数错误: %s", e.what());
            return {400, {{"success", false}, {"message", error_msg}}};
        }
    }

    ApiResponse getUserBorrows(int userId)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        char query[256];
        snprintf(query, sizeof(query), "SELECT br.id, b.id as book_id, b.title, br.borrow_date FROM books b JOIN borrow_records br ON b.id = br.book_id WHERE br.user_id = %d AND br.returned = FALSE;", userId);
        QueryResult qr = executeQuery(conn, query);
        if (!qr.success)
            return {500, {{"success", false}, {"message", qr.error_message}}};
        return {200, qr.data};
    }

    ApiResponse getAllUsers(int adminId)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        if (!isAdmin(conn, adminId))
        {
            return {403, {{"success", false}, {"message", "无管理员权限"}}};
        }
        QueryResult qr = executeQuery(conn, "SELECT id, username, is_admin FROM users;");
        if (!qr.success)
            return {500, {{"success", false}, {"message", qr.error_message}}};
        return {200, qr.data};
    }

    ApiResponse getAllUserBorrows(int adminId, int userId)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        if (!isAdmin(conn, adminId))
        {
            return {403, {{"success", false}, {"message", "无管理员权限"}}};
        }
        char query[256];
        snprintf(query, sizeof(query), "SELECT br.id as borrow_id, b.id as book_id, b.title, br.borrow_date, br.return_date, br.returned FROM books b JOIN borrow_records br ON b.id = br.book_id WHERE br.user_id = %d ORDER BY br.borrow_date DESC;", userId);
        QueryResult qr = executeQuery(conn, query);
        if (!qr.success)
            return {500, {{"success", false}, {"message", qr.error_message}}};
        return {200, qr.data};
    }

    ApiResponse getRecommendations(int userId)
    {
        DBConnection db;
        if (!db.isConnected())
            return {500, {{"success", false}, {"message", "数据库连接失败"}}};
        MYSQL *conn = db.get();
        const char *query = "SELECT b.id, b.title, b.stock "
                            "FROM books b "
                            "LEFT JOIN (SELECT book_id, COUNT(*) as borrow_count FROM borrow_records GROUP BY book_id) as popular ON b.id = popular.book_id "
                            "ORDER BY popular.borrow_count DESC, b.id ASC LIMIT 10;";
        QueryResult qr = executeQuery(conn, query);
        if (!qr.success)
            return {500, {{"success", false}, {"message", qr.error_message}}};
        return {200, qr.data};
    }
};

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IONBF, 0);
#endif

    printf("Attempting to connect to database...\n");
    printf("  Host: %s\n", DB_HOST);
    printf("  User: %s\n", DB_USER);
    printf("  Database: %s\n", DB_SCHEMA);

    DBConnection db_check;
    if (!db_check.isConnected())
    {
        fprintf(stderr, "Failed to connect to the database on startup.\n");
        fprintf(stderr, "Please ensure the database server is running and the connection details are correct.\n");
        return 1;
    }
    printf("Initial database connection check successful.\n");

    Server svr;
    LibraryService service;

    svr.Get("/", [](const Request &req, Response &res)
            {
#ifdef index_html_content
                res.set_content(index_html_content, "text/html; charset=utf-8");
#else
                FILE *file = fopen("index.html", "rb");
                if (file)
                {
                    fseek(file, 0, SEEK_END);
                    long length = ftell(file);
                    fseek(file, 0, SEEK_SET);
                    char *buffer = new char[length + 1];
                    fread(buffer, 1, length, file);
                    fclose(file);
                    buffer[length] = '\0';
                    res.set_content(buffer, "text/html; charset=utf-8");
                    delete[] buffer;
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
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "无效的JSON格式: %s", e.what());
            api_res = {400, {{"success", false}, {"message", error_msg}}};
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

    svr.Get("/api/books", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        std::string search_term = req.get_param_value("q");
        auto api_res = service.searchBooks(search_term.c_str());
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    svr.Get(R"(/api/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        int userId = atoi(req.matches[1].str().c_str());
        auto api_res = service.getUserBorrows(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    svr.Get(R"(/api/user/(\d+)/recommendations)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        int userId = atoi(req.matches[1].str().c_str());
        auto api_res = service.getRecommendations(userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    svr.Get("/api/admin/users", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        ApiResponse api_res;
        const char *adminId_str = req.has_param("adminId") ? req.get_param_value("adminId").c_str() : "-1";
        int adminId = atoi(adminId_str);
        api_res = service.getAllUsers(adminId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

    svr.Get(R"(/api/admin/user/(\d+)/borrows)", [&](const Request &req, Response &res)
            {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Content-Type", "application/json; charset=utf-8");
        ApiResponse api_res;
        const char *adminId_str = req.has_param("adminId") ? req.get_param_value("adminId").c_str() : "-1";
        int adminId = atoi(adminId_str);
        int userId = atoi(req.matches[1].str().c_str());
        api_res = service.getAllUserBorrows(adminId, userId);
        res.status = api_res.status;
        res.set_content(api_res.body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8"); });

#ifdef _WIN32
    wprintf(L"服务器已启动于 http://localhost:8080\n");
#else
    printf("服务器已启动于 http://localhost:8080\n");
#endif
    svr.listen("0.0.0.0", 8080);

    return 0;
}