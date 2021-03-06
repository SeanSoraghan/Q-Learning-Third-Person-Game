// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GameFramework/Character.h"
#include "TPGameDemo.h"
//#include "TextParserComponent.h"
#include "MazeActor.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE (FGridPositionChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE (FRoomCoordsChanged);
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

    // Hijacking the flying movement mode to implement knockback movement.
    static const EMovementMode ImpulseMovement = EMovementMode::MOVE_Flying;

public:	
	// Sets default values for this actor's properties
	AMazeActor (const FObjectInitializer& ObjectInitializer);
    

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

    /** Use this to change whether a specific implementation of MazeActor should count towards a positions actor count. */
    UFUNCTION(BlueprintCallable, Category = "Maze Actor Positions")
        void SetOccupyCells(bool bShouldOccupy);

    UFUNCTION(BlueprintCallable, Category = "Maze Actor Positions")
        void InitialisePosition(FIntPoint roomCoords);
	//=========================================================================================
    // Maze Position
    //=========================================================================================
    UPROPERTY(BlueprintReadWrite, Category = "Actor Grid Positions")
        bool ShouldUpdatePosition = true;
    UPROPERTY (BlueprintReadOnly, Category = "Actor Grid Positions")
        FIntPoint CurrentRoomCoords = FIntPoint(0,0);
    UPROPERTY (BlueprintReadOnly, Category = "Actor Grid Positions")
        int GridXPosition                 = 0;
    UPROPERTY (BlueprintReadOnly, Category = "Actor Grid Positions")
        int GridYPosition                 = 0;
    UFUNCTION(BlueprintCallable, Category = "Actor Grid Positions")
        FRoomPositionPair GetRoomAndPosition();

    UPROPERTY (BlueprintAssignable, Category = "Actor Grid Positions")
        FGridPositionChanged GridPositionChangedEvent;

    UPROPERTY (BlueprintAssignable, Category = "Actor Room Coords")
        FRoomCoordsChanged RoomCoordsChangedEvent;

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

    virtual void ActorDied();

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Health")
        virtual void CheckDeath();

    UPROPERTY (BlueprintAssignable, Category = "Maze Actor Health")
        FActorDied OnActorDied;

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Maze Model")
        void UpdateMazeDimensions();

    UFUNCTION (BlueprintCallable, Category = "Maze Actor Position")
        bool IsOnGridEdge() const;

    UFUNCTION(BlueprintCallable, Category = "Maze Actor Position")
        bool WasOnGridEdge() const;

    UFUNCTION(BlueprintCallable, Category = "Maze Actor Impulse")
        void AddImpulseForce(FVector direction, float duration, float normedForce = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "Maze Actor Impulse")
        bool UndergoingImpulse() const;

    FIntPoint GetPreviousRoomCoords() const { return PreviousRoomCoords; }

private:
    void UpdatePosition (bool broadcastChange = true);

    virtual void PositionChanged();
    virtual void RoomCoordsChanged();

    int CurrentLevelNumGridUnitsX     = 10;
    int CurrentLevelNumGridUnitsY     = 10;
    int CurrentLevelGridUnitLengthXCM = 200;
    int CurrentLevelGridUnitLengthYCM = 200;
    int PreviousGridXPosition         = 0;
    int PreviousGridYPosition         = 0;
    FIntPoint PreviousRoomCoords      = FIntPoint(0,0);
    bool bOccupyCells = true;

    /** An impulse direction that is used to add impulse force every frame when the actor takes impulses */
    FVector ImpulseDirection = FVector::ZeroVector;
    float CurrentImpulseStrength = 0.0f;
    float InitialImpulseStrength = 0.0f;
    float SecondsSinceLastImpulse = 0.0f;
    float CurrentImpulseDuration = 0.0f;
    void UpdateImpulseStrength(float deltaTime);
};
