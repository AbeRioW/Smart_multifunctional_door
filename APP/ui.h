#ifndef __UI_H__
#define __UI_H__

#include "main.h"
#include "flash.h"

// UI状态枚举
typedef enum {
    UI_STATE_IDLE = 0,              // 空闲状态
    UI_STATE_REGISTER_TYPE,         // 选择卡片类型
    UI_STATE_REGISTER_SCAN,         // 扫描NFC卡片
    UI_STATE_DISPLAY_LIST,          // 显示卡片列表
    UI_STATE_SELECT_MODE,           // 选择解锁模式
    UI_STATE_PASSWORD_INPUT,        // 密码输入界面
    UI_STATE_FINGERPRINT_SCAN,      // 指纹扫描界面
    UI_STATE_DOOR_CARD_YES,         // 显示Door Card yes界面
    UI_STATE_ADMIN_MENU,            // 后台管理菜单
    UI_STATE_ADMIN_MODIFY_PIN,      // 修改密码
    UI_STATE_ADMIN_ENROLL_FINGER,   // 录入指纹
    UI_STATE_ADMIN_DELETE_FINGER,   // 删除指纹
    UI_STATE_ADMIN_DELETE_NFC,      // 删除NFC
    UI_STATE_ADMIN_CONFIRM          // 确认界面
} UI_State_t;

// 后台菜单项
typedef enum {
    ADMIN_MENU_MODIFY_PIN = 0,      // 修改密码
    ADMIN_MENU_ENROLL_FINGER,       // 录入指纹
    ADMIN_MENU_DELETE_FINGER,       // 删除指纹
    ADMIN_MENU_DELETE_NFC,          // 删除NFC
    ADMIN_MENU_EXIT,                // 回到主界面
    ADMIN_MENU_MAX
} Admin_Menu_t;

// 检查是否有两个NFC卡已注册
uint8_t UI_CheckTwoCards(void);

// 进入解锁模式选择页面
void UI_EnterSelectMode(void);

// 初始化UI（不显示界面）
void UI_Init(void);

// 初始化UI并显示默认界面
void UI_InitWithDisplay(void);

// 显示主界面
void UI_DisplayIdle(void);

// 调试显示Flash中存储的卡片信息
void UI_DebugShowCards(void);

// 控制继电器：ON-3秒后自动OFF
void UI_TriggerRelay(void);

// 处理按键输入
// key: 按键编号 (1-16)
void UI_HandleKey(uint8_t key);

// UI主循环处理函数
void UI_Process(void);

// 进入后台管理界面
void UI_EnterAdminMode(void);

#endif /* __UI_H__ */
