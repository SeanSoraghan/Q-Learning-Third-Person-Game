// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "TimelineContainerComponent.h"
#include "MazeActor.h"
#include "BaseCharacter.generated.h"

UENUM(BlueprintType)
enum class ECameraAnimationState : uint8
{
    RotatingFollowToAim       UMETA (DisplayName = "FollowToAim"),
    RestoringFollowToPrevious UMETA (DisplayName = "FollowToPrevious"),
    RotatingAimToFollow       UMETA (DisplayName = "AimToFollow"),
    RestoringAimToPrevious    UMETA (DisplayName = "AimToPrevious"),
    NotAnimating              UMETA (DisplayName = "NotAnimating"),
    NumStates
};

UENUM(BlueprintType)
enum class EControlState : uint8
{
    Combat  UMETA (DisplayName = "Combat"),
    Explore UMETA (DisplayName = "Explore"),
    NumStates
};

UENUM(BlueprintType)
enum class ECameraControlType : uint8
{
    RotateCamera  UMETA(DisplayName = "Rotate Camera"),
    RotatePlayer UMETA(DisplayName = "Rotate Player"),
    NumTypes
};

UENUM(BlueprintType)
enum class EBuildableActorType : uint8
{
    None    UMETA (DisplayName = "None"),
    Turret  UMETA (DisplayName = "Turret"),
    Mine    UMETA (DisplayName = "Mine"),
    NumBuildables
};

enum class EMovementDirectionType : uint8
{
    Forward   UMETA (DisplayName = "Forward") = 0,
    Right     UMETA (DisplayName = "Right"),
    Backwards UMETA (DisplayName = "Backwards"),
    Left      UMETA (DisplayName = "Left"),
    NumMovementDirections
};

//===========================================================================================
//===========================================================================================

struct SMovementKeysPressedState
{
    SMovementKeysPressedState ()
    {
        for (int d = 0; d < (int) EMovementDirectionType::NumMovementDirections; d++)
            DirectionStates[d] = false;
    }

    bool AreAnyKeysPressed() 
    {
        for (uint8 d = 0; d < (uint8) EMovementDirectionType::NumMovementDirections; d++)
            if (DirectionStates[(int) d])
                return true;

        return false;
    }

    bool DirectionStates[4];
};
//===========================================================================================
//===========================================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE (FPlayerFired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE (FPlayerReleasedFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE (FPlayerControlStateChanged);
DECLARE_DELEGATE_OneParam          (FHotkeyDelegate, int);

/*
The base class for the main player character. Includes functionality for input handling, movement, and animating between viewpoints.

Two possible `control states' are implemented: explore and combat. The right mouse button (aim button) can be used to switch between 
the two control states. 

The explore control state is a bird's-eye simple movement control scheme (forward, back, left right). The controller
rotation remains constant and the inner mesh changes its relative rotation to face in the direction of movement.

The combat control state is an over-the-shoulder third person control scheme with forwards and backwards movement,
horizontal strafing, and mouse-controlled aiming. Mouse movement controls the controller rotation and the mesh 
rotation remains constant at the DefaultMeshRotation value. The mesh is animated by the animation blueprint
(in the UE editor) depending on the rotation delta between the velocity rotation and forward-look rotation 
(which can be queried from the FRotator GetNormalisedVelocityDeltaRotation() function).

When a control state changes, the viewpoint animates between the two control states. When animating from the explore
to combat control state, the controller rotation must animate to match the mesh forward rotation, or vice-versa. In 
the current implementation, only the mesh rotates to match the controller rotation. This is achieved using the 
Timeline, which calls InterpolateControlRotationToMeshRotation (float deltaTime) continuously while it is playing. In the 
derived blueprint class in the UE editor (PlayerCharacter blueprint class) there are two cameras for each of the control states.
These cameras animate their position depending on the control state, and this animation makes use of the "Base Character Camera Movement"
properties.

See MazeActor.h
*/

UCLASS()
class TPGAMEDEMO_API ABaseCharacter : public AMazeActor
{
	GENERATED_BODY()

public:
    
	//=========================================================================================
    // Initialisation
    //=========================================================================================
	ABaseCharacter (const FObjectInitializer& ObjectInitializer);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
    

	//=========================================================================================
    // Continuous Updating
    //=========================================================================================
	virtual void Tick   (float deltaSeconds) override;
    void UpdateMovement (float deltaSeconds);

	//=========================================================================================
    // Input
    //=========================================================================================
	virtual void SetupPlayerInputComponent (class UInputComponent* inputComponent) override;
    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Interaction")
        void InteractPressed();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Base Character Camera Movement")
        ECameraControlType CameraControlType = ECameraControlType::RotatePlayer;

