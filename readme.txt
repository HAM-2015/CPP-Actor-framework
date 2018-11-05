C++版本已停止维护，请转到C#版本
https://github.com/HAM-2015/CsGo
https://gitee.com/hamasm/CsGo

并发逻辑控制框架，适用于复杂业务逻辑;
支持系统 windows、x86_x64/armv7_v8-linux、armv7_v8-android;
支持编译器 VC2013/VC2015，GCC4.8+;
依赖于boost 1.5x;
可以用来构建服务端/客户端，也可以用在与网络无关的程序中;
基于stackful/stackless协程技术，actor基于stackful，generator基于stackless;
actor/generator均可以类似普通函数一样调用别的actor/generator函数及传递参数;
actor运行时按需扩张栈空间(4K-1M)，64位下4G内存最多可创建一千万以上generator对象;
actor/generator之间多种0-copy消息通信方式;
1:n的方式划分串行调度队列，在不使用消息时也可有效规避多个actor/generator逻辑之间的线程安全问题;
三级调度，主流x86处理器单线程下actor调度一千万以上，generator两千万以上;
优化过的高效定时器，定时器资源非常轻量;
基于asio优化过的tcp/udp异步IO;
支持QT-UI线程调度驱动，极大简化UI与后台逻辑之间的交互;
私有项目，不提供开发文档，有兴趣请联系 591170887@qq.com;
可以任意使用或编辑源码，而不需通知作者，当然作者也不对使用本框架造成的任何损失负责.

svn url(版本保持最新):
svn://ham2015.6655.la:1000/MyActor
Node.js版本(支持web前端js)
svn://ham2015.6655.la:1000/MyActorNodeJs

github url:
https://github.com/HAM-2015/CPP-Actor-framework

oschina url:
http://git.oschina.net/hamasm/cpp-actor-framework

ftp url:
ftp://ham2015.6655.la/files/MyActor/

2017-01-17
优化asio异步IO，在linux下tcp/udp更加高效;

2016-09-20
添加generator下的channel通信机制（无缓存/有限缓存/无限缓存），支持select-case方式读取多个channel;
添加generator下的逻辑mutex互斥.

2016-09-08
添加基于stackless的generator，支持co_yield、co_async、co_await、co_call语义.

2016-08-18
添加Node.Js与C++扩展模块之间Actor交互(测试Node.Js版本6.2.2).

2016-08-09
添加可以在Node.Js C++扩展动态库uv线程内运行Actor(测试Node.Js版本6.2.2).

2016-03-26
移植到arm-android(Linux-Kernel-3.4.0+)平台(包括 QT For Android 异步UI驱动).

2016-03-15
移植到arm-linux平台.

2015-12-28
添加linux平台下的自动栈空间伸缩管理.

2015-12-25
添加windows平台下的自动栈空间伸缩管理.

2015-12-18
添加对mingw的支持.

2015-12-09
优化windows,linux平台下的定时器性能.

2015-11-13
修改windows下使用Fiber驱动上下文切换.

2015-11-11
添加在QT-UI线程中运行Actor，驱动异步UI.

2015-11-05
添加"通知句柄"丢失检测，消息等待时可以捕获通知句柄丢失异常.

2015-10-31
移植到linux系统.

2015-09-24
改进采用TLS技术检测当前代码运行在哪个Actor下.

2015-09-21
使Actor在lock_quit()后检测到quit_msg消息仍然可以运行.

2015-08-23
让一个Actor内可以同时支持多个相同类型消息.

2015-08-02
添加在一个Actor内互斥运行多个业务逻辑段.

2015-07-30
支持任意参数个数消息.

2015-07-12
优化右值转移，使消息传递支持0拷贝.
添加值引用消息.

2015-07-10
二级调度器shared_strand添加next_tick功能，提高消息传递性能.

2015-06-26
优化Actor的内部定时器.

2015-06-12
添加同步消息(sync_msg)和CSP模型消息(csp_channel).

2015-06-09
去掉“actor_mutex”、“actor_shared_mutex”不必要的 close 功能.

2015-06-06
添加直接产生上下文的回调函数，可以不用显示使用await操作等待回调完成.

2015-06-03
优化检测堆栈溢出功能，输出具体哪个Actor溢出日志.

2015-06-01
优化等待子Actor结束的性能，取消等待返回bool值.

2015-05-29
添加actor_shared_mutex，用于业务逻辑之间的同步.

2015-05-24
添加在DEBUG下创建Actor时保存调用堆栈，方便某个Actor异常时调试跟踪.

2015-05-15
添加可以检测当前代码运行在哪个Actor下.

2015-04-14
核心功能整体优化;
添加actor_mutex;
添加可以暂时锁定Actor，不让其强制退出，用于关键逻辑段;
添加多级消息代理.

2015-04-02
添加可以直接拿一个Actor句柄发送消息，然后通过匹配弹出消息.

2015-03-19
添加消息传递的右值优化.

2015-02-11
添加定时清理Actor栈池.

2015-02-05
增加外部可以直接拿另一个Actor句柄创建一个消息管道或通知句柄.

2015-02-01
添加socket测试示例.

2015-01-26
修改了定时器在高版本VS下因与STL库冲突导致的编译错误问题;
增加异步触发和消息等待的超时处理功能.

2015-01-08
修改了挂起/恢复控制逻辑，原有逻辑在极端情况下存在安全风险.
