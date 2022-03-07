// Fill out your copyright notice in the Description page of Project Settings.


#include "SearchChannelActor.h"

#include "MySaveGame.h"
#include "Kismet/GameplayStatics.h"

#include "types.h"
#include "dsi_framer_ant.hpp"
#include "dsi_thread.h"
#include "dsi_serial_generic.hpp"
#include "dsi_debug.hpp"

extern "C" {
#include "PowerDecoder.h"
}

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <algorithm>

int APower;
int ACadence;

#define ENABLE_EXTENDED_MESSAGES

#define USER_BAUDRATE         (50000)  // For AT3/AP2, use 57600
#define USER_RADIOFREQ        (57) // RF Channel 57 (2457 MZ) is used for ANT+ FE Devices

#define USER_ANTCHANNEL       (0) // Slave (Controller or Display)

#define USER_NETWORK_KEY      {0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45}  //ANT+ Network Key
#define USER_NETWORK_NUM      (0)      // The network key is assigned to this network number

#define MESSAGE_TIMEOUT       (1000)

// Indexes into message recieved from ANT
#define MESSAGE_BUFFER_DATA1_INDEX ((UCHAR) 0)
#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)
#define MESSAGE_BUFFER_DATA3_INDEX ((UCHAR) 2)
#define MESSAGE_BUFFER_DATA4_INDEX ((UCHAR) 3)
#define MESSAGE_BUFFER_DATA5_INDEX ((UCHAR) 4)
#define MESSAGE_BUFFER_DATA6_INDEX ((UCHAR) 5)
#define MESSAGE_BUFFER_DATA7_INDEX ((UCHAR) 6)
#define MESSAGE_BUFFER_DATA8_INDEX ((UCHAR) 7)
#define MESSAGE_BUFFER_DATA9_INDEX ((UCHAR) 8)
#define MESSAGE_BUFFER_DATA10_INDEX ((UCHAR) 9)
#define MESSAGE_BUFFER_DATA11_INDEX ((UCHAR) 10)
#define MESSAGE_BUFFER_DATA12_INDEX ((UCHAR) 11)
#define MESSAGE_BUFFER_DATA13_INDEX ((UCHAR) 12)
#define MESSAGE_BUFFER_DATA14_INDEX ((UCHAR) 13)
#define MESSAGE_BUFFER_DATA15_INDEX ((UCHAR) 14)
#define MESSAGE_BUFFER_DATA16_INDEX ((UCHAR) 15)

// Sets default values
ASearchChannelActor::ASearchChannelActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

ASearchChannelActor::~ASearchChannelActor()
{
    if (pclSerialObject)
        pclSerialObject->Close();
}

// Called when the game starts or when spawned
void ASearchChannelActor::BeginPlay()
{
	Super::BeginPlay();
	
    // //Set Initial Values

    //Set pointers to null by default
    pclSerialObject = (DSISerialGeneric*)NULL;
    pclMessageObject = (DSIFramerANT*)NULL;

    //Other various variables
    ulNewEventTime = 0;
    usPreviousEventTime = 0;
    memset(aucTransmitBuffer, 0, ANT_STANDARD_DATA_PAYLOAD_SIZE);
    bPowerDecoderInitialized = FALSE;
    SetPowerMeterType(17);

    //USB set up values

    DeviceNumber[0] = 0;
    DeviceType[0] = 11;
    TransmissionType[0] = 0;

    DeviceNumber[1] = 0;
    DeviceType[1] = 17;
    TransmissionType[1] = 0;

    DeviceNumber[2] = 0;
    DeviceType[2] = 120;
    TransmissionType[2] = 0;

    //LoadChannelID(); //This loads the channel ID from save file, if there is no save file the value isn't touched, if there us device number is overwritten

    SearchType = -1;
    NewConnection = -1;
    NewDevice = false;
    PowerConnected = false;
    TrainerConnected = false;
    HeartConnected = false;

    UE_LOG(LogTemp, Warning, TEXT("Ant Channel Search Initialised"));

    if (SetUpUSB()) //If Ant+ USB is set up
    {
        for (int i = 0; i < 3; i++) //For each device
        {
            if (DeviceNumber[i]) //If a device id was loaded
            {
                UE_LOG(LogTemp, Warning, TEXT("Save for device found in slot %i, attempting to load"), i);

                //Code for creating channel
                CreateChannel(i);
            }
        }
    }
}

