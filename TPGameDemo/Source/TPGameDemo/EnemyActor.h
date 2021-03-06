// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "TPGameDemo.h"
#include "MazeActor.h"
#include "TPGameDemoGameState.h"
#include "EnemyActor.generated.h"


#define ENEMY_LIFETIME_LOGS 0

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
enum class ELogEventType : uint8
{
    Info,
    Warning,
    Error,
    NumTypes
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

    FString ToInfoString() const
    {
        FString positionString = FString::Format(TEXT("Target Position: {0}"), { Position.ToString() });
        FString actionString = FString::Format(TEXT("Action : {0}"), { DirectionHelpers::GetDisplayString(DoorAction) });
        return positionString + " | " + actionString;
    }
};


DECLARE_LOG_CATEGORY_EXTERN(LogEnemyActor, Log, All);

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

    UFUNCTION(BlueprintCallable, Category = "Enemy Position")
        bool HasReachedTargetRoom() const;

    UFUNCTION(BlueprintCallable, Category = "Enemy Position")
        bool HasReachedTargetPosition() const;

    UFUNCTION(BLueprintCallable, Category = "Enemy Position")
        bool IsOnDoor(EDirectionType direction) const;

    UFUNCTION (BlueprintCallable, Category = "Enemy Movement")
        virtual EDirectionType SelectNextAction();

    //======================================================================================================
    // Behaviour Policy
    //======================================================================================================
    //UFUNCTION (BlueprintCallable, Category = "Enemy Behaviour")
    //    virtual void UpdatePolicyForDoorType (EDirectionType doorTypem, int doorPositionOnWall);

    /* Update the TargetRoomAndPosition */
    UFUNCTION(BlueprintCallable, Category = "Enemy Behaviour")
        virtual void TargetPositionInRoom (FIntPoint targetRoomCoords, FIntPoint targetPosition);

    /* Target the very center of the maze grid (centre point in centre room) */
    UFUNCTION(BlueprintCallable, Category = "Enemy Behaviour")
        virtual void TargetCenter();

    ///* Move along the given direction, regardless of validity. (This is used to force characters through doors). */
    //UFUNCTION(BlueprintCallable, Category = "Enemy Movement")
    //    void UpdateMovementForActionType(EDirectionType actionType);

    /* Choose a door in the current room and update the policy to direct towards that door. 
        return true if the door chosen is the current position
    */
    UFUNCTION(BlueprintCallable, Category = "Enemy Behaviour")
        virtual void ChooseDoorTarget(bool& movementTargetUpdated);

    /** Determines whether the enemy should augment the reward of state actions when it receives damage or dies */
    UPROPERTY(EditAnywhere, Category = "Enemy Behaviour")
        bool AccumulateReward = false;

    /** Determines whether the enemy should update the qvalue table after every action */
    UPROPERTY(EditAnywhere, Category = "Enemy Behaviour")
        bool UpdateQValue = true;
    //======================================================================================================
    // Movement
    //====================================================================================================== 
	UFUNCTION(BlueprintCallable, Category = "Enemy Movement")
		bool IsPositionValid();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Enemy Movement")
        FVector MovementTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy Movement")
        float RotationSpeed = 0.1f;

private:
    ATPGameDemoGameState* GameState;

    FTargetPosition   TargetPositionAndAction; // Intermediate movement target, while navigating to target room.
    FRoomPositionPair TargetRoomAndPosition = { FIntPoint(0, 0), FIntPoint(4, 4) }; // default to center of central room.

    void SetTargetPositionAndAction(FTargetPosition);
    void EnteredTargetRoom();

    void ActorDied() override;
    //======================================================================================================
    // Behaviour Policy
    //======================================================================================================
    void UpdatePolicyForDoorType(EDirectionType doorTypem, int doorPositionOnWall);
    // Data to store for updating the state-action qvalues. The qvalues are only updated for an action when taking the subsequent action.
    // This is so that rewards can be accumulated while in a state, and then applied to the qvalue when moving out of that state.
    // (It's an attempt to introduce variable rewards. For example, getting shot, or dying, will accumulate negative reward).
    FRoomPositionPair PrevActionStartPos;
    EDirectionType PrevActionType = EDirectionType::NumDirectionTypes;
    FIntPoint PrevActionTarget;
    float PrevActionAccumulatedReward = 0.0f;
    struct RoomPosition
    {
        int RoomSideLength;
        FIntPoint Position;
    };
    struct RoomPosition_ComparePositive
    {
        bool operator() (const RoomPosition& lhs, const RoomPosition& rhs) const
        {
            return (lhs.Position.X * lhs.RoomSideLength + lhs.Position.Y) < (rhs.Position.X * rhs.RoomSideLength + rhs.Position.Y);
            if (lhs.Position.X < rhs.Position.X)
                return lhs.Position.Y < rhs.Position.Y;
            return false;
        }
    };
    // Used to deter loops - punish repeated positions (for the same destination target position)
    std::set<RoomPosition, RoomPosition_ComparePositive> VisitedPositions;
    //======================================================================================================
    // Movement
    //====================================================================================================== 
    void UpdateMovement();
    // Returns the room coords and position in room indicated by the given movement direction, determined by the actors current position.
    // If the movement action would cause them to change rooms, roomCoords will indicate which room they would enter.
    FRoomPositionPair GetTargetRoomAndPositionForDirectionType(EDirectionType actionType);
    /* Move along the given direction if possible. This may be called recursively if an invalid action is chosen. */
    void UpdateMovementForActionType(EDirectionType actionType, int numCalls = 0);
    bool ShouldUpdateQValue() const;
    /* The door through which the current room was entered */
    EDirectionType PreviousDoor = EDirectionType::NumDirectionTypes;

    //======================================================================================================
    // From AMazeActor
    //====================================================================================================== 
    virtual void PositionChanged() override;
    virtual void RoomCoordsChanged() override;
    virtual void TakeDamage(float damageAmount) override;
    //======================================================================================================
    // Logging
    //======================================================================================================
#if ENEMY_LIFETIME_LOGS
    FString               LogDir;
    bool                  LogDirFound = false;
    bool                  SaveLifetimeLog = false;
    FString               LifetimeLog;

    void LogLine(const FString& lineString);
    void LogEvent(const FString& eventInfo, ELogEventType logType);
    FIntPoint RoomAtLastEventLog;
    void LogRoom();
    FIntPoint PositionAtLastEventLog;
    void LogPosition();
    FTargetPosition TargetAtLastEventLog;
    void LogTarget();
    FVector WorldPosAtLastEventLog;
    void LogWorldPosition();
    void LogDetails();
    void SaveLifetimeString();
    void AssertWithErrorLog(const bool& condition, FString errorLog);
#endif
};
