Windows 平台下的超迷你 Web 服务端

# 功能描述
能够响应客户端静态资源请求，比如 *.html，图片等；


# 开发环境
IDE：CLion<br/>
编译器：MinGW-w64 6.0<br/>
运行平台：Windows 10<br/>

# 取名
JimDog<br/>
JerryMouse

# 思考
Http 请求设置了 keep-alive，如果服务端持续没有收到客户端请求，那究竟服务端保持多久呢？如果不释放，就一直占着资源。