// Called every frame
void ASearchChannelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (PowerConnected)
    {
        AveragePower = APower;
        AverageCadence = ACadence;
    }
}

bool ASearchChannelActor::SetUpUSB()
{
    UE_LOG(LogTemp, Warning, TEXT("Setting up ANT+ USB..."));

    BOOL bStatus = false;

    //Creating serial Object 

    pclSerialObject = new DSISerialGeneric(); // Create Serial object and store it in pointer.
    assert(pclSerialObject);

    bStatus = pclSerialObject->Init(USER_BAUDRATE, 0); //UC Device number (0 to wildcard [default])
    assert(bStatus);

    pclMessageObject = new DSIFramerANT(pclSerialObject);
    assert(pclMessageObject);

    // Let Serial know about Framer.
    pclSerialObject->SetCallback(pclMessageObject);

    // Open Serial.
    bStatus = pclSerialObject->Open();

    if (!bStatus) //Wasn't able to connect to ANT+ USB
    {
        IsUSBConnected = false;
        UE_LOG(LogTemp, Warning, TEXT("Failed to set up ANT+ USB!"));
        return false;
    }
    else
    {
        IsUSBConnected = true;
        UE_LOG(LogTemp, Warning, TEXT("ANT+ USB set up!"));
        return true;
    }
}

bool ASearchChannelActor::Search(int DevType)
{
    if (!IsUSBConnected)
        return false;

    FoundChannels.Empty();
    IsSearching = true;
    SearchType = SaveSlotTranslator(DevType);

    (new FAutoDeleteAsyncTask<WaitForMessagesTask>(this, pclMessageObject, SearchType))->StartBackgroundTask();

    if (!ResetChannel())
    {
        IsSearching = false;
        return false;
    }

    return true;
}

bool ASearchChannelActor::GetIsSearching()
{
    return IsSearching;
}

void ASearchChannelActor::ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_, int DevType)
{
    BOOL bStatus;
    BOOL bPrintBuffer = FALSE;
    UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages

    switch (stMessage.ucMessageID)
    {
        case MESG_RESPONSE_EVENT_ID:
        {

            int channelNum = 0;
            switch (stMessage.aucData[0])
            {
            case 0:
                channelNum = 0;
                break;
            case 1:
                channelNum = 1;
                break;
            case 2:
                channelNum = 2;
                break;
            }

                switch (stMessage.aucData[1])
                {

                case MESG_NETWORK_KEY_ID: //Config Messages (Set Network Key)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error configuring network key: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }

                    UE_LOG(LogTemp, Warning, TEXT("Network key set."));
                    UE_LOG(LogTemp, Warning, TEXT("Assigning channel..."));
                    bStatus = pclMessageObject->AssignChannel(channelNum, 0, 0, MESSAGE_TIMEOUT);
                    break;
                }

                case MESG_ASSIGN_CHANNEL_ID: //Config Messages (Assign Channel)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error assigning channel: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Channel assigned"));
                    UE_LOG(LogTemp, Warning, TEXT("Setting Channel ID..."));
                    bStatus = pclMessageObject->SetChannelID(channelNum, DeviceNumber[DevType], DeviceType[DevType], TransmissionType[DevType], MESSAGE_TIMEOUT);
                    break;
                }

                case MESG_CHANNEL_ID_ID: //Config Messages (Channel ID)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error configuring Channel ID: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Channel ID set"));
                    UE_LOG(LogTemp, Warning, TEXT("Setting Radio Frequency..."));
                    bStatus = pclMessageObject->SetChannelRFFrequency(channelNum, USER_RADIOFREQ, MESSAGE_TIMEOUT);
                    break;
                }

                case MESG_CHANNEL_RADIO_FREQ_ID: //Config Messages (Channel RF Frequency)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error configuring Radio Frequency: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Radio Frequency set"));
                    UE_LOG(LogTemp, Warning, TEXT("Setting Channel Period..."));
                    bStatus = pclMessageObject->SetChannelPeriod(channelNum, 8182, MESSAGE_TIMEOUT);
                    break;
                }

                case MESG_CHANNEL_MESG_PERIOD_ID: //Config Messages (Channel Period)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error configuring Channel Period: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Channel Period set"));
                    UE_LOG(LogTemp, Warning, TEXT("Opening channel..."));
                    bBroadcasting = TRUE;
                    bStatus = pclMessageObject->OpenChannel(channelNum, MESSAGE_TIMEOUT);
                    break;
                }

                case MESG_OPEN_CHANNEL_ID: //Control Messages (Open Channel)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error opening channel: Code 0%d"), stMessage.aucData[2]);
                        bBroadcasting = FALSE;
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Channel opened"));

                    if (stMessage.aucData[0] == 1)
                    {
                        // We register the power record receiver and initialize the bike power decoders after the channel has opened
                        InitPowerDecoder(1, 0, 10, RecordReceiver); //dRecordInterval, dTimeBase, dReSyncInterval, RecordReceiver
                        bPowerDecoderInitialized = TRUE;
                        UE_LOG(LogTemp, Warning, TEXT("APR: Power record decode library initialized"));
                    }

