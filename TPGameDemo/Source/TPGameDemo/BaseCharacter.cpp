// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "BaseCharacter.h"
#include "TPGameDemoGameMode.h"
#include "Kismet/KismetMathLibrary.h"

//======================================================================================================
// Initialisation
//====================================================================================================== 
ABaseCharacter::ABaseCharacter (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
    TimelineContainer = ObjectInitializer.CreateDefaultSubobject<UTimelineContainerComponent> (this, TEXT("TimelineContainer"));
    TimelineContainer->TimelineInterpFunction.BindUFunction   (this, FName { TEXT ("TimelineUpdated") });
    TimelineContainer->TimelineFinishedFunction.BindUFunction (this, FName { TEXT ("TimelineFinished") });
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	DefaultMeshRotation = GetMesh()->RelativeRotation;

    //======================================================================================================
    // Timeline
    //====================================================================================================== 
  #if ON_SCREEN_DEBUGGING
    PrintToScreen (TimelineContainer->CurveLoadedResponse);
  #endif
}


//=========================================================================================
// Continuous Updating
//=========================================================================================
void ABaseCharacter::Tick (float deltaTime)
{
	Super::Tick    (deltaTime);
    UpdateMovement (deltaTime);
}

void ABaseCharacter::UpdateMovement (float deltaTime)
{
    float speed = 1.0f;
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    if (gameMode != nullptr)
       speed = gameMode->Walking_Movement_Force;

    FVector movementDirection = GetMovementVector();
    
    movementDirection.Normalize();
    movementDirection *= speed * GetInputVelocity();
    AddMovementInput (movementDirection);  
}

//=========================================================================================
// Movement
//=========================================================================================
FRotator ABaseCharacter::GetNormalisedVelocityDeltaRotation()
{
    return UKismetMathLibrary::NormalizedDeltaRotator (GetMovementVector().Rotation(), GetNormalizedMeshWorldForwardRotation());
}

FRotator ABaseCharacter::GetNormalizedLookRotation()
{
    FRotator lookRotation = GetController()->GetControlRotation();
    lookRotation.Normalize();
    return lookRotation;
}

FRotator ABaseCharacter::GetNormalizedMovementRotation()
{
    FVector movementVector (FrontBackMovementForce, RightLeftMovementForce, 0.0f);
    movementVector.Normalize();
    FRotator movementRotation  = movementVector.Rotation();
    movementVector.Normalize();
    return movementRotation;
}

FRotator ABaseCharacter::GetNormalizedMeshWorldForwardRotation()
{
    FRotator relativeMeshRotation = GetMesh()->RelativeRotation;
    FRotator meshDeltaRotation    = UKismetMathLibrary::NormalizedDeltaRotator (relativeMeshRotation, DefaultMeshRotation);
    FRotator meshForwardRotation  = GetController()->GetControlRotation() + meshDeltaRotation;
    meshForwardRotation.Normalize();

    return meshForwardRotation;
}

FRotator ABaseCharacter::GetNormalizedRelativeMeshRotation()
{
    FRotator normalizedRelativeMeshRotation = GetMesh()->RelativeRotation;
    normalizedRelativeMeshRotation.Normalize();
    return normalizedRelativeMeshRotation;
}

FVector ABaseCharacter::GetMovementVector()
{
    FRotator lookRotation = GetNormalizedLookRotation();
    lookRotation.Pitch = 0.0f;
    FVector movementDirection = UKismetMathLibrary::GetForwardVector (GetNormalizedMovementRotation() + lookRotation);
    return movementDirection;
}

//=========================================================================================
// Input
//=========================================================================================
void ABaseCharacter::SetupPlayerInputComponent (class UInputComponent* inputComponent)
{
	Super::SetupPlayerInputComponent (inputComponent);
    BindInput(); 
}

void ABaseCharacter::BindInput()
{
    if (InputComponent != nullptr)
    {
        InputComponent->ClearActionBindings();
        InputComponent->AxisBindings.Empty();
        for (int axis = 0; axis < InputComponent->AxisBindings.Num(); axis++)
            InputComponent->AxisBindings.RemoveAt (axis);
        
        InputComponent->BindAction ("aim", IE_Pressed,  this, &ABaseCharacter::EnterCombatControlMode);
        InputComponent->BindAction ("aim", IE_Released, this, &ABaseCharacter::EnterExploreControlMode);

        UpdateMovementControls();
    }
}

