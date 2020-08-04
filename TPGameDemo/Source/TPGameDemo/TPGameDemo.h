// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <set>
#include "Engine.h"
#include "TPGameDemo.generated.h"

#define ON_SCREEN_DEBUGGING 0
#define DELTA_Q_CONVERGENCE_THRESHOLD 0.01f
#define CONVERGENCE_NUM_ACTIONS_MIN 100
#define CONVERGENCE_NUM_ACTIONS_MAX 300
#define NUM_TRAINING_SIMULATIONS 50
#define MAX_NUM_MOVEMENTS_PER_SIMULATION 100

UENUM(BlueprintType)
enum class EDirectionType : uint8
{
    North UMETA (DisplayName = "North"),
    East  UMETA (DisplayName = "East"),
    South UMETA (DisplayName = "South"),
    West  UMETA (DisplayName = "West"),
    NumDirectionTypes UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class EQuadrantType : uint8
{
    NorthEast,
    SouthEast,
    SouthWest,
    NorthWest,
    NumQuadrants
};

namespace DirectionHelpers
{
    EDirectionType GetOppositeDirection(EDirectionType direction);
    FString GetDisplayString(EDirectionType direction);
};

namespace Delimiters
{
    const FString DirectionSetStringDelimiter = " ";
    const FString ActionDelimiter = "_";
};

USTRUCT(BlueprintType)
struct FDirectionSet
{
    GENERATED_USTRUCT_BODY()

    // For easy construction
    enum class EDirectionFlag : uint8
    {
        NorthF = 1 << 0,
        EastF = 1 << 1,
        SouthF = 1 << 2,
        WestF = 1 << 3,
        NumDirectionFlags = 1 << 4
    };

    FDirectionSet()
    {
        DirectionSelectionSet.clear();
    }

    FDirectionSet(uint8 directionsMask) : DirectionsMask(directionsMask) 
    {
        UpdateSelectionSet();
    }

    EDirectionType ChooseDirection() 
    {
        if (!IsValid())
            return EDirectionType::NumDirectionTypes;

        float r = rand() / (float)RAND_MAX;
        int choice = r == 1.0f ? DirectionSelectionSet.size() - 1 : (int)(r * DirectionSelectionSet.size());
        std::set<EDirectionType>::iterator it = DirectionSelectionSet.begin();
        advance(it, choice);
        ensure((int)*it >= (int)EDirectionType::North && (int)*it < (int)EDirectionType::NumDirectionTypes);
        return *it;
    }

    bool IsValid() { return DirectionsMask & ((uint8)pow(2, (int)EDirectionType::NumDirectionTypes) - 1); }

    bool CheckDirection(EDirectionType direction) { return DirectionsMask & (1 << (int)direction); }
    
    void Clear()
    {
        DirectionsMask = 0;
        DirectionSelectionSet.clear();
    }

    void EnableDirection(EDirectionType direction) 
    { 
        DirectionsMask |= (1 << (int)direction);
        DirectionSelectionSet.insert(direction);
    }
    void DisableDirection(EDirectionType direction) 
    { 
        DirectionsMask &= (~(1 << (int)direction));
        DirectionSelectionSet.erase(direction);
    }

    /** return a copy if all directions are valid. */
    FDirectionSet GetInverse()
    {
        if (DirectionSelectionSet.size() == (int)EDirectionType::NumDirectionTypes)
        {
            return FDirectionSet(DirectionsMask);
        }
        return FDirectionSet(~DirectionsMask);
    }

    FString ToString(bool padSpaces = false) 
    {
        if (!IsValid())
            return FString("-1");
        FString str = "";
        if (CheckDirection(EDirectionType::North))
            str += FString::FromInt((int)EDirectionType::North);
        else if (padSpaces)
            str += " ";
        for (int d = (int)EDirectionType::East; d < (int)EDirectionType::NumDirectionTypes; ++d)
        {
            if (CheckDirection((EDirectionType)d))
                str += Delimiters::ActionDelimiter + FString::FromInt(d);
            else if (padSpaces)
                str += Delimiters::ActionDelimiter + " ";
        }
        return str;
    }

