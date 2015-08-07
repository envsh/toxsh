
toxcore lossy packet --- OK
超时时间  
压缩 --- OK
加密
crash: peer count exceed.  -- OK
client sock close处理
加大TCP读取包大小，与MTU大小接近，但不要超过。 
enet client的正确使用方式。
不使用qt库方式
开启压缩enet服务器端过早关闭连接。 --OK不是因为压缩引起的。
可能需要加长enet连接超时，最后一步才关闭enet。
还是要考虑在单独的线程内做enet_poll。这样速度快很多啊，从10k => 300k。
(enet非线程安全，还要考虑完善方案。目前使用单线程模式，把interval降低到1ms，速度也能到300k，但cpu使用高了，暂时这么用)
一方关闭socket连接后，是否关闭另一方的socket连接呢？这个应该需要关闭。


### 延时关闭enet机制
    在本地socket和远程socket都已经关闭的情况下，就认为本连接chan已经可以关闭。
    这时就清理本chan的资源，除了enet_peer本身。
    然后，使用计时器，在几秒后调用enet_peer_disconnect_xxx。
    这样，让chan的清理与enet_peer关闭有段时间差，避免enet_peer重新分配新连接时chan还未清理的问题。
    这样，实际时确认enet_peer比其他关闭的都晚。
    但有一种情况，即超时时有可能enet_peer早于socket关闭了。
    

### 浏览器设置
    把per-serverxxx设置为0,好像效果好点。
    使用了adblock好像好点了。
    这说明这个东西在连接数比较多的情况，效果不怎么好啊。
    

### enet hacks
enet的enet_host_connect函数，返回的peer实例是实例池式的。
但是每次调用enet_host_connect函数，返回的peer虽然相同，但是返回的peer->connectID却不同。
修改ENET_PROTOCOL_MAXIMUM_MTU，从4096变为1271，防止出现比TOX MSG允许更大的包。
或者ENET_HOST_DEFAULT_MTU=1400，改为1271，，防止出现比TOX MSG允许更大的包。
加大peer的TIMEOUT值，分别为peer->timeoutLimit, peer->timeoutMinimum, peer->timeoutMaximum;
*** 压缩的使用
是host级别的，无法做到每个连接使用一个压缩context啊，难道说一个host下的所有连接使用同一个压缩可以吗。
开压缩导致服务器端enet连接过早关闭？？？
*** 双层模式hacks




#### bugs:
*** Error in `./tunnel': malloc(): memory corruption (fast): 0x00007fffe8043f7f ***

*** toxenet_socket_send (socket=6, address=0x64f5cc, buffers=0x64ba78, bufferCount=2, enpeer=0x64f5a8, user_data=0x6460d0)
    at ./tunnelc.cpp:43

*** 0x000000000041259a in write (data=..., this=<error reading variable: Cannot access memory at address 0x0>)

*** Program received signal SIGSEGV, Segmentation fault.
Tunnelc::onENetPeerDisconnected (this=0x6470d0, enhost=0x64c3a0, enpeer=0x64ef70)
    at ./tunnelc.cpp:279
279         chan->enet_closed = true;

*** Error receiving incoming packets: 资源暂时不可用

*** (gdb) bt
#0  0x00007ffff6644b68 in __memcpy_avx_unaligned () from /usr/lib/libc.so.6
#1  0x00007ffff721a964 in QByteArray::reallocData(unsigned int, QFlags<QArrayData::AllocationOption>) ()
   from /usr/lib/libQt5Core.so.5
#2  0x000000000041c780 in detach (this=this@entry=0x7ffff07fbbf0) at /usr/include/qt/QtCore/qbytearray.h:485
#3  data (this=this@entry=0x7ffff07fbbf0) at /usr/include/qt/QtCore/qbytearray.h:479
#4  deserialize_packet (pkt=..., address=address@entry=0x652ed8, buffer=buffer@entry=0x7ffff07fbcc0)
    at /home/gzleo/opensource/toxsh/toxtun/serializer.cpp:100


*** serializer.cpp:108 deserialize_packet - warning size not match excepted: 32767 34 16 8 10


*** Error in `./tunnel': double free or corruption (fasttop): 0x00000000021bf140 ***
