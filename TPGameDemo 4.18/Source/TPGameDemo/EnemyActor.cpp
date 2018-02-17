// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TPGameDemoGameMode.h"
#include "TextParserComponent.h"
#include "EnemyActor.h"
#include "Kismet/KismetMathLibrary.h"

//======================================================================================================
// Initialisation
//====================================================================================================== 
AEnemyActor::AEnemyActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    LevelPoliciesDir = FPaths::GameDir();
    LevelPoliciesDir += "Content/Levels/GeneratedRooms/";
    LevelPoliciesDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists (*LevelPoliciesDir);
}

void AEnemyActor::BeginPlay()
{
	Super::BeginPlay();

  #if ON_SCREEN_DEBUGGING
    if ( ! LevelPoliciesDirFound)
	    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Couldn't find level policies directory at %s"), *LevelPoliciesDir));
  #endif

    UWorld* world = GetWorld();
    if (world != nullptr)
        world->GetTimerManager().SetTimer (MoveTimerHandle, [this](){ UpdateMovement(); }, 0.5f, true);
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
    //UpdateMovement();
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    FRotator CurrentRotation = GetActorForwardVector().Rotation();
    FRotator ToTarget = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, MovementVector.Rotation());
    SetActorRotation (UKismetMathLibrary::RLerp(CurrentRotation, MovementVector.Rotation(), RotationSpeed/*Linear*/, true));
   // UE_LOG(LogTemp, Warning, TEXT("Position Before: %f | %f | %f"), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
    SetActorLocation(GetActorLocation() + MovementVector * MovementSpeed, true);
    AddMovementInput(MovementVector * MovementSpeed);
    //UE_LOG(LogTemp, Warning, TEXT("Position After: %f | %f | %f"), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
    SetActorLocation(GetActorLocation() + MovementVector * MovementSpeed, false);
    //UE_LOG(LogTemp, Warning, TEXT("Position After No Sweep: %f | %f | %f"), GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
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
            UE_LOG(LogTemp, Warning, TEXT("Chose North"));
            MovementTarget = FVector (currentLocation.X + CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z);
            break;
        }
        case EActionType::East: 
        {
            //SetActorLocation (FVector (currentLocation.X, currentLocation.Y + CurrentLevelGridUnitLengthYCM, currentLocation.Z));
            UE_LOG(LogTemp, Warning, TEXT("Chose East"));
            MovementTarget = FVector (currentLocation.X, currentLocation.Y + CurrentLevelGridUnitLengthYCM, currentLocation.Z);
            break;
        }
        case EActionType::South: 
        {
            //SetActorLocation (FVector (currentLocation.X - CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z));
            UE_LOG(LogTemp, Warning, TEXT("Chose South"));
            MovementTarget = FVector (currentLocation.X - CurrentLevelGridUnitLengthXCM, currentLocation.Y, currentLocation.Z);
            break;
        }
        case EActionType::West: 
        {
            //SetActorLocation (FVector (currentLocation.X, currentLocation.Y - CurrentLevelGridUnitLengthYCM, currentLocation.Z));
            UE_LOG(LogTemp, Warning, TEXT("Chose West"));
            MovementTarget = FVector (currentLocation.X, currentLocation.Y - CurrentLevelGridUnitLengthYCM, currentLocation.Z);
            break;
        }
        default: break;
    }
}

EActionType AEnemyActor::SelectNextAction()
{
    if (CurrentLevelPolicy.Num() > GridXPosition && GridXPosition > 0)
        if (CurrentLevelPolicy.Num() > 0 && CurrentLevelPolicy[GridXPosition].Num() > GridYPosition && GridYPosition > 0)
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
void AEnemyActor::LoadLevelPolicyForRoomCoordinates (FIntPoint levelCoords)
{
    FString levelName = FString::FromInt(levelCoords.X) + FString("_") + FString::FromInt(levelCoords.Y);
    LoadLevelPolicy(levelName);
}

void AEnemyActor::LoadLevelPolicy (FString levelName)
{
    if (LevelPoliciesDirFound)
        CurrentLevelPolicyDir = LevelPoliciesDir + levelName + "/";  
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
            LevelBuilderHelpers::FillArrayFromTextFile (policy, CurrentLevelPolicy);
        }
        PrintLevelPolicy();
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
    UE_LOG(LogTemp, Warning, TEXT("Level Policy::"));
    LevelBuilderHelpers::PrintArray(CurrentLevelPolicy);
}
