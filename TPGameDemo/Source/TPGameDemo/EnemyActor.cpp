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
    GameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();

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
    if (GameState != nullptr)
    {
        TArray<int> neighbourPositions = GameState->GetDoorPositionsForExistingNeighbours(CurrentRoomCoords);
        switch (direction)
        {
        case EDirectionType::North: return GridXPosition == GameState->NumGridUnitsX - 1 && GridYPosition == neighbourPositions[(int)direction];
        case EDirectionType::East: return GridYPosition == GameState->NumGridUnitsY - 1 && GridXPosition == neighbourPositions[(int)direction];
        case EDirectionType::South: return GridXPosition == 0 && GridYPosition == neighbourPositions[(int)direction];
        case EDirectionType::West: return GridYPosition == 0 && GridXPosition == neighbourPositions[(int)direction];
        default: return false;
        }
    }
    return false;
}

EDirectionType AEnemyActor::SelectNextAction()
{
    if (GameState != nullptr)
    {
        FDirectionSet optimalActions = GameState->GetOptimalActions(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
        /*float exploreProbability = GameState->GetExploreProbability(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
        if (FMath::FRand() < exploreProbability)
        {
            GameState->IncrementExploreCount(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
            return optimalActions.GetInverse().ChooseDirection();
        }*/
        return optimalActions.ChooseDirection();
    }

    return EDirectionType::NumDirectionTypes;
}
//======================================================================================================
// Movement
//======================================================================================================
bool AEnemyActor::IsPositionValid()
{
	if (GameState != nullptr)
	{
		if (GridXPosition == 0)
			return GridYPosition == GameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::South);
		//if (GridXPosition == GameState->NumGridUnitsX)
			//return GridYPosition == GameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::North);
		if (GridYPosition == 0)
			return GridXPosition == GameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::West);
		//if (GridYPosition == GameState->NumGridUnitsY)
			//return GridXPosition == GameState->GetDoorPositionOnWall(CurrentRoomCoords, EDirectionType::East);
		ensure(GridXPosition > 0 && GridXPosition < GameState->NumGridUnitsX && GridYPosition > 0 && GridYPosition < GameState->NumGridUnitsY);
        FDirectionSet optimalActions = GameState->GetOptimalActions(CurrentRoomCoords, TargetPositionAndAction.Position, FIntPoint(GridXPosition, GridYPosition));
        return optimalActions.IsValid();
	}
	
    return false;
}

FVector AEnemyActor::GetNormalizedMovementVector()
{
    FVector MovementVector = MovementTarget - GetActorLocation();
    MovementVector.Normalize();
    return MovementVector;
}

void AEnemyActor::PositionChanged()
{
    /*std::set<RoomPosition>::iterator it = VisitedPositions.find({ GameState->NumGridUnitsX, { GridXPosition, GridYPosition } });
    if (it != VisitedPositions.end())
        PrevActionAccumulatedReward += GridTrainingConstants::LoopCost;
    else*/
    VisitedPositions.insert({ GameState->NumGridUnitsX, { GridXPosition, GridYPosition } });

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
            bool movementTargetUpdated = false;
            if (!HasReachedTargetRoom())
                ChooseDoorTarget(movementTargetUpdated);
            if (!movementTargetUpdated)
                UpdateMovement();
        }
    }
    else if (WasOnGridEdge() && !HasReachedTargetRoom())
    {
        bool movementTargetUpdated = false;
        ChooseDoorTarget(movementTargetUpdated);
        if (!movementTargetUpdated)
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
    {
        EnteredTargetRoom();
    }
    else
    {
        bool dummyMovementTargetUpdated = false;
        ChooseDoorTarget(dummyMovementTargetUpdated);
    }
}

void AEnemyActor::TakeDamage(float damageAmount)
{
    AMazeActor::TakeDamage(damageAmount);
    if (AccumulateReward && !HasReachedTargetRoom())
        PrevActionAccumulatedReward += GridTrainingConstants::DamageCost;
}

void AEnemyActor::EnteredTargetRoom()
{
    SetTargetPositionAndAction({ TargetRoomAndPosition.PositionInRoom, EDirectionType::NumDirectionTypes });
}

void AEnemyActor::ActorDied()
{
    if (AccumulateReward)
        PrevActionAccumulatedReward += GridTrainingConstants::DamageCost;
    if (ShouldUpdateQValue())
        GameState->UpdateQValueRealtime(PrevActionStartPos, PrevActionType, PrevActionTarget, PrevActionAccumulatedReward, GridTrainingConstants::ActorLearningRate);
}

void AEnemyActor::SetTargetPositionAndAction(FTargetPosition newTarget)
{
    VisitedPositions.clear();
    TargetPositionAndAction = newTarget;
}

FRoomPositionPair AEnemyActor::GetTargetRoomAndPositionForDirectionType(EDirectionType actionType)
{
    //0 - 9, S - N, W - E.
    FRoomPositionPair start = {CurrentRoomCoords, FIntPoint(GridXPosition, GridYPosition)};
    if (GameState != nullptr)
        return GameState->GetTargetRoomAndPositionForDirectionType(start, actionType);

    return start;
}

//======================================================================================================
// Behaviour Policy
//======================================================================================================
void AEnemyActor::TargetPositionInRoom(FIntPoint targetRoomCoords, FIntPoint targetPosition)
{
    TargetRoomAndPosition = { targetRoomCoords, targetPosition };
    bool movementTargetUpdated = false;
    if (!HasReachedTargetRoom())
        ChooseDoorTarget(movementTargetUpdated);
    else
        EnteredTargetRoom();
    if (!movementTargetUpdated)
        UpdateMovement();
}

