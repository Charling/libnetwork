# libnetwork
基于boost.asio库高性能并发tcp网络库

核心设计思想：
1、提供 server/client tcp服务，供业务逻辑层回调处理包
2、处理超过64k包的分发
3、使用google protobuffer 协议
4、支持linux/win