    uint8 DirectionsMask = 0;
    std::set<EDirectionType> DirectionSelectionSet;

private:
    void UpdateSelectionSet()
    {
        DirectionSelectionSet.clear();
        for (int d = 0; d < (int)EDirectionType::NumDirectionTypes; ++d)
            if (CheckDirection((EDirectionType)d))
                DirectionSelectionSet.insert((EDirectionType)d);
    }
};

USTRUCT(Blueprintable)
struct FRoomPositionPair
{
    GENERATED_USTRUCT_BODY()
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rooms And Positions")
        FIntPoint RoomCoords;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rooms And Positions")
        FIntPoint PositionInRoom;
};

UENUM(BlueprintType)
enum class EDoorState : uint8
{
    Open,
    Closed,
    Locked,
    Opening,
    Closing,
    NumStates
};

UENUM(BlueprintType)
enum class ECellState : uint8
{
    Open UMETA (DisplayName = "Open"),
    Closed  UMETA (DisplayName = "Closed"),
    Door UMETA (DisplayName = "Door"),
    NumStates
};

struct WallState
{

    WallState() {}
    ~WallState() {}

    void InitializeWall()
    {
        bWallExists = true;
        bDoorExists = true;
    }

    void DisableWall()
    {
        bWallExists = false;
        bDoorExists = false;
    }

    void InitializeDoor()
    {
        bDoorExists = true;
    }

    void DisableDoor()
    {
        if (DoorState != EDoorState::Locked)
        {
            DoorState = EDoorState::Open;
            bDoorExists = false;
        }
    }

    void LockDoor()
    {
        DoorState = EDoorState::Locked;
    }

    void UnlockDoor()
    {
        DoorState = EDoorState::Closed;
    }

    bool HasDoor() const
    {
        return DoorPosition != -1;
    }

    void GenerateRandomDoorPosition(int doorPositionMax)
    {
        DoorPosition = FMath::RandRange(1, doorPositionMax);
    }

    EDoorState DoorState = EDoorState::Closed;
    int DoorPosition = -1;
    bool bWallExists = false;
    bool bDoorExists = false;
};

// The game state manages a 2D array of WallStateCouple structs that is the same size as the 2D rooms array.
struct WallStateCouple
{
    WallState WestWall;
    WallState SouthWall;
};



namespace
{
    typedef TArray<TArray<FDirectionSet>> BehaviourMap;
    typedef TArray<TArray<BehaviourMap>> TargetMapsArray;
    void InitialiseBehaviourMap(BehaviourMap& behaviourMap, int numX, int numY)
    {
        for (int x = 0; x < numX; ++x)
        {
            behaviourMap.Add(TArray<FDirectionSet>());
            behaviourMap[x].AddDefaulted(numY);
        }
    }
    void InitialiseTargetMapsArray(TargetMapsArray& targetMaps, int numX, int numY)
    {
        for (int x = 0; x < numX; ++x)
        {
            targetMaps.Add(TArray<BehaviourMap>());
            for (int y = 0; y < numY; ++y)
            {
                targetMaps[x].Add(BehaviourMap());
                InitialiseBehaviourMap(targetMaps[x][y], numX, numY);
            }
        }
    }
};

namespace LevelBuilderHelpers
{
    const FString LevelsDir();

    FIntPoint GetTargetPointForAction(FIntPoint startingPoint, EDirectionType actionType, int numSpaces = 1);

    bool GridPositionIsValid(FIntPoint position, int sizeX, int sizeY);

    /*
    Takes in a text file and fills an array with FDirectionSets.
    File should be a square grid format with FDirectionSets separated by spaces.
    If invertX is true, the lines in the text file will be entered from bottom to top
    into the 2D int array (rather than top to bottom).
    */
    void FillArrayFromTextFile (FString fileName, TArray<TArray<FDirectionSet>>& arrayRef, bool invertX = false);

