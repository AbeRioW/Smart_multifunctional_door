#ifndef __AS608_H__
#define __AS608_H__

#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/* AS608命令码 */
#define AS608_CMD_HEAD          0xEF01
#define AS608_CMD_ADDRESS       0xFFFFFFFF

#define AS608_CMD_GEN_IMG       0x01
#define AS608_CMD_IMG_2TZ       0x02
#define AS608_CMD_MATCH         0x03
#define AS608_CMD_SEARCH        0x04
#define AS608_CMD_REG_MODEL     0x05
#define AS608_CMD_STORE         0x06
#define AS608_CMD_LOAD          0x07
#define AS608_CMD_UP_CHAR       0x08
#define AS608_CMD_DOWN_CHAR     0x09
#define AS608_CMD_UP_IMG        0x0A
#define AS608_CMD_DOWN_IMG      0x0B
#define AS608_CMD_DEL_CHAR      0x0C
#define AS608_CMD_EMPTY         0x0D
#define AS608_CMD_SET_SYS       0x0E
#define AS608_CMD_READ_SYS      0x0F
#define AS608_CMD_SET_PWD       0x12
#define AS608_CMD_VFY_PWD       0x13
#define AS608_CMD_GET_RANDOM    0x14
#define AS608_CMD_SET_CHIP_ADDR 0x15
#define AS608_CMD_READ_INFO     0x16
#define AS608_CMD_PORT_CONTROL  0x17
#define AS608_CMD_WRITE_NOTEPAD 0x18
#define AS608_CMD_READ_NOTEPAD  0x19
#define AS608_CMD_BURN_CODE     0x1A
#define AS608_CMD_HIGH_SPEED    0x1B
#define AS608_CMD_HANDSHAKE     0x40

/* 确认码 */
#define AS608_ACK_OK            0x00
#define AS608_ACK_RECEIVE_ERR   0x01
#define AS608_ACK_NO_FINGER     0x02
#define AS608_ACK_GET_IMG_FAIL  0x03
#define AS608_ACK_IMG_TOO_DRY   0x04
#define AS608_ACK_IMG_TOO_WET   0x05
#define AS608_ACK_IMG_TOO_NOISY 0x06
#define AS608_ACK_FEATURE_FAIL  0x07
#define AS608_ACK_NO_MATCH      0x08
#define AS608_ACK_NO_FOUND      0x09
#define AS608_ACK_MERGE_FAIL    0x0A
#define AS608_ACK_OVER_RANGE    0x0B
#define AS608_ACK_READ_ERR      0x0C
#define AS608_ACK_UPLOAD_FAIL   0x0D
#define AS608_ACK_RECEIVE_DATA  0x0E
#define AS608_ACK_UPLOAD_IMG    0x0F
#define AS608_ACK_DEL_FAIL      0x10
#define AS608_ACK_CLEAR_FAIL    0x11
#define AS608_ACK_NO_PASS       0x13
#define AS608_ACK_INVALID_IMG   0x15
#define AS608_ACK_WRITE_FLASH   0x18
#define AS608_ACK_NO_DEFINITION 0x19
#define AS608_ACK_INVALID_REG   0x1A
#define AS608_ACK_WRONG_NOTEPAD 0x1B
#define AS608_ACK_FAIL_COMM     0x1C
#define AS608_ACK_NO_FINGER2    0x23
#define AS608_ACK_SECOND_IMG    0x24
#define AS608_ACK_NO_SAME       0x25
#define AS608_ACK_BAD_PACKAGE   0x26

/* 缓冲区编号 */
#define AS608_BUFFER_CHAR1      0x01
#define AS608_BUFFER_CHAR2      0x02

/* 最大指纹容量 */
#define AS608_MAX_FINGER_NUM    300

/* 数据包大小 */
#define AS608_DATA_PACKET_SIZE  128

/* 超时时间 */
#define AS608_TIMEOUT_MS        5000

/* 结构体定义 */
typedef struct {
    uint16_t head;
    uint32_t address;
    uint8_t  identifier;
    uint16_t length;
    uint8_t  data[256];
    uint16_t checksum;
} AS608_PacketTypeDef;

typedef struct {
    uint16_t status;
    uint16_t model;
    uint32_t capacity;
    uint32_t secure_level;
    uint32_t device_addr;
    uint16_t data_packet_size;
    uint16_t baud_rate;
} AS608_InfoTypeDef;

/* 中断接收相关定义 */
#define USART3_MAX_RECV_LEN 256
#define USART3_MAX_SEND_LEN 256

extern uint32_t AS608_Address;
extern uint8_t USART3_RX_BUF[USART3_MAX_RECV_LEN];
extern volatile uint16_t USART3_RX_STA;

/* 函数声明 */
void AS608_Init(void);
uint8_t AS608_VerifyPassword(uint32_t password);
uint8_t AS608_GetImage(void);
uint8_t AS608_GenChar(uint8_t bufferID);
uint8_t AS608_Match(uint16_t *score);
uint8_t AS608_Search(uint8_t bufferID, uint16_t startPage, uint16_t pageNum, uint16_t *pageID, uint16_t *score);
uint8_t AS608_RegModel(void);
uint8_t AS608_StoreChar(uint8_t bufferID, uint16_t pageID);
uint8_t AS608_LoadChar(uint8_t bufferID, uint16_t pageID);
uint8_t AS608_DeleteChar(uint16_t pageID, uint16_t count);
uint8_t AS608_Empty(void);
uint8_t AS608_ReadSysPara(AS608_InfoTypeDef *info);
uint8_t AS608_HandShake(uint32_t *addr);

uint8_t AS608_Enroll(uint16_t pageID);
uint8_t AS608_VerifyFinger(uint16_t *pageID, uint16_t *score);

const char* AS608_GetErrorString(uint8_t errorCode);
void AS608_USART3_Init(uint32_t bound);
void AS608_USART3_IRQHandler(void);

#endif /* __AS608_H__ */
