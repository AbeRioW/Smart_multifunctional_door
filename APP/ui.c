#include "ui.h"
#include "oled.h"
#include "RC522.h"
#include "AS608.h"
#include "main.h"
#include <string.h>

// 全局变量
static UI_State_t currentState = UI_STATE_IDLE;
static uint8_t selectedType = 0;  // 选择的卡片类型
static uint8_t displayPage = 0;    // 显示页码
static uint8_t selectedMode = 0;   // 选择的解锁模式
static uint8_t password[4];        // 输入的密码
static uint8_t passwordIndex = 0;  // 密码输入位置
static uint8_t cardDetectedFlag = 0; // Door Card检测标志：避免持续检测同一卡片

// 后台管理相关变量
static Admin_Menu_t adminMenuIndex = 0;  // 当前选中的菜单项
static uint8_t newPassword[4];           // 新密码
static uint8_t newPasswordIndex = 0;     // 新密码输入位置
static uint8_t confirmPassword[4];       // 确认密码
static uint8_t confirmPasswordIndex = 0; // 确认密码输入位置
static uint16_t selectedFingerID = 0;    // 选中的指纹ID
static uint8_t adminCardDetectedFlag = 0; // Perm Card检测标志

// 按键映射为数字
// KEY1-3 -> 1-3
// KEY5-7 -> 4-6
// KEY9-11 -> 7-9
// KEY14 -> 0
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
        case 14: return 0;
        default: return 0xFF; // 无效
    }
}

// 显示空闲状态界面
static void DisplayIdleInternal(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Smart Door", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"KEY8:Reg NFC", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"KEY12:List NFC", 8, 1);
    OLED_Refresh();
}

