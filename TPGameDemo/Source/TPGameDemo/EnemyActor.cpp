// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TPGameDemoGameMode.h"
#include "TextParserComponent.h"
#include "EnemyActor.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogEnemyActor);

FString AEnemyActor::GetBehaviourString(EEnemyBehaviourState behaviourState)
{
    switch (behaviourState)
    {
        case EEnemyBehaviourState::Avoiding: return "AVOID";
        case EEnemyBehaviourState::ChangingRooms: return "CHANGE ROOMS";
        case EEnemyBehaviourState::Exploring: return "EXPLORE";
        default: return "None";
    }
}
//======================================================================================================
// Initialisation
//====================================================================================================== 
AEnemyActor::AEnemyActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    LogDir = FPaths::ProjectDir();
    LogDir += "Content/Logs/";
    LogDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*LogDir);
    LifetimeLog = GetNameSafe(this) + TEXT(" Lifetime:\n");
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

void AEnemyActor::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (SaveLifetimeLog)
        SaveLifetimeString();
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
        SetBehaviourState(EEnemyBehaviourState::Avoiding);
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
        //            SetBehaviourState(EEnemyBehaviourState::Exploring;
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
            SetBehaviourState(EEnemyBehaviourState::Exploring);
            UpdateMovement();
        }
    }
    //if (BehaviourState == EEnemyBehaviourState::Avoiding)
    //{
    //    if (GridXPosition == AvoidanceTarget.PositionInRoom.X && GridYPosition == AvoidanceTarget.PositionInRoom.Y && 
    //        CurrentRoomCoords == AvoidanceTarget.RoomCoords)
    //        TargetNearbyEmptyCell();
    //}

	// if (changingRooms)
	//   if (reached target)
	//		update position ...
	/*if (BehaviourState == EEnemyBehaviourState::ChangingRooms)
	{
        const bool onTargetGridPosition = TargetRoomPosition.Position.X == GridXPosition && TargetRoomPosition.Position.Y == GridYPosition;
        ensure(onTargetGridPosition || IsOnGridEdge());
        if (!onTargetGridPosition && !IsOnGridEdge())
        {
            SaveLifetimeLog = true;
            LogEvent("CHANGING ROOMS when not on target position or edge!", ELogEventType::Warning);
            CallEnteredNewRoom();
        }
	}
    if (TargetRoomPosition.DoorAction != EDirectionType::NumDirectionTypes &&
        BehaviourState == EEnemyBehaviourState::Exploring)
    {
        const bool onTargetGridPosition = TargetRoomPosition.Position.X == GridXPosition && TargetRoomPosition.Position.Y == GridYPosition;
        ensure(!onTargetGridPosition);
        if (onTargetGridPosition)
        {
            SaveLifetimeLog = true;
            LogEvent("REACHED TARGET without updating behaviour to CHANGE ROOMS!", ELogEventType::Warning);
            SetBehaviourState(EEnemyBehaviourState::ChangingRooms);
            UpdateMovementForActionType(TargetRoomPosition.DoorAction);
        }
    }*/
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    FRotator CurrentRotation = GetActorForwardVector().Rotation();
    FRotator ToTarget = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, MovementVector.Rotation());
    SetActorRotation (UKismetMathLibrary::RLerp(CurrentRotation, MovementVector.Rotation(), RotationSpeed/*Linear*/, true));
    AddMovementInput(MovementVector * MovementSpeed);
}

bool AEnemyActor::HasReachedTargetRoom() const
{
    return CurrentRoomCoords == TargetRoomCoords;
}

