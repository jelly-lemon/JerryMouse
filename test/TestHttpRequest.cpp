#include <iostream>
#include <gtest/gtest.h>
#include <string.h>
#include "../v1/src/HttpRequest.cpp"
using namespace std;

/**
 * GET 请求，没有 body
 */
TEST(NormalValue, test_0) {
    string rawData("GET / HTTP/1.1\r\n"
            "Content-Type:text/plain\r\n"
            "User-Agent: PostmanRuntime/7.26.10\r\n"
            "Accept: */*\r\n"
            "Accept-Encoding: gzip, deflate, br\r\n"
            "Connection: keep-alive\r\n"
            "Host:localhost:12345\r\n\r\n");
    HttpRequest request(rawData);

    // 请求行
    ASSERT_EQ("GET", request.getMethod());
    ASSERT_EQ("/", request.getURL());
    ASSERT_EQ("HTTP/1.1", request.getHttpVersion());
    // 请求头
    ASSERT_EQ("text/plain", request.getHeader("Content-Type"));
    ASSERT_EQ("PostmanRuntime/7.26.10", request.getHeader("User-Agent"));
    ASSERT_EQ("*/*", request.getHeader("Accept"));
    ASSERT_EQ("gzip, deflate, br", request.getHeader("Accept-Encoding"));
    ASSERT_EQ("keep-alive", request.getHeader("Connection"));
    ASSERT_EQ("localhost:12345", request.getHeader("Host"));
    ASSERT_EQ("", request.getHeader("hello"));  // 测试没有对应 key 时，应该返回空串
    // 请求体
    ASSERT_EQ("", request.getBody());
}

/**
 * POST 请求，有 body
 */
TEST(NormalValue, test_1) {
    string rawData("GET / HTTP/1.1\r\n"
                   "Content-Type:text/plain\r\n"
                   "User-Agent: PostmanRuntime/7.26.10\r\n"
                   "Accept: */*\r\n"
                   "Accept-Encoding: gzip, deflate, br\r\n"
                   "Connection: keep-alive\r\n"
                   "Host:localhost:12345\r\n"
                   "Content-Length: 11\r\n\r\n"
                   "hello,world");
    HttpRequest request(rawData);

    ASSERT_EQ("GET", request.getMethod());
    ASSERT_EQ("/", request.getURL());
    ASSERT_EQ("HTTP/1.1", request.getHttpVersion());
    ASSERT_EQ("text/plain", request.getHeader("Content-Type"));
    ASSERT_EQ("PostmanRuntime/7.26.10", request.getHeader("User-Agent"));
    ASSERT_EQ("*/*", request.getHeader("Accept"));
    ASSERT_EQ("gzip, deflate, br", request.getHeader("Accept-Encoding"));
    ASSERT_EQ("keep-alive", request.getHeader("Connection"));
    ASSERT_EQ("localhost:12345", request.getHeader("Host"));
    ASSERT_EQ("11", request.getHeader("Content-Length"));
    ASSERT_EQ("hello,world", request.getBody());
}



/**
 * 没有 \r\n\r\n
 */
TEST(BoundaryValue, test_0) {
    string rawData("GET / HTTP/1.1");
    ASSERT_THROW(HttpRequest request(rawData), invalid_argument);

    rawData = "GET / HTTP/1.1\r\n";
    ASSERT_THROW(HttpRequest request(rawData), invalid_argument);
}

/**
 * 只有请求行
 */
TEST(BoundaryValue, test_1) {
    string rawData("GET / HTTP/1.1\r\n\r\n");
    HttpRequest request(rawData);

    ASSERT_EQ("GET", request.getMethod());
    ASSERT_EQ("/", request.getURL());
    ASSERT_EQ("HTTP/1.1", request.getHttpVersion());
    ASSERT_EQ("", request.getHeader("Content-Type"));
    ASSERT_EQ("", request.getBody());
}

/**
 * 内容、格式不合法
 */
TEST(InvalidValue, test_0) {
    string rawData("ABC\r\n\r\n");
    ASSERT_THROW(HttpRequest request(rawData), invalid_argument);

    rawData = "GET / HTTP/1.1\r\nabc\r\n\r\n";
    ASSERT_THROW(HttpRequest request(rawData), invalid_argument);
}



int main() {
    // 初始化测试用例环境
    testing::InitGoogleTest();

    // 执行所有的测试用例
    return RUN_ALL_TESTS();
}

