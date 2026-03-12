#include "DHT.h"
#include "delay_timer.h"

//************************** Low Level Layer ********************************************************//
static void DHT_DelayInit(DHT_Name* DHT) {
    DELAY_TIM_Init(DHT->Timer);
}

static void DHT_DelayUs(DHT_Name* DHT, uint16_t Time) {
    DELAY_TIM_Us(DHT->Timer, Time);
}

static void DHT_SetPinOut(DHT_Name* DHT) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT->Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}

static void DHT_SetPinIn(DHT_Name* DHT) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT->Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT->PORT, &GPIO_InitStruct);
}

static void DHT_WritePin(DHT_Name* DHT, uint8_t Value) {
    HAL_GPIO_WritePin(DHT->PORT, DHT->Pin, Value);
}

static uint8_t DHT_ReadPin(DHT_Name* DHT) {
    return HAL_GPIO_ReadPin(DHT->PORT, DHT->Pin);
}

//********************************* Middle level Layer ****************************************************//
static uint8_t DHT_Start(DHT_Name* DHT) {
    uint32_t timeout = 0;
    
    DHT_SetPinOut(DHT);  
    DHT_WritePin(DHT, 0);
    DHT_DelayUs(DHT, DHT->Type); 
    
    // B?t d?u vùng nh?y c?m th?i gian: T?T NG?T
    __disable_irq(); 
    DHT_SetPinIn(DHT); 
    
    while(DHT_ReadPin(DHT)) {
        if(++timeout > 50000) { __enable_irq(); return 0; } // L?i: B?t l?i ng?t và thoát
    }
    
    timeout = 0;
    while(!DHT_ReadPin(DHT)) {
        if(++timeout > 50000) { __enable_irq(); return 0; } 
    }
    
    timeout = 0;
    while(DHT_ReadPin(DHT)) {
        if(++timeout > 50000) { __enable_irq(); return 0; } 
    }

    return 1; // B?t tay thành công (V?n dang t?t ng?t d? chu?n b? d?c data)
}

static uint8_t DHT_Read(DHT_Name* DHT) {
    uint8_t Value = 0;
    uint32_t timeout;
    
    for(int i = 0; i < 8; i++) {
        timeout = 0;
        while(!DHT_ReadPin(DHT)) { 
            if(++timeout > 50000) break;
        }
        
        DHT_DelayUs(DHT, 40); 
        
        if(!DHT_ReadPin(DHT)) {
            Value &= ~(1<<(7-i));	
        } else {
            Value |= 1<<(7-i);
            timeout = 0;
            while(DHT_ReadPin(DHT)) { 
                if(++timeout > 50000) break;
            }
        }
    }
    return Value;
}

//************************** High Level Layer ********************************************************//
void DHT_Init(DHT_Name* DHT, uint8_t DHT_Type, TIM_HandleTypeDef* Timer, GPIO_TypeDef* DH_PORT, uint16_t DH_Pin) {
    if(DHT_Type == DHT11) {
        DHT->Type = DHT11_STARTTIME;
    } else if(DHT_Type == DHT22) {
        DHT->Type = DHT22_STARTTIME;
    }
    DHT->PORT = DH_PORT;
    DHT->Pin = DH_Pin;
    DHT->Timer = Timer;
    DHT_DelayInit(DHT);
}

uint8_t DHT_ReadTempHum(DHT_Name* DHT) {
    uint8_t Temp1, Temp2, RH1, RH2, SUM;
    uint16_t Temp, Humi;

    if (!DHT_Start(DHT)) {
        return 0; // Ð?c l?i
    }

    RH1  = DHT_Read(DHT);
    RH2  = DHT_Read(DHT);
    Temp1 = DHT_Read(DHT);
    Temp2 = DHT_Read(DHT);
    SUM  = DHT_Read(DHT);

    // XONG D? LI?U: B?T L?I NG?T NGAY
    __enable_irq(); 

    if (SUM == (uint8_t)(RH1 + RH2 + Temp1 + Temp2)) {
        Temp = (Temp1 << 8) | Temp2;
        Humi = (RH1 << 8) | RH2;

        if (DHT->Type == DHT11_STARTTIME) {
            DHT->Temp = (float)Temp1;
            DHT->Humi = (float)RH1;
        } else {
            DHT->Temp = (float)(Temp / 10.0);
            DHT->Humi = (float)(Humi / 10.0);
        }
        return 1; // Thành công
    }

    return 0; // Sai Checksum
}
