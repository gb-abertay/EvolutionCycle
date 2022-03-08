/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#include "PowerDecoder.h"

void DecodePowerOnly_Init(double dRecordInterval_, double dTimeBase_, double dReSyncInterval_, PowerRecordReceiver powerRecordReceiverPtr_);
void DecodePowerOnly_End(void);

#define PO_TIME_QUANTIZATION (2048) // this is arbitrary, not defined by ANT+

void DecodePowerOnly_Message(double dTime_, unsigned char messagePayload_[]);
void DecodePowerOnly_SetTimeBase(double dTimeBase_);

void DecodePowerOnly_Resync(double dTime_, unsigned char messagePayload_[]);
void DecodePowerOnly(double dTime, unsigned char messagePayload_[]);
