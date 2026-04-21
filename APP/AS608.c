#include "AS608.h"
#include "main.h"

/* 全局变量 */
uint32_t AS608_Address = AS608_CMD_ADDRESS;

/* 中断接收相关变量 */
uint8_t USART3_RX_BUF[USART3_MAX_RECV_LEN]; // 接收缓冲区
volatile uint16_t USART3_RX_STA = 0; // 接收状态标志

static void AS608_SendPacket(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart3, data, len, AS608_TIMEOUT_MS);
}

// 接收数据包
static uint8_t AS608_ReceivePacket(uint8_t *data, uint16_t *len)
{
    uint32_t start_tick = HAL_GetTick();
    
    // 重置接收状态
    USART3_RX_STA = 0;
    
    while (HAL_GetTick() - start_tick < AS608_TIMEOUT_MS)
    {
        if (USART3_RX_STA > 0) // 接收到数据
        {
            if (USART3_RX_STA >= 9)
            {
                uint16_t pkg_len = (USART3_RX_BUF[7] << 8) | USART3_RX_BUF[8];
                if (USART3_RX_STA >= pkg_len + 9)
                {
                    memcpy(data, USART3_RX_BUF, USART3_RX_STA);
                    *len = USART3_RX_STA;
                    USART3_RX_STA = 0; // 重置接收状态
                    return AS608_ACK_OK;
                }
            }
        }
        HAL_Delay(1); // 短暂延迟
    }
    USART3_RX_STA = 0; // 重置接收状态
    return 0xFF; // 超时
}

// 简化的握手实现，参考库函数版本
static uint8_t AS608_SimpleHandShake(uint32_t *addr)
{
    uint8_t tx_buf[9];
    uint32_t start_tick = HAL_GetTick();
    
    // 发送握手命令（完全按照参考代码格式）
    tx_buf[0] = 0xEF;
    tx_buf[1] = 0x01;
    tx_buf[2] = (AS608_Address >> 24) & 0xFF;
    tx_buf[3] = (AS608_Address >> 16) & 0xFF;
    tx_buf[4] = (AS608_Address >> 8) & 0xFF;
    tx_buf[5] = AS608_Address & 0xFF;
    tx_buf[6] = 0x01;
    tx_buf[7] = 0x00;
    tx_buf[8] = 0x00;
    
    // 重置接收状态
    USART3_RX_STA = 0;
    
    // 直接发送数据，不使用校验和
    HAL_UART_Transmit(&huart3, tx_buf, 9, AS608_TIMEOUT_MS);
    
    // 等待响应（参考代码使用200ms延迟）
    HAL_Delay(200);
    
    // 读取响应数据
    while (HAL_GetTick() - start_tick < AS608_TIMEOUT_MS)
    {
        if (USART3_RX_STA >= 9) // 接收到足够的数据
        {
            // 检查是否接收到完整的响应
            if (USART3_RX_BUF[0] == 0xEF && USART3_RX_BUF[1] == 0x01 && USART3_RX_BUF[6] == 0x07)
            {
                *addr = ((uint32_t)USART3_RX_BUF[2] << 24) | ((uint32_t)USART3_RX_BUF[3] << 16) | 
                        ((uint32_t)USART3_RX_BUF[4] << 8) | USART3_RX_BUF[5];
                AS608_Address = *addr;
                USART3_RX_STA = 0; // 重置接收状态
                return AS608_ACK_OK;
            }
        }
        HAL_Delay(1); // 短暂延迟
    }
    USART3_RX_STA = 0; // 重置接收状态
    return 0xFF;
}

static uint16_t AS608_CalcChecksum(uint8_t *data, uint16_t len)
{
    uint16_t sum = 0;
    for (uint16_t i = 6; i < len; i++)
    {
        sum += data[i];
    }
    return sum;
}

static uint8_t AS608_SendCommand(uint8_t cmd, uint8_t *data, uint16_t data_len, uint8_t *resp, uint16_t *resp_len)
{
    uint8_t tx_buf[256];
    uint16_t tx_len = 0;
    
    tx_buf[tx_len++] = 0xEF;
    tx_buf[tx_len++] = 0x01;
    tx_buf[tx_len++] = (AS608_Address >> 24) & 0xFF;
    tx_buf[tx_len++] = (AS608_Address >> 16) & 0xFF;
    tx_buf[tx_len++] = (AS608_Address >> 8) & 0xFF;
    tx_buf[tx_len++] = AS608_Address & 0xFF;
    tx_buf[tx_len++] = 0x01;
    tx_buf[tx_len++] = ((data_len + 3) >> 8) & 0xFF;
    tx_buf[tx_len++] = (data_len + 3) & 0xFF;
    tx_buf[tx_len++] = cmd;
    
    if (data_len > 0)
    {
        memcpy(&tx_buf[tx_len], data, data_len);
        tx_len += data_len;
    }
    
    uint16_t checksum = 0;
    for (uint16_t i = 6; i < tx_len; i++)
    {
        checksum += tx_buf[i];
    }
    tx_buf[tx_len++] = (checksum >> 8) & 0xFF;
    tx_buf[tx_len++] = checksum & 0xFF;
    
    // 发送数据
    HAL_UART_Transmit(&huart3, tx_buf, tx_len, AS608_TIMEOUT_MS);
    
    // 等待接收响应
    if (resp != NULL && resp_len != NULL)
    {
        return AS608_ReceivePacket(resp, resp_len);
    }
    
    return AS608_ACK_OK;
}

