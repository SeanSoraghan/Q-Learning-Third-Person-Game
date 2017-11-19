// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TPGameDemoGameMode.h"
#include "MazeActor.h"


// Sets default values
AMazeActor::AMazeActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer) 
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMazeActor::BeginPlay()
{
	Super::BeginPlay();
    
    MaxHealth = 100.0f;

    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
    if (gameMode != nullptr)
    {
        CurrentLevelNumGridUnitsX   = gameMode->NumGridUnitsX;
        CurrentLevelNumGridUnitsY   = gameMode->NumGridUnitsY;
        CurrentLevelGridUnitLengthXCM = gameMode->GridUnitLengthXCM;
        CurrentLevelGridUnitLengthYCM = gameMode->GridUnitLengthYCM;
        MaxHealth = gameMode->DefaultMaxHealth;
    }
    
    Health = MaxHealth;

    UpdatePosition (false);
}

float AMazeActor::GetHealthPercentage()
{
    return Health / MaxHealth;
}

void AMazeActor::TakeDamage (float damageAmount)
{
    Health -= damageAmount;
    CheckDeath();
}

void AMazeActor::CheckDeath()
{
    if (Health <= 0.0f)
    {
        Health = 0.0f;
        IsAlive = false;
        OnActorDied.Broadcast();
    }
}

void AMazeActor::UpdatePosition (bool broadcastChange)
{
    FVector worldPosition = GetActorLocation();
    const int totalGridLengthCMX = CurrentLevelGridUnitLengthXCM * CurrentLevelNumGridUnitsX;
    const int totalGridLengthCMY = CurrentLevelGridUnitLengthYCM * CurrentLevelNumGridUnitsY;
    GridXPosition = (int) (worldPosition.X + totalGridLengthCMX / 2) / (CurrentLevelGridUnitLengthXCM);
    GridYPosition = (int) (worldPosition.Y + totalGridLengthCMY / 2) / (CurrentLevelGridUnitLengthYCM);

    if (broadcastChange && (GridYPosition != PreviousGridYPosition || GridXPosition != PreviousGridXPosition))
        GridPositionChangedEvent.Broadcast();

    PreviousGridXPosition = GridXPosition;
    PreviousGridYPosition = GridYPosition;
}

// Called every frame
void AMazeActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    UpdatePosition();
}