void ABaseCharacter::EnterCombatControlMode()
{
    ControlState = EControlState::Combat;

    UpdateTimelineTargetRotations();

    //VerticalLookRotation = DefaultLookCombatRotation.Pitch;
    //HorizontalLookRotation = DefaultLookCombatRotation.Yaw;
    UpdateControlRotation();
    TimelineContainer->Timeline->PlayFromStart();

    BindInput();
}


void ABaseCharacter::EnterExploreControlMode()
{
    ControlState = EControlState::Explore;

    //VerticalLookRotation = DefaultLookExploreRotation.Pitch;
    //HorizontalLookRotation = DefaultLookExploreRotation.Yaw;
    UpdateControlRotation();

    BindInput();
}

void ABaseCharacter::UpdateMovementControls()
{
    switch (ControlState)
    {
        case EControlState::Explore:
        {
            SetupExploreMovementControls();
            break;
        }
        case EControlState::Combat:
        {
            SetupCombatMovementControls();
            break;
        }
    }
}

void ABaseCharacter::SetupCombatMovementControls()
{
    if (InputComponent != nullptr)
    {
        InputComponent->BindAction ("move-forward",   IE_Pressed,  this, &ABaseCharacter::CombatForwardPressed);
        InputComponent->BindAction ("move-forward",   IE_Released, this, &ABaseCharacter::CombatForwardReleased);
        InputComponent->BindAction ("move-backwards", IE_Pressed,  this, &ABaseCharacter::CombatBackwardsPressed);
        InputComponent->BindAction ("move-backwards", IE_Released, this, &ABaseCharacter::CombatBackwardsReleased);
        InputComponent->BindAction ("move-right",     IE_Pressed,  this, &ABaseCharacter::CombatRightPressed);
        InputComponent->BindAction ("move-right",     IE_Released, this, &ABaseCharacter::CombatRightReleased);
        InputComponent->BindAction ("move-left",      IE_Pressed,  this, &ABaseCharacter::CombatLeftPressed);
        InputComponent->BindAction ("move-left",      IE_Released, this, &ABaseCharacter::CombatLeftReleased);

        InputComponent->BindAxis   ("look-up",    this, &ABaseCharacter::UpdateVerticalLookRotation);
        InputComponent->BindAxis   ("look-right", this, &ABaseCharacter::UpdateHorizontalLookRotation);

        InputComponent->BindAction ("fire", IE_Pressed, this, &ABaseCharacter::PlayerFired);
    }
}


void ABaseCharacter::SetupExploreMovementControls()
{
    if (InputComponent != nullptr)
    {
        InputComponent->BindAction ("move-forward",   IE_Pressed,  this, &ABaseCharacter::ExploreForwardPressed);
        InputComponent->BindAction ("move-forward",   IE_Released, this, &ABaseCharacter::ExploreForwardReleased);
        InputComponent->BindAction ("move-backwards", IE_Pressed,  this, &ABaseCharacter::ExploreBackwardsPressed);
        InputComponent->BindAction ("move-backwards", IE_Released, this, &ABaseCharacter::ExploreBackwardsReleased);
        InputComponent->BindAction ("move-right",     IE_Pressed,  this, &ABaseCharacter::ExploreRightPressed);
        InputComponent->BindAction ("move-right",     IE_Released, this, &ABaseCharacter::ExploreRightReleased);
        InputComponent->BindAction ("move-left",      IE_Pressed,  this, &ABaseCharacter::ExploreLeftPressed);
        InputComponent->BindAction ("move-left",      IE_Released, this, &ABaseCharacter::ExploreLeftReleased);

        InputComponent->BindAxis   ("look-right", this, &ABaseCharacter::UpdateHorizontalLookRotation);
    }
}

//=========================================================================================
// Input Responders
//=========================================================================================
void ABaseCharacter::UpdateVerticalLookRotation (float delta)
{
    if (! TimelineContainer->Timeline->IsPlaying())
    {
        VerticalLookRotation += delta;
        LimitVerticalLookRotation();
        UpdateControlRotation();
    }
}

void ABaseCharacter::UpdateHorizontalLookRotation (float delta)
{
    if (! TimelineContainer->Timeline->IsPlaying())
    {
        HorizontalLookRotation += delta;
        LimitHorizontalLookRotation();
        UpdateControlRotation();
    }
}

