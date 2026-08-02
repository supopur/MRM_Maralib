/**
 * @file    CanUtils.c
 * @brief   Simple universal CAN transmit/receive library for STM32F103C8T6 (HAL).
 */

#include "CanUtils.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static UartUtilsHandle *debugUart = NULL;
void CanUtils_SetDebugUart(UartUtilsHandle *uartHandle)
{
    debugUart = uartHandle;
}

/**
 * Apply an accept-all (mask = 0x00000000) 32-bit mask filter on bank 0,
 * FIFO 0.  This is called by CanUtils_Init(); CanUtils_SetFilter() lets
 * the user narrow it down afterwards.
 */
static HAL_StatusTypeDef _ApplyAcceptAll(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef f = {0};

    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterIdHigh         = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation     = ENABLE;
    f.SlaveStartFilterBank = 14; /* Only one CAN on F103 – value irrelevant */

    return HAL_CAN_ConfigFilter(hcan, &f);
}

/* -------------------------------------------------------------------------
 * Public API implementation
 * ---------------------------------------------------------------------- */

HAL_StatusTypeDef CanUtils_Init(CanUtilsHandle *handle, CAN_HandleTypeDef *hcan)
{
    if (handle == NULL || hcan == NULL) {
        if (debugUart) UartUtils_PrintLn(debugUart, "CAN Init: NULL handle/hcan");
        return HAL_ERROR;
    }

    handle->hcan = hcan;
    handle->rxCallback = NULL;

    if (debugUart) UartUtils_PrintLn(debugUart, "CAN Init: Applying accept-all filter...");

    HAL_StatusTypeDef status = _ApplyAcceptAll(hcan);
    if (status != HAL_OK) {
        if (debugUart) UartUtils_Printf(debugUart, "CAN Init: Filter config failed (status=%d)\r\n", status);
        return status;
    }
    if (debugUart) UartUtils_PrintLn(debugUart, "CAN Init: Filter OK, starting CAN...");

    status = HAL_CAN_Start(hcan);
    if (status == HAL_OK) {
        if (debugUart) UartUtils_PrintLn(debugUart, "CAN Init: Started successfully.");
    } else {
        if (debugUart) UartUtils_Printf(debugUart, "CAN Init: Start failed (status=%d)\r\n", status);
    }
    return status;
}

HAL_StatusTypeDef CanUtils_SetFilter(CanUtilsHandle *handle,
                                     uint32_t filterId,
                                     uint32_t mask,
                                     CanIdType idType)
{
    if (handle == NULL || handle->hcan == NULL) {
        if (debugUart) UartUtils_PrintLn(debugUart, "CAN SetFilter: NULL handle or hcan");
        return HAL_ERROR;
    }

    if (debugUart) {
        UartUtils_Printf(debugUart, "CAN SetFilter: id=0x%08lX, mask=0x%08lX, type=%s\r\n",
                         filterId, mask, (idType == CAN_ID_EXTENDED) ? "EXT" : "STD");
    }

    CAN_FilterTypeDef f = {0};
    f.FilterBank           = 0;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterActivation     = ENABLE;
    f.SlaveStartFilterBank = 14;

    if (idType == CAN_ID_EXTENDED) {
        f.FilterIdHigh     = (uint16_t)((filterId >> 13) & 0xFFFF);
        f.FilterIdLow      = (uint16_t)(((filterId & 0x1FFF) << 3) | CAN_ID_EXT);
        f.FilterMaskIdHigh = (uint16_t)((mask >> 13) & 0xFFFF);
        f.FilterMaskIdLow  = (uint16_t)(((mask & 0x1FFF) << 3) | CAN_ID_EXT);
    } else {
        f.FilterIdHigh     = (uint16_t)((filterId & 0x7FF) << 5);
        f.FilterIdLow      = 0x0000;
        f.FilterMaskIdHigh = (uint16_t)((mask & 0x7FF) << 5);
        f.FilterMaskIdLow  = 0x0000;
    }

    HAL_StatusTypeDef status = HAL_CAN_ConfigFilter(handle->hcan, &f);
    if (debugUart) {
        if (status == HAL_OK) {
            UartUtils_PrintLn(debugUart, "CAN SetFilter: OK");
        } else {
            UartUtils_Printf(debugUart, "CAN SetFilter: FAILED (status=%d)\r\n", status);
        }
    }
    return status;
}

