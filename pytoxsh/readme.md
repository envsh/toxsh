
TODO 压缩，提高传输效率
稳定性，传输一半掉线后的处理
还是要使用类似udp模拟tcp功能。 OK1
还是要多注意使用Qt.QueuedConnection
关闭序列有待改进。分别为客户端主动关闭，服务器端主动关闭，或者有可能同时发送FIN1关闭序列。
还需要改进rudp的发送拥塞控制，不要太快了。
优化传输速度，现在的等待响应再发送下个窗口，比较慢。

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



这几个node还是比较快的在200ms左右，最后一个不稳定，时快时慢的。
144.76.60.215 	
195.154.109.148
178.62.250.138 	


几个常量记录：
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



