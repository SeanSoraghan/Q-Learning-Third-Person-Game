// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TPGameDemo.h"
#include "CoreMinimal.h"
#include "TPGameDemoGameMode.h"
#include "GameFramework/GameStateBase.h"
#include <memory>
#include "TPGameDemoGameState.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class TPGAMEDEMO_API ARoomBuilder : public AActor
{
public:
	GENERATED_BODY()

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Room Building")
        void BuildRoom(float complexity = 0.0f, float density = 0.0f);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Room Building")
        void DestroyRoom();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Room Health")
        void HealthChanged(float health);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Room Health")
        void TrainingProgressUpdated(float progress);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Room Health")
        void RoomWasConnected();
};

/**
 * 
 */
UCLASS(Blueprintable)
class TPGAMEDEMO_API AWallBuilder : public AActor
{
public:
	GENERATED_BODY()

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Wall Building")
        void BuildSouthWall();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Wall Building")
        void BuildWestWall();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Wall Building")
        void DestroySouthWall();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Wall Building")
        void DestroyWestWall();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void SpawnSouthDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void SpawnWestDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void DestroySouthDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void DestroyWestDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void LockSouthDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void LockWestDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void UnlockSouthDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void UnlockWestDoor();

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "World Door States")
        void TrainingProgressUpdatedForDoor(EDirectionType doorWallType, float progress);
};

DECLARE_MULTICAST_DELEGATE (FMazeDimensionsChanged);
DECLARE_EVENT(ATPGameDemoGameState, SignalLostEvent);
DECLARE_DYNAMIC_DELEGATE(FOnSignalLost);
DECLARE_EVENT(ATPGameDemoGameState, PerimeterCompleteEvent);
DECLARE_DYNAMIC_DELEGATE(FOnPerimeterComplete);
DECLARE_EVENT(ATPGameDemoGameState, EnemiesPausedChangedEvent);
DECLARE_DYNAMIC_DELEGATE(FOnEnemiesPausedChanged);

