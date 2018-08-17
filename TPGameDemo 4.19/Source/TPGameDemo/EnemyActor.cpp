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
    LevelPoliciesDir = FPaths::ProjectDir();
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

    //UWorld* world = GetWorld();
    //if (world != nullptr)
    //    world->GetTimerManager().SetTimer (MoveTimerHandle, [this](){ UpdateMovement(); }, 0.5f, true);
}

//void AEnemyActor::BeginDestroy()
//{
//    ClearAvoidanceTimer();
//}

void AEnemyActor::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason); 

    ClearAvoidanceTimer();
    // Clear Movement Timer
}


//======================================================================================================
// Continuous Updating
//====================================================================================================== 
void AEnemyActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    TimeSinceLastPositionChange += DeltaTime;
    if (MovementStuckThresholdSeconds > 0.0f && TimeSinceLastPositionChange >= MovementStuckThresholdSeconds && BehaviourState != EEnemyBehaviourState::Avoiding)
    {
        TimeSpentAvoiding = 0.0f;
        BehaviourState = EEnemyBehaviourState::Avoiding;
        AvoidingBehaviourTimeout = FMath::RandRange(1.5f, 2.5f);
        TargetRoomPosition.DoorAction = EDirectionType::NumDirectionTypes;
        UWorld* world = GetWorld();
        //if (world != nullptr)
        //{
        //    world->GetTimerManager().SetTimer (StopAvoidanceTimerHandle, [this]()
        //    { 
        //        AsyncTask(ENamedThreads::GameThread, [this]()
        //        {
        //            ChooseDoorTarget();
        //            BehaviourState = EEnemyBehaviourState::Exploring;
        //            //ClearAvoidanceTimer();
        //            UpdateMovement();
        //        });
        //    }, FMath::RandRange(1.5f, 3.5f), false);
        //}
        TargetNearbyEmptyCell();
    }
    if (BehaviourState == EEnemyBehaviourState::Avoiding)
    {
        TimeSpentAvoiding += DeltaTime;
        if (TimeSpentAvoiding >= AvoidingBehaviourTimeout)
        {
            ChooseDoorTarget();
            BehaviourState = EEnemyBehaviourState::Exploring;
            UpdateMovement();
        }
    }
    //if (BehaviourState == EEnemyBehaviourState::Avoiding)
    //{
    //    if (GridXPosition == AvoidanceTarget.PositionInRoom.X && GridYPosition == AvoidanceTarget.PositionInRoom.Y && 
    //        CurrentRoomCoords == AvoidanceTarget.RoomCoords)
    //        TargetNearbyEmptyCell();
    //}
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    FRotator CurrentRotation = GetActorForwardVector().Rotation();
    FRotator ToTarget = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, MovementVector.Rotation());
    SetActorRotation (UKismetMathLibrary::RLerp(CurrentRotation, MovementVector.Rotation(), RotationSpeed/*Linear*/, true));
    AddMovementInput(MovementVector * MovementSpeed);
}

//======================================================================================================
// Movement
//======================================================================================================
void AEnemyActor::PositionChanged()
{
    TimeSinceLastPositionChange = 0.0f;
    if (GridXPosition == TargetRoomPosition.Position.X && GridYPosition == TargetRoomPosition.Position.Y)
    {
        if (TargetRoomPosition.DoorAction != EDirectionType::NumDirectionTypes && 
            BehaviourState == EEnemyBehaviourState::Exploring)
        {
            BehaviourState = EEnemyBehaviourState::ChangingRooms;
            UpdateMovementForActionType(TargetRoomPosition.DoorAction);
        }
    }
    else if (BehaviourState == EEnemyBehaviourState::Exploring)
    {
        UpdateMovement();
    }
    else if (BehaviourState == EEnemyBehaviourState::ChangingRooms)
    {
        if (IsOnGridEdge())
        {
            ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
            if (gameState != nullptr)
            {
                const float z = GetActorLocation().Z;
                FRoomPositionPair roomAndPosition = GetTargetRoomAndPositionForDirectionType(TargetRoomPosition.DoorAction);
                FVector2D targetXY = gameState->GetWorldXYForRoomAndPosition(roomAndPosition);
                MovementTarget = FVector(targetXY.X, targetXY.Y, z);
            }
        }
        else
        {
            EnteredNewRoom();
        }
    }
    else if (BehaviourState == EEnemyBehaviourState::Avoiding)
    {
        if (GridXPosition == AvoidanceTarget.PositionInRoom.X && GridYPosition == AvoidanceTarget.PositionInRoom.Y && 
            CurrentRoomCoords == AvoidanceTarget.RoomCoords)
            TargetNearbyEmptyCell();
    }
}