#if defined (ENABLE_EXTENDED_MESSAGES)
                    UE_LOG(LogTemp, Warning, TEXT("Enabling extended messages..."));
                    pclMessageObject->SetLibConfig(ANT_LIB_CONFIG_MESG_OUT_INC_TIME_STAMP | ANT_LIB_CONFIG_MESG_OUT_INC_DEVICE_ID, MESSAGE_TIMEOUT);
#endif
                    break;
                }

                case MESG_ANTLIB_CONFIG_ID: //Config Messages (Lib Config)
                {
                    if (stMessage.aucData[2] == INVALID_MESSAGE)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Extended messages not supported in this ANT product"));
                        break;
                    }
                    else if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error enabling extended messages: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Extended messages enabled"));
                    break;
                }

                case MESG_UNASSIGN_CHANNEL_ID: //Config Messages (Unassign Channel)
                {
                    if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error unassigning channel: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    UE_LOG(LogTemp, Warning, TEXT("Channel unassigned"));
                    return;
                    break;
                }

                case MESG_CLOSE_CHANNEL_ID: //Control Messages (Close Channel)
                {
                    if (stMessage.aucData[2] == CHANNEL_IN_WRONG_STATE)
                    {
                        // We get here if we tried to close the channel after the search timeout (slave)
                        UE_LOG(LogTemp, Warning, TEXT("Channel is already closed"));
                        UE_LOG(LogTemp, Warning, TEXT("Unassigning channel..."));
                        bStatus = pclMessageObject->UnAssignChannel(channelNum, MESSAGE_TIMEOUT);
                        break;
                    }
                    else if (stMessage.aucData[2] != RESPONSE_NO_ERROR)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Error closing channel: Code 0%d"), stMessage.aucData[2]);
                        break;
                    }
                    // If this message was successful, wait for EVENT_CHANNEL_CLOSED to confirm channel is closed
                    break;
                }

                case MESG_REQUEST_ID: //Control Messages (Request Message)
                {
                    if (stMessage.aucData[2] == INVALID_MESSAGE)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Requested message not supported in this ANT product"));
                    }
                    break;
                }

                case MESG_EVENT_ID: //Channel Event (NOT CHANNEL RESPONSE)
                {

                    switch (stMessage.aucData[2]) //Event Code
                    {

                    case EVENT_CHANNEL_CLOSED:
                    {
                        /*The channel has been successfully closed. When the Host sends a message to close a channel, it first receives a
                        RESPONSE_NO_ERROR to indicate that the message was successfully received by ANT; however, EVENT_CHANNEL_CLOSED
                        is the actual indication of the closure of the channel. As such, the Host must use this event message rather
                        than the RESPONSE_NO_ERROR message to let a channel state machine continue.*/
                        UE_LOG(LogTemp, Warning, TEXT("Channel Closed"));
                        UE_LOG(LogTemp, Warning, TEXT("Unassigning channel..."));
                        bStatus = pclMessageObject->UnAssignChannel(channelNum, MESSAGE_TIMEOUT);
                        break;
                    }

                    case EVENT_TX:
                    {
                        /*A Broadcast message has been transmitted successfully. This event should be used to send the next message
                        for transmission to the ANT device if the node is setup as a master.*/
                        // This event indicates that a message has just been
                        // sent over the air. We take advantage of this event to set
                        // up the data for the next message period.
                        static UCHAR ucIncrement = 0;      // Increment the first byte of the buffer

                        aucTransmitBuffer[0] = ucIncrement++;

                        // Broadcast data will be sent over the air on
                        // the next message period.
                        if (bBroadcasting)
                        {
                            pclMessageObject->SendBroadcastData(channelNum, aucTransmitBuffer);

                            static int iIndex = 0;
                            static char ac[] = { '|', '/', '-', '\\' };
                            UE_LOG(LogTemp, Warning, TEXT("Tx: %c\r"), ac[iIndex++]);
                            fflush(stdout);
                            iIndex &= 3;
                        }
                        break;
                    }

                    case EVENT_RX_SEARCH_TIMEOUT:
                    {
                        /*A receive channel has timed out on searching. The search is terminated, and the channel has been
                        automatically closed. In order to restart the search the Open Channel message must be sent again.*/
                        UE_LOG(LogTemp, Warning, TEXT("Search Timeout"));
                        IsSearching = false;
                        break;
                    }

                    case EVENT_RX_FAIL:
                    {
                        /*A receive channel missed a message which it was expecting. This happens when a slave is tracking a
                        master and is expecting a message at the set message rate. */
                        UE_LOG(LogTemp, Warning, TEXT("Rx Fail"));
                        break;
                    }

                    case EVENT_TRANSFER_RX_FAILED:
                    {
                        /*A receive transfer has failed. This occurs when a Burst Transfer Message was incorrectly received*/
                        UE_LOG(LogTemp, Warning, TEXT("Burst receive has failed"));
                        break;
                    }

                    case EVENT_TRANSFER_TX_COMPLETED:
                    {
                        /*An Acknowledged Data message or a Burst Transfer sequence has been completed successfully. When transmitting
                        Acknowledged Data or Burst Transfer, there is no EVENT_TX message*/
                        UE_LOG(LogTemp, Warning, TEXT("Tranfer Completed"));
                        break;
                    }

                    case EVENT_TRANSFER_TX_FAILED:
                    {
                        /*An Acknowledged Data message, or a Burst Transfer Message has been initiated and the transmission failed to complete
                        successfully*/
                        UE_LOG(LogTemp, Warning, TEXT("Tranfer Failed"));
                        break;
                    }

                    case EVENT_RX_FAIL_GO_TO_SEARCH:
                    {
                        /*The channel has dropped to search mode after missing too many messages.*/
                        UE_LOG(LogTemp, Warning, TEXT("Go to Search"));
                        break;
                    }

                    case EVENT_CHANNEL_COLLISION:
                    {
                        /*Two channels have drifted into each other and overlapped in time on the device causing one channel to be blocked.*/
                        UE_LOG(LogTemp, Warning, TEXT("Channel Collision"));
                        break;
                    }

                    case EVENT_TRANSFER_TX_START:
                    {
                        /*Sent after a burst transfer begins, effectively on the next channel period after the burst transfer message has been
                        sent to the device. */
                        UE_LOG(LogTemp, Warning, TEXT("Burst Started"));
                        break;
                    }

                    default:
                    {
                        /*Wtf happened lol*/
                        UE_LOG(LogTemp, Warning, TEXT("Unhandled channel event: 0x%X"), stMessage.aucData[2]);
                        break;
                    }

                    }

                    break;

                }

                default:
                {
                    UE_LOG(LogTemp, Warning, TEXT("Unhandled response 0%d to message 0x%X"), stMessage.aucData[2], stMessage.aucData[1]);
                    break;
                }

                }
        }

        case MESG_STARTUP_MESG_ID: //Notifications (Start-up Messsage)
        {
            //UE_LOG(LogTemp, Warning, TEXT("RESET Complete, reason: "));

            //UCHAR ucReason = stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX];

            //if (ucReason == RESET_POR)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_POR"));
            //if (ucReason & RESET_SUSPEND)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_SUSPEND "));
            //if (ucReason & RESET_SYNC)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_SYNC "));
            //if (ucReason & RESET_CMD)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_CMD "));
            //if (ucReason & RESET_WDT)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_WDT "));
            //if (ucReason & RESET_RST)
            //    UE_LOG(LogTemp, Warning, TEXT("RESET_RST "));
            //UE_LOG(LogTemp, Warning, TEXT(""));

            break;
        }

        case MESG_ACKNOWLEDGED_DATA_ID:
        case MESG_BURST_DATA_ID:
        case MESG_BROADCAST_DATA_ID:
        {
            if (usSize_ > MESG_DATA_SIZE)
            {
                UCHAR ucFlag = stMessage.aucData[MESSAGE_BUFFER_DATA10_INDEX];

                if (ucFlag & ANT_EXT_MESG_BITFIELD_DEVICE_ID)
                {
                    // Channel ID of the device that we just recieved a message from.
                    int usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA11_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA12_INDEX] << 8);
                    int ucDeviceType = stMessage.aucData[MESSAGE_BUFFER_DATA13_INDEX];
                    int ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA14_INDEX];
                    switch (stMessage.aucData[0])
                    {
                    case 0:
                        if (IsNewDevice(usDeviceNumber, ucDeviceType, ucTransmissionType))
                        {
                            FoundChannels.Push(FChannelID{ usDeviceNumber,ucDeviceType,ucTransmissionType });
                        }
                        break;

                    case 1:
                        // In case we miss messages for 2 seconds or longer, we use the system time from the standard C time library to calculate rollovers
                        time_t currentRxTime = time(NULL);
                        if (currentRxTime - previousRxTime >= 2)
                        {
                            ulNewEventTime += (currentRxTime - previousRxTime) / 2 * 32768;
                        }
                        previousRxTime = currentRxTime;

                        unsigned short usCurrentEventTime = stMessage.aucData[MESSAGE_BUFFER_DATA15_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA16_INDEX] << 8);
                        unsigned short usDeltaEventTime = usCurrentEventTime - usPreviousEventTime;
                        ulNewEventTime += usDeltaEventTime;
                        usPreviousEventTime = usCurrentEventTime;
                        UE_LOG(LogTemp, Warning, TEXT("APR: %f-"), (double)ulNewEventTime / 32768);

                        // NOTE: In this example we use the incoming message timestamp as it typically has the most accuracy
                        // NOTE: The library will handle the received time discrepancy caused by power only event count linked messages
                        if (bPowerDecoderInitialized)
                        {
                            DecodePowerMessage((double)ulNewEventTime / 32768, &stMessage.aucData[ucDataOffset]);
                        }

                        // NOTE: We must compensate for the power only event count/rx time discrepance here, because the library does not decode Te/Ps
                        // The torque effectiveness/pedal smoothness page is tied to the power only page and vice versa,
                        // so both pages share the same "received time" depending on which page was received first and if the event count updated.
                        if (stMessage.aucData[ucDataOffset] == ANT_TEPS || stMessage.aucData[ucDataOffset] == ANT_POWERONLY)
                        {
                            UCHAR ucNewPowerOnlyUpdateEventCount = stMessage.aucData[ucDataOffset + 1];

                            if (ucNewPowerOnlyUpdateEventCount != ucPowerOnlyUpdateEventCount)
                            {
                                ucPowerOnlyUpdateEventCount = ucNewPowerOnlyUpdateEventCount;
                                dRxTimeTePs = (double)ulNewEventTime / 32768;
                            }

                            if (stMessage.aucData[ucDataOffset] == ANT_TEPS)
                            {
                                // NOTE: Any value greater than 200 or 100% should be considered "INVALID"
                                FLOAT fLeftTorqueEffectiveness = (float)stMessage.aucData[ucDataOffset + 2] / 2;
                                FLOAT fRightTorqueEffectiveness = (float)stMessage.aucData[ucDataOffset + 3] / 2;
                                FLOAT fLeftOrCombPedalSmoothness = (float)stMessage.aucData[ucDataOffset + 4] / 2;
                                FLOAT fRightPedalSmoothness = (float)stMessage.aucData[ucDataOffset + 4] / 2;
                                TePsReceiver(dRxTimeTePs, fLeftTorqueEffectiveness, fRightTorqueEffectiveness, fLeftOrCombPedalSmoothness, fRightPedalSmoothness);
                            }
                            else
                            {
                                // NOTE: Power only is a separate data stream containing similar power data compared to torque data pages but containing pedal power balance
                                // On power only sensors, it would be valuable to average power balance between generated records
                                FLOAT fPowerBalance = (float)(0x7F & stMessage.aucData[ucDataOffset + 2]);
                                BOOL bPowerBalanceRightPedalIndicator = (0x80 & stMessage.aucData[ucDataOffset + 2]) != 0;
                                PowerBalanceReceiver(dRxTimeTePs, fPowerBalance, bPowerBalanceRightPedalIndicator);
                            }
                        }
                        break;
                    }


                    UE_LOG(LogTemp, Warning, TEXT("Chan ID(%i/%i/%i) - "), usDeviceNumber, ucDeviceType, ucTransmissionType);
                }
            }
            break;
        }

        case MESG_EXT_BROADCAST_DATA_ID:
        case MESG_EXT_ACKNOWLEDGED_DATA_ID:
        case MESG_EXT_BURST_DATA_ID:
        {
            // Channel ID of the device that we just recieved a message from.
            USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX] << 8);
            UCHAR ucDeviceType = stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
            UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];
            switch (stMessage.aucData[0])
            {
            case 0:
                if (IsNewDevice(usDeviceNumber, ucDeviceType, ucTransmissionType))
                {
                    FoundChannels.Push(FChannelID{ usDeviceNumber,ucDeviceType,ucTransmissionType });
                }
                break;
            }

            // Display the channel id
            UE_LOG(LogTemp, Warning, TEXT("Chan ID(%d/%d/%d) "), usDeviceNumber, ucDeviceType, ucTransmissionType);
            break;
        }

    }

}