    /*
    Writes a 2D FDirectionSet array to a text file. File will have a square grid format with FDirectionSets 
    separated by spaces. If invertX is true, the lines in the text file will be entered 
    from bottom to top (rather than top to bottom).
    */
    void WriteArrayToTextFile (TArray<TArray<FDirectionSet>>& arrayRef, FString fileName, bool invertX = false);

    /*
    Takes in a text file and fills an array with ints.
    File should be a square grid format with ints separated by spaces.
    If invertX is true, the lines in the text file will be entered from bottom to top
    into the 2D int array (rather than top to bottom).
    */
    void FillArrayFromTextFile(FString fileName, TArray<TArray<int>>& arrayRef, bool invertX = false);

    /*
    Writes a 2D int array to a text file. File will have a square grid format with ints
    separated by spaces. If invertX is true, the lines in the text file will be entered
    from bottom to top (rather than top to bottom).
    */
    void WriteArrayToTextFile(TArray<TArray<int>>& arrayRef, FString fileName, bool invertX = false);

    void PrintArray(TArray<TArray<FDirectionSet>>& arrayRef);
    void PrintArray(TArray<TArray<int>>& arrayRef);
};

//====================================================================================================
// NavigationState State
//====================================================================================================

namespace GridTrainingConstants
{
    static const float GoalReward = 1.0f;
    static const float MovementCost = -0.04f;
    static const float LoopCost = -1.0f;
    static const float DamageCost = -1.0f;
    static const float SimLearningRate = 0.5f;
    static const float ActorLearningRate = 0.5f;
    static const float SimDiscountFactor = 0.9f;
    static const float ActorDiscountFactor = 0.9f;
    /* The chances of exploration from a certain position for a certain target will get smaller and smaller until this many explorations have been carried out, 
    at which point exploration will never occur.*/
    static const float ExploreCount = 100.0f;
};

/* Action targets for actions taken from a given position. Action targets are FIntPoint positions. Actions are North, East, South, West. */
class NavPositionState
{
public:
    void SetIsGoal(bool isGoal);
    bool IsGoalState() const;

    FRoomPositionPair GetActionTarget(EDirectionType actionType) const;

    void SetValid(bool valid);
    bool IsStateValid() const;

    void SetActionTarget(EDirectionType actionType, FRoomPositionPair roomAndPosition);

private:
    bool IsGoal = false;
    bool IsValid = true;
    
    TArray<FRoomPositionPair> ActionTargets{ {{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}}, {{0,0},{0,0}} };
};

/* QLearning qvalues and rewards for actions taken from a position in a room (for a specific target). Actions are North, East, South, West. */
class NavigationState
{
public:
    struct RewardTracker
    {
        float GetAverage() const
        {
            return AverageReward;
        }

        void AddObservation(float value)
        {
            if (ObservationCount >= MAX_FLT - 1.0f)
            {
                ObservationCount = 1.0f;
                ObservationSum = value;
                return;
            }
            ++ObservationCount;
            ObservationSum += value;
            AverageReward = ObservationSum / ObservationCount;
        }

    private:
        float ObservationSum = 0.0f;
        float ObservationCount = 0.0f;
        float AverageReward = 0.0f;
    };

    const TArray<float> GetQValues() const;
    const TArray<float> GetRewards() const;

    const float GetOptimalQValueAndActions(FDirectionSet& Actions) const;

    const float GetOptimalQValueAndActions_Valid(FDirectionSet& Actions) const;

    void UpdateQValue(EDirectionType actionType, float learningRate, float deltaQ);
    void ResetQValues();

    void SetActionReward(EDirectionType movementDirection, float reward)
    {
        ActionRewards[(int)movementDirection] = reward;
    }

    void SetAsGoal()
    {
        ActionRewards = { GridTrainingConstants::GoalReward, GridTrainingConstants::GoalReward,
                          GridTrainingConstants::GoalReward, GridTrainingConstants::GoalReward };
    }

    void AddActionRewardObservation(EDirectionType action, float reward)
    {
        ActionRewardTrackers[(int)action].AddObservation(reward);
    }