void AEnemyActor::RoomCoordsChanged()
{
    ClearPreviousDoorTarget();
    LoadLevelPolicyForRoomCoordinates(CurrentRoomCoords);
    
    // If the enemy wasn't changing rooms, trigger entered new room immediately.
    if (BehaviourState != EEnemyBehaviourState::ChangingRooms)
    {
        EnteredNewRoom();
    }// Otherwise only trigger entered new room if the enemy has made it all the way through the door (is not on the edge).
    else if (!IsOnGridEdge())
    {
        EnteredNewRoom();
    }
}

void AEnemyActor::EnteredNewRoom_Implementation()
{
    BehaviourState = EEnemyBehaviourState::Exploring;
    ChooseDoorTarget();
}

void AEnemyActor::ClearPreviousDoorTarget()
{
    PreviousDoorTarget = EDirectionType::NumDirectionTypes;
}

void AEnemyActor::UpdateMovement()
{
    if (!IsOnGridEdge())
    {
        EDirectionType actionType = SelectNextAction();
        UpdateMovementForActionType(actionType);
    }
}

void AEnemyActor::UpdateMovementForActionType(EDirectionType actionType)
{ 
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        const float z = GetActorLocation().Z;
        FRoomPositionPair roomAndPosition = GetTargetRoomAndPositionForDirectionType(actionType);
        //if (!gameState->RoomTilePositionIsEmpty(roomAndPosition) && BehaviourState != EEnemyBehaviourState::Avoiding)
        //{
        //    BehaviourState = EEnemyBehaviourState::Avoiding;
        //    TargetRoomPosition.DoorAction = EDirectionType::NumDirectionTypes;
        //    UWorld* world = GetWorld();
        //    if (world != nullptr)
        //    {
        //        world->GetTimerManager().SetTimer (StopAvoidanceTimerHandle, [this]()
        //        { 
        //            ChooseDoorTarget();
        //            BehaviourState = EEnemyBehaviourState::Exploring;
        //            //ClearAvoidanceTimer();
        //            UpdateMovement();
        //        }, FMath::RandRange(2.5f, 4.5f), false);
        //    }
        //    TargetNearbyEmptyCell();
        //    return;
        //}
        FVector2D targetXY = gameState->GetWorldXYForRoomAndPosition(roomAndPosition);
        MovementTarget = FVector(targetXY.X, targetXY.Y, z);
    }
}

bool AEnemyActor::TargetNearbyEmptyCell()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        // Target neighbour cells to see if they are empty.
        // When an empty one is found, update the MovementTarget.
        // If the neighbour cell is not empty, continue testing the others in a clockwise motion.
        // Randomize the starting neighbour so that lots of avoiding enemies dont tend to head in the same direction.
        const int randomStartingDirection = FMath::RandRange(0, (int)EDirectionType::NumDirectionTypes - 1);
        for (int i = 0; i < (int)EDirectionType::NumDirectionTypes; ++i)
        {
            int clampedDirection = FMath::Fmod (i + randomStartingDirection, (int)EDirectionType::NumDirectionTypes);
            EDirectionType direction = (EDirectionType) (clampedDirection);
            FRoomPositionPair avoidanceTarget = GetTargetRoomAndPositionForDirectionType(direction);
            if (gameState->RoomTilePositionIsEmpty(avoidanceTarget))
            {
                AvoidanceTarget = avoidanceTarget;
                FVector2D targetXY = gameState->GetWorldXYForRoomAndPosition(avoidanceTarget);
                MovementTarget = FVector(targetXY.X, targetXY.Y, GetActorLocation().Z);
                return true;
            }
        }
    }
    return false;
}