void ASearchChannelActor::SaveChannelID(int DevID, int DevType, int TransType)
{
    FString SaveName = SaveNameTranslator(DevType);
    int SaveSlot = SaveSlotTranslator(DevType);

    if (UMySaveGame* SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())))
    {
        // Set data on the savegame object.
        SaveGameInstance->DeviceID = DevID;
        SaveGameInstance->DeviceType = DevType;
        SaveGameInstance->TransmissionType = TransType;

        // Save the data immediately.
        if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveName, SaveSlot))
        {
            // Save succeeded.
            UE_LOG(LogTemp, Warning, TEXT("SAVED --> DeviceID: %i, DeviceType: %i, TransmissionType: %i"), SaveGameInstance->DeviceID, SaveGameInstance->DeviceType, SaveGameInstance->TransmissionType);
        }
    }
}

void ASearchChannelActor::ClearChannelID(int DevType)
{
    FString SaveName = SaveNameTranslator(DevType);
    int SaveSlot = SaveSlotTranslator(DevType);

    if (UMySaveGame* SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())))
    {
        // Set data on the savegame object.
        SaveGameInstance->DeviceID = 0;
        SaveGameInstance->DeviceType = DevType;
        SaveGameInstance->TransmissionType = 0;

        // Save the data immediately.
        if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveName, SaveSlot))
        {
            // Save succeeded.
            UE_LOG(LogTemp, Warning, TEXT("CLEARED --> DeviceID: %i, DeviceType: %i, TransmissionType: %i"), SaveGameInstance->DeviceID, SaveGameInstance->DeviceType, SaveGameInstance->TransmissionType);
        }
    }
}