/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ATPGameDemoGameState : public AGameState
{
public:
	GENERATED_BODY()
	
	ATPGameDemoGameState (const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void InitialiseArrays();

    ~ATPGameDemoGameState();

    //============================================================================
    // AActor Overrides
    //============================================================================

    void Tick( float DeltaTime ) override;
    
    //============================================================================
    // Acessors
    //============================================================================
    
    // --------------------- room grid properties -------------------------------------

    /** Returns the world position of cell x|y mapped from positive indexing (0,0 -> n,n) to world positions centered at 0,0 (-n/2 * gridUnitX, -n/2 * gridUnitY -> n/2 * gridUnitX, n/2 * gridUnityY). 
        The x position is offset by RoomOffsetX * grid (room) width. Similar for the y position.
        If getCentre is true, the returned position will be centred within the grid cell. 
        If getCentre is false, the returned position will be the back left corner of the cell.
    */
    UFUNCTION (BlueprintCallable, Category = "Room Grid Positions")
        FVector2D GetGridCellWorldPosition (int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre = true);
    /** Static version of GetCellWorldPosition that can be called from any UObject by passing in a pointer to self.
     */
    UFUNCTION(BlueprintCallable, Category = "World Layout")
        static FVector2D GetCellWorldPosition(UObject* worldContextObject, int x, int y, int RoomOffsetX, int RoomOffsetY, bool getCentre = true);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool InnerRoomPositionValid(FIntPoint positionInRoom) const;

    /** Checks if position is valid. If not, wraps the position modulo room size, and updates the room coords. */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void WrapRoomPositionPair (FRoomPositionPair& roomPositionPair);

    /** Returns the world position for a given room and position in room. */
    UFUNCTION (BlueprintCallable, Category = "Room Grid Positions")
        FVector2D GetWorldXYForRoomAndPosition(FRoomPositionPair roomPositionPair);

    /** Returns true if the given position is on the edge of a room. */
    UFUNCTION(BlueprintCallable, Category = "Room Grid Positions")
        bool IsOnGridEdge(const FIntPoint& positionInRoom) const;

    UFUNCTION(BlueprintCallable, Category = "Room Grid Positions")
        EDirectionType GetWallTypeForPosition(const FIntPoint& positionInRoom) const;

    /** Returns the room and position in room for a given world position. */
    UFUNCTION(BlueprintCallable, Category = "Room Grid Positions")
        FRoomPositionPair GetRoomAndPositionForWorldXY(FVector2D worldXY);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool IsBuildableItemPlaced(FRoomPositionPair roomAndPosition, EDirectionType direction);

    // --------------------- room properties -------------------------------------
    const NavigationEnvironment& GetNavEnvironment(FIntPoint roomCoords) const;
    const RoomTargetsQValuesRewardsSets& GetRoomQValuesRewardsSets(FIntPoint roomCoords);
    const QValuesRewardsSet& GetRoomQValuesRewardsSetForTargetPosition(FIntPoint roomCoords, FIntPoint targetPosition);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool DoesRoomExist(FIntPoint roomCoords) const;

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool IsRoomConnected(FIntPoint roomCoords) const;

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool IsRoomTrained(FIntPoint roomCoords) const; 

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool DoesWallExist(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool DoesDoorExist(FIntPoint roomCoords, EDirectionType wallType);
    
    UFUNCTION(BlueprintCallable, Category = "World Room States")
        float GetRoomHealth(FIntPoint roomCoords) const;

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        EQuadrantType GetQuadrantTypeForRoomCoords(FIntPoint roomCoords) const;

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        int GetDoorPositionOnWall(FIntPoint roomCoords, EDirectionType wallType); 

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        FRoomPositionPair GetDoorPosition(FIntPoint roomCoords, EDirectionType wallType);

    /* Get door index on each wall, north east south west */
    UFUNCTION(BlueprintCallable, Category = "World Room States")
        void GetDoorPositionsNESW(FIntPoint roomCoords, TArray<int>& doorPositionsNESW);

    UFUNCTION(BlueprintCallable, Category = "World Door States")
        bool IsDoorUnlocked(FIntPoint roomCoords, EDirectionType wallDirection);

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        FIntPoint GetSignalPointPositionInRoom(FIntPoint roomCoords) const;

    /** Counts MazeActors only (so doesnt count walls) */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool TilePositionIsEmpty(FIntPoint roomCoords, FIntPoint tilePosition) const;
    /** Counts MazeActors only (so doesnt count walls) */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool RoomTilePositionIsEmpty(FRoomPositionPair roomAndPosition) const;

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool RoomIsWithinPerimeter(FIntPoint roomCoords) const;
    // --------------------- neighbouring rooms -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        FIntPoint GetNeighbouringRoomIndices(FIntPoint roomCoords, EDirectionType neighbourPosition) const;

    /** Returns a neighbourng cell determined by direction, wrapped if neccessary*/
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        FRoomPositionPair GetNeighbouringCell(FRoomPositionPair roomAndPosition, EDirectionType direction);

    /** Returns a bool array indicating whether the neihgbouring rooms in each direction (North through West, clockwise) exist. */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        TArray<bool> GetNeighbouringRoomStates(FIntPoint doorPosition) const;
    
    /* returns an array of 4 elements (N,E,S,W) where 0 indicates no neighbour, and 1 -> NumGridUnitsX - 1 indicates the position of a door to an existing neighbour. */
    UFUNCTION(BlueprintCallable, Category = "World Room States")
        TArray<int> GetDoorPositionsForExistingNeighbours(FIntPoint roomCoords);

    // --------------------- Builders -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Room Builders")
        ARoomBuilder* GetRoomBuilder(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Room Building")
        AWallBuilder* GetWallBuilder(FIntPoint roomCoords, EDirectionType direction);
    
    // --------------------- Behaviour -------------------------------------

    FDirectionSet GetOptimalActions(FIntPoint roomCoords, FIntPoint targetGridPosition, FIntPoint currentGridPosition)
    {
        FDirectionSet directionSet = GetValidActions({roomCoords, currentGridPosition});
        GetActionQValuesRewards({ roomCoords, currentGridPosition }, targetGridPosition).GetOptimalQValueAndActions_Valid(directionSet);
        return directionSet;
    }

    float GetExploreProbability(FIntPoint roomCoords, FIntPoint targetGridPosition, FIntPoint currentGridPosition)
    {
        return GetActionQValuesRewards({ roomCoords, currentGridPosition }, targetGridPosition).GetExploreProbability();
    }

    void IncrementExploreCount(FIntPoint roomCoords, FIntPoint targetGridPosition, FIntPoint currentGridPosition)
    {
        GetActionQValuesRewards({ roomCoords, currentGridPosition }, targetGridPosition).IncrementExplorations();
    }

    UFUNCTION(BlueprintCallable, Category = "World Room Building")
        FRoomPositionPair GetTargetRoomAndPositionForDirectionType(FRoomPositionPair startingRoomAndPosition, EDirectionType actionType);

    FDirectionSet GetValidActions(FRoomPositionPair roomAndPosition)
    {
        const NavigationEnvironment& environment = GetNavEnvironment(roomAndPosition.RoomCoords);
        FDirectionSet directions;
        directions.Clear();
        for (int i = (int)EDirectionType::North; i < (int)EDirectionType::NumDirectionTypes; ++i)
        {
            FIntPoint position = roomAndPosition.PositionInRoom;
            FRoomPositionPair actionTarget = environment[position.X][position.Y].GetActionTarget((EDirectionType)i);
            WrapRoomPositionPair(actionTarget);
            FIntPoint targetPos = actionTarget.PositionInRoom;
            if (targetPos != roomAndPosition.PositionInRoom || actionTarget.RoomCoords != roomAndPosition.RoomCoords)
                directions.EnableDirection((EDirectionType)i);
        }
        return directions;
    }
    //============================================================================
    // Modifiers
    //============================================================================

    //----------------- inner grid properties ----------------------------------------

    UFUNCTION (BlueprintCallable, Category = "Inner Grid Size")
        void SetGridUnitLengthXCM (int x);

    UFUNCTION (BlueprintCallable, Category = "Inner Grid Size")
        void SetGridUnitLengthYCM (int y);

    UFUNCTION (BlueprintCallable, Category = "Inner Grid Size")
        void SetNumGridUnitsX (int numUnitsX);

    UFUNCTION (BlueprintCallable, Category = "Inner Grid Size")
        void SetNumGridUnitsY (int numUnitsY);
    
    // --------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DestroyDoorInRoom(FIntPoint roomCoords, EDirectionType doorWallPosition);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void ActorEnteredTilePosition(FIntPoint roomCoords, FIntPoint tilePosition);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void ActorExitedTilePosition(FIntPoint roomCoords, FIntPoint tilePosition);

    // --------------------- neighbouring rooms -------------------------------------

    /** Destroys the door on each adjoining wall in the neighbouring rooms (North through West, clockwise). */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy);

    // --------------------- Setters -------------------------------------
    
    UFUNCTION(BlueprintCallable, Category = "World Room States")
        void SetRoomHealth(FIntPoint roomCoords, float health);

    UFUNCTION(BlueprintCallable, Category = "World Room Building")
        void SetWallBuilder(FIntPoint roomCoords, AWallBuilder* builder);

    UFUNCTION(BlueprintCallable, Category = "World Room Builders")
        void SetRoomBuilder(FIntPoint roomCoords, ARoomBuilder* roomBuilderActor);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void UpdateRoomHealth(FIntPoint roomCoords, float healthDelta);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void UpdateSignalStrength(float delta);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void SetBuildableItemPlaced(FRoomPositionPair roomAndPosition, EDirectionType direction, bool placed);

    // --------------------- Room & Wall Initialization / Destruction -------------------------------------
    void SetRoomInnerStructure(FIntPoint roomCoords, InnerRoomBitmask roomBitmask);
    InnerRoomBitmask GetRoomInnerStructure(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableRoomState(FIntPoint roomCoords, float complexity = 0.0f, float density = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void SetRoomConnected(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void SetSignalPointInRoom(FIntPoint roomCoords, FIntPoint signalPointInRoom);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableRoomState(FIntPoint roomCoords);

    UFUNCTION(BLueprintCallable, Category = "World Rooms Training")
        void SetRoomTrainingProgress(FIntPoint roomCoords, float progress);

    UFUNCTION(BlueprintCallable, Category = "World Rooms Training")
        void SetRoomTrained(FIntPoint roomCoords);

    /* Return true if the action leads somewhere. */
    bool SimulateAction(FRoomPositionPair& roomAndPosition, EDirectionType actionToTake, FIntPoint targetPosition);
    /* A realtime version of UpdateQValue. This is to be performed by actors as they navigate the level.*/
    void UpdateQValueRealtime(FRoomPositionPair& roomAndPosition, EDirectionType actionToTake, FIntPoint targetPosition, float accumulatedReward, float learningRate);
    /* Update the qvalue for an action from a given position in a given room for a given goal position.*/
    void UpdateQValue(const FRoomPositionPair& roomAndPosition, FIntPoint goalPosition, EDirectionType actionToTake, float learningRate, float deltaQ);
    /* Update the qvalue for an action from a given position in a given room.*/
    void UpdateQValueForCurrentNavState(EDirectionType actionType, float learningRate, float deltaQ);
    /* Set the action targets for the room, given the cell state structure */
    void UpdateRoomNavEnvironmentForStructure(FIntPoint roomCoords, TArray<TArray<int>> roomStructure);
    void UpdateRoomNavEnvironment(FIntPoint roomCoords, const NavigationEnvironment& navEnvironment);
    /* Set the qvalues and rewards set for a target position in a room. */
    void SetRoomQValuesRewardsSet(FIntPoint RoomCoords, FIntPoint targetPosition, const QValuesRewardsSet& navSet);
    /* Reset the action qvalues and rewards on a given position for a given goal position in a room. */
    void ClearQValuesAndRewards(FIntPoint RoomCoords, FIntPoint GoalPosition);
    /* Set whether a position in a room is the goal position. Used by LevelTrainerComponent when training. */
    void SetPositionIsGoal(FIntPoint RoomCoords, FIntPoint GoalPosition, bool isGoal);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableWallState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableWallState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableDoorState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableDoorState(FIntPoint roomCoords, EDirectionType wallType);

	UFUNCTION(BlueprintCallable, Category = "World Rooms States")
		void GeneratePerimeterRooms();

	UFUNCTION(BlueprintCallable, Category = "World Rooms States")
		void ConnectPerimeterRooms();

    UFUNCTION(BlueprintCallable, Category = "World Perimeter State")
    int GetNumRoomsOnPerimeter();

    /** Assumes square rooms */
    UFUNCTION(BlueprintCallable, Category = "World Perimeter State")
    int GetNumRoomsOnPerimeterSide();

    /** Assumes square rooms */
    UFUNCTION(BlueprintCallable, Category = "World Perimeter State")
    int GetPerimeterSideLength();

    // --------------------- Door Interaction -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Door Interaction")
        void DoorOpened(FIntPoint roomCoords, EDirectionType wallDirection, float complexity = 0.0f, float density = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "Door Interaction")
        void LockDoor(FIntPoint roomCoords, EDirectionType wallDirection);

    UFUNCTION(BlueprintCallable, Category = "Door Interaction")
        void UnlockDoor(FIntPoint roomCoords, EDirectionType wallDirection);

    // ----------------------- Delegates -------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Signal State")
        void RegisterSignalLostCallback(const FOnSignalLost& Callback);
    UFUNCTION(BlueprintCallable, Category = "World Perimeter State")
        void RegisterPerimeterCompleteCallback(const FOnPerimeterComplete& Callback);
    UFUNCTION(BlueprintCallable, Category = "World Perimeter State")
        void RegisterEnemiesPausedCallback(const FOnEnemiesPausedChanged& Callback);
    //============================================================================
    // Properties
    //============================================================================
    FMazeDimensionsChanged OnMazeDimensionsChanged;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Inner Grid Size")
        int GridUnitLengthXCM = 200;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Inner Grid Size")
        int GridUnitLengthYCM = 200;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Inner Grid Size")
        int NumGridUnitsX = 10;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Inner Grid Size")
        int NumGridUnitsY = 10;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "World Grid Size")
        int NumGridsXY = 20;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Room Health")
        float MaxRoomHealth = 100.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Room Health")
        float SignalStrength = 100.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Room Health")
        float MaxSignalStrength = 100.0f;

    //============================================================================
    // Enemy Movement
    //============================================================================        
    UFUNCTION(BlueprintCallable, Category = "Enemy Movement")
        void SetEnemyMovementPaused(bool MovementPaused);

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Enemy Movement")
        bool EnemyMovementPaused = false;

private:
    TArray<TArray<ARoomBuilder*>> RoomBuilders;
    TArray<TArray<AWallBuilder*>> WallBuilders;
	TArray<TArray<RoomState>> RoomStates;

    NavigationEnvironment& GetmNavEnvironment(FIntPoint roomCoords);
    ActionTargets& GetActionTargets(FRoomPositionPair roomAndPosition);
    QValuesRewardsSet& GetQValuesRewardsSet(FIntPoint roomCoords, FIntPoint targetPosition);
    ActionQValuesAndRewards& GetActionQValuesRewards(const FRoomPositionPair& roomAndPosition, FIntPoint targetPosition);

    bool LevelPoliciesDirFound = false;

    EnemiesPausedChangedEvent EnemiesPausedChanged;
    
    // An unwrapped '2d' array containing An FDirectionSet for each space in the maze,
    // indicating if a buildable has been placed facing each direction.
    // (This is mainly applicable to turrets attached to walls).
    TArray<FDirectionSet> MazePositionBuildableStates;
    
    // Struct containing the coordinates of a west/south wall state couple and the specific wall type.
    struct WallPosition
    {
        FIntPoint WallCoupleCoords;
        EDirectionType WallType;
    };
    // This is added to during a frame when rooms are created / destroyed. It is consumed during the tick function and cleared.
    TArray<WallPosition> WallsToUpdate;
    // Checks if the list of WallsToUpdate already contains the given wall.
    bool IsWallInUpdateList(FIntPoint coords, EDirectionType wallType);
    // Adds all of the walls of a room to the WallsToUpate list.
    void FlagWallsForUpdate(FIntPoint roomCoords);

    WallState& GetWallState(FIntPoint roomCoords, EDirectionType direction);
    // Indexed as North, East, South, West
    TArray<WallState*> GetWallStatesForRoom(FIntPoint roomCoords);
    
    /** Converts coords from centred at 0 to centred at max num grids / 2. */
    FIntPoint GetRoomXYIndicesChecked(FIntPoint roomCoords) const;
    const RoomState& GetRoomStateChecked(FIntPoint roomCoords) const;
    FIntPoint GetRoomCoords(FIntPoint roomIndices) const;
    bool RoomXYIndicesValid(FIntPoint roomInidces) const;
    bool WallXYIndicesValid(FIntPoint wallRoomCoords) const;

    SignalLostEvent OnSignalLost;
    PerimeterCompleteEvent OnPerimeterComplete;

    int CurrentPerimeter = 1;
    int NumPerimeterRoomsConnected = 0;
    void RoomWasConnected(FIntPoint roomCoords);
    void LockDoorIfOnPerimeter(FIntPoint roomCoords);
    void UnlockPerimeterDoors();
    void UpdateDoorLockedStateForNeighbouringRooms(FIntPoint roomCoords, FIntPoint neighbourCoords, EDirectionType wallType);
    bool DoorIsOnPerimeter(FIntPoint roomCoords, EDirectionType doorDirection);
    FThreadSafeBool PerimeterDoorsNeedUnlocked = false;
};
