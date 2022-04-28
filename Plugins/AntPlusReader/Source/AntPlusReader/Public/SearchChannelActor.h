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
#include "SearchChannelActor.generated.h"

USTRUCT(BlueprintType)
struct FChannelID
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search")
        int DevNum;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search")
        int DevType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search")
        int TransType;

    bool operator==(const FChannelID& RHS) const noexcept
    {
        return ((DevNum == RHS.DevNum) && (DevType == RHS.DevType) && (TransType == RHS.TransType));
    }
};

UCLASS()
class ANTPLUSREADER_API ASearchChannelActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASearchChannelActor();
	~ASearchChannelActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Search")
        bool SetUpUSB();

    UFUNCTION(BlueprintCallable, Category = "Search")
        bool Search(int DevType);

    void ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_);

    UFUNCTION(BlueprintCallable, Category = "Search")
        void SaveChannelID(int DevID, int DevType, int TransType);

    UFUNCTION(BlueprintCallable, Category = "Search")
        void ClearChannelID(int DevType);

     UFUNCTION(BlueprintCallable, Category = "Search")
         void closeChannel(int DevType);

     UFUNCTION(BlueprintCallable, Category = "Search")
        bool CreateChannel(int DevID, int DevType, int TransType);

     UFUNCTION(BlueprintCallable, Category = "Search")
        void SetResistance(int resistance);

     UFUNCTION(BlueprintCallable, Category = "Search")
        void SetPower(int power);

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int SearchType;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool IsUSBConnected;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool IsSearching;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int NewConnection;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool NewDevice;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool PowerConnected;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool TrainerConnected;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        bool HeartConnected;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        TArray<FChannelID> FoundChannels;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int AveragePower;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int AverageCadence;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
        int HeartRate;

    bool GetIsSearching();
    void SetIsUSBConnected(bool b);

private:

    bool ResetChannel();
    void LoadChannelID();
    bool CreateChannel(int i);
    FString SaveNameTranslator(int DevType);
    int SaveSlotTranslator(int DevType);
    bool IsNewDevice(int DevNum, int DevType, int TransType);
    bool CheckUSBConnection();

    //Receiver for the power records from the power decoder
    static void RecordReceiver(double dLastRecordTime_, double dTotalRotation_, double dTotalEnergy_, float fAverageCadence_, float fAveragePower_);

    //Receiver for Torque Effectiveness/Pedal Smoothness data
    void TePsReceiver(double dRxTime_, float fLeftTorqEff_, float fRightTorqEff_, float fLeftOrCPedSmth_, float fRightPedSmth_);

    //Receiver for Power Balance data
    void PowerBalanceReceiver(double dRxTime_, float fPowerBalance_, bool bPowerBalanceRightPedalIndicator_);

    float TimeElapsed;
    bool firstSearch;
    BOOL bBroadcasting;
    BOOL bPowerDecoderInitialized;
    time_t previousRxTime;
    USHORT DeviceNumber[3];
    USHORT DeviceType[3];
    USHORT TransmissionType[3];
    DSISerialGeneric* pclSerialObject;
    DSIFramerANT* pclMessageObject;
    UCHAR ucPowerOnlyUpdateEventCount;
    DOUBLE dRxTimeTePs;
    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];

    unsigned long ulNewEventTime;
    unsigned short usPreviousEventTime;

};

//==========================================
// Thread to stop game hanging when

class WaitForMessagesTask : public FNonAbandonableTask
{
public:

    WaitForMessagesTask(ASearchChannelActor* SCActor, DSIFramerANT* pclMsgObj);
    ~WaitForMessagesTask();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(WaitForMessagesTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

private:
    ASearchChannelActor* SearchChannelActor;
    DSIFramerANT* pclMessageObject;
};

class ConnectToUSBTask : public FNonAbandonableTask
{
public:

    ConnectToUSBTask(ASearchChannelActor* SCActor, DSISerialGeneric* pclSerialObject);
    ~ConnectToUSBTask();

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(ConnectToUSBTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();

private:
    ASearchChannelActor* SearchChannelActor;
    DSISerialGeneric* pclSerialObject;
};