void ABaseCharacter::UpdateControlRotation()
{
    FRotator currentLookRotation = GetNormalizedLookRotation();
    FRotator newLookRotation (VerticalLookRotation, HorizontalLookRotation, currentLookRotation.Roll);
    newLookRotation.Normalize();
    GetController()->SetControlRotation (newLookRotation);
    OnPlayerControlRotationUpdated.Broadcast();
}

void ABaseCharacter::CombatDirectionPressed (EDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = true;
    UpdateMovementForcesForDirectionKey (direction, true);
}

void ABaseCharacter::CombatDirectionReleased (EDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = false;
    UpdateMovementForcesForDirectionKey (direction, false);
}

void ABaseCharacter::CombatForwardPressed()     { CombatDirectionPressed  (EDirectionType::Forward); }
void ABaseCharacter::CombatForwardReleased()    { CombatDirectionReleased (EDirectionType::Forward); }
void ABaseCharacter::CombatBackwardsPressed()   { CombatDirectionPressed  (EDirectionType::Backwards); }
void ABaseCharacter::CombatBackwardsReleased()  { CombatDirectionReleased (EDirectionType::Backwards); }
void ABaseCharacter::CombatRightPressed()       { CombatDirectionPressed  (EDirectionType::Right); }
void ABaseCharacter::CombatRightReleased()      { CombatDirectionReleased (EDirectionType::Right); }
void ABaseCharacter::CombatLeftPressed()        { CombatDirectionPressed  (EDirectionType::Left); }
void ABaseCharacter::CombatLeftReleased()       { CombatDirectionReleased (EDirectionType::Left); }

void ABaseCharacter::ExploreDirectionPressed (EDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = true;
    UpdateMovementForcesForDirectionKey (direction, true);
    UpdateMeshRotationForExploreDirection();
}

void ABaseCharacter::ExploreDirectionReleased (EDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = false;
    UpdateMovementForcesForDirectionKey (direction, false);

    if (MovementKeysPressedState.AreAnyKeysPressed())
        UpdateMeshRotationForExploreDirection();
}

void ABaseCharacter::UpdateMeshRotationForExploreDirection()
{
    FRotator newNormalizedMeshRotation = GetNormalizedMovementRotation() + DefaultMeshRotation;
    newNormalizedMeshRotation.Normalize();
    GetMesh()->SetRelativeRotation (newNormalizedMeshRotation);
}

void ABaseCharacter::UpdateMovementForcesForDirectionKey (EDirectionType direction, bool pressed)
{
    switch (direction)
    {
        case EDirectionType::Forward:
        {
            FrontBackMovementForce = pressed ? 1.0f : IsDirectionPressed (EDirectionType::Backwards) ? -1.0f : 0.0f; 
            break;
        }
        case EDirectionType::Backwards: 
        { 
            FrontBackMovementForce = pressed ? -1.0f : IsDirectionPressed (EDirectionType::Forward) ? 1.0f : 0.0f; 
            break; 
        }
        case EDirectionType::Right:     
        { 
            RightLeftMovementForce = pressed ? 1.0f  : IsDirectionPressed (EDirectionType::Left) ? -1.0f : 0.0f; 
            break; 
        }
        case EDirectionType::Left:      
        { 
            RightLeftMovementForce = pressed ? -1.0f : IsDirectionPressed (EDirectionType::Right) ? 1.0f : 0.0f; 
            break; 
        }
    }
}

void ABaseCharacter::CheckForPressedKeys()
{
    for (int d = 0; d < (int) EDirectionType::NumDirections; d++)
    {
        if (MovementKeysPressedState.DirectionStates[d])
        {
            ExploreDirectionPressed ((EDirectionType) d);
            return;
        }
    }
}

void ABaseCharacter::ExploreForwardPressed()    { ExploreDirectionPressed  (EDirectionType::Forward); }
void ABaseCharacter::ExploreForwardReleased()   { ExploreDirectionReleased (EDirectionType::Forward); }
void ABaseCharacter::ExploreBackwardsPressed()  { ExploreDirectionPressed  (EDirectionType::Backwards); }
void ABaseCharacter::ExploreBackwardsReleased() { ExploreDirectionReleased (EDirectionType::Backwards); }
void ABaseCharacter::ExploreRightPressed()      { ExploreDirectionPressed  (EDirectionType::Right); }
void ABaseCharacter::ExploreRightReleased()     { ExploreDirectionReleased (EDirectionType::Right); }
void ABaseCharacter::ExploreLeftPressed()       { ExploreDirectionPressed  (EDirectionType::Left); }
void ABaseCharacter::ExploreLeftReleased()      { ExploreDirectionReleased (EDirectionType::Left); }

