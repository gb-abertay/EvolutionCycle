// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/SaveGame.h"
#include "MySaveGame.generated.h"

/**
 * 
 */
UCLASS()
class ANTPLUSREADER_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
		UMySaveGame();

		int DeviceID;
		int DeviceType;
		int TransmissionType;
};