// 显示主界面
void UI_DisplayIdle(void)
{
    DisplayIdleInternal();
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

// 显示解锁模式选择界面
static void DisplaySelectMode(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Select Mode:", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"1.Password", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"2.Fingerprint", 8, 1);
    
    // 显示当前选择
    if(selectedMode == 1)
    {
        OLED_ShowString(104, 16, (uint8_t*)"<-", 8, 1);
    }
    else if(selectedMode == 2)
    {
        OLED_ShowString(104, 32, (uint8_t*)"<-", 8, 1);
    }
    
    OLED_Refresh();
}

// 显示密码输入界面
static void DisplayPasswordInput(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Enter Password:", 8, 1);
    
    // 显示已输入的密码
    for(uint8_t i = 0; i < 4; i++)
    {
        if(i < passwordIndex)
        {
            OLED_ShowNum(i * 16, 16, password[i], 1, 8, 1);
        }
        else
        {
            OLED_ShowString(i * 16, 16, (uint8_t*)"_", 8, 1);
        }
    }
    
    OLED_ShowString(0, 32, (uint8_t*)"KEY16:Confirm", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 显示指纹扫描界面
static void DisplayFingerprintScan(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Scan Fingerprint", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"Place finger...", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 控制继电器：ON-3秒后自动OFF
void UI_TriggerRelay(void)
{
    // 拉高继电器
    HAL_GPIO_WritePin(LAY_GPIO_Port, LAY_Pin, GPIO_PIN_SET);
    
    // 显示yes
    OLED_Clear();
    OLED_ShowString(0, 16, (uint8_t*)"     yes     ", 8, 1);
    OLED_Refresh();
    
    // 等待3秒
    HAL_Delay(3000);
    
    // 拉低继电器
    HAL_GPIO_WritePin(LAY_GPIO_Port, LAY_Pin, GPIO_PIN_RESET);
    
    // 重置密码输入状态
    passwordIndex = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        password[i] = 0;
    }
    
    // 重新进入选择模式
    UI_EnterSelectMode();
    
    // 设置卡片检测标志，防止卡片仍然在天线范围内时立即再次触发
    cardDetectedFlag = 1;
}

// 显示Door Card yes界面
static void DisplayDoorCardYes(void)
{
    // 触发继电器
    UI_TriggerRelay();
}

// 显示后台管理菜单界面
static void DisplayAdminMenu(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Admin Menu:", 8, 1);
    OLED_ShowString(0, 8, (uint8_t*)"1.Modify PIN", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"2.Enroll FP", 8, 1);
    OLED_ShowString(0, 24, (uint8_t*)"3.Delete FP", 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"4.Del NFC", 8, 1);
    OLED_ShowString(0, 40, (uint8_t*)"5.Exit", 8, 1);

    // 显示当前选择
    switch(adminMenuIndex)
    {
        case ADMIN_MENU_MODIFY_PIN:
            OLED_ShowString(120, 8, (uint8_t*)"<", 8, 1);
            break;
        case ADMIN_MENU_ENROLL_FINGER:
            OLED_ShowString(120, 16, (uint8_t*)"<", 8, 1);
            break;
        case ADMIN_MENU_DELETE_FINGER:
            OLED_ShowString(120, 24, (uint8_t*)"<", 8, 1);
            break;
        case ADMIN_MENU_DELETE_NFC:
            OLED_ShowString(120, 32, (uint8_t*)"<", 8, 1);
            break;
        case ADMIN_MENU_EXIT:
            OLED_ShowString(120, 40, (uint8_t*)"<", 8, 1);
            break;
        default:
            break;
    }

    OLED_Refresh();
}

// 显示修改密码界面
static void DisplayModifyPin(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Modify PIN:", 8, 1);
    
    if(newPasswordIndex < 4)
    {
        OLED_ShowString(0, 16, (uint8_t*)"New PIN:", 8, 1);
        for(uint8_t i = 0; i < 4; i++)
        {
            if(i < newPasswordIndex)
                OLED_ShowNum(i * 16 + 64, 16, newPassword[i], 1, 8, 1);
            else
                OLED_ShowString(i * 16 + 64, 16, (uint8_t*)"_", 8, 1);
        }
    }
    else
    {
        OLED_ShowString(0, 16, (uint8_t*)"Confirm:", 8, 1);
        for(uint8_t i = 0; i < 4; i++)
        {
            if(i < confirmPasswordIndex)
                OLED_ShowNum(i * 16 + 64, 16, confirmPassword[i], 1, 8, 1);
            else
                OLED_ShowString(i * 16 + 64, 16, (uint8_t*)"_", 8, 1);
        }
    }
    
    OLED_ShowString(0, 40, (uint8_t*)"KEY16:OK KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 显示录入指纹界面
static void DisplayEnrollFinger(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Enroll Fingerprint", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"ID:", 8, 1);
    OLED_ShowNum(24, 16, selectedFingerID, 3, 8, 1);
    OLED_ShowString(0, 32, (uint8_t*)"Place finger...", 8, 1);
    OLED_ShowString(0, 48, (uint8_t*)"KEY16:Start", 8, 1);
    OLED_ShowString(0, 56, (uint8_t*)"KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 显示删除指纹界面
static void DisplayDeleteFinger(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Delete Fingerprint", 8, 1);
    OLED_ShowString(0, 16, (uint8_t*)"ID:", 8, 1);
    OLED_ShowNum(24, 16, selectedFingerID, 3, 8, 1);
    OLED_ShowString(0, 40, (uint8_t*)"KEY16:Delete", 8, 1);
    OLED_ShowString(0, 56, (uint8_t*)"KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 显示删除NFC界面
static void DisplayDeleteNFC(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Delete NFC Card", 8, 1);
    
    NFC_CardInfo cards[MAX_NFC_IDS];
    uint8_t count = Flash_ReadNFCIDs(cards);
    
    if(count == 0)
    {
        OLED_ShowString(0, 24, (uint8_t*)"No cards!", 8, 1);
    }
    else
    {
        uint8_t startIdx = (displayPage * 3);
        for(uint8_t i = 0; i < 3 && (startIdx + i) < count; i++)
        {
            uint8_t y = 24 + i * 12;
            uint8_t idx = startIdx + i;
            
            OLED_ShowNum(0, y, idx + 1, 1, 8, 1);
            OLED_ShowString(8, y, (uint8_t*)":", 8, 1);
            
            if(cards[idx].type == NFC_TYPE_DOOR)
                OLED_ShowString(16, y, (uint8_t*)"D", 8, 1);
            else
                OLED_ShowString(16, y, (uint8_t*)"P", 8, 1);
            
            OLED_ShowHex(28, y, (uint8_t)(cards[idx].id >> 24), 2, 8, 1);
            OLED_ShowHex(44, y, (uint8_t)(cards[idx].id >> 16), 2, 8, 1);
            OLED_ShowHex(60, y, (uint8_t)(cards[idx].id >> 8), 2, 8, 1);
            OLED_ShowHex(76, y, (uint8_t)(cards[idx].id), 2, 8, 1);
        }
        
        OLED_ShowString(0, 56, (uint8_t*)"1:Up 2:Dn 3:Del", 8, 1);
    }
    
    OLED_ShowString(0, 64, (uint8_t*)"KEY8:Back", 8, 1);
    OLED_Refresh();
}

// 进入后台管理界面
void UI_EnterAdminMode(void)
{
    currentState = UI_STATE_ADMIN_MENU;
    adminMenuIndex = 0;
    adminCardDetectedFlag = 1; // 设置标志，防止重复检测
    DisplayAdminMenu();
}

// 检查是否有两个NFC卡已注册
uint8_t UI_CheckTwoCards(void)
{
    NFC_CardInfo cards[MAX_NFC_IDS];
    uint8_t count = Flash_ReadNFCIDs(cards);
    
    // 检查是否至少有开门卡和权限卡各一张
    uint8_t hasDoor = 0, hasPermission = 0;
    for(uint8_t i = 0; i < count; i++)
    {
        if(cards[i].type == NFC_TYPE_DOOR) hasDoor = 1;
        if(cards[i].type == NFC_TYPE_PERMISSION) hasPermission = 1;
    }
    
    return (hasDoor && hasPermission) ? 1 : 0;
}

// 调试显示Flash中存储的卡片信息
void UI_DebugShowCards(void)
{
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"Flash Cards:", 8, 1);
    
    NFC_CardInfo cards[MAX_NFC_IDS];
    uint8_t count = Flash_ReadNFCIDs(cards);
    
    if(count == 0)
    {
        OLED_ShowString(0, 16, (uint8_t*)"No cards!", 8, 1);
    }
    else
    {
        for(uint8_t i = 0; i < count && i < 3; i++)
        {
            uint8_t y = 16 + i * 16;
            OLED_ShowNum(0, y, i+1, 1, 8, 1);
            OLED_ShowString(8, y, (uint8_t*)":", 8, 1);
            OLED_ShowNum(16, y, cards[i].type, 1, 8, 1);
        }
    }
    OLED_Refresh();
    HAL_Delay(3000);
}

// 进入解锁模式选择页面
void UI_EnterSelectMode(void)
{
    currentState = UI_STATE_SELECT_MODE;
    selectedMode = 0;
    DisplaySelectMode();
}

// 初始化UI（不显示界面）
void UI_Init(void)
{
    currentState = UI_STATE_IDLE;
    selectedType = 0;
    displayPage = 0;
    selectedMode = 0;
    passwordIndex = 0;
    cardDetectedFlag = 0;
    for(uint8_t i = 0; i < 4; i++)
    {
        password[i] = 0;
    }
}

// 初始化UI并显示默认界面
void UI_InitWithDisplay(void)
{
    UI_Init();
    DisplayIdleInternal();
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
                DisplayIdleInternal();
            }
            break;
        }
        
        case UI_STATE_REGISTER_SCAN:
        {
            if(key == 8) // 返回键
            {
                currentState = UI_STATE_IDLE;
                DisplayIdleInternal();
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
                DisplayIdleInternal();
            }
            break;
        }
        
        case UI_STATE_SELECT_MODE:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num == 1) // KEY1直接进入密码输入
            {
                currentState = UI_STATE_PASSWORD_INPUT;
                passwordIndex = 0;
                DisplayPasswordInput();
            }
            else if(num == 2) // KEY2直接进入指纹扫描
            {
                currentState = UI_STATE_FINGERPRINT_SCAN;
                DisplayFingerprintScan();
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_IDLE;
                DisplayIdleInternal();
            }
            break;
        }
        
        case UI_STATE_PASSWORD_INPUT:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num != 0xFF && passwordIndex < 4) // 数字键
            {
                password[passwordIndex] = num;
                passwordIndex++;
                DisplayPasswordInput();
            }
            else if(key == 16 && passwordIndex == 4) // 确定键
            {
                // 从Flash读取存储的密码
                uint8_t storedPin[4];
                Flash_ReadPIN(storedPin);
                
                // 检查输入的密码是否与存储的密码匹配
                // 输入的password是数字(0-9)，存储的是ASCII码('0'=0x30)
                uint8_t correct = 1;
                for(uint8_t i = 0; i < 4; i++)
                {
                    if(password[i] != (storedPin[i] - '0'))
                    {
                        correct = 0;
                        break;
                    }
                }
                
                if(correct)
                {
                    currentState = UI_STATE_DOOR_CARD_YES;
                    DisplayDoorCardYes();
                }
                else
                {
                    // 密码错误，重新输入
                    passwordIndex = 0;
                    DisplayPasswordInput();
                }
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_SELECT_MODE;
                DisplaySelectMode();
            }
            break;
        }
        
        case UI_STATE_FINGERPRINT_SCAN:
        {
            if(key == 8) // 返回键
            {
                currentState = UI_STATE_SELECT_MODE;
                DisplaySelectMode();
            }
            break;
        }
        
        case UI_STATE_DOOR_CARD_YES:
        {
            // 正在显示yes，等待继电器完成，忽略按键
            break;
        }
        
        case UI_STATE_ADMIN_MENU:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num >= 1 && num <= 5)
            {
                adminMenuIndex = num - 1;
                DisplayAdminMenu();
            }
            else if(key == 16) // 确定键，进入选中的菜单
            {
                switch(adminMenuIndex)
                {
                    case ADMIN_MENU_MODIFY_PIN:
                        currentState = UI_STATE_ADMIN_MODIFY_PIN;
                        newPasswordIndex = 0;
                        confirmPasswordIndex = 0;
                        DisplayModifyPin();
                        break;
                    case ADMIN_MENU_ENROLL_FINGER:
                        currentState = UI_STATE_ADMIN_ENROLL_FINGER;
                        selectedFingerID = 0;
                        DisplayEnrollFinger();
                        break;
                    case ADMIN_MENU_DELETE_FINGER:
                        currentState = UI_STATE_ADMIN_DELETE_FINGER;
                        selectedFingerID = 0;
                        DisplayDeleteFinger();
                        break;
                    case ADMIN_MENU_DELETE_NFC:
                        currentState = UI_STATE_ADMIN_DELETE_NFC;
                        displayPage = 0;
                        DisplayDeleteNFC();
                        break;
                    case ADMIN_MENU_EXIT:
                        currentState = UI_STATE_SELECT_MODE;
                        adminCardDetectedFlag = 0;
                        selectedMode = 0;
                        DisplaySelectMode();
                        break;
                    default:
                        break;
                }
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_SELECT_MODE;
                adminCardDetectedFlag = 0;
                selectedMode = 0;
                DisplaySelectMode();
            }
            break;
        }
        
        case UI_STATE_ADMIN_MODIFY_PIN:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num != 0xFF && newPasswordIndex < 4) // 输入新密码
            {
                newPassword[newPasswordIndex] = num;
                newPasswordIndex++;
                DisplayModifyPin();
            }
            else if(num != 0xFF && confirmPasswordIndex < 4) // 输入确认密码
            {
                confirmPassword[confirmPasswordIndex] = num;
                confirmPasswordIndex++;
                DisplayModifyPin();
            }
            else if(key == 16 && confirmPasswordIndex == 4) // 确定键
            {
                // 检查两次输入是否一致
                uint8_t match = 1;
                for(uint8_t i = 0; i < 4; i++)
                {
                    if(newPassword[i] != confirmPassword[i])
                    {
                        match = 0;
                        break;
                    }
                }
                
                OLED_Clear();
                if(match)
                {
                    // 将数字密码转换为ASCII码后保存
                    uint8_t pinAscii[4];
                    for(uint8_t i = 0; i < 4; i++)
                    {
                        pinAscii[i] = newPassword[i] + '0';
                    }
                    Flash_WritePIN(pinAscii);
                    OLED_ShowString(0, 16, (uint8_t*)"PIN Changed!", 8, 1);
                }
                else
                {
                    OLED_ShowString(0, 16, (uint8_t*)"PIN Mismatch!", 8, 1);
                }
                OLED_Refresh();
                HAL_Delay(2000);
                
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            break;
        }
        
        case UI_STATE_ADMIN_ENROLL_FINGER:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num != 0xFF && num <= 9 && selectedFingerID < 100)
            {
                selectedFingerID = selectedFingerID * 10 + num;
                if(selectedFingerID > 299) selectedFingerID = 299;
                DisplayEnrollFinger();
            }
            else if(key == 16) // 开始录入
            {
                if(selectedFingerID < AS608_MAX_FINGER_NUM)
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, (uint8_t*)"Enrolling...", 8, 1);
                    OLED_Refresh();
                    
                    uint8_t result = AS608_Enroll(selectedFingerID);
                    
                    OLED_Clear();
                    if(result == AS608_ACK_OK)
                    {
                        OLED_ShowString(0, 16, (uint8_t*)"Enroll Success!", 8, 1);
                    }
                    else
                    {
                        OLED_ShowString(0, 16, (uint8_t*)"Enroll Failed!", 8, 1);
                    }
                    OLED_Refresh();
                    HAL_Delay(2000);
                }
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            break;
        }
        
        case UI_STATE_ADMIN_DELETE_FINGER:
        {
            uint8_t num = KeyToNumber(key);
            
            if(num != 0xFF && num <= 9 && selectedFingerID < 100)
            {
                selectedFingerID = selectedFingerID * 10 + num;
                if(selectedFingerID > 299) selectedFingerID = 299;
                DisplayDeleteFinger();
            }
            else if(key == 16) // 删除指纹
            {
                if(selectedFingerID < AS608_MAX_FINGER_NUM)
                {
                    OLED_Clear();
                    OLED_ShowString(0, 0, (uint8_t*)"Deleting...", 8, 1);
                    OLED_Refresh();
                    
                    uint8_t result = AS608_DeleteChar(selectedFingerID, 1);
                    
                    OLED_Clear();
                    if(result == AS608_ACK_OK)
                    {
                        OLED_ShowString(0, 16, (uint8_t*)"Delete Success!", 8, 1);
                    }
                    else
                    {
                        OLED_ShowString(0, 16, (uint8_t*)"Delete Failed!", 8, 1);
                    }
                    OLED_Refresh();
                    HAL_Delay(2000);
                }
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
            }
            break;
        }
        
        case UI_STATE_ADMIN_DELETE_NFC:
        {
            NFC_CardInfo cards[MAX_NFC_IDS];
            uint8_t count = Flash_ReadNFCIDs(cards);
            uint8_t totalPages = (count + 2) / 3;
            
            if(key == 1) // 上一页
            {
                if(displayPage > 0)
                {
                    displayPage--;
                    DisplayDeleteNFC();
                }
            }
            else if(key == 2) // 下一页
            {
                if(displayPage < totalPages - 1)
                {
                    displayPage++;
                    DisplayDeleteNFC();
                }
            }
            else if(key == 3) // 删除当前页的第一个卡片
            {
                uint8_t idx = displayPage * 3;
                if(idx < count)
                {
                    Flash_RemoveNFCID(cards[idx].id);
                    OLED_Clear();
                    OLED_ShowString(0, 16, (uint8_t*)"Card Deleted!", 8, 1);
                    OLED_Refresh();
                    HAL_Delay(1500);
                }
                DisplayDeleteNFC();
            }
            else if(key == 8) // 返回键
            {
                currentState = UI_STATE_ADMIN_MENU;
                DisplayAdminMenu();
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
    // 如果是显示yes状态，直接返回，不做任何处理
    if(currentState == UI_STATE_DOOR_CARD_YES)
    {
        return;
    }
    
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
                DisplayIdleInternal();
            }
        }
    }
    else if(currentState == UI_STATE_FINGERPRINT_SCAN)
    {
        // 指纹扫描处理
        uint16_t pageID, score;
        uint8_t result = AS608_VerifyFinger(&pageID, &score);
        
        if(result == AS608_ACK_OK)
        {
            // 指纹识别成功
            currentState = UI_STATE_DOOR_CARD_YES;
            DisplayDoorCardYes();
        }
    }
    else if(currentState == UI_STATE_SELECT_MODE || currentState == UI_STATE_PASSWORD_INPUT)
    {
        // 在选择模式或密码输入模式下检测Door Card
        uint8_t status;
        uint8_t cardType[2];
        
        status = PCD_Request(PICC_REQIDL, cardType);
        if(status == PCD_OK)
        {
            // 卡片在天线附近
            if(!cardDetectedFlag)
            {
                // 第一次检测到卡片
                uint8_t cardId[4];
                status = PCD_Anticoll(cardId);
                if(status == PCD_OK)
                {
                    // 组合4字节ID为32位整数
                    uint32_t id = ((uint32_t)cardId[0] << 24) | 
                                  ((uint32_t)cardId[1] << 16) | 
                                  ((uint32_t)cardId[2] << 8) | 
                                  (uint32_t)cardId[3];
                    
                    // 检查是否是Door Card
                    NFC_CardInfo cards[MAX_NFC_IDS];
                    uint8_t count = Flash_ReadNFCIDs(cards);
                    
                    for(uint8_t i = 0; i < count; i++)
                    {
                        if(cards[i].id == id && cards[i].type == NFC_TYPE_DOOR)
                        {
                            // 找到Door Card
                            cardDetectedFlag = 1; // 设置标志，避免持续检测
                            currentState = UI_STATE_DOOR_CARD_YES;
                            DisplayDoorCardYes();
                            return; // 立即返回，避免继续执行后续代码
                        }
                        else if(cards[i].id == id && cards[i].type == NFC_TYPE_PERMISSION)
                        {
                            // 找到Perm Card，进入后台管理
                            cardDetectedFlag = 1;
                            UI_EnterAdminMode();
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            // 卡片不在天线附近，清除标志
            cardDetectedFlag = 0;
        }
    }
    else if(currentState == UI_STATE_ADMIN_MENU)
    {
        // 后台管理模式下检测Perm Card移开
        uint8_t status;
        uint8_t cardType[2];
        
        status = PCD_Request(PICC_REQIDL, cardType);
        if(status != PCD_OK)
        {
            // Perm Card已移开
            adminCardDetectedFlag = 0;
        }
    }
}
