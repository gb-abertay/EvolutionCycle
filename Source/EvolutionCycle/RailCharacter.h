// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "RailCharacter.generated.h"

UENUM(BlueprintType)
enum class ERailCharacterStates : uint8
{
	IDLE UMETA(DisplayName = "Idle"),
	SMALL UMETA(DisplayName = "Small"),
	MEDIUM UMETA(DisplayName = "Medium"),
	LARGE UMETA(DisplayName = "Large"),
};

UCLASS()
class EVOLUTIONCYCLE_API ARailCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARailCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Character components
	UPROPERTY(BlueprintReadWrite)
	USplineComponent* CurrentSpline;

	// Character variables
	UPROPERTY(BlueprintReadWrite)
	float DistanceCovered;
	UPROPERTY(BlueprintReadWrite)
	float Speed;
	UPROPERTY(BlueprintReadWrite)
	bool IsBikeInputEnabled;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERailCharacterStates RailCharacterState;

	UFUNCTION(BlueprintCallable, Category = "RailCharacter")
	void CharacterMovement(float DeltaTime);
	UFUNCTION(BlueprintCallable, Category = "RailCharacter")
	void ChangeStates(float AveragePower);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


};
