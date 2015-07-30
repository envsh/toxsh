
Requirement:
PyQt5.5


TODO 压缩，提高传输效率
稳定性，传输一半掉线后的处理。
还是要使用类似udp模拟tcp功能。 OK1+
还是要多注意使用Qt.QueuedConnection
关闭序列有待改进。分别为客户端主动关闭，服务器端主动关闭，或者有可能同时发送FIN1关闭序列。
关闭序列需要继续改进，现在偶尔出现无法关闭的连接问题。
关闭序列采用服务端主动关闭机制，降低复杂度。
还需要改进rudp的发送拥塞控制，不要太快了。 OK
优化传输速度，现在的等待响应再发送下个窗口，比较慢。--已优化，速度能到100k/s了。 OK
原始数据使用base64编码，提高网络利用率。--OK 已完成
优化程序执行速度，目前一个连接，达到100k/s时，srv端和cli端CPU占用都90%+了。
chano重复的问题。OK，已采用双向chano机制。
超时没收到回包的情况，要重传。 OK
控制序列包类型超时重传。OK-，实现关闭序列包超时重传。
使用文件的方式...
内部状态采集与管理接口服务。。。OK
连接超时、连接失败的改进。。。
关闭包重传机制支持。OK
传输内容长度检验。。。   (原来是因为nginx upstream的问题，默认使用了http/1.0) OK
http访问时有下载数据下载不完全的问题。 (同上) OK



对比测试：


 +------------------------------------------+----------------+---------------+-------------+-----------+
 |                                          |    toxtun      | shadowsocks   |    ssh      |   size    |
 +------------------------------------------+----------------+---------------+-------------+-----------+
 | https://twitter.com/abcdefg              |     16s        |     2.1s      |    2.1s     |   103k    |
 +------------------------------------------+----------------+---------------+-------------+-----------+
 |                                          |                |               |             |           |
 +------------------------------------------+----------------+---------------+-------------+-----------+
 |                                          |                |               |             |           |
 +------------------------------------------+----------------+---------------+-------------+-----------+


传输速度相关因素分析：
do_iterate的调用步骤。应该以默认频率可能是比较好的。
发送缓存大小。不能太大，防止网络拥塞。
发送窗口大小。（该值应该大于发送缓存一个数量级，因为这是发送到网络上的数据，只是在等待确认）
每次发送的包大小。（应该尽量接近一条消息的大小，即1371字节）
ACK确认包回复频率。

这几个node还是比较快的在200ms左右，最后一个不稳定，时快时慢的。
144.76.60.215 	
195.154.109.148
178.62.250.138 	
128.199.78.247  --bot
128.199.78.247 33445 34922396155AA49CE6845A2FE34A73208F6FCD6190D981B1DBBC816326F26C6C


192.210.149.121
205.185.116.116 	
23.226.230.47



toxcore几个常量记录：
#define crypto_box_curve25519xsalsa20poly1305_ZEROBYTES 32U
#define crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES 16U
#define crypto_box_curve25519xsalsa20poly1305_MACBYTES \    
    (crypto_box_curve25519xsalsa20poly1305_ZEROBYTES - \
     crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES)

#define crypto_box_MACBYTES crypto_box_curve25519xsalsa20poly1305_MACBYTES
crypto_box_MACBYTES=16
#define CRYPTO_DATA_PACKET_MIN_SIZE (1 + sizeof(uint16_t) + (sizeof(uint32_t) + sizeof(uint32_t)) + crypto_box_MACBYTES)
CRYPTO_DATA_PACKET_MIN_SIZE=(3 + 8 + 16) = 27
#define MAX_CRYPTO_PACKET_SIZE 1400
#define MAX_CRYPTO_DATA_SIZE (MAX_CRYPTO_PACKET_SIZE - CRYPTO_DATA_PACKET_MIN_SIZE)
MAX_CRYPTO_DATA_SIZE = (1400 - 27) = 1373
#define MAX_FILE_DATA_SIZE (MAX_CRYPTO_DATA_SIZE - 2)
MAX_FILE_DATA_SIZE=1371
也就是说文件流每次发送都是1371字节。
这个值和一个message消息的大小一样。