HAL_StatusTypeDef CanUtils_Send(CanUtilsHandle *handle,
                                const CanFrame *frame,
                                uint32_t timeoutMs)
{
    if (handle == NULL || handle->hcan == NULL || frame == NULL) {
        if (debugUart) UartUtils_PrintLn(debugUart, "CAN Send: NULL parameter");
        return HAL_ERROR;
    }
    if (frame->dlc > 8) {
        if (debugUart) UartUtils_PrintLn(debugUart, "CAN Send: DLC > 8");
        return HAL_ERROR;
    }

    uint32_t freeLevel = HAL_CAN_GetTxMailboxesFreeLevel(handle->hcan);
    if (debugUart) {
        UartUtils_Printf(debugUart, "CAN Send: Mailboxes free=%lu, timeout=%lu ms\r\n", freeLevel, timeoutMs);
    }

    if (timeoutMs == 0) {
        if (freeLevel == 0) {
            if (debugUart) UartUtils_PrintLn(debugUart, "CAN Send: No free mailbox (timeout=0)");
            return HAL_TIMEOUT;
        }
    } else {
        uint32_t deadline = HAL_GetTick() + timeoutMs;
        while (HAL_CAN_GetTxMailboxesFreeLevel(handle->hcan) == 0) {
            if (HAL_GetTick() >= deadline) {
                if (debugUart) UartUtils_PrintLn(debugUart, "CAN Send: Timeout waiting for mailbox");
                return HAL_TIMEOUT;
            }
        }
    }

    CAN_TxHeaderTypeDef txHeader = {0};
    uint32_t txMailbox;

    if (frame->idType == CAN_ID_EXTENDED) {
        txHeader.ExtId = frame->id & 0x1FFFFFFF;
        txHeader.IDE   = CAN_ID_EXT;
    } else {
        txHeader.StdId = frame->id & 0x7FF;
        txHeader.IDE   = CAN_ID_STD;
    }
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = frame->dlc;
    txHeader.TransmitGlobalTime = DISABLE;

    if (debugUart) {
        UartUtils_Printf(debugUart, "CAN Send: Header ID=0x%08lX, DLC=%d, IDE=%d\r\n",
                         (frame->idType == CAN_ID_EXTENDED) ? txHeader.ExtId : txHeader.StdId,
                         txHeader.DLC, txHeader.IDE);
    }

    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(handle->hcan, &txHeader,
                                                    (uint8_t *)frame->data, &txMailbox);
    if (debugUart) {
        if (status == HAL_OK) {
            UartUtils_Printf(debugUart, "CAN Send: OK, mailbox=%lu\r\n", txMailbox);
        } else {
            UartUtils_Printf(debugUart, "CAN Send: FAILED (status=%d)\r\n", status);
        }
    }
    return status;
}

void CanUtils_RegisterRxCallback(CanUtilsHandle *handle, CanUtils_RxCallback callback)
{
    if (handle != NULL) {
        handle->rxCallback = callback;
    }
}

void CanUtils_Poll(CanUtilsHandle *handle)
{
    if (handle == NULL || handle->hcan == NULL) {
        return;
    }

    CAN_RxHeaderTypeDef rxHeader;
    CanFrame frame;

    uint32_t pending = HAL_CAN_GetRxFifoFillLevel(handle->hcan, CAN_RX_FIFO0);
    if (pending > 0 && debugUart) {
        UartUtils_Printf(debugUart, "CAN Poll: %lu messages pending\r\n", pending);
    }

    while (HAL_CAN_GetRxFifoFillLevel(handle->hcan, CAN_RX_FIFO0) > 0) {

        if (HAL_CAN_GetRxMessage(handle->hcan, CAN_RX_FIFO0,
                                 &rxHeader, frame.data) != HAL_OK) {
            if (debugUart) UartUtils_PrintLn(debugUart, "CAN Poll: GetRxMessage failed");
            break;
                                 }

        if (rxHeader.IDE == CAN_ID_EXT) {
            frame.id     = rxHeader.ExtId;
            frame.idType = CAN_ID_EXTENDED;
        } else {
            frame.id     = rxHeader.StdId;
            frame.idType = CAN_ID_STANDARD;
        }
        frame.dlc = (uint8_t)rxHeader.DLC;

        if (debugUart) {
            UartUtils_Printf(debugUart, "CAN Poll: RX ID=0x%08lX, DLC=%d, Data: ",
                             frame.id, frame.dlc);
            for (int i = 0; i < frame.dlc; i++) {
                UartUtils_Printf(debugUart, "%02X ", frame.data[i]);
            }
            UartUtils_PrintLn(debugUart, "");
        }

        if (handle->rxCallback != NULL) {
            handle->rxCallback(&frame);
        }
    }
}

