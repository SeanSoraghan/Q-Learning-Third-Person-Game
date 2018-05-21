// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TPGameDemo.h"
#include "CoreMinimal.h"
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

    void DisableRoom()
    {
        RoomStatus = Dead;
    }

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

    float RoomHealth = 100.0f;
    float Complexity = 0.0f;
    float Density = 0.0f;
    Status RoomStatus = Dead;
    float TrainingProgress = 0.0f;
    FIntPoint SignalPoint = FIntPoint(-1,-1);
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
        FIntPoint GetSignalPointPositionInRoom(FIntPoint roomCoords);

    // --------------------- neighbouring rooms -------------------------------------

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        FIntPoint GetNeighbouringRoomIndices(FIntPoint roomCoords, EDirectionType neighbourPosition) const;

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

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DestroyDoorInRoom(FIntPoint roomCoords, EDirectionType doorWallPosition);

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
    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "World Grid Size")
        int NumGridsXY = 20;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Room Health")
        float MaxRoomHealth = 100.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Room Health")
        float SignalStrength = 100.0f;

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
    FIntPoint GetRoomXYIndices(FIntPoint roomCoords) const;
    FIntPoint GetRoomCoords(FIntPoint roomIndices) const;
    bool RoomXYIndicesValid(FIntPoint roomCoords) const;
    bool WallXYIndicesValid(FIntPoint wallRoomCoords) const;

    SignalLostEvent OnSignalLost;
};
