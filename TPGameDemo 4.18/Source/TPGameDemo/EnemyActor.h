// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "MazeActor.h"
#include "TextParserComponent.h"
#include "EnemyActor.generated.h"



/*
The base class for an enemy actor. Enemies behave according to `level policies' which are action-value tables that have been trained using Q-Learning.
See https://webdocs.cs.ualberta.ca/~sutton/book/ebook/node1.html for details on reinforcement learning and Q-Learning specifically. 

The SelectNextAction function is used to query which action should be taken when the enemy is in its current postion and the goal (player character) is at
a given position.

When the player character changes position, the UpdatePolicyForPlayerPosition (int playerX, int playerY) function is called. This functionality is implemented
in the level bueprint in the UE4 editor. The Player Character broadcasts a delegate function when its grid position changes. In the level blueprint in the editor, 
this delegate is bound to a function that calls UpdatePolicyForPlayerPosition (int playerX, int playerY) on an instance of this class.

See MazeActor.h
*/

UCLASS()
class TPGAMEDEMO_API AEnemyActor : public AMazeActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemyActor (const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        virtual EActionType SelectNextAction();

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        virtual void UpdatePolicyForPlayerPosition (int playerX, int playerY);

    UPROPERTY (BlueprintReadOnly, Category = "Enemy Movement Timing")
        FTimerHandle MoveTimerHandle;

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement Timing")
        virtual void StopMovementTimer();

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement Timing")
        virtual void SetMovementTimerPaused (bool movementTimerShouldBePaused);
    //======================================================================================================
    // Behaviour Policy
    //====================================================================================================== 
    UFUNCTION (BlueprintCallable, Category = "Enemy Policy")
        void LoadLevelPolicy (FString levelName);

    //======================================================================================================
    // Movement
    //====================================================================================================== 
    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Enemy Movement")
        FVector MovementTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float MovementSpeed = 1.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float RotationSpeed = 0.1f;
private:
    FString               LevelPolciesDir;
    FString               CurrentLevelPolicyDir;
    TArray<TArray<int>>   CurrentLevelPolicy;
    bool                  LevelPoliciesDirFound = false;

    //======================================================================================================
    // Movement
    //====================================================================================================== 
    void UpdateMovement();
    void CheckPositionLimits();

    //======================================================================================================
    // Behaviour Policy
    //====================================================================================================== 
    void ResetPolicy();
    void PrintLevelPolicy();
    
};