    void IncrementExplorations() { ++NumExplorations; }
    float GetExploreProbability() const { return FMath::Clamp(1.0f - (NumExplorations / GridTrainingConstants::ExploreCount), 0.0f, 1.0f); }
private:
    TArray<float> ActionQValues{ 0.0f, 0.0f, 0.0f, 0.0f };
    TArray<float> ActionRewards{ GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost,
                                 GridTrainingConstants::MovementCost, GridTrainingConstants::MovementCost };

    TArray<RewardTracker> ActionRewardTrackers{RewardTracker(), RewardTracker(), RewardTracker(), RewardTracker()};
    float NumExplorations = (float)NUM_TRAINING_SIMULATIONS;
};

namespace
{
    /* Nav position states for each position in a room. */
    typedef TArray<TArray<NavPositionState>> NavigationEnvironment;
    /* Navigation states for each position in a room (for a fixed target position). */
    typedef TArray<TArray<NavigationState>> NavigationSet;
    /* Navigation states for each position in a room for each target position in the room. */
    typedef TArray<TArray<NavigationSet>> RoomTargetsNavSets;

    NavPositionState& GetPosState(NavigationEnvironment& navEnvironment, FIntPoint position) { return navEnvironment[position.X][position.Y]; }
    NavigationState& GetNavState(NavigationSet& navSet, FIntPoint position) { return navSet[position.X][position.Y]; }
    NavigationSet& GetRoomNavSet(RoomTargetsNavSets& roomNavSets, FIntPoint goalPosition) { return roomNavSets[goalPosition.X][goalPosition.Y]; }
    NavigationState& GetNavStateForGoalPosition(RoomTargetsNavSets& roomNavSets, FIntPoint goalPosition, FIntPoint position)
    {
        return GetNavState(GetRoomNavSet(roomNavSets, goalPosition), position);
    }

    void InitialiseNavEnvironment(NavigationEnvironment& navSet, int numX, int numY)
    {
        for (int x = 0; x < numX; ++x)
        {
            navSet.Add(TArray<NavPositionState>());
            navSet[x].AddDefaulted(numY);
        }
    }
    void InitialiseNavigationSet(NavigationSet& navSet, int numX, int numY)
    {
        for (int x = 0; x < numX; ++x)
        {
            navSet.Add(TArray<NavigationState>());
            navSet[x].AddDefaulted(numY);
        }
    }
    void InitialiseRoomTargetsNavSets(RoomTargetsNavSets& targetNavSets, int numX, int numY)
    {
        for (int x = 0; x < numX; ++x)
        {
            targetNavSets.Add(TArray<NavigationSet>());
            for (int y = 0; y < numY; ++y)
            {
                targetNavSets[x].Add(NavigationSet());
                InitialiseNavigationSet(targetNavSets[x][y], numX, numY);
                if (x > 0)
                    targetNavSets[x][y][x - 1][y].SetActionReward(EDirectionType::North, GridTrainingConstants::GoalReward);
                if (y > 0)
                    targetNavSets[x][y][x][y - 1].SetActionReward(EDirectionType::East, GridTrainingConstants::GoalReward);
                if (x < numX - 1)
                    targetNavSets[x][y][x + 1][y].SetActionReward(EDirectionType::South, GridTrainingConstants::GoalReward);
                if (y < numY - 1)
                    targetNavSets[x][y][x][y + 1].SetActionReward(EDirectionType::West, GridTrainingConstants::GoalReward);
                //targetNavSets[x][y][x][y].SetAsGoal();
            }
        }
    }

