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
};
