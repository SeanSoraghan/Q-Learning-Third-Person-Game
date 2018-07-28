// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TPGameDemo.h"
#include "CoreMinimal.h"
#include "TPGameDemoGameMode.h"
#include "GameFramework/GameStateBase.h"
#include <memory>
#include "TPGameDemoGameState.generated.h"

UENUM(BlueprintType)
enum class EQuadrantType : uint8
{
    NorthEast,
    SouthEast,
    SouthWest,
    NorthWest,
    NumQuadrants
};

UENUM(BlueprintType)
enum class EDoorState : uint8
{
    Open,
    Closed,
    Opening,
    Closing,
    NumStates
};

struct WallState
{

    WallState(){}
    ~WallState(){}

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
        bDoorExists = false;
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

struct RoomState
{
    enum Status : uint8
    {
        Dead,
        Training,
        Trained,
        Connected
    };

    RoomState(){}

    RoomState(FIntPoint roomDimensions)
    {
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

    float RoomHealth = 100.0f;
    float Complexity = 0.0f;
    float Density = 0.0f;
    Status RoomStatus = Dead;
    float TrainingProgress = 0.0f;
    FIntPoint SignalPoint = FIntPoint(-1,-1);
    /** Count of the number of actors occupying each grid position in the room. */
    TArray<TArray<FThreadSafeCounter>> TileActorCounters;
};

/**
 * 
 */
UCLASS(Blueprintable)
class TPGAMEDEMO_API ARoomBuilder : public AActor
{
public:
	GENERATED_BODY()

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Room Building")
        void BuildRoom(const TArray<int>& doorPositionsOnWalls, float complexity = 0.0f, float density = 0.0f);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Room Building")
        void DestroyRoom();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Room Health")
        void HealthChanged(float health);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Room Health")
        void TrainingProgressUpdated(float progress);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Room Health")
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

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Wall Building")
        void BuildSouthWall();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Wall Building")
        void BuildWestWall();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Wall Building")
        void DestroySouthWall();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Wall Building")
        void DestroyWestWall();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Door States")
        void SpawnSouthDoor();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Door States")
        void SpawnWestDoor();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Door States")
        void DestroySouthDoor();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Door States")
        void DestroyWestDoor();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "World Door States")
        void TrainingProgressUpdatedForDoor(EDirectionType doorWallType, float progress);
};

DECLARE_MULTICAST_DELEGATE (FMazeDimensionsChanged);
DECLARE_EVENT(ATPGameDemoGameState, SignalLostEvent);
DECLARE_DYNAMIC_DELEGATE(FOnSignalLost);

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

    // --------------------- room properties -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool DoesRoomExist(FIntPoint roomCoords) const;

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
        FIntPoint GetSignalPointPositionInRoom(FIntPoint roomCoords) const;

    /** Counts MazeActors only (so doesnt count walls) */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool TilePositionIsEmpty(FIntPoint roomCoords, FIntPoint tilePosition) const;
    /** Counts MazeActors only (so doesnt count walls) */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool RoomTilePositionIsEmpty(FRoomPositionPair roomAndPosition) const;

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

    // --------------------- Room & Wall Initialization / Destruction -------------------------------------

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

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableWallState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableWallState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableDoorState(FIntPoint roomCoords, EDirectionType wallType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableDoorState(FIntPoint roomCoords, EDirectionType wallType);

    // --------------------- Door Interaction -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Door Interaction")
        void DoorOpened(FIntPoint roomCoords, EDirectionType wallDirection, float complexity = 0.0f, float density = 0.0f); 

    // ----------------------- Delegates -------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Signal State")
        void RegisterSignalLostCallback(const FOnSignalLost& Callback);

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


private:
    TArray<TArray<ARoomBuilder*>> RoomBuilders;
    TArray<TArray<AWallBuilder*>> WallBuilders;
	TArray<TArray<RoomState>> RoomStates;
    TArray<TArray<WallStateCouple>> WallStates;

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
};
