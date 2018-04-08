// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "TPGameDemo.h"
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include <memory>
#include "TPGameDemoGameState.generated.h"

UENUM(BlueprintType)
enum class EWallPosition : uint8
{
    North UMETA (DisplayName = "NorthWall"),
    East  UMETA (DisplayName = "EastWall"),
    South UMETA (DisplayName = "SouthWall"),
    West  UMETA (DisplayName = "WestWall"),
    NumWallPositions
};

struct DoorState
{
    DoorState(){DoorActor = nullptr;}
    ~DoorState(){DoorActor = nullptr;}

    void InitializeDoor(AActor* doorActor, int positionOnWall)
    {
        DoorActor = doorActor;
        PositionOnWall = positionOnWall;
    }

    AActor* DoorActor;
    int PositionOnWall = 0;
};

struct RoomState
{
    ~RoomState()
    {
        bRoomExists = false;
    }

    void InitializeRoom(TArray<AActor*> doors, TArray<int> doorPositionsOnWalls)
    {
        bRoomExists = true;
        NorthDoor.InitializeDoor(doors[(int)EWallPosition::North], doorPositionsOnWalls[(int)EWallPosition::North]);
        EastDoor.InitializeDoor(doors[(int)EWallPosition::East], doorPositionsOnWalls[(int)EWallPosition::East]);
        SouthDoor.InitializeDoor(doors[(int)EWallPosition::South], doorPositionsOnWalls[(int)EWallPosition::South]);
        WestDoor.InitializeDoor(doors[(int)EWallPosition::West], doorPositionsOnWalls[(int)EWallPosition::West]);
    }

    void DisableRoom(TArray<bool> NeighbourStates)
    {
        bRoomExists = false;
        // If a neighbour doesn't exist, we kill the door that would lead to it.
        for (int p = 0; p < (int)EWallPosition::NumWallPositions; ++p)
        {
            if (NeighbourStates[p])
                DestroyDoor((EWallPosition)p);
        }
    }

    void DestroyDoor(EWallPosition wallPosition)
    {
        switch (wallPosition)
        {
            case EWallPosition::North: 
            {
                if (NorthDoor.DoorActor != nullptr)
                {
                    NorthDoor.DoorActor->Destroy();
                    NorthDoor.DoorActor = nullptr;                
                }
                break;
            }
            case EWallPosition::East: 
            {
                if (EastDoor.DoorActor != nullptr)
                {
                    EastDoor.DoorActor->Destroy();
                    EastDoor.DoorActor = nullptr;
                }
                break;
            }
            case EWallPosition::South: 
            {
                if (SouthDoor.DoorActor != nullptr)
                {
                    SouthDoor.DoorActor->Destroy();
                    SouthDoor.DoorActor = nullptr;
                }
                break;
            }
            case EWallPosition::West: 
            {
                if (WestDoor.DoorActor != nullptr)
                {
                    WestDoor.DoorActor->Destroy();
                    WestDoor.DoorActor = nullptr;
                }
                break;
            }
        }
    }

    void SetDoor(AActor* doorActor, EWallPosition wallPosition)
    {
        switch(wallPosition)
        {
            case EWallPosition::North: NorthDoor.DoorActor = doorActor;
            case EWallPosition::East:  EastDoor.DoorActor = doorActor;
            case EWallPosition::South: SouthDoor.DoorActor = doorActor;
            case EWallPosition::West:  WestDoor.DoorActor = doorActor;
            default: ensure(false);
        }
    }

    int GetDoorPositionOnWall(EWallPosition wallPosition)
    {
        switch(wallPosition)
        {
            case EWallPosition::North: return NorthDoor.PositionOnWall;
            case EWallPosition::East: return EastDoor.PositionOnWall;
            case EWallPosition::South: return SouthDoor.PositionOnWall;
            case EWallPosition::West: return WestDoor.PositionOnWall;
            default: ensure(false); return -1;
        }
    }

    DoorState NorthDoor;
    DoorState EastDoor;
    DoorState SouthDoor;
    DoorState WestDoor;
    float RoomHealth = 100.0f;
    bool bRoomExists = false;
};

//Bind all room builders to destroy their inner room walls when this happens.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRoomDiedDelegate, FIntPoint, RoomCoords);

//Bind in one place to spawn doors when this happens.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSpawnDoorDelegate, FIntPoint, RoomCoords, 
                                              EWallPosition, WallPosition, int, PositionOnWall, 
                                              FIntPoint, TargetRoomCoords);

/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ATPGameDemoGameState : public AGameState
{
public:
	GENERATED_BODY()
	
    ~ATPGameDemoGameState();

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        static EWallPosition GetWallPositionForActionType(EActionType actionType);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        bool DoesRoomExist(FIntPoint roomCoords);
    
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        FIntPoint GetNeighbouringRoomIndices(FIntPoint roomCoords, EWallPosition neighbourPosition);

    /** Returns a bool array indicating whether the neihgbouring rooms in each direction (North through West, clockwise) exist. */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        TArray<bool> GetNeighbouringRoomStates(FIntPoint doorPosition);

    /** Returns the position of a wall from a neighbouring room's perspective. */
    UFUNCTION(BlueprintCallable, Category = "World Rooms Layout")
        static EWallPosition GetWallPositionInNeighbouringRoom(EWallPosition wallPosition);

    /** Destroys the door on each adjoining wall in the neighbouring rooms (North through West, clockwise). */
    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DestroyDoorInRoom(FIntPoint roomCoords, EWallPosition doorWallPosition);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void EnableRoomState(FIntPoint roomCoords, TArray<AActor*> doors, TArray<int> doorPositionsOnWalls);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void DisableRoomState(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void InitialiseArrays();

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "World Grid Size")
        int NumGridsXY = 20;

    UFUNCTION(BlueprintCallable, Category = "World Room Builders")
        AActor* GetRoomBuilder(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Room Builders")
        void SetRoomBuilder(FIntPoint roomCoords, AActor* roomBuilderActor);

    UFUNCTION(BlueprintCallable, Category = "World Rooms States")
        void UpdateRoomHealth(FIntPoint roomCoords, float healthDelta);

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        void SetRoomHealth(FIntPoint roomCoords, float health);

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        float GetRoomHealth(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Room States")
        void KillRoom(FIntPoint roomCoords); 

    UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = "World Rooms States")
        FRoomDiedDelegate OnRoomDied;

    UPROPERTY(BlueprintAssignable, VisibleAnywhere, Category = "World Rooms Spawning")
        FOnSpawnDoorDelegate OnSpawnDoor;
        
private:
    TArray<TArray<AActor*>> RoomBuilders;
	TArray<TArray<RoomState>> RoomStates;
    /** Converts coords from centred at 0 to centred at max num grids / 2. */
    FIntPoint GetRoomXYIndices(FIntPoint roomCoords);
    FIntPoint GetRoomCoords(FIntPoint roomIndices);
    bool RoomXYIndicesValid(FIntPoint roomCoords);
};
