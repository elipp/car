#ifndef PROTOCOL_H
#define PROTOCOL_H

#define S_HANDSHAKE_OK (unsigned char) 0x01
#define S_PUPD (unsigned char) 0x02
#define S_PEER_LIST (unsigned char) 0x03
#define S_QUIT (unsigned char) 0x0F

#define C_HANDSHAKE (unsigned char) 0x11
#define C_KEYSTATE (unsigned char) 0x12
#define C_QUIT (unsigned char) 0x1F

#endif
