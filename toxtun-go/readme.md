

### 连接流程

每个连接分布一个KCP。所以，KCP创建需要一个协商过程。

* tun客户端接收到客户端请求连接
* tun客户端创建半连接Channel对象，保存该连接。
* 通过tox FriendSendMessage发送KCP连接协商请求，需要确定消息发送成功。
* tun服务端收到KCP连接协商请求，创建Channel对象，并发送分配的KCP的conv结果。
* tun服务端创建KCP实例，与上一步创建的Channel对象关联。
* tun客户端收到协商响应，创建KCP实现，与当前的Channel对象关系。


### 协商KCP的conv机制
* KCP conv值的协商，根据当前时间与客户端ID实现。也就是动态conv值，防止产生碰撞冲突。
* 需要设置一个基准时间，而不是绝对时间。（也可以用绝对时间，还可以加上其他信息，像服务器IP与端口）。
* 两个值结合起来，生成crc32值，作为KCP的conv(sation)值。
* 总的来说，是客户端发起协商请求，服务端计算并分配conv(sation)值。
* 协商过程不使用KCP连接，而是使用tox FriendMessage，是因为KCP连接无法做连接超时控制。

### 速度

KCP+tox(lossy packet)默认配置：100K+/s

开启KCP NoDelay模式后：K/s

### 线程模型

由于有回调的事件模式，会涉及多线程的数据结构操作，因此要考虑线程同步问题，否则出现一些程序崩溃问题。

第一种，全部使用同一线程，其他线程的数据库通过channel传递到该事件处理线程（主线程）进行处理。

要注意，已经在同一线程中的操作不需要再通过channel发送事件，

这样更直接，也更不容易导致多channel的死锁问题。

客户端goroutine个数，main+tox+kcp+client

服务端goroutine个数，main+tox+kcp+client


第二种，使用锁，对产生多线程操作冲突的代码通过锁来同步。


### 选型

虽然可以直接用tox的lossless packet，但仍旧要加一层KCP，因为toxnet可能暂时性掉线(offline)。

如果有活动连接和数据包，则要考虑自己处理重发。

