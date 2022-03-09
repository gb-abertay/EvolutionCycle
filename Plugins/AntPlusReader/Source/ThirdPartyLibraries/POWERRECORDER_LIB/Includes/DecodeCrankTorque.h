/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#if !defined (DECODE_CRANKTORQUE_H)
#define DECODE_CRANKTORQUE_H

#define CT_TIME_QUANTIZATION (2048) // Timer ticks per second

#include "PowerDecoder.h"

void DecodeCrankTorque_Init(double dRecordInterval_, double dTimeBase_, double dReSyncInterval_, PowerRecordReceiver powerRecordReceiverPtr_);
void DecodeCrankTorque_End(void);

void DecodeCrankTorque_Message(double dTime_, unsigned char messagePayload_[]);
void DecodeCrankTorque_Resync(double dTime_, unsigned char messagePayload_[]);
void DecodeCrankTorque(double dTime_, unsigned char messagePayload_[]);

void RecordCrankTorque(void);
void RecordCrankTorque_Resync(double dLastRecordTime);

#endif