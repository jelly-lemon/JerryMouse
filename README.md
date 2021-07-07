# JerryMouse

C++ 开发，windows 平台下迷你 web/http 后台。

功能描述：

能够响应客户端 HTTP GET 静态资源请求，比如 *.html，图片等。

技术概要：

- 线程同步：互斥锁、使用 lock_guard，在退出作用域时自动释放锁
- 智能指针：unique_ptr，避免野指针
- 命令行参数解析：
- 异步日志：生产者消费者模型
- 线程池：
- select/poll/epoll：IO 多路复用
- IOCP：异步 IO
- 并发模型：reactor、proactor

# 更新日志


# 目录说明





# 运行方法

Windows/CLion/MinGW-64（我开发使用的工具链，推荐）：即在 Win 10 下，IDE 为 CLion，C++ 编译器为 MinGW-64。

1. 下载和安装 CLion 和 MinGW-64

   CLion 版本无所谓，但编译器版本一定要相同：mingw64-x86_64-8.1.0-release-posix-sjlj-rt_v6-rev0（注意关键字：x86_64、8.1.0、posix、sjlj）

2. 使用 CLion 克隆项目

3. 在 CLion 中配置 C++ 工具链

4. 选择目标运行



# 性能测试

## 本机配置

电脑配置

操作系统：Windows 10 

CPU：i7-7700 3.60GHz

内存：8.00 GB (2667 MHz)

固态硬盘：Colorful SL300 120GB



虚拟机配置

操作系统：Centos7 

CPU：1 核

内存：1 G



测压软件

Webbench 1.5



## 对比对象

对比实验使用 httpd-2.4.47-win64-VS16/Apache24/bin/httpd.exe 作为对比的对象。Apache HTTP Server 于1995年推出，自1996年4月以来，它一直是互联网上最流行的网络服务器。Apache HTTP Server 官网：https://httpd.apache.org/。

用于测试的网页：http://10.66.38.27/home.html （10.66.38.27 是我的局域网 IP），截图如下：



整个测试过程为：首先，启动 httpd.exe；其次，打开虚拟机，运行 Centos7；最后，在 Centos7 中运行 Webbench，对 httpd.exe 进行压力测试。

测试结果：

```shell
[root@VM_1 webbench-1.5]# ./webbench -c3000 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://10.66.38.27/home.html
3000 clients, running 5 sec.

Speed=360996 pages/min, 2050272 bytes/sec.
Requests: 7123 susceed, 22960 failed.
```

其中 -c3000 -t5 表示模拟 3000 个客户端在 5s 内反复访问指定网页。

计算 QPS：

QPS(httpd.exe) = 7123 succeed / 5 sec = 1426，即 QPS 约为 1426。



## 测试 JerryMouse

测试前的配置：关闭控制台输出、使用 Release 编译







# TODO

- [ ] URL base64 解析

# 发现 Bug 及优化

因本人能力有限，并且可能存在一些知识误区，所以会出现很多问题。

若你遇到问题或 Bug，欢迎提 issue；
方法如下：

若你想参与开发及优化代码，欢迎提 Pull Request。
方法如下：

我将及时做出回复。

# 遇到的问题

Q: TCP 三次握手完成后，客户端迟迟不发送请求，那服务端保持连接多久呢？如果立即释放连接，说不定客户端请求已经在路上了。如果不释放，服务端又一直干等着，就会浪费内存资源。

A:



Q: 类文件相互 include?

A: 这种类依赖关系是不好的，应避免这种设计。正确的类依赖关系应该是像一颗树那样，具有层次关系。



Q: 多线程环境下，控制台打印混乱，输出被截断？

A: 加一个互斥锁。但是，可能先 printf 和后 printf 同时阻塞，但后 print 的得到锁，然后输出，导致次序混乱。

可以开一个线程，把打印内容放到队列里面。但是，遇到异常时最后一条内容还没打印程序就退出了。

自己再写一个安全退出函数，判断队列是否为空，为空才退出，否则等待。



Q: 经常忘记 delete，导致内存泄露？

A: 



Q: exit 导致最后一条日志没有输出？

A: exit 太快，要等待日志写入完成。可以判断日志队列是否为空，为空才退出，否则等待日志线程写完。



Q: 多线程环境很难调试？

A:



Q：如何获取实际 backlog 的大小？

A：无法获取



Q: 队列日志，读一条消息，就打开、关闭文件，要是有大量日志，会不会造成写入缓慢？

A: 



Q: 类 B 回调类 A 的成员函数？

A:



Q: 在大量短连接且建立连接就立即发送数据的情况下，IO 多路复用对并发处理能力有帮助吗？

A: 



Q: 当服务端空闲时，把线程挂起还是销毁？

A:

Q: 程序当中有没有办法判断 TCP 半连接和全连接队列的大小呢？

A: 没有办法。

Q: 如何获取任务执行完毕后的返回值？也就是异步执行返回值？