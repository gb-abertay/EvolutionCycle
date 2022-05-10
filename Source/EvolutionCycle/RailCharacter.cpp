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
	PowerStateSmall = 100;
	PowerStateMedium = 200;
	PowerStateLarge = 300;
	FiveSecondSpeed = {100, 100, 100, 100, 100};
	CurrentObstacle = EObstacleTypes::None;
	RailCharacterState = ERailCharacterStates::IDLE;
}

//Function to tell the rail character it start an obstacle and which specific type of obstacle is started.
void ARailCharacter::StartObstacle(EObstacleTypes obstacle)
{
	//Resets the timings
	ObstacleTimings = { 0.0, 0.0, 0.0 };

	//Set the current obstacle to the obstacle passed in. (from bp in startobstacle trigger)
	CurrentObstacle = obstacle;

}

//Function to tell the rail character that it has finished the obstacle it is on
bool ARailCharacter::EndObstacle()
{
	//Calculate the total time taken for the obstacle so the percentage can be calculated later
	float totalTime = ObstacleTimings.SmallTime + ObstacleTimings.MediumTime + ObstacleTimings.LargeTime;

	//Set initial percentage to zero
	float percentage = 0.0f;
	bool passed = false;

	//Calculate the percentage of time in the right state.
	switch (CurrentObstacle)
	{
	case EObstacleTypes::Through:
		percentage = ObstacleTimings.SmallTime / totalTime;
		break;
	case EObstacleTypes::Over:
		percentage = ObstacleTimings.MediumTime / totalTime;
		break;
	case EObstacleTypes::Smash:
		percentage = ObstacleTimings.LargeTime / totalTime;
		break;
	}

	//If character was in right state for over 80% of the obstacle and is currently in the right state.
	if ((percentage >= 0.5) && ((uint8)RailCharacterState == (uint8)CurrentObstacle))
	{
		passed = true;
	}

	//Set the current obstacle to none
	CurrentObstacle = EObstacleTypes::None;

	return passed;
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

	if (CurrentObstacle != EObstacleTypes::None)
	{
		switch (RailCharacterState)
		{
		case ERailCharacterStates::SMALL:
			ObstacleTimings.SmallTime += DeltaTime;
			break;
		case ERailCharacterStates::MEDIUM:
			ObstacleTimings.MediumTime += DeltaTime;
			break;
		case ERailCharacterStates::LARGE:
			ObstacleTimings.LargeTime += DeltaTime;
			break;
		default:
			ObstacleTimings.SmallTime += DeltaTime;
			break;
		}
	}

	// Pause if player stops
	TimeElapsed += DeltaTime;
	if (TimeElapsed >= 1)
	{
		TimeElapsed = 0;
		FiveSecondSpeed.pop_front();
		FiveSecondSpeed.push_back(Speed);

		float sum = 0;
		for (auto it : FiveSecondSpeed)
		{
			sum += it;
		}
		sum /= 5;
		if (sum <= 0.1)
			PauseGame();
	}
}

// Called to bind functionality to input
void ARailCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
}

void ARailCharacter::CharacterMovement(float DeltaTime, float AveragePower)
{
	// Increase speed based on average power
	if (IsBikeInputEnabled)
	{
		Speed += AveragePower / 50;
	}

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
		Speed -= (Speed / 250) + 0.5;
		
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
			if (AveragePower <= PowerStateSmall)
			{
				RailCharacterState = ERailCharacterStates::SMALL;
				return;
			}
			if (AveragePower >= (PowerStateSmall + PowerStateMedium) / 2 || AveragePower <= PowerStateMedium)
			{
				RailCharacterState = ERailCharacterStates::MEDIUM;
			}
			if (AveragePower >= (PowerStateMedium + PowerStateLarge) / 2)
			{
				RailCharacterState = ERailCharacterStates::LARGE;
			}
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

void ARailCharacter::ResetFiveSecondSpeed()
{
	FiveSecondSpeed = { 100, 100, 100, 100, 100 };
}

void ARailCharacter::PauseGame_Implementation()
{

}