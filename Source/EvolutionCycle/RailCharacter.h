// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SplineComponent.h"
#include "RailCharacter.generated.h"

UENUM(BlueprintType)
enum class EObstacleTypes : uint8 {
	None		UMETA(DisplayName = "None"),
	Through       UMETA(DisplayName = "Through"),
	Over        UMETA(DisplayName = "Over"),
	Smash        UMETA(DisplayName = "Smash"),
};

USTRUCT()
struct FObstacleTimings
{
	GENERATED_BODY()
public:
		float SmallTime = 0.0;
		float MediumTime = 0.0;
		float LargeTime = 0.0;
};

UCLASS()
class EVOLUTIONCYCLE_API ARailCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARailCharacter();

	UFUNCTION(BlueprintCallable, Category = "RailCharacter")
		void StartObstacle(EObstacleTypes obstacle);

	UFUNCTION(BlueprintCallable, Category = "RailCharacter")
		void EndObstacle();

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

	UFUNCTION(BlueprintCallable, Category = "RailCharacter")
	void CharacterMovement(float DeltaTime);
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	EObstacleTypes CurrentObstacle;
	FObstacleTimings ObstacleTimings;

};
