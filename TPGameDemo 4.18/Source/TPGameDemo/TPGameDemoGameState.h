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

struct RoomState
{
    ~RoomState()
    {
        NorthDoor = nullptr;
        EastDoor = nullptr;
        SouthDoor = nullptr;
        WestDoor = nullptr;
        bRoomExists = false;
    }

    void InitializeRoom(TArray<AActor*> doors)
    {
        bRoomExists = true;
        NorthDoor = doors[(int)EWallPosition::North];
        EastDoor  = doors[(int)EWallPosition::East];
        SouthDoor = doors[(int)EWallPosition::South];
        WestDoor  = doors[(int)EWallPosition::West];
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
                if (NorthDoor != nullptr)
                {
                    NorthDoor->Destroy();
                    NorthDoor = nullptr;                
                }
                break;
            }
            case EWallPosition::East: 
            {
                if (EastDoor != nullptr)
                {
                    EastDoor->Destroy();
                    EastDoor = nullptr;
                }
                break;
            }
            case EWallPosition::South: 
            {
                if (SouthDoor != nullptr)
                {
                    SouthDoor->Destroy();
                    SouthDoor = nullptr;
                }
                break;
            }
            case EWallPosition::West: 
            {
                if (WestDoor != nullptr)
                {
                    WestDoor->Destroy();
                    WestDoor = nullptr;
                }
                break;
            }
        }
    }

    AActor* NorthDoor = nullptr;
    AActor* EastDoor = nullptr;
    AActor* SouthDoor = nullptr;
    AActor* WestDoor = nullptr;
    float RoomHealth = 100.0f;
    bool bRoomExists = false;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRoomDiedDelegate, FIntPoint RoomCoords);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnSpawnDoorDelegate, FIntPoint RoomCoords, FIntPoint PositionInRoom);

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

    void BeginPlay() override;

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
        void EnableRoomState(FIntPoint roomCoords, TArray<AActor*> doors);

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

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "World Rooms States")
        FRoomDiedDelegate OnRoomDied;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "World Rooms Spawning")
        FOnSpawnDoorDelegate OnSpawnDoor;

private:
    TArray<TArray<AActor*>> RoomBuilders;
	TArray<TArray<RoomState>> RoomStates;
    /** Converts coords from centred at 0 to centred at max num grids / 2. */
    FIntPoint GetRoomXYIndices(FIntPoint roomCoords);
    FIntPoint GetRoomCoords(FIntPoint roomIndices);
    bool RoomXYIndicesValid(FIntPoint roomCoords);
};
