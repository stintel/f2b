#ifndef F2B_CMSG_H_
#define F2B_CMSG_H_

#define DATA_LEN_MAX 1476 /* 1500 - (16 bytes of cmsg header + 8 bytes of udp) */
#define DATA_ARGS_MAX 6  /* number of args in data */
#define F2B_PROTO_VER 1

#define CMSG_FLAG_NEED_REPLY 0x01
#define CMSG_FLAG_AUTH_PASS  0x02

/**
 * @struct f2b_cmsg_t
 * @brief f2b control message
 *
 * Use sendmsg/recvmsg and iovec structs to pack/unpack
 */
typedef struct f2b_cmsg_t {
  char magic[3];   /**< magic string "F2B" */
  uint8_t version; /**< protocol version */
  /* 4 bytes */
  uint8_t type;    /**< command type, cast from enum f2b_cmd_type */
  uint8_t flags;   /**< CMSG_FLAG_* */
  uint16_t size;   /**< payload length */
  /* 8 bytes */
  char pass[8];
  /* 16 bytes */
  /* end of header */
  char data[DATA_LEN_MAX];      /**< set of "\n"-terminated strings */
  /* end of data */
} f2b_cmsg_t;

void f2b_cmsg_convert_args(f2b_cmsg_t *msg);
int  f2b_cmsg_extract_args(const f2b_cmsg_t *msg, const char **argv);

#endif /* F2B_CMSG_H_ */
