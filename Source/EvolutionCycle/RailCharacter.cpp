// Fill out your copyright notice in the Description page of Project Settings.


#include "RailCharacter.h"

// Sets default values
ARailCharacter::ARailCharacter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Set variables
	Speed = 0;
	IsBikeInputEnabled = 0;
	DistanceCovered = 0;
}

// Called when the game starts or when spawned
void ARailCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARailCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ARailCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
}

void ARailCharacter::CharacterMovement(float DeltaTime)
{
	if (Speed > 0)
	{
		//Adjust to spline
		FVector NewLocation;
		FVector SplineLocation = CurrentSpline->FindLocationClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
		if (GetActorLocation().Equals(SplineLocation, 0.05))
		{
			NewLocation.X = SplineLocation.X;
			NewLocation.Y = SplineLocation.Y;
			NewLocation.Z = GetActorLocation().Z;
			SetActorLocation(NewLocation);
		}

		//Move along forward vector
		NewLocation = GetActorLocation() + (GetActorForwardVector() * Speed * DeltaTime);
		SetActorLocation(NewLocation);

		// Rotate to spline
		FVector NewRotation = CurrentSpline->FindDirectionClosestToWorldLocation(GetActorLocation(), ESplineCoordinateSpace::World);
		SetActorRelativeRotation(NewRotation.Rotation());

		// Speed Falloff
		Speed -= (Speed / 400) + 0.5;
		
		// Calculate distance covered
		DistanceCovered = CurrentSpline->GetDistanceAlongSplineAtSplineInputKey(CurrentSpline->FindInputKeyClosestToWorldLocation(GetActorLocation()));
	}

}

void ARailCharacter::ChangeStates(float AveragePower)
{
	if (IsBikeInputEnabled)
	{
		if (Speed <= 0.01)
			RailCharacterState = ERailCharacterStates::IDLE;
		else if (Speed > 0.01)
		{ 
			RailCharacterState = ERailCharacterStates::SMALL;
			if (AveragePower >= 70)
				RailCharacterState = ERailCharacterStates::MEDIUM;
			if (AveragePower >= 120)
				RailCharacterState = ERailCharacterStates::LARGE;
		}
	}
	else
	{
		if (Speed <= 0.01)
			RailCharacterState = ERailCharacterStates::IDLE;
		else if (Speed > 0.01)
		{
			RailCharacterState = ERailCharacterStates::SMALL;
			if (Speed >= 2000)
				RailCharacterState = ERailCharacterStates::MEDIUM;
			if (Speed >= 3500)
				RailCharacterState = ERailCharacterStates::LARGE;
		}
	}
}