uint32_t CanUtils_RxPending(const CanUtilsHandle *handle)
{
    if (handle == NULL || handle->hcan == NULL) {
        return 0;
    }
    return HAL_CAN_GetRxFifoFillLevel(handle->hcan, CAN_RX_FIFO0);
}

void CanUtils_PrintStatus(CAN_HandleTypeDef *hcan)
{
    if (debugUart == NULL || hcan == NULL) return;

    uint32_t error = HAL_CAN_GetError(hcan);
    uint32_t state = HAL_CAN_GetState(hcan);
    uint32_t esr   = hcan->Instance->ESR;
    uint32_t tsr   = hcan->Instance->TSR;

    uint32_t tec = (esr & CAN_ESR_TEC) >> 24;   // Transmit Error Counter
    uint32_t rec = (esr & CAN_ESR_REC) >> 16;   // Receive Error Counter
    uint32_t lec = (esr & CAN_ESR_LEC) >> 4;    // Last Error Code

    UartUtils_Printf(debugUart, "CAN Status: State=%lu, Error=0x%08lX\r\n", state, error);
    UartUtils_Printf(debugUart, "  ESR=0x%08lX  TEC=%lu  REC=%lu  LEC=%lu\r\n", esr, tec, rec, lec);
    UartUtils_Printf(debugUart, "  TSR=0x%08lX  TXOK0=%d  TXOK1=%d  TXOK2=%d\r\n", tsr,
                     (tsr & CAN_TSR_TXOK0) ? 1 : 0,
                     (tsr & CAN_TSR_TXOK1) ? 1 : 0,
                     (tsr & CAN_TSR_TXOK2) ? 1 : 0);
    UartUtils_Printf(debugUart, "  Mailbox pending: RQCP0=%d RQCP1=%d RQCP2=%d\r\n",
                     (tsr & CAN_TSR_RQCP0) ? 1 : 0,
                     (tsr & CAN_TSR_RQCP1) ? 1 : 0,
                     (tsr & CAN_TSR_RQCP2) ? 1 : 0);

    if (esr & CAN_ESR_BOFF)  UartUtils_PrintLn(debugUart, "  *** Bus-Off ***");
    if (esr & CAN_ESR_EPVF)  UartUtils_PrintLn(debugUart, "  *** Error Passive ***");
    if (esr & CAN_ESR_EWGF)  UartUtils_PrintLn(debugUart, "  *** Error Warning ***");

    // Decode LEC
    const char *lec_msg[] = {
        "No error",
        "Stuff error",
        "Form error",
        "Acknowledgment error",
        "Bit recessive error",
        "Bit dominant error",
        "CRC error",
        "Set by software"
    };
    if (lec <= 7) {
        UartUtils_Printf(debugUart, "  LEC: %s\r\n", lec_msg[lec]);
    }
}

void CanUtils_PrintError(CAN_HandleTypeDef *hcan)
{
    if (debugUart == NULL || hcan == NULL) return;
    uint32_t error = HAL_CAN_GetError(hcan);
    if (error != HAL_CAN_ERROR_NONE) {
        UartUtils_Printf(debugUart, "CAN Error: 0x%08lX\r\n", error);
    }
    // Also print the ESR quickly
    uint32_t esr = hcan->Instance->ESR;
    if (esr & (CAN_ESR_BOFF | CAN_ESR_EPVF | CAN_ESR_EWGF)) {
        UartUtils_Printf(debugUart, "ESR flags: BOFF=%d EPVF=%d EWGF=%d\r\n",
                         (esr & CAN_ESR_BOFF) ? 1 : 0,
                         (esr & CAN_ESR_EPVF) ? 1 : 0,
                         (esr & CAN_ESR_EWGF) ? 1 : 0);
    }
}