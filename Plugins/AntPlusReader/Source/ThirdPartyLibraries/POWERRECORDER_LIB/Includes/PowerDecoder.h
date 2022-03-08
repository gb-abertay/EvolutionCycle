/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except in compliance
with this license.

Copyright (c) Dynastream Innovations Inc. 2014
All rights reserved.
*/

#if !defined (POWER_DECODER_H)
#define POWER_DECODER_H
// We will designate the transmitting device type according to the
// type of message that it sends.
#define ANT_POWERONLY 0x10
#define ANT_WHEELTORQUE 0x11
#define ANT_CRANKTORQUE 0x12
#define ANT_CRANKFREQ   0x20

// We define the Torque Efficiency / Pedal Smoothness page here
// even though it doesn't imply anything regarding the above types.
#define ANT_TEPS        0x13

// Calibration definitions
#define ANT_CALIBRATION_MESSAGE     (0x01)
#define CALIBRATION_ID_BYTE         (1)
#define ANT_CTF_CALIBRATION_ID      (0x10)
#define ANT_CTF_CAL_TYPE_BYTE       (2)
#define ANT_CTF_CAL_ZERO            (0x01)
#define ANT_CTF_CAL_SLOPE           (0x02)
#define ANT_CTF_CAL_ESN             (0x03)
#define ANT_CTF_CAL_ACK             (0xAC)
#define ANT_CTF_CAL_ZERO_MSB_BYTE   (6)
#define ANT_CTF_CAL_ZERO_LSB_BYTE   (7)

#define MAXIMUM_TIME_GAP (240.0) // This is the power-down interval for several power meters...


typedef struct _BPSAMPLER_t_
{
    unsigned char ucPedalBalance;
    unsigned char ucCadence;
    unsigned long  ulEventTime;         // Quantized time of current event
    unsigned long  ulLastRecordTime;    // Quantized time of last resample output
    unsigned short usRecordInterval;    // Quantized recording interval
    unsigned short usTimeBase;          // Quantized event interval (for time based data)
    unsigned char ucRecordGapCount;     // Number of resample outputs in message gap

    unsigned short usTorqueOffset;      // CTF specific parameter

    float  fPendingRotation;            // Amount of rotation in latest event that will count towards the pending output
    float  fGapRotation;                // Amount of rotation in message gap
    float  fAccumRotation;              // Amount of rotation to carry forwards to next message event
    double dTotalRotation;              // Total crank or wheel rotation (in rotations)

    float  fPendingEnergy;              // Amount of rotation in latest event that will count towards the pending output
    float  fGapEnergy;                  // Amount of rotation in message gap
    float  fAccumEnergy;                // Amount of rotation to carry forwards to next message event
    double dTotalEnergy;                // Total crank or wheel rotation (in rotations)

    double dLastRecordTime;             // absolute time (in seconds) of last resample output
    double dLastMessageTime;            // absolute time (in seconds) of last message

    unsigned short usLastAccumPeriod;   // Accumulated period or timestamp in last received message
    unsigned short usLastAccumTorque;   // Accumulated torque in last received message
    unsigned char ucLastEventCount;     // Message Event count in last received message
    unsigned char ucLastRotationTicks;  // Crank or Wheel rotation count in last received messsage

} BPSAMPLER;

// Power receiver signature
typedef void(*PowerRecordReceiver) (double dLastRecordTime_, double  dTotalRotation_, double dTotalEnergy_, float  fAverageCadence_, float fAveragePower_);

// Initializes the power decoder library with the record interval (s) and the power meter timebase (s) or event base (0).
void InitPowerDecoder(double dRecordInterval_, double dTimeBase_, double dReSyncInterval_, PowerRecordReceiver powerRecordReceiverPtr_);

// Pass Bike Power messages for the power decoder library to process
void DecodePowerMessage(double dRxTime_, unsigned char messagePayload_[]);

// 16 = Power Only, 17 = Wheel Torque, 18 = Crank Torque, 32 = Crank Torque Frequency, 255 = Unknown
void SetPowerMeterType(unsigned char ucPowerMeterType_);

#endif
