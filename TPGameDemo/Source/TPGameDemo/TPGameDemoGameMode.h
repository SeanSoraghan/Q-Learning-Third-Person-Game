// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameFramework/GameMode.h"
#include "TPGameDemoGameMode.generated.h"

UENUM(BlueprintType)
enum class ECameraControlType : uint8
{
    RotateCamera  UMETA(DisplayName = "Rotate Camera"),
    RotatePlayer UMETA(DisplayName = "Rotate Player"),
    TwinStick UMETA(DisplayName = "Twin Stick"),
    NumTypes
};

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

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Room Health")
        float DefaultRoomHealth = 100.0f;

    UPROPERTY (BlueprintReadOnly, VisibleAnywhere, Category = "Game Mode Room Health")
        float DefaultSignalStrength = 100.0f;

    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Maze Actor Properties")
        float DefaultMaxHealth = 100.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Character Camera Movement")
        ECameraControlType CameraControlType = ECameraControlType::RotatePlayer;
};
