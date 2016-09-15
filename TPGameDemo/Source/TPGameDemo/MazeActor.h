// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameFramework/Character.h"
#include "MazeActor.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE (FGridPositionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE (FActorDied);

/*
The base class for an actor that can exist and navigate within a maze level. Maze actors maintain grid x and grid y position values.
The UpdatePosition (bool broadcastChange) function is used to update their grid position based on their current world position. The
broadcastChange flag indicates whether they should broadcast the change via the GridPositionChangedEvent delegate function.
For example, the player character will broadcast when their position has changed such that enemies can update their behaviour policy
(see EnemyActor.h).

*/

UCLASS()
class TPGAMEDEMO_API AMazeActor : public ACharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMazeActor (const FObjectInitializer& ObjectInitializer);
    

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	//=========================================================================================
    // Maze Position
    //=========================================================================================
    UPROPERTY (BlueprintReadOnly, Category = "Actor Grid Positions")
        int GridXPosition                 = 0;
    UPROPERTY (BlueprintReadOnly, Category = "Actor Grid Positions")
        int GridYPosition                 = 0;

    int CurrentLevelNumGridUnitsX     = 10;
    int CurrentLevelNumGridUnitsY     = 10;
    int CurrentLevelGridUnitLengthXCM = 200;
    int CurrentLevelGridUnitLengthYCM = 200;
    int PreviousGridXPosition         = 0;
    int PreviousGridYPosition         = 0;

    //UFUNCTION (BlueprintCallable, Category = "Actor Grid Positions")
    UPROPERTY (BlueprintAssignable, Category = "Actor Grid Positions")
        FGridPositionChanged GridPositionChangedEvent;

    UPROPERTY (BlueprintReadWrite, Category = "Maze Actor Health")
        float Health;

    UPROPERTY (BlueprintReadWrite, Category = "Maze Actor Health")
        float MaxHealth;

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Health")
        virtual float GetHealthPercentage();

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Health")
        virtual void TakeDamage (float damageAmount);

    UPROPERTY (BlueprintReadWrite, Category = "Maze Actor Health")
        bool IsAlive;

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Health")
        virtual void CheckDeath();

    UPROPERTY (BlueprintAssignable, Category = "Maze Actor Health")
        FActorDied OnActorDied;

private:
    void UpdatePosition (bool broadcastChange = true);
};
