CREATE DATABASE IF NOT EXISTS library_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE library_db;
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(255) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    is_admin BOOLEAN NOT NULL DEFAULT FALSE
);
CREATE TABLE IF NOT EXISTS books (
    id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    stock INT NOT NULL
);
CREATE TABLE IF NOT EXISTS borrow_records (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    book_id INT NOT NULL,
    borrow_date TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    returned BOOLEAN NOT NULL DEFAULT FALSE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE,
    INDEX idx_user_book_returned (user_id, book_id, returned)
);
-- 插入初始数据
-- 注意：每次执行此脚本都会清空旧数据
TRUNCATE TABLE borrow_records;
TRUNCATE TABLE users;
TRUNCATE TABLE books;
-- 添加管理员和普通用户
INSERT INTO users (username, password, is_admin)
VALUES ('admin', '123456', TRUE),
    ('test', 'password', FALSE);
-- 添加图书
INSERT INTO books (title, stock)
VALUES ('C++ Primer 中文版', 5),
    ('百年孤独', 4),
    ('深入理解计算机系统', 3),
    ('三体', 2),
    ('时间简史', 1),
    ('活着', 3);
-- 添加借阅记录，用于测试推荐功能
INSERT INTO borrow_records (user_id, book_id, returned)
VALUES (1, 1, TRUE),
    -- admin 借过 C++ Primer
    (2, 1, FALSE),
    -- test 借了 C++ Primer
    (2, 2, FALSE);
-- test 借了 百年孤独