bool AEnemyActor::IsOnDoor(EDirectionType direction) const
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        TArray<int> neighbourPositions = gameState->GetDoorPositionsForExistingNeighbours(CurrentRoomCoords);
        switch (direction)
        {
        case EDirectionType::North: return GridXPosition == gameState->NumGridUnitsX - 1 && GridYPosition == neighbourPositions[(int)direction];
        case EDirectionType::East: return GridYPosition == gameState->NumGridUnitsY - 1 && GridXPosition == neighbourPositions[(int)direction];
        case EDirectionType::South: return GridXPosition == 0 && GridYPosition == neighbourPositions[(int)direction];
        case EDirectionType::West: return GridYPosition == 0 && GridXPosition == neighbourPositions[(int)direction];
        default: return false;
        }
    }
    return false;
}
//======================================================================================================
// Movement
//======================================================================================================
bool AEnemyActor::IsPositionValid()
{
	ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
	if (gameState != nullptr)
	{
		if (GridXPosition == 0)
			return GridYPosition == gameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::South);
		//if (GridXPosition == gameState->NumGridUnitsX)
			//return GridYPosition == gameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::North);
		if (GridYPosition == 0)
			return GridXPosition == gameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::West);
		//if (GridYPosition == gameState->NumGridUnitsY)
			//return GridXPosition == gameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::East);
		ensure(GridXPosition > 0 && GridXPosition < gameState->NumGridUnitsX && GridYPosition > 0 && GridYPosition < gameState->NumGridUnitsY);
        FDirectionSet optimalActions = gameState->GetOptimalActions(CurrentRoomCoords, TargetRoomPosition.Position, FIntPoint(GridXPosition, GridYPosition));
        return optimalActions.IsValid();
	}
	
    return false;
}

void AEnemyActor::PositionChanged()
{
    UE_LOG(LogEnemyActor, Display, TEXT("Position Changed"));

    AssertWithErrorLog(IsPositionValid(), TEXT("Position Invalid in PositionChanged!"));
	if (!IsPositionValid())
	{
		Destroy();
		return;
	}

    TimeSinceLastPositionChange = 0.0f;
    LogEvent("Position Change START", ELogEventType::Info);
    FString eventInfo = TEXT("");
    /*if (GridXPosition == TargetRoomPosition.Position.X && GridYPosition == TargetRoomPosition.Position.Y)
    {
        eventInfo += TEXT(" - reached target");
        if (TargetRoomPosition.DoorAction != EDirectionType::NumDirectionTypes && 
            BehaviourState == EEnemyBehaviourState::Exploring)
        {
            eventInfo += TEXT(" - has valid target");
            SetBehaviourState(EEnemyBehaviourState::ChangingRooms);
            UpdateMovementForActionType(TargetRoomPosition.DoorAction);
        }
    }
    else if (BehaviourState == EEnemyBehaviourState::Exploring)
    {
        eventInfo += TEXT(" - exploring");
        UpdateMovement();
    }
    else if (BehaviourState == EEnemyBehaviourState::ChangingRooms)
    {
        if (IsOnGridEdge())
        {
            eventInfo += TEXT(" - changing rooms on grid edge");
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
            eventInfo += TEXT(" - changing rooms, not reached target, not on edge - CallEnteredNewRoom()");
            CallEnteredNewRoom();
        }
    }
    else if (BehaviourState == EEnemyBehaviourState::Avoiding)
    {
        if (GridXPosition == AvoidanceTarget.PositionInRoom.X && GridYPosition == AvoidanceTarget.PositionInRoom.Y && 
            CurrentRoomCoords == AvoidanceTarget.RoomCoords)
            TargetNearbyEmptyCell();
    }*/
    if (IsOnGridEdge())
    {
        if (HasReachedTargetRoom())
            UpdateMovement();
        else
            UpdateMovementForActionType(TargetRoomPosition.DoorAction);
    }
    else if (WasOnGridEdge())
    {
        ChooseDoorTarget();
        UpdateMovement();
    }
    else
    {
        UpdateMovement();
    }
    LogEvent("Position Changed END (" + eventInfo + ")", ELogEventType::Info);
}

