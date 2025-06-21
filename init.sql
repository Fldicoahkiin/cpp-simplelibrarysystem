-- Active: 1720593414059@@127.0.0.1@3306@library_db
CREATE DATABASE IF NOT EXISTS library_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE library_db;
-- 为了确保脚本可以重复执行，我们先按正确顺序删除旧表（如果存在的话）
-- 注意：必须先删除有外键依赖的表
DROP TABLE IF EXISTS borrow_records;
DROP TABLE IF EXISTS books;
DROP TABLE IF EXISTS users;
-- 用户表，增加 is_admin 字段
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID,主键',
    username VARCHAR(255) NOT NULL UNIQUE COMMENT '用户名,唯一',
    password VARCHAR(255) NOT NULL COMMENT '用户密码',
    is_admin BOOLEAN NOT NULL DEFAULT FALSE COMMENT '是否为管理员, 0:普通用户, 1:管理员'
) COMMENT '用户信息表';
-- 图书表
CREATE TABLE books (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '图书ID,主键',
    title VARCHAR(255) NOT NULL COMMENT '图书标题',
    stock INT NOT NULL COMMENT '图书库存量'
) COMMENT '图书信息表';
-- 借阅记录表，增加借阅日期和索引
CREATE TABLE borrow_records (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '借阅记录ID,主键',
    user_id INT NOT NULL COMMENT '借阅用户的ID,外键关联users表',
    book_id INT NOT NULL COMMENT '被借阅图书的ID,外键关联books表',
    borrow_date TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '借阅日期,默认为当前时间',
    return_date DATETIME NULL DEFAULT NULL COMMENT '归还日期,默认为NULL',
    returned BOOLEAN NOT NULL DEFAULT FALSE COMMENT '是否已归还, 0:未归还, 1:已归还',
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (book_id) REFERENCES books(id) ON DELETE CASCADE,
    INDEX idx_user_book_returned (user_id, book_id, returned)
) COMMENT '图书借阅记录表';
-- 插入初始数据
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
INSERT INTO borrow_records (user_id, book_id, returned, return_date)
VALUES 
    -- admin 曾借阅并归还了 'C++ Primer 中文版'
    (1, 1, TRUE, '2024-01-15 11:00:00'),
    -- test 当前借阅了 'C++ Primer 中文版' (未归还)
    (2, 1, FALSE, NULL),
    -- test 当前借阅了 '百年孤独' (未归还)
    (2, 2, FALSE, NULL);
-- test 借了 百年孤独