// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
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
        TerminateRoom();
    }

    void InitializeRoom(TArray<AActor*> doors)
    {
        bRoomExists = true;
        NorthDoor = doors[(int)EWallPosition::North];
        EastDoor  = doors[(int)EWallPosition::East];
        SouthDoor = doors[(int)EWallPosition::South];
        WestDoor  = doors[(int)EWallPosition::West];
    }

    void TerminateRoom()
    {
        NorthDoor = nullptr;
        EastDoor = nullptr;
        SouthDoor = nullptr;
        WestDoor = nullptr;
        bRoomExists = false;
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
            }
            case EWallPosition::East: 
            {
                if (EastDoor != nullptr)
                {
                    EastDoor->Destroy();
                    EastDoor = nullptr;
                }
            }
            case EWallPosition::South: 
            {
                if (SouthDoor != nullptr)
                {
                    SouthDoor->Destroy();
                    SouthDoor = nullptr;
                }
            }
            case EWallPosition::West: 
            {
                if (WestDoor != nullptr)
                {
                    WestDoor->Destroy();
                    WestDoor = nullptr;
                }
            }
        }
    }

    AActor* NorthDoor = nullptr;
    AActor* EastDoor = nullptr;
    AActor* SouthDoor = nullptr;
    AActor* WestDoor = nullptr;
    
    bool bRoomExists = false;
};
/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ATPGameDemoGameState : public AGameStateBase
{
public:
	GENERATED_BODY()
	
    void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        bool DoesRoomExist(FIntPoint roomCoords);
    
    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        FIntPoint GetNeighbouringRoomIndices(FIntPoint roomCoords, EWallPosition neighbourPosition);

    /** Returns a bool array indicating whether the neihgbouring rooms in each direction (North through West, clockwise) exist. */
    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        TArray<bool> GetNeighbouringRoomStates(FIntPoint doorPosition);

    /** Destroys the door on each adjoining wall in the neighbouring rooms (North through West, clockwise). */
    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        void DestroyNeighbouringDoors(FIntPoint roomCoords, TArray<bool> positionsToDestroy);

    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        void EnableRoomState(FIntPoint roomCoords, TArray<AActor*> doors);

    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        void DisableRoomState(FIntPoint roomCoords);

    UFUNCTION(BlueprintCallable, Category = "World Grids State")
        void InitialiseRoomStates();

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "World Grid Size")
        int NumGridsXY = 20;

private:
	TArray<TArray<RoomState>> RoomStates;
    /** Converts coords from centred at 0 to centred at max num grids / 2. */
    FIntPoint GetRoomXYIndices(FIntPoint roomCoords);
    bool RoomXYIndicesValid(FIntPoint roomCoords);
};
