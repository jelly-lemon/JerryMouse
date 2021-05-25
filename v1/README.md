# 版本特点：v1
首先，主线程用 while 循环从 accept 队列取出 socket；其次，根据线程池创建子线程响应 socket；最后，响应完毕后，立即关闭 socket 并结束子线程。
