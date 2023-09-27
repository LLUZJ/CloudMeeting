# CloudMeeting_Server
视频会议后台。涉及进程池，线程池，socket通信，unix域套接字。客户端地址：https://github.com/muleimulei/CloudMeeting

## 使用方法
``
usage: ./app [host] <port #> <#threads> <#processes>
eg:
./app 0.0.0.0 8888 2 4
``

#### threads 用于处理客户端连接的线程数
#### processes 开启的房间数
