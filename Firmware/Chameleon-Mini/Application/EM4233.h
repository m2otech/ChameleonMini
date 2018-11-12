/*
 * MifareClassic.h
 *
 *  Created on: 01.03.2017
 *      Author: skuser
 *  modified by rickventura
 *  modified by m2otech
 */

#ifndef EM4233_H_
#define EM4233_H_

#include "Application.h"
#include "ISO15693-A.h"

#define EM4233_STD_UID_SIZE        ISO15693_GENERIC_UID_SIZE
#define EM4233_STD_MEM_SIZE        128
#define EM4233_BYTES_PER_PAGE      4
#define EM4233_NUMBER_OF_SECTORS   ( EM4233_STD_MEM_SIZE / EM4233_BYTES_PER_PAGE )

void EM4233AppInit(void);
void EM4233AppReset(void);
void EM4233AppTask(void);
void EM4233AppTick(void);
uint16_t EM4233AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);
void EM4233GetUid(ConfigurationUidType Uid);
void EM4233SetUid(ConfigurationUidType Uid);
void EM4233FlipUid(ConfigurationUidType Uid);

#endif /* VICINITY_H_ */
