/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#if !defined (RECORD_OUTPUT_H)
#define RECORD_OUTPUT_H

#include "string.h"
#include "stdlib.h"
#include "math.h"

#include "PowerDecoder.h"

void ResamplerOutput_Init(BPSAMPLER *pstDecoder_, unsigned short usRecordInterval_, double dRecordInterval_, unsigned short usTimeBase_);

void RecordOutput(PowerRecordReceiver powerRecordReceiver_, BPSAMPLER *pstDecoder_);
void RecordOutput_FillGap(PowerRecordReceiver powerRecordReceiver_, BPSAMPLER *pstDecoder_);

#endif