Windows 平台下的超迷你 Web 服务端

# 功能描述
能够响应客户端静态资源请求，比如 *.html，图片等；


# 开发环境
IDE：CLion<br/>
编译器：mingw64-x86_64-8.1.0-release-posix-sjlj-rt_v6-rev0<br/>
运行平台：Windows 10<br/>

# 开发技术点
命令行传参：
单例模式：
异步日志：


# 取名
JimDog<br/>
JerryMouse

# TODO
[] URL base64 解析


# 思考
Q: Http 请求设置了 keep-alive，如果服务端持续没有收到客户端请求，那究竟服务端保持多久呢？如果不释放，就一直占着资源。
A:

# 遇到的问题
Q: 类文件相互 include?
A: 这种类依赖关系是不好的，应避免这种设计。正确的类依赖关系应该是像一颗树那样，具有层次关系。

Q: 多线程环境下，控制台打印混乱？
A:

Q: 经常忘记 delete，导致内存泄露？
A:

Q: exit 导致最后一条日志没有输出？
A: exit 太快，要等待日志写入完成

Q: 多线程环境很难调试？
A:
