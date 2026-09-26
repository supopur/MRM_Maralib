//
// Created by Matouš Smékal on 04.09.2026.
//

#ifndef MAJAK_NETMANAGER_H
#define MAJAK_NETMANAGER_H
#include <stdint.h>

#include "stm32f1xx_hal.h"

HAL_StatusTypeDef NetManager_SendCanFrame(uint16_t stdId, uint8_t *payload, uint8_t length);

///@brief Helper function for sending Slave Out messages
void NetManager_SendCanByte(uint8_t subMsgID, uint8_t state);

///@brief Helper function for sending arbitrary multi-byte Slave Out messages
HAL_StatusTypeDef NetManager_SendSlaveOut(uint8_t subMsgID, uint8_t *payload, uint8_t length);

void NetManager_AddMessage(uint8_t message_id, uint8_t payload[8]);

void NetManager_ProcessMessage();

void NetManager_DelayBeforeDHCPRequest();

void NetManager_SendDHCPRequest();

void NetManager_ProcessDHCPMessage();

uint8_t NetManager_GetCurrentAddr();

#endif //MAJAK_NETMANAGER_H