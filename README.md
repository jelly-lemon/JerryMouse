# JerryMouse

C++/面向对象/IO 多路复用/并发模型，Windows 平台下简单 web/http 后台，学习用。

功能描述：

能够响应客户端 HTTP GET 静态资源请求，比如 *.html，图片等。
功能简单，不涉及数据库。

技术概要：

- 线程同步：互斥锁、条件变量、lock_guard、同步队列
- 智能指针：unique_ptr/shared_ptr，避免忘记 delete、内存泄露
- 命令行参数解析：自定义参数运行
- 异步日志：生产者消费者模型，不阻塞主线程
- 线程池：合理利用线程资源，使 CPU 利用率最大化
- select/poll/epoll：I/O 多路复用，I/O 密集型程序必用接口
- IOCP：Windows 下真正的异步 I/O 接口
- 并发模型：reactor、proactor


# 目录说明
```
--JerryMouse
  |--include        头文件
  |--web_root       web 根目录，也就是浏览器可访问的资源目录
  |--v0_one_thread  单线程模型
  |--v1_per_connection_per_thread   一个连接一个线程模型
  |--v1_1_threadpool    线程池模型
  |--v2_select_threadpoll   select + 线程池模型
```

# 更新日志


# 运行方法
## 方法 1
Windows/CLion/MinGW-64（我开发使用的工具链，推荐）：即在 Win 10 下，IDE 为 CLion，C++ 编译器为 MinGW-64。

1. 下载和安装 CLion 和 MinGW-64

CLion 版本无所谓，但编译器版本一定要相同：mingw64-x86_64-8.1.0-release-posix-sjlj-rt_v6-rev0（注意关键字：x86_64、8.1.0、posix、sjlj）

2. 在 CLion 中配置 C++ 工具链 
   
即在 CLion 中配置 MinGW-64 路径。

3. 使用 CLion/get from VCS 克隆本项目到你的电脑

4. 选择目标运行



# 性能测试
## 对比对象
对比实验使用 httpd-2.4.47-win64-VS16/Apache24/bin/httpd.exe 作为对比的对象（在 Windows 端运行）。Apache HTTP Server 于1995年推出，自1996年4月以来，它一直是互联网上最流行的网络服务器。Apache HTTP Server 官网：https://httpd.apache.org/。

用于测试的网页：http://10.66.38.27/home.html （10.66.38.27 是我的局域网 IP），截图如下：
![](https://github.com/jelly-lemon/JerryMouse/blob/master/img/test_page.png?raw=true)

## 测试 1
在虚拟机中使用 Webbench 1.5 进行压力测试。

整个测试过程为：首先，启动 httpd.exe；其次，打开虚拟机，运行 Centos7；最后，在 Centos7 中运行 Webbench，对 httpd.exe 进行压力测试。

运行命令：./Webbench -c100 -t20 http://10.66.38.27/home.html

### 本机配置

操作系统：Windows 10

CPU：i7-7700 3.60GHz

内存：8.00 GB (2667 MHz)

固态硬盘：Colorful SL300 120GB

虚拟机配置：

操作系统：Centos7

CPU：1 核

内存：1 G

### 测试 httpd.exe



## 测试 2
使用 ab.exe（ApacheBench 2.3） 进行压力测试。

测试参数：
ab.exe -t20 -c100 http://127.0.0.1/home.html

含义为：在 20s，并发用户数为 100，不断请求指定网页，最多累计访问 5w 次（本实验中，请求内容比较简单，所以在 20s 内能完成 5w 次）。

### 本机配置
同测试 1

### 测试 httpd.exe
RPS = 8224 （这是 ab.exe 给出的平均值，峰值应该还要再大几百）


### v0_one_thread（单线程）
max RPS = 6311

因为是单线程，并且没有使用 I/O 多路复用接口，所以一次只能处理一个 socket，所以并发用户数最大只能为 1。

### v1_per_connection_per_thread
max RPS = 6169 

比 v0_one_thread 低的原因：大量的线程切换。

### v1_1_threadpool
max RPS = 6719

比 v0_one_thread 高的原因：控制了线程数量，合理利用多核 CPU。

### v2_select_threadpool
max RPS = 6311

和 v0_one_thread 性能相同、比 v1_1_threadpool 差的原因：使用了 threadpool 应该比单线程表现要好的，并且 v2_select_threadpool 
还使用了 select 来遍历 socket。但是，在我们的实验中，所有 socket 都是建立连接后立即请求的，并且服务端响应一次后就立即关闭。
换言之，所有 socket 都是就绪的，不存在需要等待的情况，I/O 多路复用就没有用。所以 select 并不会带来任何帮助，反而会降低表现性能。


# 性能瓶颈分析
可能存在的性能瓶颈：

- [ ] CPU：CPU 处理速度太慢；

- [ ] 内存：内存有限，无法同时创建过多的资源；虽然有虚拟内存，可以解决内存不足的问题，但是频繁页面置换会花费不少 CPU 资源。

- [ ] 网络：网络传输速度过慢；

- [ ] 频繁地线程切换

## 评估指标
- CPU 利用率

- 线程切换次数（Context Switches）：

- 缺页中断次数（Page Fault）：缺页中断越多，证明内存越不够用。

- 下载上传网速：

- 文件读写速度：

# TODO
- [ ] HttpConnection 封装



# 发现 Bug 及优化

因本人能力有限，并且可能存在一些知识误区，所以会出现很多问题。

若你遇到问题或 Bug，欢迎提 issue；
方法如下：

若你想参与开发及优化代码，欢迎提 Pull Request。
方法如下：

我将及时做出回复。