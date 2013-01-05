#ifndef PROTOCOL_H
#define PROTOCOL_H

#define S_HANDSHAKE_OK (unsigned char) 0xE1
#define S_PUPD (unsigned char) 0xE2
#define S_PEER_LIST (unsigned char) 0xE3
#define S_QUIT (unsigned char) 0xEF

#define C_HANDSHAKE (unsigned char) 0xF1
#define C_KEYSTATE (unsigned char) 0xF2
#define C_QUIT (unsigned char) 0xFF

#endif
