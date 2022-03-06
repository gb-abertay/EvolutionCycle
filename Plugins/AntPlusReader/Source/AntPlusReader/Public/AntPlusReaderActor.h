// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "types.h"
#include "dsi_framer_ant.hpp"
#include "dsi_thread.h"
#include "dsi_serial_generic.hpp"

#define CHANNEL_TYPE_MASTER   (0)
#define CHANNEL_TYPE_SLAVE		(1)
#define CHANNEL_TYPE_INVALID	(2)

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AntPlusReaderActor.generated.h"

UCLASS()
class ANTPLUSREADER_API AAntPlusReaderActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAntPlusReaderActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "UMG")
        void Quit();

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int AveragePower;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int AverageCadence;

    bool SetChannelID(int DevID, int DevType, int TransType, DSISerialGeneric* pclSO, DSIFramerANT* pclMO);

    void ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_, int DeviceType);

private:
    


    //Receiver for the power records from the power decoder
    static void RecordReceiver(double dLastRecordTime_, double dTotalRotation_, double dTotalEnergy_, float fAverageCadence_, float fAveragePower_);

    //Receiver for Torque Effectiveness/Pedal Smoothness data
    void TePsReceiver(double dRxTime_, float fLeftTorqEff_, float fRightTorqEff_, float fLeftOrCPedSmth_, float fRightPedSmth_);

    //Receiver for Power Balance data
    void PowerBalanceReceiver(double dRxTime_, float fPowerBalance_, bool bPowerBalanceRightPedalIndicator_);

    USHORT DeviceNumber;
    UCHAR DeviceType;
    UCHAR TransmissionType;
    int type;

    BOOL bBroadcasting;
    BOOL bPowerDecoderInitialized;
    UCHAR ucChannelType;
    USHORT usAntDeviceNumber;
    DOUBLE dRecordInterval;
    DOUBLE dTimeBase;
    DOUBLE dReSyncInterval;
    DSISerialGeneric* pclSerialObject;
    DSIFramerANT* pclMessageObject;
    time_t previousRxTime;
    UCHAR ucPowerOnlyUpdateEventCount;
    DOUBLE dRxTimeTePs;

    BOOL bDisplay;

    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];

    unsigned long ulNewEventTime;
    unsigned short usPreviousEventTime;

};

//==========================================
// Thread to stop game hanging when

class WaitForMessagesTask2 : public FNonAbandonableTask
{
public:

    WaitForMessagesTask2(AAntPlusReaderActor* APRActor, DSIFramerANT* pclMsgObj, int DevType);
    ~WaitForMessagesTask2();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(WaitForMessagesTask2, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

private:
    AAntPlusReaderActor* AntPlusReaderActor;
    DSIFramerANT* pclMessageObject;
    int DeviceType;
};