void AEnemyActor::RoomCoordsChanged()
{
    LogEvent("Room coords changed", ELogEventType::Info);
    UE_LOG(LogEnemyActor, Display, TEXT("Room Coords Changed"));
    if (HasReachedTargetRoom())
        EnteredTargetRoom();
    else
        ChooseDoorTarget();
    //// If we're not on the grid edge, call entered new room immediately.
    //if (!IsOnGridEdge())
    //{
    //    LogLine("enemy was NOT on edge - CallEnteredNewRoom()");
    //    CallEnteredNewRoom();
    //}
    //// otherwise if we're not changing rooms, direct ourselves towards one of the rooms from the doorway.
    //else //if (BehaviourState != EEnemyBehaviourState::ChangingRooms)
    //{
    //    ensure(BehaviourState == EEnemyBehaviourState::ChangingRooms);
    //    //ReachedGridEdgeWithoutChangingRooms();
    //}
}

void AEnemyActor::EnteredTargetRoom()
{
    TargetRoomPosition.DoorAction = EDirectionType::NumDirectionTypes;
    TargetRoomPosition.Position = FIntPoint(4, 4);
    UE_LOG(LogEnemyActor, Display, TEXT("Reached centre"));
    return;
}

void AEnemyActor::CallEnteredNewRoom()
{
    LogEvent("Entered new room", ELogEventType::Info);
    SetBehaviourState(EEnemyBehaviourState::Exploring);
    //you're testing if this is sufficient for enemies not getting stuck between rooms' ...
    if (TargetRoomPosition.DoorAction < EDirectionType::NumDirectionTypes)
        PreviousDoor = DirectionHelpers::GetOppositeDirection(TargetRoomPosition.DoorAction);
    ChooseDoorTarget();
    EnteredNewRoom();
}

void AEnemyActor::EnteredNewRoom_Implementation()
{
    SetBehaviourState(EEnemyBehaviourState::Exploring);
    ChooseDoorTarget();
}

void AEnemyActor::ReachedGridEdgeWithoutChangingRooms()
{
#pragma message("make sure in blueprint when the enemy has reached the center, it doesnt updatepolicyforcenter when it changes rooms on edge...")
    // Due to the way rooms overlap, if an enemy is inside a doorway, we can assume both rooms that use this door currently exist.
    SaveLifetimeLog = true;
    LogEvent("Reached grid edge without changing rooms", ELogEventType::Error);
    
}

void AEnemyActor::UpdateMovement()
{
	AssertWithErrorLog(IsPositionValid(), TEXT("Position invalid in UpdateMovement!"));
	if (!IsPositionValid())
	{
		Destroy();
		return;
	}

    /*if (!IsOnGridEdge())
    {*/
        EDirectionType actionType = SelectNextAction();
        LogLine("Selected action " + DirectionHelpers::GetDisplayString(actionType));
        UE_LOG(LogEnemyActor, Display, TEXT("Selected action %s"), *DirectionHelpers::GetDisplayString(actionType));
        UpdateMovementForActionType(actionType);
    //}
}

void AEnemyActor::SetBehaviourState(EEnemyBehaviourState newState)
{
    FString currentBehaviourString = GetBehaviourString(BehaviourState);
    FString newBehaviourString = GetBehaviourString(newState);
    FString changeString = "CHANGE BEHAVIOUR from " + currentBehaviourString + " to " + newBehaviourString;
    LogLine(changeString);
    if (newState != EEnemyBehaviourState::ChangingRooms)
    {
        //this sometimes fails.SetBehaviourState is called from various places ... check that each makes sense, and that the surrounding calls make sense ...?
        AssertWithErrorLog(!IsOnGridEdge(), TEXT("Behaviour is not set to CHANGE ROOMS but enemy is on edge!"));
    }
    BehaviourState = newState;
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
        //    SetBehaviourState(EEnemyBehaviourState::Avoiding);
        //    TargetRoomPosition.DoorAction = EDirectionType::NumDirectionTypes;
        //    UWorld* world = GetWorld();
        //    if (world != nullptr)
        //    {
        //        world->GetTimerManager().SetTimer (StopAvoidanceTimerHandle, [this]()
        //        { 
        //            ChooseDoorTarget();
        //            SetBehaviourState(EEnemyBehaviourState::Exploring);
        //            //ClearAvoidanceTimer();
        //            UpdateMovement();
        //        }, FMath::RandRange(2.5f, 4.5f), false);
        //    }
        //    TargetNearbyEmptyCell();
        //    return;
        //}
        
        LogEvent(TEXT("Move ") + DirectionHelpers::GetDisplayString(actionType) 
            + " to " + roomAndPosition.PositionInRoom.ToString() 
            + " in room " + roomAndPosition.RoomCoords.ToString(), ELogEventType::Info);

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
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        FDirectionSet optimalActions = gameState->GetOptimalActions(CurrentRoomCoords, TargetRoomPosition.Position, FIntPoint(GridXPosition, GridYPosition));
        return optimalActions.ChooseDirection();
    }

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
void AEnemyActor::UpdatePolicyForPlayerPosition (int targetX, int targetY)
{
    if (BehaviourState != EEnemyBehaviourState::Avoiding && !IsOnGridEdge())
    {
        LogEvent("Updating policy for player position", ELogEventType::Info);
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
        SetBehaviourState(EEnemyBehaviourState::Exploring);
        UpdateMovement();
    }
}