void AEnemyActor::TargetCenter()
{
    FIntPoint TargetRoom = FIntPoint(0, 0);
    FIntPoint TargetPosition = FIntPoint(4, 4);
    if (GameState != nullptr)
    {
        TargetPosition = FIntPoint(GameState->NumGridUnitsX / 2, GameState->NumGridUnitsY / 2);
    }
    TargetPositionInRoom(TargetRoom, TargetPosition);
}

void AEnemyActor::ChooseDoorTarget(bool& movementTargetUpdated)
{
    movementTargetUpdated = false;
    if (GameState != nullptr)
    {
        TArray<int> neighbourPositions = GameState->GetDoorPositionsForExistingNeighbours(CurrentRoomCoords);
        EQuadrantType quadrant = GameState->GetQuadrantTypeForRoomCoords(CurrentRoomCoords);
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
            movementTargetUpdated = true;
        }
    }
}

bool AEnemyActor::ShouldUpdateQValue() const
{
    return UpdateQValue && GameState != nullptr && PrevActionType != EDirectionType::NumDirectionTypes /*&& PrevActionAccumulatedReward != 0.0f*/;
}

void AEnemyActor::UpdateMovementForActionType(EDirectionType actionType, int numCalls /*= 0*/)
{
#if ENEMY_LIFETIME_LOGS
    if (numCalls > 0)
        SaveLifetimeLog = true;
#endif
    ensure(numCalls < (int)EDirectionType::NumDirectionTypes);
    if (numCalls >= (int)EDirectionType::NumDirectionTypes)
        return;

    if (GameState != nullptr)
    {
        const float z = GetActorLocation().Z;
        FRoomPositionPair targetRoomAndPosition = GetTargetRoomAndPositionForDirectionType(actionType);

#if ENEMY_LIFETIME_LOGS
        LogLine("Simulate action " + DirectionHelpers::GetDisplayString(actionType));
#endif
        FRoomPositionPair simulationResult = { CurrentRoomCoords, {GridXPosition, GridYPosition } };
        bool simulationSuccessful = GameState->SimulateAction(simulationResult, actionType, TargetPositionAndAction.Position);
#if ENEMY_LIFETIME_LOGS
        LogLine("Simulate " + DirectionHelpers::GetDisplayString(actionType) + " returned " 
            + simulationResult.PositionInRoom.ToString() + " in room " + simulationResult.RoomCoords.ToString());
#endif
        if (!simulationSuccessful)
        {
            if (ShouldUpdateQValue())
                GameState->UpdateQValueRealtime(PrevActionStartPos, PrevActionType, PrevActionTarget, PrevActionAccumulatedReward, GridTrainingConstants::ActorLearningRate);
            actionType = (EDirectionType)(((int)actionType + 1) % (int)EDirectionType::NumDirectionTypes);
            PrevActionStartPos = { CurrentRoomCoords, {GridXPosition, GridYPosition} };
            PrevActionType = actionType;
            PrevActionTarget = TargetPositionAndAction.Position;
            PrevActionAccumulatedReward = 0.0f;
            return UpdateMovementForActionType(actionType, numCalls + 1);
        }

#pragma message("this target might not agree with the training target in SimulateAction ...")
#if ENEMY_LIFETIME_LOGS
        LogEvent(TEXT("Move ") + DirectionHelpers::GetDisplayString(actionType)
            + " to " + targetRoomAndPosition.PositionInRoom.ToString()
            + " in room " + targetRoomAndPosition.RoomCoords.ToString(), ELogEventType::Info);
#endif
        if (ShouldUpdateQValue())
        {
            GameState->UpdateQValueRealtime(PrevActionStartPos, PrevActionType, PrevActionTarget, PrevActionAccumulatedReward, GridTrainingConstants::ActorLearningRate);
        }
        PrevActionStartPos = { CurrentRoomCoords, {GridXPosition, GridYPosition} };
        PrevActionType = actionType;
        PrevActionTarget = TargetPositionAndAction.Position;
        PrevActionAccumulatedReward = 0.0f;
        FVector2D targetXY = GameState->GetWorldXYForRoomAndPosition(targetRoomAndPosition);
        MovementTarget = FVector(targetXY.X, targetXY.Y, z);
    }
}

void AEnemyActor::UpdatePolicyForDoorType(EDirectionType doorType, int doorPositionOnWall)
{
    if (GameState != nullptr)
    {
#if ENEMY_LIFETIME_LOGS
        LogEvent("Updating policy for door type " + DirectionHelpers::GetDisplayString(doorType), ELogEventType::Info);
#endif
        switch (doorType)
        {
        case EDirectionType::North:
        {
            SetTargetPositionAndAction({ {GameState->NumGridUnitsX - 1, doorPositionOnWall}, doorType });
            break;
        }
        case EDirectionType::East:
        {
            SetTargetPositionAndAction({ {doorPositionOnWall, GameState->NumGridUnitsY - 1}, doorType });
            break;
        }
        case EDirectionType::South:
        {
            SetTargetPositionAndAction({ {0, doorPositionOnWall}, doorType });
            break;
        }
        case EDirectionType::West:
        {
            SetTargetPositionAndAction({ {doorPositionOnWall, 0}, doorType });
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
