// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameFramework/GameMode.h"
#include "TPGameDemoGameMode.generated.h"

DECLARE_MULTICAST_DELEGATE (FMazeDimensionsChanged);

/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ATPGameDemoGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Game Mode Player Control")
        float Walking_Movement_Force = 0.7f;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Game Mode Player Control")
        float Running_Movement_Force = 1.0f;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Game Mode Player Control")
        float Max_Pitch_Look = 50.0f;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Game Mode Player Control")
        float Min_Pitch_Look = -50.0f;
	
	UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Game Mode Player Control")
        float Camera_Animation_Speed = 1.0f;
    
    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Grid Size")
        int GridUnitLengthXCM = 200;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Grid Size")
        int GridUnitLengthYCM = 200;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Grid Size")
        int NumGridUnitsX = 10;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Grid Size")
        int NumGridUnitsY = 10;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Room Health")
        float DefaultRoomHealth = 100.0f;

    FMazeDimensionsChanged OnMazeDimensionsChanged;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Maze Actor Properties")
        float DefaultMaxHealth = 100.0f;

    UFUNCTION (BlueprintCallable, Category = "Game Mode Grid Size")
        void SetGridUnitLengthXCM (int x);

    UFUNCTION (BlueprintCallable, Category = "Game Mode Grid Size")
        void SetGridUnitLengthYCM (int y);

    UFUNCTION (BlueprintCallable, Category = "Game Mode Grid Size")
        void SetNumGridUnitsX (int numUnitsX);

    UFUNCTION (BlueprintCallable, Category = "Game Mode Grid Size")
        void SetNumGridUnitsY (int numUnitsY);

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

};
