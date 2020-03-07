// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "TPGameDemo.h"
#include "MazeActor.h"
#include "TPGameDemoGameState.h"
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

UENUM(BlueprintType)
enum class EEnemyBehaviourState : uint8
{
    Exploring,
    Avoiding,
    ChangingRooms,
    NumBehaviourStates
};

USTRUCT()
struct FTargetPosition
{
    GENERATED_BODY()
    // Invalid initial values to avoid TargetReached miss-fire
    FIntPoint Position = FIntPoint(-1,-1);
    EDirectionType DoorAction = EDirectionType::NumDirectionTypes;

    bool TargetIsDoor() const
    {
        return DoorAction != EDirectionType::NumDirectionTypes;
    }
};

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
        virtual EDirectionType SelectNextAction();

    UFUNCTION (BlueprintCallable, Category = "Enemy Behaviour")
        virtual void ChooseDoorTarget();

    UPROPERTY (BlueprintReadOnly, Category = "Enemy Movement Timing")
        FTimerHandle MoveTimerHandle;

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement Timing")
        virtual void StopMovementTimer();

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement Timing")
        virtual void SetMovementTimerPaused (bool movementTimerShouldBePaused);

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        void UpdateMovementForActionType(EDirectionType actionType);
    //======================================================================================================
    // Behaviour Policy
    //====================================================================================================== 
    UFUNCTION (BlueprintCallable, Category = "Enemy Policy")
        void LoadLevelPolicyForRoomCoordinates (FIntPoint levelCoords);

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        virtual void UpdatePolicyForPlayerPosition (int playerX, int playerY);

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        virtual void UpdatePolicyForDoorType (EDirectionType doorTypem, int doorPositionOnWall);

    UFUNCTION (BlueprintCallable, Category = "Enemy Policy")
        void LoadLevelPolicy (FString levelName);

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        bool TargetNearbyEmptyCell();
    //======================================================================================================
    // Movement
    //====================================================================================================== 
	UFUNCTION(BlueprintCallable, Category = "Enemy Movement")
		bool IsPositionValid();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Enemy Movement")
        FVector MovementTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float MovementSpeed = 1.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float RotationSpeed = 0.1f; 
    /** If the actor doesn't move within this time period, their behaviour will be changed to 'avoid' for a short time. 
    *   A value of 0 will cause no avoidance behaviour.
    */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float MovementStuckThresholdSeconds = 1.5f;
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Enemy Movement")
        void EnteredNewRoom();
private:
    EEnemyBehaviourState  BehaviourState = EEnemyBehaviourState::Exploring;
    FTargetPosition       TargetRoomPosition;
    FRoomPositionPair     AvoidanceTarget;
    FString               LevelPoliciesDir;
    FString               CurrentLevelPolicyDir;
    TArray<TArray<int>>   CurrentLevelPolicy;
    bool                  LevelPoliciesDirFound = false;

    // Can't seem to call parent implementation of EnteredNewRoom from blueprint, so use this call function instead.
    void CallEnteredNewRoom();

    //======================================================================================================
    // Behaviour Policy
    //====================================================================================================== 
    void ResetPolicy();
    void PrintLevelPolicy();
    void UpdatePolicyForTargetPosition();
    //======================================================================================================
    // Movement
    //====================================================================================================== 
    void UpdateMovement();
    void SetBehaviourState(EEnemyBehaviourState newState);
    // Returns the room coords and position in room indicated by the given movement direction, determined by the actors current position.
    // If the movement action would cause them to change rooms, roomCoords will indicate which room they would enter.
    FRoomPositionPair GetTargetRoomAndPositionForDirectionType(EDirectionType actionType);
    
    FTimerHandle StopAvoidanceTimerHandle;
    void ClearAvoidanceTimer();
    float TimeSinceLastPositionChange = 0.0f;
    float AvoidingBehaviourTimeout = 1.0f;
    float TimeSpentAvoiding = 0.0f;
    //float CurrentDistanceTravelled = 0.0f;
    //FVector LastDistanceTrackerStartPosition = FVector::ZeroVector;
    //float DistanceTrackerTime = 0.0f;

    EDirectionType PreviousDoorTarget = EDirectionType::NumDirectionTypes;
    void ClearPreviousDoorTarget();
    //======================================================================================================
    // From AMazeActor
    //====================================================================================================== 
    virtual void PositionChanged() override;
    virtual void RoomCoordsChanged() override;
};