static void AS608_Reset(void)
{
    // 重置AS608模块
    HAL_Delay(100);
    // 发送握手命令前的延迟
    HAL_Delay(500);
}

void AS608_Init(void)
{
    MX_USART3_UART_Init();
    
    // 增加初始化延迟
    HAL_Delay(1000);
    
    // 重置AS608模块
    AS608_Reset();
    
    // 再次延迟确保模块就绪
    HAL_Delay(500);
}

uint8_t AS608_VerifyPassword(uint32_t password)
{
    uint8_t data[4];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = (password >> 24) & 0xFF;
    data[1] = (password >> 16) & 0xFF;
    data[2] = (password >> 8) & 0xFF;
    data[3] = password & 0xFF;
    
    if (AS608_SendCommand(AS608_CMD_VFY_PWD, data, 4, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11 && resp[9] == AS608_ACK_OK)
        {
            return AS608_ACK_OK;
        }
        return resp[9];
    }
    return 0xFF;
}

uint8_t AS608_GetImage(void)
{
    uint8_t resp[32];
    uint16_t resp_len;
    
    if (AS608_SendCommand(AS608_CMD_GEN_IMG, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_GenChar(uint8_t bufferID)
{
    uint8_t data[1];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = bufferID;
    
    if (AS608_SendCommand(AS608_CMD_IMG_2TZ, data, 1, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_Match(uint16_t *score)
{
    uint8_t resp[32];
    uint16_t resp_len;
    
    if (AS608_SendCommand(AS608_CMD_MATCH, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 13 && resp[9] == AS608_ACK_OK)
        {
            *score = (resp[10] << 8) | resp[11];
            return AS608_ACK_OK;
        }
        return resp[9];
    }
    return 0xFF;
}

uint8_t AS608_Search(uint8_t bufferID, uint16_t startPage, uint16_t pageNum, uint16_t *pageID, uint16_t *score)
{
    uint8_t data[6];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = bufferID;
    data[1] = (startPage >> 8) & 0xFF;
    data[2] = startPage & 0xFF;
    data[3] = (pageNum >> 8) & 0xFF;
    data[4] = pageNum & 0xFF;
    
    if (AS608_SendCommand(AS608_CMD_SEARCH, data, 5, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 15)
        {
            if (resp[9] == AS608_ACK_OK)
            {
                *pageID = (resp[10] << 8) | resp[11];
                *score = (resp[12] << 8) | resp[13];
            }
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_RegModel(void)
{
    uint8_t resp[32];
    uint16_t resp_len;
    
    if (AS608_SendCommand(AS608_CMD_REG_MODEL, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_StoreChar(uint8_t bufferID, uint16_t pageID)
{
    uint8_t data[3];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = bufferID;
    data[1] = (pageID >> 8) & 0xFF;
    data[2] = pageID & 0xFF;
    
    if (AS608_SendCommand(AS608_CMD_STORE, data, 3, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_LoadChar(uint8_t bufferID, uint16_t pageID)
{
    uint8_t data[3];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = bufferID;
    data[1] = (pageID >> 8) & 0xFF;
    data[2] = pageID & 0xFF;
    
    if (AS608_SendCommand(AS608_CMD_LOAD, data, 3, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_DeleteChar(uint16_t pageID, uint16_t count)
{
    uint8_t data[4];
    uint8_t resp[32];
    uint16_t resp_len;
    
    data[0] = (pageID >> 8) & 0xFF;
    data[1] = pageID & 0xFF;
    data[2] = (count >> 8) & 0xFF;
    data[3] = count & 0xFF;
    
    if (AS608_SendCommand(AS608_CMD_DEL_CHAR, data, 4, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_Empty(void)
{
    uint8_t resp[32];
    uint16_t resp_len;
    
    if (AS608_SendCommand(AS608_CMD_EMPTY, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 11)
        {
            return resp[9];
        }
    }
    return 0xFF;
}

uint8_t AS608_ReadSysPara(AS608_InfoTypeDef *info)
{
    uint8_t resp[32];
    uint16_t resp_len;
    
    if (AS608_SendCommand(AS608_CMD_READ_SYS, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
    {
        if (resp_len >= 27 && resp[9] == AS608_ACK_OK)
        {
            info->status = (resp[10] << 8) | resp[11];
            info->model = (resp[12] << 8) | resp[13];
            info->capacity = (resp[14] << 8) | resp[15];
            info->secure_level = (resp[16] << 8) | resp[17];
            info->device_addr = ((uint32_t)resp[18] << 24) | ((uint32_t)resp[19] << 16) | ((uint32_t)resp[20] << 8) | resp[21];
            info->data_packet_size = (resp[22] << 8) | resp[23];
            info->baud_rate = (resp[24] << 8) | resp[25];
            return AS608_ACK_OK;
        }
        return resp[9];
    }
    return 0xFF;
}

uint8_t AS608_HandShake(uint32_t *addr)
{
    // 尝试多次握手，增加成功率
    for (uint8_t i = 0; i < 3; i++)
    {
        // 尝试使用简化的握手实现
        uint8_t result = AS608_SimpleHandShake(addr);
        if (result == AS608_ACK_OK)
        {
            return AS608_ACK_OK;
        }
        
        // 延迟后重试
        HAL_Delay(300);
    }
    
    // 如果简化版本失败，尝试标准命令
    for (uint8_t i = 0; i < 2; i++)
    {
        uint8_t resp[32];
        uint16_t resp_len;
        
        if (AS608_SendCommand(AS608_CMD_HANDSHAKE, NULL, 0, resp, &resp_len) == AS608_ACK_OK)
        {
            if (resp_len >= 15)
            {
                *addr = ((uint32_t)resp[10] << 24) | ((uint32_t)resp[11] << 16) | ((uint32_t)resp[12] << 8) | resp[13];
                AS608_Address = *addr;
                return AS608_ACK_OK;
            }
        }
        
        // 延迟后重试
        HAL_Delay(300);
    }
    
    return 0xFF;
}

uint8_t AS608_Enroll(uint16_t pageID)
{
    uint8_t result;
    uint16_t score;
    
    result = AS608_GetImage();
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_GenChar(AS608_BUFFER_CHAR1);
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    HAL_Delay(2000);
    
    result = AS608_GetImage();
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_GenChar(AS608_BUFFER_CHAR2);
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_RegModel();
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_StoreChar(AS608_BUFFER_CHAR1, pageID);
    return result;
}

uint8_t AS608_VerifyFinger(uint16_t *pageID, uint16_t *score)
{
    uint8_t result;
    
    result = AS608_GetImage();
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_GenChar(AS608_BUFFER_CHAR1);
    if (result != AS608_ACK_OK)
    {
        return result;
    }
    
    result = AS608_Search(AS608_BUFFER_CHAR1, 0, AS608_MAX_FINGER_NUM, pageID, score);
    return result;
}

const char* AS608_GetErrorString(uint8_t errorCode)
{
    switch (errorCode)
    {
        case AS608_ACK_OK:              return "OK";
        case AS608_ACK_RECEIVE_ERR:     return "Receive error";
        case AS608_ACK_NO_FINGER:       return "No finger";
        case AS608_ACK_GET_IMG_FAIL:    return "Get image fail";
        case AS608_ACK_IMG_TOO_DRY:     return "Image too dry";
        case AS608_ACK_IMG_TOO_WET:     return "Image too wet";
        case AS608_ACK_IMG_TOO_NOISY:   return "Image too noisy";
        case AS608_ACK_FEATURE_FAIL:    return "Feature fail";
        case AS608_ACK_NO_MATCH:        return "No match";
        case AS608_ACK_NO_FOUND:        return "No found";
        case AS608_ACK_MERGE_FAIL:      return "Merge fail";
        case AS608_ACK_OVER_RANGE:      return "Over range";
        case AS608_ACK_READ_ERR:        return "Read error";
        case AS608_ACK_UPLOAD_FAIL:     return "Upload fail";
        case AS608_ACK_RECEIVE_DATA:    return "Receive data";
        case AS608_ACK_UPLOAD_IMG:      return "Upload image";
        case AS608_ACK_DEL_FAIL:        return "Delete fail";
        case AS608_ACK_CLEAR_FAIL:      return "Clear fail";
        case AS608_ACK_NO_PASS:         return "No pass";
        case AS608_ACK_INVALID_IMG:     return "Invalid image";
        case AS608_ACK_WRITE_FLASH:     return "Write flash";
        case AS608_ACK_NO_DEFINITION:   return "No definition";
        case AS608_ACK_INVALID_REG:     return "Invalid reg";
        case AS608_ACK_WRONG_NOTEPAD:   return "Wrong notepad";
        case AS608_ACK_FAIL_COMM:       return "Fail communication";
        case AS608_ACK_NO_FINGER2:      return "No finger 2";
        case AS608_ACK_SECOND_IMG:      return "Second image";
        case AS608_ACK_NO_SAME:         return "Not same";
        case AS608_ACK_BAD_PACKAGE:     return "Bad package";
        default:                        return "Unknown error";
    }
}
