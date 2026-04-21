#include "ui.h"
#include "oled.h"
#include "RC522.h"
#include <string.h>

// 全局变量
static UI_State_t currentState = UI_STATE_IDLE;
static uint8_t selectedType = 0;  // 选择的卡片类型
static uint8_t displayPage = 0;    // 显示页码

// 按键映射为数字
// KEY1-3 -> 1-3
// KEY5-7 -> 4-6
// KEY9-11 -> 7-9
// KEY13 -> 0
static uint8_t KeyToNumber(uint8_t key)
{
    switch(key)
    {
        case 1: return 1;
        case 2: return 2;
        case 3: return 3;
        case 5: return 4;
        case 6: return 5;
        case 7: return 6;
        case 9: return 7;
        case 10: return 8;
        case 11: return 9;
        case 13: return 0;
        default: return 0xFF; // 无效
    }
}

// 显示空闲状态界面
static void DisplayIdle(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Smart Door", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"KEY8:Reg NFC", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"KEY12:List NFC", 8, 1);
    OLED_Refresh();
}

// 显示选择卡片类型界面
static void DisplaySelectType(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Select Type:", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"1:Door Card", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"2:Perm Card", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"KEY16:Confirm", 8, 1);
    
    // 显示当前选择
    if(selectedType == 1)
    {
        OLED_ShowString(104, 16, (uint8_t*)"<-", 8, 1);
    }
    else if(selectedType == 2)
    {
        OLED_ShowString(104, 32, (uint8_t*)"<-", 8, 1);
    }
    
    OLED_Refresh();
}

// 显示扫描NFC卡片界面
static void DisplayScanNFC(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Scan NFC Card", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Type:", 8, 1);
    if(selectedType == 1)
    {
        OLED_ShowString(40, 16, (uint8_t*)"Door", 8, 1);
    }
    else if(selectedType == 2)
    {
        OLED_ShowString(40, 16, (uint8_t*)"Perm", 8, 1);
    }
    OLED_ShowString(0, 32, (uint8_t*)"Place card...", 8, 1);
    OLED_Refresh();
}

// 显示卡片列表界面
static void DisplayCardList(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"NFC Card List:", 8, 1);
    
    NFC_CardInfo cards[MAX_NFC_IDS];
    uint8_t count = Flash_ReadNFCIDs(cards);
    
    if(count == 0)
    {
        OLED_ShowString(0, 16, (uint8_t*)"No cards!", 8, 1);
    }
    else
    {
        uint8_t cardsPerPage = 3;
        uint8_t totalPages = (count + cardsPerPage - 1) / cardsPerPage;
        uint8_t startIndex = displayPage * cardsPerPage;
        
        for(uint8_t i = 0; i < cardsPerPage && (startIndex + i) < count; i++)
        {
            uint8_t y = 16 + i * 16;
            uint8_t index = startIndex + i;
            
            // 显示序号
            OLED_ShowNum(0, y, index + 1, 1, 8, 1);
            OLED_ShowString(8, y, (uint8_t*)":", 8, 1);
            
            // 显示类型
            if(cards[index].type == NFC_TYPE_DOOR)
            {
                OLED_ShowString(16, y, (uint8_t*)"D:", 8, 1);
            }
            else if(cards[index].type == NFC_TYPE_PERMISSION)
            {
                OLED_ShowString(16, y, (uint8_t*)"P:", 8, 1);
            }
            else
            {
                OLED_ShowString(16, y, (uint8_t*)"?:", 8, 1);
            }
            
            // 显示ID
            OLED_ShowHex(32, y, (uint8_t)(cards[index].id >> 24), 2, 8, 1);
            OLED_ShowHex(48, y, (uint8_t)(cards[index].id >> 16), 2, 8, 1);
            OLED_ShowHex(64, y, (uint8_t)(cards[index].id >> 8), 2, 8, 1);
            OLED_ShowHex(80, y, (uint8_t)(cards[index].id), 2, 8, 1);
        }
        
        // 显示页码
        OLED_ShowString(0, 56, (uint8_t*)"Page:", 8, 1);
        OLED_ShowNum(40, 56, displayPage + 1, 1, 8, 1);
        OLED_ShowString(48, 56, (uint8_t*)"/", 8, 1);
        OLED_ShowNum(56, 56, totalPages, 1, 8, 1);
    }
    
    OLED_ShowString(88, 56, (uint8_t*)"KEY8:Exit", 8, 1);
    OLED_Refresh();
}

