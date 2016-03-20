### 特性：
* 多TCP端口tunnel支持
* TCP长连接支持
* tunnel配置类似ssh -L

### 实测
在早上7-9点，能提供100k的国际通路，可以比流畅地看youtube视频。

### 用法

使用ini格式配置文件toxtun_foo.ini

配置文件包含两段：server/client

配置文件格式示例：

    [server]
    name = whttpd
    
    [client]
    toxtun1 = *:81:127.0.0.1:8181:A5A02FECA08E3EAC7E646B89C0507A8AAF9136E05DC756FF09F86230951820670F908F2E7719
    toxtun2 = *:82:127.0.0.1:8282:6BA28AC06C1D57783FE017FA9322D0B356E61404C92155A04F64F3B19C75633E8BDDEFFA4856


对于[server]段，只有一个配置项，name，会设置成tunnel服务端的tox的nickname。

对于[client]段，是主要的配置部分，第一行表示一个tunnel通道配置。

配置格式类似ssh的-L模式 [localip:]localport:remoteip:remoteport

不过，格式上有一点不同，toxtun实现则多一项pubkey，指的远程服务器端tox 节点 pubkey。

完整格式为： name = [localip:]localport:remoteip:remoteport:pubkey

这里的name，与[server]段一样，会设置成tunnel客户端的tox的nickname。

配置完成后的启动：

    ./tunnel <server | client> [toxtun_xxx.ini]


### 安装

    git clone https://github.com/kitech/toxtun
    cd toxtun/
    cmake .
    make
    

### TODO
- [x]  toxcore lossy packet
- [ ] 超时时间
- [x] 压缩
- [ ] 加密
- [x] crash: peer count exceed.
- [ ] client sock close处理
- [ ] 加大TCP读取包大小，与MTU大小接近，但不要超过。 
- [ ] enet client的正确使用方式。
- [ ] 不使用qt库方式
- [x] 开启压缩enet服务器端过早关闭连接。OK不是因为压缩引起的。
- [ ] 可能需要加长enet连接超时，最后一步才关闭enet。
- [ ] 还是要考虑在单独的线程内做enet_poll。这样速度快很多啊，从10k => 300k。
- [ ] (enet非线程安全，还要考虑完善方案。目前使用单线程模式，把interval降低到1ms，速度也能到300k，但cpu使用高了，暂时这么用)
- [ ] 一方关闭socket连接后，是否关闭另一方的socket连接呢？这个应该需要关闭。
- [ ]    这个可以先考虑cli->srv方向的测试，但要考虑是否客户端还有数据没有发送完。
- [ ]    因为虽然cli的socket关闭，有可能还有缓存的发送包队列没有完成。
- [ ] 考虑禁用enet的ping。是否会影响关闭，是否会影响重发包。
- [ ] 缓存包队列占用内存。
- [ ] 统计监控信息


### 延时关闭enet机制
在本地socket和远程socket都已经关闭的情况下，就认为本连接chan已经可以关闭。

这时就清理本chan的资源，除了enet\_peer本身。

然后，使用计时器，在几秒后调用enet\_peer\_disconnect\_xxx。

这样，让chan的清理与enet\_peer关闭有段时间差，避免enet_peer重新分配新连接时chan还未清理的问题。

这样，实际时确认enet\_peer比其他关闭的都晚。

但有一种情况，即超时时有可能enet\_peer早于socket关闭了。
    

### 浏览器设置
把per-serverxxx设置为0,好像效果好点。

使用了adblock好像好点了。

这说明这个东西在连接数比较多的情况，效果不怎么好啊。
    

### enet hacks
enet的enet\_host\_connect函数，返回的peer实例是实例池式的。

但是每次调用enet\_host\_connect函数，返回的peer虽然相同，但是返回的peer->connectID却不同。

修改ENET\_PROTOCOL\_MAXIMUM\_MTU，从4096变为1271，防止出现比TOX MSG允许更大的包。

或者ENET\_HOST\_DEFAULT\_MTU=1400，改为1271，，防止出现比TOX MSG允许更大的包。

加大peer的TIMEOUT值，分别为peer->timeoutLimit, peer->timeoutMinimum, peer->timeoutMaximum;

enet压缩的使用：

是host级别的，无法做到每个连接使用一个压缩context啊，难道说一个host下的所有连接使用同一个压缩可以吗。

开压缩导致服务器端enet连接过早关闭？？？

双层模式hacks：


#### 其他

[BUGS](BUGS.txt)