void AEnemyActor::UpdatePolicyForDoorType (EDirectionType doorType, int doorPositionOnWall)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        TargetRoomPosition.DoorAction = doorType;
        LogEvent("Updating policy for door type " + DirectionHelpers::GetDisplayString(doorType), ELogEventType::Info);
        switch (doorType)
        {
            case EDirectionType::North:
            {
                TargetRoomPosition.Position.X = gameState->NumGridUnitsX - 1;
                TargetRoomPosition.Position.Y = doorPositionOnWall;
                break;
            }
            case EDirectionType::East:
            {
                TargetRoomPosition.Position.X = doorPositionOnWall;
                TargetRoomPosition.Position.Y = gameState->NumGridUnitsY - 1;
                break;
            }
            case EDirectionType::South:
            {
                TargetRoomPosition.Position.X = 0;
                TargetRoomPosition.Position.Y = doorPositionOnWall;
                break;
            }
            case EDirectionType::West:
            {
                TargetRoomPosition.Position.X = doorPositionOnWall;
                TargetRoomPosition.Position.Y = 0;
                break;
            }
        }
        UpdateMovement();
    }
}

void AEnemyActor::ChooseDoorTarget()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
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
        //if (possibleDoors.Num() > 1 && possibleDoors.Contains(PreviousDoorTarget))
        //    possibleDoors.Remove(PreviousDoorTarget);
        if (possibleDoors.Num() > 1 && possibleDoors.Contains(PreviousDoor))
                possibleDoors.Remove(PreviousDoor);
        if (possibleDoors.Contains(PreviousDoor))
        {
            LogEvent(TEXT("Previous door left in action list"), ELogEventType::Warning);
        }
        if (possibleDoors.Num() > 1)
        {
            for (auto direction : possibleDoors)
            {
                if (IsOnDoor(direction))
                {
                    possibleDoors.Remove(direction);
                    break;
                }
            }
        }
        int doorIndex = FMath::RandRange(0, possibleDoors.Num() - 1);
        EDirectionType doorAction = possibleDoors[doorIndex];        
        int doorPositionOnWall = neighbourPositions[(int)possibleDoors[doorIndex]];
        //PreviousDoorTarget = doorAction;
        PreviousDoor = EDirectionType::NumDirectionTypes;
        LogEvent("Chose door position " + DirectionHelpers::GetDisplayString(doorAction), ELogEventType::Info);
        UE_LOG(LogEnemyActor, Display, TEXT("Chose door position %s"), *DirectionHelpers::GetDisplayString(doorAction));
        UpdatePolicyForDoorType(doorAction, doorPositionOnWall);

        //If the enemy enters a door on a corner which is also a door to a different room, we need to ensure their state is changed here.
        const bool onTargetGridPosition = TargetRoomPosition.Position.X == GridXPosition && TargetRoomPosition.Position.Y == GridYPosition;
        if (onTargetGridPosition)
        {
            LogLine(">>> Chose door at current grid position! <<<");
            SetBehaviourState(EEnemyBehaviourState::ChangingRooms);
            UpdateMovementForActionType(TargetRoomPosition.DoorAction);
        }
    }
}

