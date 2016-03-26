并发逻辑控制框架(Actor Model)，适用于复杂业务逻辑，有问题或BUG反馈至591170887@qq.com;
依赖于boost 1.59;
暂无开发文档，等作者认为可以定型后开放文档;
目前仅在 VS2013,MINGW49_x64,GCC4.9-linux_x86_x64_arm32,arm-linux-eabi-gcc49(android4.4.3) 中测试;
可以用来构建服务端/客户端，也可以用在与网络无关的程序中（例如UI控制逻辑）;
可以任意使用或编辑源码，而不需通知作者，当然作者也不对使用本框架造成的任何损失负责.

github url:
https://github.com/HAM-2015/CPP-Actor-framework

oschina url:
http://git.oschina.net/hamasm/cpp-actor-framework
http://www.oschina.net/code/snippet_2274073_45577

2016-03-26
移植到arm-android(4.4.3+)平台(支持在 QT-UI For Android 框架中运行).

2016-03-15
移植到arm-linux平台.

2015-12-28
添加linux平台下的自动栈空间伸缩管理;
感谢libsigsegv(http://www.gnu.org/software/libsigsegv/).

2015-12-25
添加windows平台下的自动栈空间伸缩管理.

2015-12-18
添加对mingw的支持.

2015-12-09
扩展对windows平台下的稳定定时器支持.

2015-11-13
添加对windows-fiber的支持.

2015-11-11
添加在QT-UI线程中运行Actor.

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
去掉“actor_mutex”、“actor_condition_variable”、“actor_shared_mutex”不必要的 close 功能.

2015-06-06
添加直接产生上下文的回调函数，可以不用显示使用await操作等待回调完成.

2015-06-03
优化检测堆栈溢出功能，输出具体哪个Actor溢出日志.

2015-06-01
优化等待子Actor结束的性能，取消等待返回bool值.

2015-05-29
添加actor_shared_mutex，在Actor下运行的“互斥锁(可递归)”、“条件变量”、“读写锁”已备齐，用于业务逻辑之间的同步.

2015-05-25
添加能在Actor下运行的条件变量actor_condition_variable.

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