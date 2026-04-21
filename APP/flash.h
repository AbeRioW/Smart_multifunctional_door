#ifndef __FLASH_H__
#define __FLASH_H__

#include "main.h"

// STM32F103 128KB FLASH, 1KB per page, total 128 pages
#define FLASH_PAGE_SIZE      0x400  // 1KB per page
#define FLASH_TOTAL_PAGES    128    // 128 pages for 128KB FLASH
#define FLASH_4TH_LAST_PAGE  (FLASH_TOTAL_PAGES - 4)  // 倒数第4页
#define FLASH_3RD_LAST_PAGE  (FLASH_TOTAL_PAGES - 3)  // 倒数第3页
#define FLASH_2ND_LAST_PAGE  (FLASH_TOTAL_PAGES - 2)  // 倒数第2页（存储NFC类型）
#define FLASH_PIN_ADDR       (0x08000000 + FLASH_4TH_LAST_PAGE * FLASH_PAGE_SIZE)
#define FLASH_NFC_IDS_ADDR   (0x08000000 + FLASH_3RD_LAST_PAGE * FLASH_PAGE_SIZE)
#define FLASH_NFC_TYPES_ADDR (0x08000000 + FLASH_2ND_LAST_PAGE * FLASH_PAGE_SIZE)

#define DEFAULT_PIN          0 // 存储为数字0，但读取时转换为"0000"字符串
#define MAX_NFC_IDS          10 // 最多存储10个NFC卡片ID

// NFC卡片类型
#define NFC_TYPE_DOOR        1 // 开门卡
#define NFC_TYPE_PERMISSION  2 // 权限卡

typedef struct {
    uint32_t id;
    uint8_t type;
} NFC_CardInfo;

void Flash_Init(void);
void Flash_ReadPIN(uint8_t* pin);
void Flash_WritePIN(uint8_t* pin);
uint8_t Flash_ReadNFCIDs(NFC_CardInfo* cards);
uint8_t Flash_AddNFCID(uint32_t id, uint8_t type);
uint8_t Flash_RemoveNFCID(uint32_t id);

#endif /* __FLASH_H__ */
