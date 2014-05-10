#ifndef _XSHDEFS_H_
#define _XSHDEFS_H_

#define RELAY_SERVER_ADDR "81.4.106.67"
#define RELAY_SERVER_PORT 7890

#define STUN_SERVER_ADDR "66.228.45.110"
#define STUN_SERVER_PORT 3478


#define STUN_CLIENT_ADDR "0.0.0.0"

// for regsrv
#define STUN_CLIENT_PORT 37766
// for xshsrv
#define STUN_CLIENT_PORT_ADD1 (STUN_CLIENT_PORT + 1)
// for xshcli
#define STUN_CLIENT_PORT_ADD2 (STUN_CLIENT_PORT + 2)

#endif /* _XSHDEFS_H_ */