void AEnemyActor::ClearAvoidanceTimer()
{
    UWorld* world = GetWorld();
    if (world != nullptr)
        world->GetTimerManager().ClearTimer(StopAvoidanceTimerHandle);
}

EDirectionType AEnemyActor::SelectNextAction()
{
    if (CurrentLevelPolicy.Num() > GridXPosition && GridXPosition > 0)
        if (CurrentLevelPolicy.Num() > 0 && CurrentLevelPolicy[GridXPosition].Num() > GridYPosition && GridYPosition > 0)
            return (EDirectionType)CurrentLevelPolicy[GridXPosition][GridYPosition];

    return EDirectionType::NumDirectionTypes;
}

FRoomPositionPair AEnemyActor::GetTargetRoomAndPositionForDirectionType(EDirectionType actionType)
{
    //0 - 9, S - N, W - E.
    FRoomPositionPair target = {CurrentRoomCoords, FIntPoint(GridXPosition, GridYPosition)};
    int roomSizeX = 10;
    int roomSizeY = 10;
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        roomSizeX = gameState->NumGridUnitsX;
        roomSizeY = gameState->NumGridUnitsY;
    }
    switch (actionType)
    {
        case EDirectionType::North: 
        {
            target.PositionInRoom = FIntPoint(GridXPosition + 1, GridYPosition);
            break;
        }
        case EDirectionType::East:
        {
            target.PositionInRoom = FIntPoint(GridXPosition, GridYPosition + 1);
            break;
        }
        case EDirectionType::South:
        {
            target.PositionInRoom = FIntPoint(GridXPosition - 1, GridYPosition);
            break;
        }
        case EDirectionType::West:
        {
            target.PositionInRoom = FIntPoint(GridXPosition, GridYPosition - 1);
            break;
        }
        default: ensure(!"Unrecognized Direction Type"); break;
    }
    gameState->WrapRoomPositionPair(target);
    return target;
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
    CurrentRoomCoords = levelCoords;
    FString levelName = FString::FromInt(levelCoords.X) + FString("_") + FString::FromInt(levelCoords.Y);
    LoadLevelPolicy(levelName);
}

void AEnemyActor::LoadLevelPolicy (FString levelName)
{
    if (LevelPoliciesDirFound)
    {
        CurrentLevelPolicyDir = LevelPoliciesDir + levelName + "/";
        //UpdateMovement();
    }
}

void AEnemyActor::UpdatePolicyForPlayerPosition (int targetX, int targetY)
{
    if (BehaviourState != EEnemyBehaviourState::Avoiding)
    {
        TargetRoomPosition.Position.X = targetX;
        TargetRoomPosition.Position.Y = targetY;
        TargetRoomPosition.DoorAction = EDirectionType::NumDirectionTypes;
        ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
        if (gameState != nullptr)
        {
            // If the player is in a doorway, we set the target according to the doorway they are in.
            // Due to the way rooms overlap, enemies will never recieve a player position changed notification
            // when the player is in the north or east door, as these are part of the neighbouring rooms.
            if (targetX == 0)
            {
                UpdatePolicyForDoorType(EDirectionType::South, targetY);
                return;
            }
            if (targetY == 0)
            {
                UpdatePolicyForDoorType(EDirectionType::West, targetX);
                return;
            }
        }
        BehaviourState = EEnemyBehaviourState::Exploring;
        UpdatePolicyForTargetPosition();
    }
}

