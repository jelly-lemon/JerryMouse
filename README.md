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

# 更新日志




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

## 本机配置

操作系统：Windows 10

CPU：i7-7700 3.60GHz

内存：8.00 GB (2667 MHz)

固态硬盘：Colorful SL300 120GB



## 虚拟机配置

操作系统：Centos7

CPU：1 核

内存：1 G



## 测压软件

Webbench 1.5 or ApacheBench 2.3


## 测试 1
在虚拟机中使用 Webbench 进行压力测试。

### 对比对象

对比实验使用 httpd-2.4.47-win64-VS16/Apache24/bin/httpd.exe 作为对比的对象（在 Windows 端运行）。Apache HTTP Server 于1995年推出，自1996年4月以来，它一直是互联网上最流行的网络服务器。Apache HTTP Server 官网：https://httpd.apache.org/。

用于测试的网页：http://10.66.38.27/home.html （10.66.38.27 是我的局域网 IP），截图如下：

实验假设：所有的 HTTP 请求在建立连接后会立即发送请求数据，不存在连接建立后不发送数据的情况。我们使用 WebBench 来保证这一点。

![](https://github.com/jelly-lemon/JerryMouse/blob/master/img/test_page.png?raw=true)

整个测试过程为：首先，启动 httpd.exe；其次，打开虚拟机，运行 Centos7；最后，在 Centos7 中运行 Webbench，对 httpd.exe 进行压力测试。

测试结果：

```shell
[root@VM_1 WebBench]# ./webbench -c250 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /home.html HTTP/1.0
User-Agent: WebBench 1.5
Host: 10.66.38.27


Runing info: 250 clients, running 5 sec.

Speed=38724 pages/min, 929376 bytes/sec.
Requests: 3227 susceed, 0 failed.
```

其中 -c250 -t5 表示模拟 250 个客户端在 5s 内反复访问指定网页，可以理解成 250 个人分别用自己的电脑在浏览器中疯狂按 F5 刷新网页。（当我设置 300 个及以上客户端数量时，httpd.exe 就顶不住了，压测会出现 failed。）具体过程为，Webbench 保证同时有 250 个和服务端相连的 Http 连接。一旦一个 Http 连接完成，Webbench 立即又创建新的一个 Http 连接进行请求。若 250 个 Http 连接都没完成，那 Webbench 就一直等待。还需要注意的是，Webbench 创建一个 Http 连接后，只会 GET 请求一次，换言之，不会复用 Http 连接，每一次请求都会单独创建一个 Http 连接。



Webbench 对于一次 request 算 susceed 的情况：

- request http://10.66.38.27/home.html ---> 服务端响应 ---> 服务端关闭 socket --->  Webbench 收到 ---> susceed



一次 request 算 failed 的情况：

- Webbench 创建 socket 失败 ---> failed

- Webbench 创建 socket ---> write 失败 ---> failed

- Webbench 创建 socket ---> write ---> 关闭 socket 写操作失败 ---> failed

- Webbench 创建 socket ---> write ---> 关闭 socket 写操作 ---> read 失败（服务端没有响应数据或者没有关闭 socket，导致 read 阻塞，最后超时） ---> failed

- Webbench 创建 socket ---> write ---> 关闭 socket 写操作 ---> read ---> close socket 失败 ---> failed



计算 QPS：

QPS(httpd.exe) = 3227 succeed / 5 sec = 645，即 QPS 约为 645。



### 测试 JerryMouse
JerryMouse 对 HTTP 请求的处理：响应完毕后立即关闭连接。


#### v0_one_thread（单线程）

(-c 为 250 时会出现 failed)

```shell
[root@VM_1 WebBench]# ./webbench -c200 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /home.html HTTP/1.0
User-Agent: WebBench 1.5
Host: 10.66.38.27


Runing info: 200 clients, running 5 sec.

Speed=24708 pages/min, 524378 bytes/sec.
Requests: 2059 susceed, 0 failed.
```

QPS = 2059 / 5 = 411



#### v1_per_connection_per_thread（一个连接开一个线程）

(-c 为 250 时会出现 failed)

```shell
[root@VM_1 WebBench]# ./webbench -c200 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /home.html HTTP/1.0
User-Agent: WebBench 1.5
Host: 10.66.38.27


Runing info: 200 clients, running 5 sec.

Speed=29364 pages/min, 622986 bytes/sec.
Requests: 2447 susceed, 0 failed.
```

QPS = 2447 / 5 = 489



#### v1_1_threadpool（线程池）

```shell
[root@VM_1 WebBench]# ./webbench -c250 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /home.html HTTP/1.0
User-Agent: WebBench 1.5
Host: 10.66.38.27


Runing info: 250 clients, running 5 sec.

Speed=25260 pages/min, 536099 bytes/sec.
Requests: 2105 susceed, 0 failed.
```

QPS = 2105 / 5 = 421



#### v2_select_threadpool

```shell
[root@VM_1 WebBench]# ./webbench -c200 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET /home.html HTTP/1.0
User-Agent: WebBench 1.5
Host: 10.66.38.27


Runing info: 200 clients, running 5 sec.

Speed=24168 pages/min, 512912 bytes/sec.
Requests: 2014 susceed, 0 failed.
```

QPS = 2014 / 5 = 402

#### v2_1_pool_threadpool





#### v2_2_reactor（单线程+select）

```shell
[root@VM_1 webbench-1.5]# ./webbench -c250 -t5 http://10.66.38.27/home.html
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://10.66.38.27/home.html
250 clients, running 5 sec.

Speed=42624 pages/min, 905049 bytes/sec.
Requests: 3552 susceed, 0 failed.
```

QPS = 3552 / 5 = 710



#### v3_epoll_threadpool



#### v4_IOCP_threadpool



#### v4_1_acceptEx_IOCP_threadpool



#### v4_2_proactor

## 测试 2
使用 ab.exe 进行压力测试。

测试参数：
ab.exe -t20 -c100 http://127.0.0.1/home.html

含义为：在 20s，并发用户数为 100，不断请求指定网页，最多累计访问 5w 次。

### 测试 httpd.exe
RQS = 8224

### v0_one_thread（单线程）
max RQS = 6311

因为是单线程，并且没有使用 I/O 多路复用接口，所以一次只能处理一个 socket，所以并发用户数最大只能为 1。

# 性能瓶颈分析
可能存在的性能瓶颈：

- [ ] CPU：CPU 处理速度太慢；

- [ ] 内存：内存有限，无法同时创建过多的资源；虽然有虚拟内存，可以解决内存不足的问题，但是频繁页面置换会花费不少 CPU 资源。

- [ ] 网络：网络传输速度过慢；

- [ ] 频繁地线程切换

## 评估指标
CPU 利用率

线程切换次数（Context Switches）：

缺页中断次数（Page Fault）：缺页中断越多，证明内存越不够用。

下载上传网速：

文件读写速度：

# TODO
- [ ] HttpConnection 封装

- [ ] URL base64 解析，以支持中文



# 发现 Bug 及优化

因本人能力有限，并且可能存在一些知识误区，所以会出现很多问题。

若你遇到问题或 Bug，欢迎提 issue；
方法如下：

若你想参与开发及优化代码，欢迎提 Pull Request。
方法如下：

我将及时做出回复。