    NavigationEnvironment GetNavigationEnvironmentForRoom(TArray<TArray<int>> roomStructure, FIntPoint roomCoords)
    {
        const int sizeX = roomStructure.Num();
        const int sizeY = roomStructure[0].Num();
        NavigationEnvironment navEnvironment;
        for (int x = 0; x < sizeX; ++x)
        {
            navEnvironment.Add(TArray<NavPositionState>());
            TArray<NavPositionState>& row = navEnvironment[x];
            for (int y = 0; y < sizeY; ++y)
            {
                row.Add(NavPositionState());
                NavPositionState& state = row[y];
                state.SetValid(roomStructure[x][y] == (int)ECellState::Open || roomStructure[x][y] == (int)ECellState::Door);
                if (state.IsStateValid())
                {
                    for (int a = 0; a < (int)EDirectionType::NumDirectionTypes; ++a)
                    {
                        EDirectionType actionType = EDirectionType(a);
                        FIntPoint targetPoint = LevelBuilderHelpers::GetTargetPointForAction(FIntPoint(x, y), actionType);

                        FRoomPositionPair actionTarget = {roomCoords, targetPoint};

                        const bool targetValid = LevelBuilderHelpers::GridPositionIsValid(targetPoint, sizeX, sizeY) &&
                            (roomStructure[targetPoint.X][targetPoint.Y] == (int)ECellState::Open || roomStructure[targetPoint.X][targetPoint.Y] == (int)ECellState::Door);

                        if (!targetValid)
                            actionTarget = {roomCoords, FIntPoint(x, y) };
                        
                        state.SetActionTarget(actionType, actionTarget);
                    }
                }
            }
        }
        return navEnvironment;
    }
};

struct RoomState
{
    enum Status : uint8
    {
        Dead,
        Training,
        Trained,
        Connected
    };

    RoomState() 
    {}

    RoomState(FIntPoint roomDimensions)
    {
        InitialiseRoomTargetsNavSets(TargetNavSets, roomDimensions.X, roomDimensions.Y);
        for (int x = 0; x < roomDimensions.X; ++x)
        {
            TArray<FThreadSafeCounter> states;
            states.AddDefaulted(roomDimensions.Y);
            TileActorCounters.Add(states);
        }
    }

    ~RoomState()
    {}

    void InitializeRoom(float health, float complexity = 0.0f, float density = 0.0f)
    {
        RoomStatus = Training;
        TrainingProgress = 0.0f;
        RoomHealth = health;
        Complexity = complexity;
        Density = density;
    }

    void SetRoomTrained()
    {
        RoomStatus = Trained;
    }

    void DisableRoom();


    void SetRoomConnected()
    {
        RoomStatus = Connected;
    }

    bool RoomExists() const
    {
        return RoomStatus != Dead;
    }

    bool IsRoomConnected() const
    {
        return RoomStatus == Connected;
    }

    bool TileIsEmpty(FIntPoint TilePosition) const
    {
        return TileActorCounters[TilePosition.X][TilePosition.Y].GetValue() == 0;
    }

    void ActorEnteredTilePosition(FIntPoint TilePosition)
    {
        TileActorCounters[TilePosition.X][TilePosition.Y].Increment();
    }

    void ActorExitedTilePosition(FIntPoint TilePosition)
    {
        TileActorCounters[TilePosition.X][TilePosition.Y].Decrement();
    }

    void SetNavSetForTarget(FIntPoint targetPosition, const NavigationSet& navSet)
    {
        TargetNavSets[targetPosition.X][targetPosition.Y] = navSet;
    }

    void SetTargetPosition(FIntPoint targetPosition)
    {
        if (PrevTargetPos != FIntPoint(-1, -1))
        {

            NavEnvironment[targetPosition.X][targetPosition.Y].SetIsGoal(false);
        }
        NavEnvironment[targetPosition.X][targetPosition.Y].SetIsGoal(true);
        PrevTargetPos = targetPosition;
    }

    float RoomHealth = 100.0f;
    float Complexity = 0.0f;
    float Density = 0.0f;
    Status RoomStatus = Dead;
    float TrainingProgress = 0.0f;
    FIntPoint SignalPoint = FIntPoint(-1, -1);
    /** Count of the number of actors occupying each grid position in the room. */
    TArray<TArray<FThreadSafeCounter>> TileActorCounters;
    /** Action rewards and targets for each of the positions in the room. */
    NavigationEnvironment NavEnvironment;
    /** Navigation sets for each target position in room */
    RoomTargetsNavSets TargetNavSets;
    FIntPoint PrevTargetPos = FIntPoint(-1, -1);
};