    void ControlStateChanged();
    //=========================================================================================
    // Camera
    //=========================================================================================
	UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Base Character Camera Movement")
        ECameraAnimationState CameraAnimationState = ECameraAnimationState::NotAnimating;
    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Base Character Camera Movement")
        FRotator DefaultLookExploreRotation = FRotator (0.0f, 0.0f, 0.0f);
    UPROPERTY (BlueprintReadWrite, EditAnywhere, Category = "Base Character Camera Movement")
        FRotator DefaultLookCombatRotation  = FRotator (0.0f, 0.0f, 0.0f);

    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Camera Movement")
        void UpdateFollowCameraPosition (float delta);
    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Camera Movement")
        void ZoomOut();
    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Camera Movement")
        void ZoomIn();
    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Camera Movement")
        void MapViewPressed();
    UFUNCTION(BlueprintImplementableEvent, Category = "Base Character Camera Movement")
        void MapViewReleased();
    //=========================================================================================
    // Movement
    //=========================================================================================
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FRotator GetNormalisedVelocityDeltaRotation();
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FRotator GetNormalizedMeshWorldForwardRotation();
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FRotator GetNormalizedRelativeMeshRotation();
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FRotator GetNormalizedMovementRotation();
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FRotator GetNormalizedLookRotation();
    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        virtual FVector GetMovementVector();

    UFUNCTION (BlueprintCallable, Category = "Base Character Movement")
        void UpdateControlRotation();
    UFUNCTION (BlueprintImplementableEvent, Category = "Base Character Movement")
        void OnPlayerControlRotationUpdated();

    UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Base Character Movement")
        EControlState ControlState = EControlState::Explore;

    //=========================================================================================
    // Shooting
    //=========================================================================================
    UPROPERTY (BlueprintAssignable, Category = "Base Character Shooting")
        FPlayerFired OnPlayerFired;

    UPROPERTY(BlueprintAssignable, Category = "Base Character Shooting")
        FPlayerReleasedFire OnFireReleased;

    UPROPERTY(BlueprintAssignable, Category = "Base Character Shooting")
        FPlayerControlStateChanged OnControlStateChanged;
    
    //=========================================================================================
    // Building
    //=========================================================================================
    UPROPERTY (BlueprintReadWrite, Category = "Base Character Building")
        EBuildableActorType BuildableItem;
    UFUNCTION (BlueprintImplementableEvent, Category = "Base Character Building")
        void OnPlayerBuildItemChanged();
    
private:
    //=========================================================================================
    // Input Responders
    //=========================================================================================
    void BindInput();
    void EnterCombatControlMode();
    void EnterExploreControlMode();

    void UpdateMovementControls();

    void SetupCombatMovementControls();
    void SetupExploreMovementControls();

    void UpdateMovementForcesForDirectionKey (EMovementDirectionType direction, bool pressed);

    void CombatDirectionPressed  (EMovementDirectionType direction);
    void CombatDirectionReleased (EMovementDirectionType direction);

    void CombatForwardPressed();
    void CombatForwardReleased();
    void CombatBackwardsPressed();
    void CombatBackwardsReleased();
    void CombatRightPressed();
    void CombatRightReleased();
    void CombatLeftPressed();
    void CombatLeftReleased();

    void ExploreDirectionPressed  (EMovementDirectionType direction);
    void ExploreDirectionReleased (EMovementDirectionType direction);
    void CheckForPressedKeys();

    void ExploreForwardPressed();
    void ExploreForwardReleased();
    void ExploreBackwardsPressed();
    void ExploreBackwardsReleased();
    void ExploreRightPressed();
    void ExploreRightReleased();
    void ExploreLeftPressed();
    void ExploreLeftReleased();
    void ItemHotkeyPressed(int itemNumber);
    void UpdateMeshRotationForExploreDirection();

    SMovementKeysPressedState MovementKeysPressedState;
    

    void UpdateVerticalLookRotation   (float delta);
    void UpdateHorizontalLookRotation (float delta);
    
    //=========================================================================================
    // Movement Forces
    //=========================================================================================
    float FrontBackMovementForce = 0.0f;
    float RightLeftMovementForce = 0.0f;

    float VerticalLookRotation   = 0.0f;
    float HorizontalLookRotation = 0.0f;
    FRotator DefaultMeshRotation;

    //=========================================================================================
    // Shooting
    //=========================================================================================
    void PlayerFired();
    void FireReleased();

    //=========================================================================================
    // Timeline
    //=========================================================================================
    UTimelineContainerComponent* TimelineContainer;

    UFUNCTION()
    void TimelineUpdated  (float val);

    UFUNCTION()
    void TimelineFinished();

    FRotator TimelineTargetLookRotation;
    FRotator TimelineTargetMeshRotation;

    //=========================================================================================
    // Build Items
    //=========================================================================================
    

    //=========================================================================================
    // Helper Functions
    //=========================================================================================
    void     LimitVerticalLookRotation();
    void     LimitHorizontalLookRotation();
    
    float    GetInputVelocity();
    void     UpdateTimelineTargetRotations();
    void     InterpolateControlRotationToMeshRotation (float deltaTime);
    void     PrintToScreen                            (const FString message);
    bool     IsDirectionPressed                       (EMovementDirectionType direction);
    //Debugging
    
};