//bool ASearchChannelActor::SpawnActor(int DevID, int DevType, int TransType)
//{
//    bool status;
//
//    switch (SaveSlotTranslator(DevType))
//    {
//    case 0:
//        SpawnPowerReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnPowerReaderActor->SetChannelID(DevID, DevType, TransType, pclSerialObject, pclMessageObject);
//        break;
//    case 1:
//        SpawnTrainerReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnTrainerReaderActor->SetChannelID(DevID, DevType, TransType, pclSerialObject, pclMessageObject);
//        break;
//    case 2:
//        SpawnHeartReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnHeartReaderActor->SetChannelID(DevID, DevType, TransType, pclSerialObject, pclMessageObject);
//        break;
//    }
//
//    if (!status)
//    {
//        return false;
//    }
//    return true;
//}

bool ASearchChannelActor::CreateChannel(int DevID, int DevType, int TransType)
{
    BOOL bStatus;

    int type = SaveSlotTranslator(DevType) + 1;

    pclMessageObject->CloseChannel(0, MESSAGE_TIMEOUT);

    bStatus = pclMessageObject->AssignChannel(type, 0, 0, MESSAGE_TIMEOUT);
    DeviceNumber[type - 1] = DevID;
    DeviceType[type - 1] = DevType;
    TransmissionType[type - 1] = TransType;

    if (bStatus)
    {
        UE_LOG(LogTemp, Warning, TEXT("New Channel setup with id %i/%i/%i"), DeviceNumber[type - 1], DeviceType[type - 1], TransmissionType[type - 1]);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("New Channel setup with id %i/%i/%i"), DeviceNumber[type - 1], DeviceType[type - 1], TransmissionType[type - 1]);
        return false;
    }
}

