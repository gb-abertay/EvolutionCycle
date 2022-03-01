/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#if !defined (DECODE_WHEELTORQUE_H)
#define DECODE_WHEELTORQUE_H

#define WT_TIME_QUANTIZATION (2048) // Timer ticks per second

#include "PowerDecoder.h"

void DecodeWheelTorque_Init(double dRecordInterval_, double dTimeBasedPeriod, double dReSyncInterval_, PowerRecordReceiver powerRecordReceiverPtr_);
void DecodeWheelTorque_End(void);

void DecodeWheelTorque_Message(double dTime_, unsigned char aucByte_[]);
void DecodeWheelTorque_Resync(double dTime_, unsigned char aucByte_[]);
void DecodeWheelTorque(double dTime_, unsigned char aucByte_[]);
#endif