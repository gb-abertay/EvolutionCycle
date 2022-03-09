/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#if !defined (DECODE_CRANKTORQUE_FREQ_H)
#define DECODE_CRANKTORQUE_FREQ_H

#define CTF_TIME_QUANTIZATION (2000) // Timer ticks per second

#include "PowerDecoder.h"

void DecodeCrankTorqueFreq_Init(double dRecordInterval_, double dTimeBase_, double dReSyncInterval_, PowerRecordReceiver powerRecordReceiverPtr_);
void DecodeCrankTorqueFreq_End(void);

void DecodeCrankTorqueFreq_Message(double dTime_, unsigned char messagePayload_[]);
void DecodeCrankTorqueFreq_Calibration(double dTime_, unsigned char messagePayload_[]);

void DecodeCrankTorqueFreq_Resync(double dTime_, unsigned char messagePayload_[]);
void DecodeCrankTorqueFreq(double dTime_, unsigned char messagePayload_[]);
#endif