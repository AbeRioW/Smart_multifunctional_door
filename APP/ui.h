#ifndef __UI_H__
#define __UI_H__

#include "main.h"
#include "flash.h"

// UI状态枚举
typedef enum {
    UI_STATE_IDLE = 0,         // 空闲状态
    UI_STATE_REGISTER_TYPE,    // 选择卡片类型
    UI_STATE_REGISTER_SCAN,    // 扫描NFC卡片
    UI_STATE_DISPLAY_LIST      // 显示卡片列表
} UI_State_t;

// 初始化UI
void UI_Init(void);

// 处理按键输入
// key: 按键编号 (1-16)
void UI_HandleKey(uint8_t key);

// UI主循环处理函数
void UI_Process(void);

#endif /* __UI_H__ */