//bool ASearchChannelActor::SpawnActor(int i)
//{
//    bool status;
//
//    switch (i)
//    {
//    case 0:
//        SpawnPowerReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnPowerReaderActor->SetChannelID(DeviceNumber[0], DeviceType[0], DeviceType[0], pclSerialObject, pclMessageObject);
//        break;
//    case 1:
//        SpawnTrainerReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnTrainerReaderActor->SetChannelID(DeviceNumber[1], DeviceType[1], DeviceType[1], pclSerialObject, pclMessageObject);
//        break;
//    case 2:
//        SpawnHeartReaderActor = GetWorld()->SpawnActor<AAntPlusReaderActor>(ActorToSpawn, FVector(0, 0, 0), FRotator(0));
//        status = SpawnHeartReaderActor->SetChannelID(DeviceNumber[2], DeviceType[2], DeviceType[2], pclSerialObject, pclMessageObject);
//        break;
//    }
//
//    if (!status)
//    {
//        return false;
//    }
//    return true;
//}

bool ASearchChannelActor::CreateChannel(int i)
{
    BOOL bStatus;

    int type = i + 1;

    bStatus = pclMessageObject->AssignChannel(type, 0, 0, MESSAGE_TIMEOUT);

    if (bStatus)
    {
        UE_LOG(LogTemp, Warning, TEXT("New Channel setup with id %i/%i/%i"), DeviceNumber[type - 1], DeviceType[type - 1], TransmissionType[type - 1]);
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("New Channel setup with id %i/%i/%i"), DeviceNumber[type - 1], DeviceType[type - 1], TransmissionType[type - 1]);
        return false;
    }
}