void AEnemyActor::LogLine(const FString& lineString)
{
    if (!LifetimeLog.IsEmpty())
        LifetimeLog += "\n";
    LifetimeLog += lineString;
}

void AEnemyActor::LogRoom()
{
    LogLine(FString("Previous Room: ") + RoomAtLastEventLog.ToString() + FString(" | Room: ") + CurrentRoomCoords.ToString());
    RoomAtLastEventLog = CurrentRoomCoords;
}
void AEnemyActor::LogPosition()
{
    LogLine(TEXT("Previous GridPos: ") + PositionAtLastEventLog.ToString() + FString::Format(TEXT(" | GridPos: X={0} Y={1}"), { GridXPosition, GridYPosition }));
    PositionAtLastEventLog = FIntPoint(GridXPosition, GridYPosition);
}
void AEnemyActor::LogBehaviour()
{
    LogLine(FString("Previous Behaviour: ") + GetBehaviourString(BehaviourAtLastEventLog) + FString(" | Behaviour: ") + GetBehaviourString(BehaviourState));
    BehaviourAtLastEventLog = BehaviourState;
}
void AEnemyActor::LogTarget()
{
    LogLine("Previous " + TargetAtLastEventLog.ToInfoString() + " | " + TargetRoomPosition.ToInfoString());
    TargetAtLastEventLog = TargetRoomPosition;
}
void AEnemyActor::LogWorldPosition()
{
    LogLine("Prev World Pos: " + WorldPosAtLastEventLog.ToString() + " | World Pos: " + GetActorLocation().ToString() + " | Movement Target: " + MovementTarget.ToString());
    WorldPosAtLastEventLog = GetActorLocation();
}
void AEnemyActor::LogDetails()
{
    if (WorldPosAtLastEventLog != GetActorLocation())
    {
        LogWorldPosition();
    }
    if (RoomAtLastEventLog != CurrentRoomCoords)
    {
        LogPosition();
        LogRoom();
    }
    else if (PositionAtLastEventLog.X != GridXPosition || PositionAtLastEventLog.Y != GridYPosition)
    {
        LogPosition();
    }
    if (BehaviourAtLastEventLog != BehaviourState)
    {
        LogBehaviour();
    }
    if (TargetAtLastEventLog.DoorAction != TargetRoomPosition.DoorAction || TargetAtLastEventLog.Position != TargetRoomPosition.Position)
    {
        LogTarget();
    }
}
void AEnemyActor::LogEvent(const FString& eventInfo, ELogEventType logType)
{
    int seconds = 0;
    float partialSeconds = 0.0f;
    UGameplayStatics::GetAccurateRealTime(GetWorld(), seconds, partialSeconds);
    partialSeconds *= 1000.0f;
    switch (logType)
    {
    case ELogEventType::Info:
        LogLine(FString("----------------------------------------------------------------------------------"));
        LogLine(FString::Format(TEXT("{0} s, {1} ms"), { seconds, partialSeconds }));
        LogLine(FString("----------------------------------------------------------------------------------"));
        break;
    case ELogEventType::Warning:
        LogLine(FString("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"));
        LogLine(FString::Format(TEXT("{0} s, {1} ms"), { seconds, partialSeconds }));
        LogLine(FString("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"));
        break;
    case ELogEventType::Error:
        LogLine(FString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
        LogLine(FString::Format(TEXT("{0} s, {1} ms"), { seconds, partialSeconds }));
        LogLine(FString("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
        break;
    default: break;
    }
    
    LogDetails();
    LogLine(eventInfo);
}
void AEnemyActor::AssertWithErrorLog(const bool& condition, FString errorLog)
{
    if (!condition)
    {
        LogEvent(errorLog, ELogEventType::Error);
        SaveLifetimeString();
    }
    ensure(condition);
}

void AEnemyActor::SaveLifetimeString()
{
    if (LogDirFound)
    {
        FString filename = GetNameSafe(this) + "_lifetime.txt";
        FFileHelper::SaveStringToFile(LifetimeLog, *( LogDir + "/" + filename));
    }
}
