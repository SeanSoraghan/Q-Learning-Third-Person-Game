// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TPGameDemoGameMode.h"
#include "TextParserComponent.h"
#include "EnemyActor.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogEnemyActor);

//======================================================================================================
// Initialisation
//====================================================================================================== 
AEnemyActor::AEnemyActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
#if ENEMY_LIFETIME_LOGS
    LogDir = FPaths::ProjectDir();
    LogDir += "Content/Logs/";
    LogDirFound = FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*LogDir);
    LifetimeLog = GetNameSafe(this) + TEXT(" Lifetime:\n");
#endif
}

void AEnemyActor::BeginPlay()
{
	Super::BeginPlay();

  #if ON_SCREEN_DEBUGGING
    if ( ! LevelPoliciesDirFound)
	    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf (TEXT("Couldn't find level policies directory at %s"), *LevelPoliciesDir));
  #endif
}

void AEnemyActor::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
#if ENEMY_LIFETIME_LOGS
    if (SaveLifetimeLog)
        SaveLifetimeString();
#endif
}


//======================================================================================================
// Continuous Updating
//====================================================================================================== 
void AEnemyActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    if (PreviousActionWasInvalid)
        UpdateMovement();
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    FRotator CurrentRotation = GetActorForwardVector().Rotation();
    FRotator ToTarget = UKismetMathLibrary::NormalizedDeltaRotator(CurrentRotation, MovementVector.Rotation());
    SetActorRotation (UKismetMathLibrary::RLerp(CurrentRotation, MovementVector.Rotation(), RotationSpeed/*Linear*/, true));
    if (!(UndergoingImpulse()))
        AddMovementInput(MovementVector);
}

bool AEnemyActor::HasReachedTargetRoom() const
{
    return CurrentRoomCoords == TargetRoomAndPosition.RoomCoords;
}

bool AEnemyActor::HasReachedTargetPosition() const
{
    return TargetPositionAndAction.Position.X == GridXPosition && TargetPositionAndAction.Position.Y == GridYPosition;
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

EDirectionType AEnemyActor::SelectNextAction()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        FDirectionSet optimalActions = gameState->GetOptimalActions(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
        return optimalActions.ChooseDirection();
    }

    return EDirectionType::NumDirectionTypes;
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
        FDirectionSet optimalActions = gameState->GetOptimalActions(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
        return optimalActions.IsValid();
	}
	
    return false;
}

void AEnemyActor::PositionChanged()
{
#if ENEMY_LIFETIME_LOGS
    AssertWithErrorLog(IsPositionValid(), TEXT("Position Invalid in PositionChanged!"));
#else
    ensure(IsPositionValid());
#endif
	if (!IsPositionValid())
	{
		Destroy();
		return;
	}
#if ENEMY_LIFETIME_LOGS
    LogEvent("Position Change START", ELogEventType::Info);
#endif
    FString eventInfo = TEXT("");
    if (IsOnGridEdge())
    {
        if (HasReachedTargetPosition() && TargetPositionAndAction.DoorAction != EDirectionType::NumDirectionTypes)
        {
            UpdateMovementForActionType(TargetPositionAndAction.DoorAction);
        }
        else
        {
            if (!HasReachedTargetRoom())
                ChooseDoorTarget();
            UpdateMovement();
        }
    }
    else if (WasOnGridEdge() && !HasReachedTargetRoom())
    {
        ChooseDoorTarget();
        UpdateMovement();
    }
    else
    {
        UpdateMovement();
    }
#if ENEMY_LIFETIME_LOGS
    LogEvent("Position Changed END (" + eventInfo + ")", ELogEventType::Info);
#endif
}

void AEnemyActor::RoomCoordsChanged()
{
#if ENEMY_LIFETIME_LOGS
    LogEvent("Room coords changed", ELogEventType::Info);
#endif
    if (HasReachedTargetRoom())
        EnteredTargetRoom();
    else
        ChooseDoorTarget();
}