// 初始化UI
void UI_Init(void)
{
    currentState = UI_STATE_IDLE;
    selectedType = 0;
    displayPage = 0;
    DisplayIdle();
}

// 处理按键输入
void UI_HandleKey(uint8_t key)
{
    switch(currentState)
    {
        case UI_STATE_IDLE:
        {
            if(key == 8)
            {
                // 进入NFC注册流程
                currentState = UI_STATE_REGISTER_TYPE;
                selectedType = 0;
                DisplaySelectType();
            }
            else if(key == 12)
            {
                // 进入NFC卡片显示
                currentState = UI_STATE_DISPLAY_LIST;
                displayPage = 0;
                DisplayCardList();
            }
            break;
        }
        
        case UI_STATE_REGISTER_TYPE:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num == 1)
            {
                selectedType = 1;
                DisplaySelectType();
            }
            else if(num == 2)
            {
                selectedType = 2;
                DisplaySelectType();
            }
            else if(key == 16) // 确定键
            {
                if(selectedType == 1 || selectedType == 2)
                {
                    currentState = UI_STATE_REGISTER_SCAN;
                    DisplayScanNFC();
                }
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_IDLE;
                DisplayIdle();
            }
            break;
        }
        
        case UI_STATE_REGISTER_SCAN:
        {
            if(key == 8) // 返回键
            {
                currentState = UI_STATE_IDLE;
                DisplayIdle();
            }
            break;
        }
        
        case UI_STATE_DISPLAY_LIST:
        {
            NFC_CardInfo cards[MAX_NFC_IDS];
            uint8_t count = Flash_ReadNFCIDs(cards);
            uint8_t totalPages = (count + 2) / 3; // 每页3张
            
            if(key == 1) // 上一页
            {
                if(displayPage > 0)
                {
                    displayPage--;
                    DisplayCardList();
                }
            }
            else if(key == 3) // 下一页
            {
                if(displayPage < totalPages - 1)
                {
                    displayPage++;
                    DisplayCardList();
                }
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_IDLE;
                DisplayIdle();
            }
            break;
        }
        
        default:
            break;
    }
}

// UI主循环处理函数
void UI_Process(void)
{
    if(currentState == UI_STATE_REGISTER_SCAN)
    {
        uint8_t status;
        uint8_t cardType[2];
        uint8_t cardId[4];
        
        status = PCD_Request(PICC_REQIDL, cardType);
        if(status == PCD_OK)
        {
            status = PCD_Anticoll(cardId);
            if(status == PCD_OK)
            {
                // 组合4字节ID为32位整数
                uint32_t id = ((uint32_t)cardId[0] << 24) | 
                              ((uint32_t)cardId[1] << 16) | 
                              ((uint32_t)cardId[2] << 8) | 
                              (uint32_t)cardId[3];
                
                // 写入FLASH
                uint8_t result = Flash_AddNFCID(id, selectedType);
                
                // 显示结果
                OLED_Clear();
                if(result == 0)
                {
                    OLED_ShowString(0, 0, (uint8_t*)"Register Success!", 8, 1);
                    OLED_ShowString(0, 16, (uint8_t*)"ID:", 8, 1);
                    OLED_ShowHex(24, 16, cardId[0], 2, 8, 1);
                    OLED_ShowHex(40, 16, cardId[1], 2, 8, 1);
                    OLED_ShowHex(56, 16, cardId[2], 2, 8, 1);
                    OLED_ShowHex(72, 16, cardId[3], 2, 8, 1);
                }
                else if(result == 1)
                {
                    OLED_ShowString(0, 0, (uint8_t*)"Full!", 8, 1);
                }
                else if(result == 2)
                {
                    OLED_ShowString(0, 0, (uint8_t*)"Already Exist!", 8, 1);
                }
                else
                {
                    OLED_ShowString(0, 0, (uint8_t*)"Error!", 8, 1);
                }
                
                OLED_ShowString(0, 48, (uint8_t*)"KEY8:Back", 8, 1);
                OLED_Refresh();
                
                // 等待返回
                HAL_Delay(2000);
                currentState = UI_STATE_IDLE;
                DisplayIdle();
            }
        }
    }
}