void ABaseCharacter::PlayerFired() { OnPlayerFired.Broadcast(); }
//=========================================================================================
// Timeline
//=========================================================================================
void ABaseCharacter::TimelineUpdated (float val)
{
    InterpolateControlRotationToMeshRotation (val);
}

void ABaseCharacter::TimelineFinished()
{
    GetMesh()->SetRelativeRotation (DefaultMeshRotation);
}

//=========================================================================================
// Helper Functions
//=========================================================================================
bool ABaseCharacter::IsDirectionPressed (EDirectionType direction)
{
    return MovementKeysPressedState.DirectionStates[(int) direction]; 
}

void ABaseCharacter::LimitVerticalLookRotation()
{
        float maxPitch = 90.0f;
        float minPitch = -90.0f;
        ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

        if (gameMode != nullptr)
        {
           maxPitch = gameMode->Max_Pitch_Look;
           minPitch = gameMode->Min_Pitch_Look;
        }

        FMath::Clamp (VerticalLookRotation, minPitch, maxPitch);
}

void ABaseCharacter::LimitHorizontalLookRotation()
{
    // map to 0.0f - 360.0f
    HorizontalLookRotation = fmod (HorizontalLookRotation, 360.0f);
    if (HorizontalLookRotation < 0.0f)
        HorizontalLookRotation += 360.0f;

    // map to -180.0f - 180.0f
    if (HorizontalLookRotation > 180.0f)
        HorizontalLookRotation = (((HorizontalLookRotation - 180.0f) / 180.0f) - 1.0f) * 180.0f;
}

float ABaseCharacter::GetInputVelocity()
{
    FVector movementVector (FrontBackMovementForce, RightLeftMovementForce, 0.0f);
    
    if (movementVector.Size() > 1.0f)
        movementVector.Normalize();
    
    return movementVector.Size();
}

// Called continuously by the timeline while it's playing.
void ABaseCharacter::InterpolateControlRotationToMeshRotation (float deltaTime)
{
    float interpSpeed = 1.0f;
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    if (gameMode != nullptr)
        interpSpeed = gameMode->Camera_Animation_Speed;

    /* To interpolate to mesh forward rotation */
    //FRotator newRotation = UKismetMathLibrary::RInterpTo (GetNormalizedLookRotation(), TimelineTargetLookRotation, deltaTime, interpSpeed);
    //
    //VerticalLookRotation = DefaultLookCombatPitch;
    //HorizontalLookRotation = newRotation.Yaw;
    //LimitVerticalLookRotation();
    //LimitHorizontalLookRotation();
    //UpdateControlRotation();

    GetMesh()->SetRelativeRotation (UKismetMathLibrary::RInterpTo (GetNormalizedRelativeMeshRotation(), TimelineTargetMeshRotation, deltaTime, interpSpeed));
}

void ABaseCharacter::UpdateTimelineTargetRotations()
{
    TimelineTargetLookRotation = GetNormalizedLookRotation() + UKismetMathLibrary::NormalizedDeltaRotator (GetNormalizedMeshWorldForwardRotation(), GetNormalizedLookRotation());
    TimelineTargetLookRotation.Normalize();

    if (FMath::Abs (FMath::Abs (DefaultMeshRotation.Yaw) - FMath::Abs(GetNormalizedRelativeMeshRotation().Yaw)) < 2.0f)
        GetMesh()->SetRelativeRotation (DefaultMeshRotation);

    TimelineTargetMeshRotation = GetNormalizedRelativeMeshRotation() + UKismetMathLibrary::NormalizedDeltaRotator (DefaultMeshRotation, GetNormalizedRelativeMeshRotation());
    TimelineTargetMeshRotation.Normalize();
}

//Debugging
void ABaseCharacter::PrintToScreen (const FString message) { GEngine->AddOnScreenDebugMessage(-1, 8.f, FColor::Red, message); }