void AEnemyActor::EnteredTargetRoom()
{
    TargetPositionAndAction.DoorAction = EDirectionType::NumDirectionTypes;
    TargetPositionAndAction.Position = TargetRoomAndPosition.PositionInRoom;
    return;
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
// Behaviour Policy
//======================================================================================================
void AEnemyActor::TargetPositionInRoom(FIntPoint targetRoomCoords, FIntPoint targetPosition)
{
    TargetRoomAndPosition = { targetRoomCoords, targetPosition };
    if (!HasReachedTargetRoom())
        ChooseDoorTarget();
    else
        EnteredTargetRoom();
    UpdateMovement();
}

void AEnemyActor::TargetCenter()
{
    FIntPoint TargetRoom = FIntPoint(0, 0);
    FIntPoint TargetPosition = FIntPoint(4, 4);
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        TargetPosition = FIntPoint(gameState->NumGridUnitsX / 2, gameState->NumGridUnitsY / 2);
    }
    TargetPositionInRoom(TargetRoom, TargetPosition);
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
        if (possibleDoors.Num() > 1 && possibleDoors.Contains(PreviousDoor))
                possibleDoors.Remove(PreviousDoor);
#if ENEMY_LIFETIME_LOGS
        if (possibleDoors.Contains(PreviousDoor))
        {
            LogEvent(TEXT("Previous door left in action list"), ELogEventType::Warning);
        }
#endif
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
        if (possibleDoors.Num() <= 0)
        {
#if ENEMY_LIFETIME_LOGS
            LogEvent(TEXT("No available doors! Destroying enemy"), ELogEventType::Warning);
#endif
            Destroy();
            return;
        }
        int doorIndex = FMath::RandRange(0, possibleDoors.Num() - 1);
        EDirectionType doorAction = possibleDoors[doorIndex];        
        int doorPositionOnWall = neighbourPositions[(int)possibleDoors[doorIndex]];
        //PreviousDoorTarget = doorAction;
        PreviousDoor = EDirectionType::NumDirectionTypes;
#if ENEMY_LIFETIME_LOGS
        LogEvent("Chose door position " + DirectionHelpers::GetDisplayString(doorAction), ELogEventType::Info);
#endif
        UpdatePolicyForDoorType(doorAction, doorPositionOnWall);

        //If the enemy enters a door on a corner which is also a door to a different room, we need to ensure their state is changed here.
        const bool onTargetGridPosition = TargetPositionAndAction.Position.X == GridXPosition && TargetPositionAndAction.Position.Y == GridYPosition;
        if (onTargetGridPosition)
        {
#if ENEMY_LIFETIME_LOGS
            LogLine(">>> Chose door at current grid position! <<<");
#endif
            UpdateMovementForActionType(TargetPositionAndAction.DoorAction);
        }
    }
}

void AEnemyActor::UpdateMovementForActionType(EDirectionType actionType)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        const float z = GetActorLocation().Z;
        FRoomPositionPair roomAndPosition = GetTargetRoomAndPositionForDirectionType(actionType);
#if ENEMY_LIFETIME_LOGS
        LogEvent(TEXT("Move ") + DirectionHelpers::GetDisplayString(actionType)
            + " to " + roomAndPosition.PositionInRoom.ToString()
            + " in room " + roomAndPosition.RoomCoords.ToString(), ELogEventType::Info);
#endif
        if (roomAndPosition.RoomCoords == CurrentRoomCoords)
        {
            ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
            if (gameState != nullptr)
            {
                PreviousActionWasInvalid = gameState->SimulateAction({ CurrentRoomCoords, {GridXPosition, GridYPosition } }, actionType, TargetPositionAndAction.Position);
            }
        }
        else
        {
            PreviousActionWasInvalid = false;
        }
        FVector2D targetXY = gameState->GetWorldXYForRoomAndPosition(roomAndPosition);
        MovementTarget = FVector(targetXY.X, targetXY.Y, z);
    }
}

void AEnemyActor::UpdatePolicyForDoorType(EDirectionType doorType, int doorPositionOnWall)
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        TargetPositionAndAction.DoorAction = doorType;
#if ENEMY_LIFETIME_LOGS
        LogEvent("Updating policy for door type " + DirectionHelpers::GetDisplayString(doorType), ELogEventType::Info);
#endif
        switch (doorType)
        {
        case EDirectionType::North:
        {
            TargetPositionAndAction.Position.X = gameState->NumGridUnitsX - 1;
            TargetPositionAndAction.Position.Y = doorPositionOnWall;
            break;
        }
        case EDirectionType::East:
        {
            TargetPositionAndAction.Position.X = doorPositionOnWall;
            TargetPositionAndAction.Position.Y = gameState->NumGridUnitsY - 1;
            break;
        }
        case EDirectionType::South:
        {
            TargetPositionAndAction.Position.X = 0;
            TargetPositionAndAction.Position.Y = doorPositionOnWall;
            break;
        }
        case EDirectionType::West:
        {
            TargetPositionAndAction.Position.X = doorPositionOnWall;
            TargetPositionAndAction.Position.Y = 0;
            break;
        }
        }
        UpdateMovement();
    }
}

void AEnemyActor::UpdateMovement()
{
#if ENEMY_LIFETIME_LOGS
    AssertWithErrorLog(IsPositionValid(), TEXT("Position invalid in UpdateMovement!"));
#else
    ensure(IsPositionValid());
#endif
    if (!IsPositionValid())
    {
        Destroy();
        return;
    }

    EDirectionType actionType = SelectNextAction();
#if ENEMY_LIFETIME_LOGS
    LogLine("Selected action " + DirectionHelpers::GetDisplayString(actionType));
#endif
    UpdateMovementForActionType(actionType);
}

#if ENEMY_LIFETIME_LOGS
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
void AEnemyActor::LogTarget()
{
    LogLine("Previous " + TargetAtLastEventLog.ToInfoString() + " | " + TargetPositionAndAction.ToInfoString());
    TargetAtLastEventLog = TargetPositionAndAction;
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
    if (TargetAtLastEventLog.DoorAction != TargetPositionAndAction.DoorAction || TargetAtLastEventLog.Position != TargetPositionAndAction.Position)
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
#endif