void AEnemyActor::UpdatePolicyForDoorType (EDirectionType doorType, int doorPositionOnWall)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        TargetRoomPosition.DoorAction = doorType;
        switch (doorType)
        {
            case EDirectionType::North:
            {
                TargetRoomPosition.Position.X = gameState->NumGridUnitsX - 2;
                TargetRoomPosition.Position.Y = doorPositionOnWall;
                break;
            }
            case EDirectionType::East:
            {
                TargetRoomPosition.Position.X = doorPositionOnWall;
                TargetRoomPosition.Position.Y = gameState->NumGridUnitsY - 2;
                break;
            }
            case EDirectionType::South:
            {
                TargetRoomPosition.Position.X = 1;
                TargetRoomPosition.Position.Y = doorPositionOnWall;
                break;
            }
            case EDirectionType::West:
            {
                TargetRoomPosition.Position.X = doorPositionOnWall;
                TargetRoomPosition.Position.Y = 1;
                break;
            }
        }
        UpdatePolicyForTargetPosition();
    }
}

void AEnemyActor::UpdatePolicyForTargetPosition()
{
    int targetX = TargetRoomPosition.Position.X;
    int targetY = TargetRoomPosition.Position.Y;
    if (LevelPoliciesDirFound)
    {
        ResetPolicy();
        FString policy = CurrentLevelPolicyDir + FString::FromInt (targetX) + "_" + FString::FromInt (targetY) + ".txt";
        LevelBuilderHelpers::FillArrayFromTextFile (policy, CurrentLevelPolicy);
        UpdateMovement();
        //PrintLevelPolicy();
    }
}

void AEnemyActor::ChooseDoorTarget()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr && !(CurrentRoomCoords.X == 0 && CurrentRoomCoords.Y == 0))
    {
        TArray<int> neighbourPositions = gameState->GetDoorPositionsForExistingNeighbours(CurrentRoomCoords);
        EQuadrantType quadrant = gameState->GetQuadrantTypeForRoomCoords(CurrentRoomCoords);
        TArray<EDirectionType> possibleDoors;
        // for certain quadrants, we want to check certain doors first (doors that lead towards the centre).
        // We check these first, and if they are both closed, we check the other directions.
        TArray<bool> DoorPriorities = {false,false,false,false};
        switch(quadrant)
        {
            case EQuadrantType::NorthEast:
            {
                DoorPriorities[(int)EDirectionType::West] = true;
                DoorPriorities[(int)EDirectionType::South] = true;
                break;
            }
            case EQuadrantType::SouthEast:
            {
                DoorPriorities[(int)EDirectionType::West] = true;
                DoorPriorities[(int)EDirectionType::North] = true;
                break;
            }
            case EQuadrantType::SouthWest:
            {
                DoorPriorities[(int)EDirectionType::East] = true;
                DoorPriorities[(int)EDirectionType::North] = true;
                break;
            }
            case EQuadrantType::NorthWest:
            {
                DoorPriorities[(int)EDirectionType::East] = true;
                DoorPriorities[(int)EDirectionType::South] = true;
                break;
            }
            default: break;
        }
        for (int i = 0; i < (int)EDirectionType::NumDirectionTypes; ++i)
            if(DoorPriorities[i])
                if (neighbourPositions[i] != 0)
                    possibleDoors.Add((EDirectionType)i);
        if (possibleDoors.Num() <= 0)
            for (int i = 0; i < (int)EDirectionType::NumDirectionTypes; ++i)
                if(DoorPriorities[i])
                    if (neighbourPositions[i] == 0)
                        possibleDoors.Add((EDirectionType)i);
        if (possibleDoors.Num() > 1 && possibleDoors.Contains(PreviousDoorTarget))
            possibleDoors.Remove(PreviousDoorTarget);
        int doorIndex = FMath::RandRange(0, possibleDoors.Num() - 1);
        EDirectionType doorAction = possibleDoors[doorIndex];        
        int doorPositionOnWall = neighbourPositions[(int)possibleDoors[doorIndex]];
        PreviousDoorTarget = doorAction;
        UpdatePolicyForDoorType(doorAction, doorPositionOnWall);
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