bool ASearchChannelActor::ResetChannel()
{
    BOOL bStatus;

    // Reset system
    UE_LOG(LogTemp, Warning, TEXT("Resetting module..."));
    bStatus = pclMessageObject->ResetSystem();
    DSIThread_Sleep(1000);

    // Start the test by setting network key
    UE_LOG(LogTemp, Warning, TEXT("Setting network key..."));
    UCHAR ucNetKey[8] = USER_NETWORK_KEY;

    bStatus = pclMessageObject->SetNetworkKey(USER_NETWORK_NUM, ucNetKey, MESSAGE_TIMEOUT);

    if (!bStatus)
    {
        UE_LOG(LogTemp, Warning, TEXT("Resetting Failed"));
        return false;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Resetting Complete"));
        return true;
    }

}

void ASearchChannelActor::LoadChannelID()
{
    FString SaveName;

    for (int i = 0; i < 3; i++)
    {
        if (i == 0)
            SaveName = TEXT("Power");
        else if (i == 1)
            SaveName = TEXT("Trainer");
        else
            SaveName = TEXT("Heart");

        // Retrieve and cast the USaveGame object to UMySaveGame.
        if (UMySaveGame* LoadedGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(SaveName, i)))
        {
            DeviceNumber[i] = LoadedGame->DeviceID;
            DeviceType[i] = LoadedGame->DeviceType;
            TransmissionType[i] = LoadedGame->TransmissionType;

            // The operation was successful, so LoadedGame now contains the data we saved earlier.
            UE_LOG(LogTemp, Warning, TEXT("LOADED --> DeviceID: %i, DeviceType: %i, TransmissionType: %i"), LoadedGame->DeviceID, LoadedGame->DeviceType, LoadedGame->TransmissionType);
        }
    }
}

