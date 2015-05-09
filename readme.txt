并发逻辑控制框架(Actor Model)，适用于复杂逻辑控制场合，有问题或BUG反馈至 591170887@qq.com;
依赖于boost 1.57;
仅在VC2010,2013编译器中测试，其它编译器不保证.

github url:
https://github.com/HAM-2015/CPP-Actor-framework

oschina url
http://git.oschina.net/hamasm/cpp-actor-framework
http://www.oschina.net/code/snippet_2274073_45577

2015-05-09
添加在wxWidgets库UI线程中运行Actor.

2015-04-14
核心功能整体优化;
添加actor_mutex;
添加可以暂时锁定Actor，不让其强制退出，用于关键逻辑段;
添加多级消息代理.

2015-04-02
添加可以直接拿一个Actor句柄发送消息，然后通过匹配弹出消息.

2015-03-19
添加消息传递的右值优化.

2015-03-05
添加可以在MFC线程中运行Actor.

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