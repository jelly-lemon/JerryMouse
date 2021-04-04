#include <iostream>
#include <gtest/gtest.h>
#include <string.h>
#include "../HttpResponse.cpp"
using namespace std;


//class TestHttpResponse: public testing::Test {
//    virtual void SetUp() {
//        // 创建监听 socket
//    }
//};

TEST(NormalTest, test_0) {
    SOCKET socket1;
    HttpResponse response(socket1);
}


int main() {
    // 初始化测试用例环境
    testing::InitGoogleTest();

    // 执行所有的测试用例
    return RUN_ALL_TESTS();
}