FString ASearchChannelActor::SaveNameTranslator(int DevType)
{
    if (DevType == 11)
        return TEXT("Power");
    if (DevType == 17)
        return TEXT("Trainer");
    return TEXT("Heart");
}

int ASearchChannelActor::SaveSlotTranslator(int DevType)
{
    if (DevType == 11)
        return 0;
    if (DevType == 17)
        return 1;
    return 2;
}

bool ASearchChannelActor::IsNewDevice(int DevNum, int DevType, int TransType)
{
    if (FoundChannels.Contains(FChannelID{ DevNum,DevType,TransType }))
        return false;
    NewDevice = true;
    return true;
}

void ASearchChannelActor::RecordReceiver(double dLastRecordTime_, double dTotalRotation_, double dTotalEnergy_, float fAverageCadence_, float fAveragePower_)
{
    //Handle new records from power recording library.
    UE_LOG(LogTemp, Warning, TEXT("APR: %lf, %lf, %lf, %f, %f"),
        dLastRecordTime_, dTotalRotation_, dTotalEnergy_, fAverageCadence_, fAveragePower_);

    APower = fAveragePower_;

    ACadence = fAverageCadence_;
}

void ASearchChannelActor::TePsReceiver(double dRxTime_, float fLeftTorqEff_, float fRightTorqEff_, float fLeftOrCPedSmth_, float fRightPedSmth_)
{
    //Handle new torque effectivenessand pedal smoothness data page.
    UE_LOG(LogTemp, Warning, TEXT("APR: RxTime,LTE,RTE,LCPS,RPS,%f, %f, %f, %f, %f"),
        dRxTime_, fLeftTorqEff_, fRightTorqEff_, fLeftOrCPedSmth_, fRightPedSmth_);
}

void ASearchChannelActor::PowerBalanceReceiver(double dRxTime_, float fPowerBalance_, bool bPowerBalanceRightPedalIndicator_)
{
    //Handle power balance from power only data page.
    UE_LOG(LogTemp, Warning, TEXT("APR: RxTime,PwrBal,RightPedal,%f, %f, %d"),
        dRxTime_, fPowerBalance_, bPowerBalanceRightPedalIndicator_);
}

//==============================================
// Task for Threading

WaitForMessagesTask::WaitForMessagesTask(ASearchChannelActor* SCActor, DSIFramerANT* pclMsgObj, int DevType)
{
    SearchChannelActor = SCActor;
    pclMessageObject = pclMsgObj;
    DeviceType = DevType;
}

WaitForMessagesTask::~WaitForMessagesTask()
{
    UE_LOG(LogTemp, Warning, TEXT("Stopped Waiting For Messages!!"))
}

void WaitForMessagesTask::DoWork()
{
    UE_LOG(LogTemp, Warning, TEXT("Thread Started"))
    while (true)
    {
        ANT_MESSAGE stMessage;
        USHORT usSize;

        if (pclMessageObject->WaitForMessage(1000))
        {
            usSize = pclMessageObject->GetMessage(&stMessage);

            if (usSize == DSI_FRAMER_ERROR)
            {
                // Get the message to clear the error
                usSize = pclMessageObject->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);
                UE_LOG(LogTemp, Warning, TEXT("thread error"));
            }

            else if (usSize != DSI_FRAMER_TIMEDOUT && usSize != 0)
            {
                SearchChannelActor->ProcessMessage(stMessage, usSize, DeviceType);
            }
        }
    }
}