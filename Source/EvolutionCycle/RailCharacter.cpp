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
	CurrentObstacle = EObstacleTypes::None;
}

//Function to tell the rail character it start an obstacle and which specific type of obstacle is started.
void ARailCharacter::StartObstacle(EObstacleTypes obstacle)
{
	//Resets the timings
	ObstacleTimings = { 0.0, 0.0, 0.0 };

	//Set the current obstacle to the obstacle passed in. (from bp in startobstacle trigger)
	CurrentObstacle = obstacle;

	//Print Debug message to screen
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Started obstacle"));
}

//Function to tell the rail character that it has finished the obstacle it is on
void ARailCharacter::EndObstacle()
{
	//Calculate the total time taken for the obstacle so the percentage can be calculated later
	float totalTime = ObstacleTimings.SmallTime + ObstacleTimings.MediumTime + ObstacleTimings.LargeTime;

	//Set initial percentage to zero
	float percentage = 0.0f;

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
	if ((percentage >= 0.8) && (RailCharacterState == CurrentObstacle))
	{
		//passed obstacle would be true (add logic)
	}

	//Set the current obstacle to none
	CurrentObstacle = EObstacleTypes::None;

	//Print Debug message to screen
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("End of obstacle"));
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
		case ERailCharacterState::Small:
			ObstacleTimings.SmallTime += DeltaTime;
			break;
		case ERailCharacterState::Medium:
			ObstacleTimings.MediumTime += DeltaTime;
			break;
		case ERailCharacterState::Large:
			ObstacleTimings.LargeTime += DeltaTime;
			break;
		default:
			ObstacleTimings.SmallTime += DeltaTime;
			break;
		}
	}
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