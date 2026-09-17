//
// Created by Matouš Smékal on 04.09.2026.
//

#include "NetManager.h"

#include <string.h>

#include "can.h"
#include "crc.h"
#include "stm32f1xx_hal_can.h"
#include "stm32f1xx_it.h"
#include "MaraLib/CANProtocol.h"
#include "MaraLib/LightUtils.h"

// data for the bootloader
#define NODEADDR_MAGIC  0x00C0FFEEUL
#define NODEADDR_ADDR   ((uint32_t*)(SRAM_BASE + 0x1004))
#define MAGIC_VAL  0x36051bf3UL
#define MAGIC_ADDR ((uint32_t*)(SRAM_BASE + 0x1000))
#define BOOT_NODE_SHIFT 8U

// Takedowns bit mask
#define TAKEDOWNS_BIT_MASK 0b11111111

__attribute__((section(".boot_ipc"), used))
static volatile uint32_t g_boot_magic    __attribute__((aligned(4)));
__attribute__((section(".boot_ipc"), used))
static volatile uint32_t g_boot_nodeaddr __attribute__((aligned(4)));

// our own address assigned by the master controller, 0 falls back to "localhost"
uint8_t ourAddr = 0;

HAL_StatusTypeDef NetManager_SendCanFrame(uint16_t stdId, uint8_t *payload, uint8_t length) {
    CAN_TxHeaderTypeDef header = {
        .StdId = stdId,
        .IDE = CAN_ID_STD,
        .RTR = CAN_RTR_DATA,
        .DLC = length,
        .TransmitGlobalTime = DISABLE,
    };

    uint32_t mailbox;

    return HAL_CAN_AddTxMessage(&hcan, &header, payload, &mailbox) != HAL_OK;
}

void NetManager_SendCanByte(uint8_t subMsgID, uint8_t state) {
    //pack the stdID
    uint16_t stdID = ((uint16_t) (CAN_PROTOCOL_SLAVE_OUT & 0x07) << 8) | CAN_MASTER_ADDR;

    uint8_t data[3] = {ourAddr, subMsgID, state};

    if (NetManager_SendCanFrame(stdID, data, 3) != HAL_OK) {
        HardFault_Handler();
    }
}

static uint8_t currentMessageStdID;
static uint8_t currentPayload[8];

//todo implement a lock/queue system
void NetManager_AddMessage(const uint8_t messageId, uint8_t payload[8]) {
    currentMessageStdID = messageId;
    memcpy(currentPayload, payload, 8);
}

void NetManager_ProcessMessage() {
    // current_payload[0] is the sub message id
    switch (currentPayload[0]) {
        case CAN_PROTOCOL_GET_LIGHTS:
            NetManager_SendCanByte(CAN_PROTOCOL_GET_LIGHTS, LightUtils_GetLights());
            break;
        case CAN_PROTOCOL_SET_LIGHTS:
            uint8_t status = currentPayload[1];
            //last bit
            bool lightsActive = status >> 7 & 1;
            //6th bit
            bool nightMode = status >> 6 & 1;

            LightUtils_SetLights(lightsActive);
            LightUtils_SetNightMode(nightMode);
            break;
        case CAN_PROTOCOL_SET_BRIGHTNESS:
            // 0-100%
            uint8_t brightness = currentPayload[1];

            LightUtils_SetBrightness(brightness);

            break;
        case CAN_PROTOCOL_SET_TAKEDOWNS:
            if ((currentPayload[1] & TAKEDOWNS_BIT_MASK) != 0) {
                LightUtils_SetTakedowns(currentPayload[2] != 0);
            }
            break;
        case CAN_PROTOCOL_ENTER_BOOT:
            *MAGIC_ADDR = MAGIC_VAL;
            *NODEADDR_ADDR = ((uint32_t)NODEADDR_MAGIC << BOOT_NODE_SHIFT) | (uint32_t)ourAddr;
            __DSB();
            NVIC_SystemReset();
    }
}

void NetManager_DelayBeforeDHCPRequest() {
    //combine the first UID w and the current tick (for retries) for a pseudo random delay across all slaves
    uint32_t x = HAL_GetUIDw0() ^ HAL_GetTick();

    x ^= x >> 16;
    x *= 0x45D9F3B;
    x ^= x >> 16;

    // 100-4999 ms delay
    HAL_Delay(100 + (x % 4900));
}

void calculateOurMAC(uint8_t *p_buf) {
    uint32_t uid_words[3];
    memcpy(uid_words, (const void *) UID_BASE, sizeof(uid_words));

    uint32_t crc_lo = HAL_CRC_Calculate(&hcrc, uid_words, 3);

    //Add a distinguishing salt word for the second pass to get
    //an independent 32 bits out of the (32-bit-only) peripheral
    uint32_t salted[4];
    memcpy(salted, uid_words, sizeof(uid_words));
    salted[3] = 0xA5A5A5A5U;
    uint32_t crc_hi = HAL_CRC_Calculate(&hcrc, salted, 4);

    uint64_t crc = ((uint64_t) crc_hi << 32) | crc_lo;

    for (uint8_t i = 0; i < 7; i++) {
        p_buf[i] = (crc >> (i * 8)) & 0xFF;
    }
}

void NetManager_SendDHCPRequest() {
    uint16_t stdID = ((uint16_t) (CAN_PROTOCOL_DHCP & 0x07) << 8) | CAN_MASTER_ADDR;

    uint8_t ourMAC[8];
    calculateOurMAC(ourMAC);

    uint8_t payload[8] = {0};

    // first byte in the payload is the DHCP message type
    payload[0] = CAN_PROTOCOL_DHCP_DISCOVER;

    // add 6 bytes of our MAC (dismiss the last two) to the payload
    memcpy(payload + 1, ourMAC, 6);

    NetManager_SendCanFrame(stdID, payload, 7);
}

void NetManager_ProcessDHCPMessage() {
    // we only care about addr offers
    if (currentPayload[0] != CAN_PROTOCOL_DHCP_OFFER) return;

    uint8_t ourMAC[8];
    calculateOurMAC(ourMAC);

    // payload[2-7] target MAC (6B long)
    if (memcmp(currentPayload + 2, ourMAC, 6) == 0) {
        ourAddr = currentPayload[1];

        // store the address for the bootloader in case we need to reboot into it
        *NODEADDR_ADDR = ((uint32_t)NODEADDR_MAGIC << BOOT_NODE_SHIFT) | (uint32_t)ourAddr;
    }
}

uint8_t NetManager_GetCurrentAddr() {
    return ourAddr;
}
