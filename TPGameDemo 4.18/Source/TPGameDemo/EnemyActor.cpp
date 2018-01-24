// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TextParserComponent.cpp"
#include "TPGameDemoGameMode.h"
#include "EnemyActor.h"
#include "Kismet/KismetMathLibrary.h"

//======================================================================================================
// Initialisation
//====================================================================================================== 
AEnemyActor::AEnemyActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    LevelPolciesDir = FPaths::GameDir();
    LevelPolciesDir += "Content/Characters/Enemies/LevelPolicies/";
    LevelPoliciesDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists (*LevelPolciesDir);
}

void AEnemyActor::BeginPlay()
{
	Super::BeginPlay();

  #if ON_SCREEN_DEBUGGING
    if ( ! LevelPoliciesDirFound)
	    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Couldn't find level policies directory at %s"), *LevelPolciesDir));
  #endif

    //UWorld* world = GetWorld();
    //if (world != nullptr)
    //    world->GetTimerManager().SetTimer (MoveTimerHandle, [this](){ UpdateMovement(); }, 0.5f, true);
}

void AEnemyActor::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UWorld* world = GetWorld();
    if (world != nullptr)
        GetWorld()->GetTimerManager().ClearAllTimersForObject (this);
}


//======================================================================================================
// Continuous Updating
//====================================================================================================== 
void AEnemyActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    UpdateMovement();
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    FRotator CurrentRotation = GetActorForwardVector().Rotation();
    FRotator ToTarget = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, MovementVector.Rotation());
    SetActorRotation (UKismetMathLibrary::RLerp(CurrentRotation, MovementVector.Rotation(), RotationSpeed/*Linear*/, true));
    AddMovementInput(MovementVector * MovementSpeed);
     //GetActorForwardVector().Rotation()
}

//======================================================================================================
// Movement
//====================================================================================================== 
void AEnemyActor::UpdateMovement()
{
    EActionType actionType = SelectNextAction();
    FVector currentLocation = GetActorLocation();
    switch (actionType)
    {
        case EActionType::North: 
        {
            //SetActorLocation (FVector (currentLocation.X + CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z));
            MovementTarget = FVector (currentLocation.X + CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z);
            break;
        }
        case EActionType::East: 
        {
            //SetActorLocation (FVector (currentLocation.X, currentLocation.Y + CurrentLevelGridUnitLengthYCM, currentLocation.Z));
            MovementTarget = FVector (currentLocation.X, currentLocation.Y + CurrentLevelGridUnitLengthYCM, currentLocation.Z);
            break;
        }
        case EActionType::South: 
        {
            //SetActorLocation (FVector (currentLocation.X - CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z));
            MovementTarget = FVector (currentLocation.X - CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z);
            break;
        }
        case EActionType::West: 
        {
            //SetActorLocation (FVector (currentLocation.X, currentLocation.Y - CurrentLevelGridUnitLengthYCM, currentLocation.Z));
            MovementTarget = FVector (currentLocation.X, currentLocation.Y - CurrentLevelGridUnitLengthYCM, currentLocation.Z);
            break;
        }
        default: break;
    }
    CheckPositionLimits();
}

void AEnemyActor::CheckPositionLimits()
{
    FVector currentLocation    = GetActorLocation();
    const int halfGridStepX    = CurrentLevelGridUnitLengthXCM / 2;
    const int totalGridLengthX = CurrentLevelGridUnitLengthXCM * CurrentLevelNumGridUnitsX;
    const int edgeLimitX       = ((CurrentLevelNumGridUnitsX / 2) - (CurrentLevelNumGridUnitsX % 2 == 0 ? 1 : 0) - 1) * CurrentLevelGridUnitLengthXCM + halfGridStepX; 
    const int halfGridStepY    = CurrentLevelGridUnitLengthYCM / 2;
    const int totalGridLengthY = CurrentLevelGridUnitLengthYCM * CurrentLevelNumGridUnitsY;
    const int edgeLimitY       = ((CurrentLevelNumGridUnitsY / 2) - (CurrentLevelNumGridUnitsY % 2 == 0 ? 1 : 0) - 1) * CurrentLevelGridUnitLengthYCM + halfGridStepY; 

    if (FMath::Abs (currentLocation.X) > totalGridLengthX / 2)
        currentLocation.X = edgeLimitX * FMath::Sign (currentLocation.X);

    if (FMath::Abs (currentLocation.Y) > totalGridLengthY / 2)
        currentLocation.Y = edgeLimitY * FMath::Sign (currentLocation.Y);

    SetActorLocation (currentLocation);
}

EActionType AEnemyActor::SelectNextAction()
{
    if (CurrentLevelPolicy.Num() > GridXPosition)
        if (CurrentLevelPolicy.Num() > 0 && CurrentLevelPolicy[GridXPosition].Num() > GridYPosition)
            return (EActionType)CurrentLevelPolicy[GridXPosition][GridYPosition];

    return EActionType::NumActionTypes;
}

//======================================================================================================
// Movement Timing
//====================================================================================================== 
void AEnemyActor::StopMovementTimer()
{
    UWorld* world = GetWorld();
    if (world != nullptr)
    {
        world->GetTimerManager().PauseTimer (MoveTimerHandle);
        world->GetTimerManager().ClearTimer (MoveTimerHandle);
    }  
}

void AEnemyActor::SetMovementTimerPaused (bool movementTimerShouldBePaused)
{
    UWorld* world = GetWorld();
    if (world != nullptr)
    {
        if (movementTimerShouldBePaused && ! world->GetTimerManager().IsTimerPaused (MoveTimerHandle))
            world->GetTimerManager().PauseTimer (MoveTimerHandle);
        else if ( ! movementTimerShouldBePaused && world->GetTimerManager().IsTimerPaused (MoveTimerHandle))
            world->GetTimerManager().UnPauseTimer (MoveTimerHandle);
    }  
}
//======================================================================================================
// Behaviour Policy
//======================================================================================================
void AEnemyActor::LoadLevelPolicy (FString levelName)
{
    if (LevelPoliciesDirFound)
        CurrentLevelPolicyDir = LevelPolciesDir + levelName + "/";
}

void AEnemyActor::UpdatePolicyForPlayerPosition (int targetX, int targetY)
{
    if (LevelPoliciesDirFound)
    {
        ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
        if (gameMode != nullptr)
        {
            ResetPolicy();
            FString policy = CurrentLevelPolicyDir + FString::FromInt (targetX) + "_" + FString::FromInt (targetY) + ".txt";
            FillArrayFromTextFile (policy, CurrentLevelPolicy);
        }
    }
}

void AEnemyActor::ResetPolicy()
{
    for (int row = 0; row < CurrentLevelPolicy.Num(); row++)
        for (int col = 0; col < CurrentLevelPolicy[row].Num(); col++)
            CurrentLevelPolicy[row][col] = 0;
}

void AEnemyActor::PrintLevelPolicy()
{
    for (int row = 0; row < CurrentLevelPolicy.Num(); row++)
    {
        FString s = "";

        for (int col = 0; col < CurrentLevelPolicy[row].Num(); col++)
            s += FString::FromInt (CurrentLevelPolicy[row][col]);

        GEngine->AddOnScreenDebugMessage(-1, 1000.f, FColor::Red, s);
    }
}
