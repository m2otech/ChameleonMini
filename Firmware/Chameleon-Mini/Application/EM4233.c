/*
 * EM4233.c
 *
 *  Created on: 01-03-2017
 *      Author: Phillip Nash
 *      Modified by rickventura for texas 15693 tag-it STANDARD
 *      Modified by ceres-c to finish things up
 *      Modified by m2otech for EM4233
 *  TODO:
 *    - Selected mode has to be impemented altogether - ceres-c
 *    - Focus is on EM4233SLIC at the moment => Extend to support EM4233 (2k version)
 */

#include "EM4233.h"
#include "../Codec/ISO15693.h"
#include "../Memory.h"
#include "Crypto1.h"
#include "../Random.h"
#include "ISO15693-A.h"

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

//UID = E0 16 .. .. .. .. .. ..
//0xE0 = iso15693
//0x16 = em microelectronic
ISO15693UidType Uid = {0x42, 0x60, 0x8E, 0x09, 0x01, 0x28, 0x16, 0xE0};
// TODO Do not hardcode UID...

void EM4233AppInit(void)
{
    State = STATE_READY;
}

void EM4233AppReset(void)
{
    State = STATE_READY;
}


void EM4233AppTask(void)
{

}

void EM4233AppTick(void)
{

}

uint16_t EM4233AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    if (FrameBytes >= ISO15693_MIN_FRAME_SIZE) {
        if(ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE)) {
            // At this point, we have a valid ISO15693 frame
            uint8_t Command = FrameBuf[1];
            uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;

            // UID is defined above (TODO do not hardcode it)
            //uint8_t Uid[ActiveConfiguration.UidSize];
            //EM4233GetUid(Uid);

            switch(State) {
                case STATE_READY:
                if (Command == ISO15693_CMD_INVENTORY) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_INVENTORY_DSFID;
                    ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], Uid);
                    ResponseByteCount = 10;

                } else if (Command == ISO15693_CMD_STAY_QUIET) {
                    if (ISO15693Addressed(FrameBuf) && ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], Uid))
                        State = STATE_QUIET;

                } else if (Command == ISO15693_CMD_READ_SINGLE) {
		                uint8_t *FramePtr;
                    uint8_t PageAddress;
                    if (ISO15693Addressed(FrameBuf)) {
                        if (ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], Uid)) /* read is addressed to us */
                            /* pick block 2 + 8 (UID Length) */
			                      PageAddress = FrameBuf[ISO15693_REQ_ADDR_PARAM + 0x08];
                        else /* we are not the addressee of the read command */
                            break;
                    } else /* request is not addressed */
			              PageAddress = FrameBuf[ISO15693_REQ_ADDR_PARAM];

          					if (PageAddress >= EM4233_NUMBER_OF_SECTORS) { /* the reader is requesting a sector out of bound */
          						  FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
          						  FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; /* real TiTag standard reply with this error */
          						  ResponseByteCount = 2;
          						  break;
          					}

                    if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set */
                        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                        /* UID is stored in blocks 8 and 9 which are blocked */
                        FrameBuf[1] = ( PageAddress == 8 || PageAddress == 9) ? 0x02 : 0x00; /* block security status: when request has the option flag set */
  			                FramePtr = FrameBuf + 2;
                        ResponseByteCount = 6;
                    } else { /* request with option flag not set*/
                        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* Flags */
  			                FramePtr = FrameBuf + 1;
                        ResponseByteCount = 5;
                    }
  		              MemoryReadBlock(FramePtr, PageAddress * TITAGIT_BYTES_PER_PAGE, TITAGIT_BYTES_PER_PAGE);
                }


            	else if (Command == ISO15693_CMD_WRITE_SINGLE) {
					           uint8_t* Dataptr;
					uint8_t PageAddress;

                    if (ISO15693Addressed(FrameBuf)) {
                        if (ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], Uid)) {/* write is addressed to us */
                            /* pick block 2 + 8 (UID Lenght) */
                            PageAddress = FrameBuf[ISO15693_REQ_ADDR_PARAM + 0x08]; /*when receiving an addressed request pick block number from 10th byte in the request*/
						    /* pick block 2 + 8 (UID Lenght) + 1 (data starts here) */
                            Dataptr = &FrameBuf[ISO15693_REQ_ADDR_PARAM + 0x08 + 0x01]; /* addr of sent data to write in memory */
                        }
                        else /* we are not the addressee of the write command */
                            break;
		            } else { /* request is not addressed */
			            PageAddress = FrameBuf[ISO15693_REQ_ADDR_PARAM];
                        Dataptr = &FrameBuf[ISO15693_REQ_ADDR_PARAM + 0x01];
                    }

					MemoryWriteBlock(Dataptr, PageAddress * EM4233_BYTES_PER_PAGE, EM4233_BYTES_PER_PAGE);
					FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
					ResponseByteCount = 1;
				}
                break;
            case STATE_SELECTED:
                /* TODO: Selected has to be impemented altogether */
                break;

            case STATE_QUIET:
                if (Command == ISO15693_CMD_RESET_TO_READY) {
                    if (ISO15693Addressed(FrameBuf)) {
                        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                        ResponseByteCount = 1;
                        State = STATE_READY;
                    }
                }
                break;

            default:
                break;
            }

            if (ResponseByteCount > 0) {
                /* There is data to be sent. Append CRC */
                ISO15693AppendCRC(FrameBuf, ResponseByteCount);
                ResponseByteCount += ISO15693_CRC16_SIZE;
            }

            return ResponseByteCount;

        } else { // Invalid CRC
            return ISO15693_APP_NO_RESPONSE;
        }
    } else { // Min frame size not met
        return ISO15693_APP_NO_RESPONSE;
    }

}

void EM4233GetUid(ConfigurationUidType Uid)
{
    // TODO EM4233

    //MemoryReadBlock(&Uid[0], TITAGIT_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);

    // Reverse UID after reading it
    //EM4233FlipUid(Uid);
}

void EM4233SetUid(ConfigurationUidType Uid)
{
    // TODO EM4233
    // Reverse UID before writing it
    //EM4233FlipUid(Uid);

    //MemoryWriteBlock(Uid, TITAGIT_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void EM4233FlipUid(ConfigurationUidType Uid)
{
    uint8_t UidTemp[ActiveConfiguration.UidSize];
    int i;
    for (i = 0; i < ActiveConfiguration.UidSize; i++)
        UidTemp[i] = Uid[i];
    for (i = 0; i < ActiveConfiguration.UidSize; i++)
        Uid[i] = UidTemp[ActiveConfiguration.UidSize - i - 1];
}
