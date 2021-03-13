// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "BaseCharacter.h"
#include "TPGameDemoGameMode.h"
#include "Kismet/KismetMathLibrary.h"

#define ENABLE_CLOSE_CAMERA 1

//======================================================================================================
// Initialisation
//====================================================================================================== 
ABaseCharacter::ABaseCharacter (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer)
{
    SetOccupyCells(false);
	PrimaryActorTick.bCanEverTick = true;
    TimelineContainer = ObjectInitializer.CreateDefaultSubobject<UTimelineContainerComponent> (this, TEXT("TimelineContainer"));
    TimelineContainer->TimelineInterpFunction.BindUFunction   (this, FName { TEXT ("TimelineUpdated") });
    TimelineContainer->TimelineFinishedFunction.BindUFunction (this, FName { TEXT ("TimelineFinished") });
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
#if UE_4_25_OR_LATER
	DefaultMeshRotation = GetMesh()->GetRelativeRotation();
#else
    DefaultMeshRotation = GetMesh()->RelativeRotation;
#endif
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
	Super::Tick (deltaTime);
    if (GetCameraControlType() == ECameraControlType::TwinStick && ControlState == EControlState::Combat)
    {
        RotateMeshToMousePosition();
    }
    if (USkeletalMeshComponent* mesh = GetMesh())
    {
        FRotator meshRotation = mesh->GetRelativeRotation();
        if (!meshRotation.Equals(TargetLookRotation, 0.1f))
        {
            if (ControlState == EControlState::Explore)
            {
                mesh->SetRelativeRotation(UKismetMathLibrary::RInterpTo(meshRotation, TargetLookRotation, deltaTime, LookRotationSpeed));
            }
            else if (ControlState == EControlState::Combat)
            {
                if (FMath::FindDeltaAngleDegrees(meshRotation.Euler().Z, TargetLookRotation.Euler().Z) > 20.0f)
                    mesh->SetRelativeRotation(UKismetMathLibrary::RInterpTo(meshRotation, TargetLookRotation, deltaTime, LookRotationSpeed));
                else
                    mesh->SetRelativeRotation(TargetLookRotation);
            }
        }
    }
    
    UpdateMovement (deltaTime);
}

void ABaseCharacter::UpdateMovement (float deltaTime)
{
    float speed = 1.0f;
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();

    if (gameMode != nullptr)
    {
        if (ControlState == EControlState::Combat)
            speed = gameMode->Walking_Movement_Force;
        else if (ControlState == EControlState::Explore)
            speed = gameMode->Running_Movement_Force;
    }

    FVector movementDirection = GetMovementVector();    
    movementDirection.Normalize();
    if (IsBoosting)
    {
        BoostLerpLinear = TimeSinceBoost / BoostLengthSeconds;
        float boostLog = FMath::Loge(9.0f * (BoostLerpLinear) + 1.0f);
        FVector newLocation = FMath::Lerp(BoostStartPos, BoostDestPos, boostLog);
        FHitResult hitResult;
        SetActorLocation(newLocation, true, &hitResult, ETeleportType::TeleportPhysics);
        TimeSinceBoost += deltaTime;
        if (TimeSinceBoost >= BoostLengthSeconds || hitResult.bBlockingHit)
        {
            IsBoosting = false;
            OnBoostEnded.Broadcast();
        }
    }
    movementDirection *= GetInputVelocity();
    AddMovementInput(movementDirection, speed);
}

//=========================================================================================
// Input
//=========================================================================================
void  ABaseCharacter::ControlStateChanged()
{
    OnControlStateChanged.Broadcast();
}

ECameraControlType  ABaseCharacter::GetCameraControlType()
{
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*)GetWorld()->GetAuthGameMode();

    if (gameMode != nullptr)
        return gameMode->CameraControlType;

    return ECameraControlType::RotatePlayer;
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
    FRotator lookRotation; 
    if (GetController() != nullptr)
        lookRotation = GetController()->GetControlRotation();
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
#if UE_4_25_OR_LATER
    FRotator relativeMeshRotation = GetMesh()->GetRelativeRotation();
#else
    FRotator relativeMeshRotation = GetMesh()->RelativeRotation;
#endif
    FRotator meshDeltaRotation    = UKismetMathLibrary::NormalizedDeltaRotator (relativeMeshRotation, DefaultMeshRotation);
    FRotator meshForwardRotation;
    if (GetController() != nullptr)
        meshForwardRotation  = GetController()->GetControlRotation() + meshDeltaRotation;
    meshForwardRotation.Normalize();

    return meshForwardRotation;
}

FRotator ABaseCharacter::GetNormalizedRelativeMeshRotation()
{
#if UE_4_25_OR_LATER
    FRotator normalizedRelativeMeshRotation = GetMesh()->GetRelativeRotation();
#else
    FRotator normalizedRelativeMeshRotation = GetMesh()->RelativeRotation;
#endif
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
// Shooting
//=========================================================================================
void ABaseCharacter::DrawLineToTarget(FVector WorldStartPos, FVector WorldEndPos, FLinearColor lineColor, float lineThickness)
{
    GetWorld()->LineBatcher->DrawLine(WorldStartPos, WorldEndPos, lineColor, SDPG_World, lineThickness);
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
        InputComponent->BindAction("aim-buildable-placement", IE_Pressed, this, &ABaseCharacter::PlacementModePressed);
        InputComponent->BindAction("aim-buildable-placement", IE_Released, this, &ABaseCharacter::PlacementModeReleased);
        if (GetCameraControlType() == ECameraControlType::TwinStick)
        {
            InputComponent->BindAxis("move-cursor-up", this, &ABaseCharacter::MoveCursorUp);
            InputComponent->BindAxis("move-cursor-right", this, &ABaseCharacter::MoveCursorRight);
        }
        InputComponent->BindAction ("interact", IE_Pressed, this, &ABaseCharacter::InteractPressed);
        InputComponent->BindAction("follow-camera-zoom-out", IE_Pressed, this, &ABaseCharacter::ZoomOut);
        InputComponent->BindAction("follow-camera-zoom-in", IE_Pressed, this, &ABaseCharacter::ZoomIn);
        for (int i = (int)EBuildableActorType::None; i < (int)EBuildableActorType::NumBuildables; ++i)
        {
            InputComponent->BindAction<FHotkeyDelegate> (*(FString("item-hotkey-") + FString::FromInt(i)), IE_Pressed, this, &ABaseCharacter::ItemHotkeyPressed, i);
        }

        UpdateMovementControls();
    }
}

void ABaseCharacter::EnterCombatControlMode()
{
    UE_LOG(LogTemp, Warning, TEXT("Combat"));
    ControlState = EControlState::Combat;
    
    switch (GetCameraControlType())
    {
        case ECameraControlType::RotateCamera:
        {
            UpdateTimelineTargetRotations();

            //VerticalLookRotation = DefaultLookCombatRotation.Pitch;
            //HorizontalLookRotation = DefaultLookCombatRotation.Yaw;
            UpdateControlRotation();
            TimelineContainer->Timeline->PlayFromStart();
            break;
        }
        case ECameraControlType::RotatePlayer:
        {
            GetMesh()->SetRelativeRotation(DefaultMeshRotation);
            break;
        }
        case ECameraControlType::TwinStick:
        {    
            break;
        }
        default: break;
    }
    BindInput();
    ControlStateChanged();
}


void ABaseCharacter::EnterExploreControlMode()
{
    UE_LOG(LogTemp, Warning, TEXT("Explore"));
    ControlState = EControlState::Explore;

    switch (GetCameraControlType())
    {
        case ECameraControlType::RotateCamera:
        {
            //VerticalLookRotation = DefaultLookExploreRotation.Pitch;
            //HorizontalLookRotation = DefaultLookExploreRotation.Yaw;
            UpdateControlRotation();
            break;
        }
        case ECameraControlType::RotatePlayer:
        {
            break;
        }
        case ECameraControlType::TwinStick:
        {
            break;
        }
        default: break;
    }
    BindInput();
    UpdateMeshRotationForExploreDirection();
    ControlStateChanged();
}

void ABaseCharacter::PlacementModePressed()
{
    PlacementModeActive = true;
}

void ABaseCharacter::PlacementModeReleased()
{
    PlacementModeActive = false;
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

        if (GetCameraControlType() != ECameraControlType::TwinStick)
        {
            InputComponent->BindAxis("look-up", this, &ABaseCharacter::UpdateVerticalLookRotation);
            InputComponent->BindAxis("look-right", this, &ABaseCharacter::UpdateHorizontalLookRotation);
        }

        InputComponent->BindAction ("fire", IE_Pressed, this, &ABaseCharacter::PlayerFired);
        InputComponent->BindAction ("fire", IE_Released, this, &ABaseCharacter::FireReleased);
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
        InputComponent->BindAction ("map-view",       IE_Pressed,  this, &ABaseCharacter::MapViewPressed);
        InputComponent->BindAction ("map-view",       IE_Released, this, &ABaseCharacter::MapViewReleased);
        InputComponent->BindAction ("boost",          IE_Pressed,  this, &ABaseCharacter::BoostPressed);

        if (GetCameraControlType() != ECameraControlType::TwinStick)
        {
            InputComponent->BindAxis("look-right", this, &ABaseCharacter::UpdateHorizontalLookRotation);
        }
        InputComponent->BindAxis   ("follow-camera-zoom", this, &ABaseCharacter::UpdateFollowCameraPosition);
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

void ABaseCharacter::MoveCursorUp (float delta)
{
#pragma message("This should NOT happen in this class. Should probably be setup as part of the game mode, or something ...")
    if (delta != 0.0f)
    {
        APlayerController* PController = GetWorld()->GetFirstPlayerController();
        if (PController != nullptr)
        {
            FViewport* Viewport = CastChecked<ULocalPlayer>(PController->Player)->ViewportClient->Viewport;
            if (Viewport != nullptr)
                Viewport->SetMouse(Viewport->GetMouseX(), Viewport->GetMouseY() + delta);
        }
        OnMoveCursorUp(delta);
    }
}

void ABaseCharacter::MoveCursorRight(float delta)
{
#pragma message("This should NOT happen in this class. Should probably be setup as part of the game mode, or something ...")
    if (delta != 0.0f)
    {
        APlayerController* PController = GetWorld()->GetFirstPlayerController();
        if (PController != nullptr)
        {
            FViewport* Viewport = CastChecked<ULocalPlayer>(PController->Player)->ViewportClient->Viewport;
            if (Viewport != nullptr)
                Viewport->SetMouse(Viewport->GetMouseX() + delta, Viewport->GetMouseY());
        }
        OnMoveCursorRight(delta);
    }
}

void ABaseCharacter::UpdateControlRotation()
{
    FRotator currentLookRotation = GetNormalizedLookRotation();
    FRotator newLookRotation (VerticalLookRotation, HorizontalLookRotation, currentLookRotation.Roll);
    newLookRotation.Normalize();
    if (GetController() != nullptr)
        GetController()->SetControlRotation (newLookRotation);
    OnPlayerControlRotationUpdated();
}

void ABaseCharacter::RotateMeshToMousePosition()
{
    APlayerController* PController = GetWorld()->GetFirstPlayerController();
    if (PController != nullptr)
    {
        FVector ProjectedPoint, Direction;
        PController->DeprojectMousePositionToWorld(ProjectedPoint, Direction);
        FPlane ZPlane(FVector::ZeroVector, FVector::UpVector);
        FVector MouseWorldLocation = FVector::ZeroVector;
        float T = 0.0f;
        if (UKismetMathLibrary::LinePlaneIntersection(ProjectedPoint, ProjectedPoint + Direction * 10000.0f, ZPlane, T, MouseWorldLocation))
        {
            FVector ActorLocation = GetActorLocation();
            MouseWorldLocation.Z = ActorLocation.Z;
            FRotator Rotator = UKismetMathLibrary::FindLookAtRotation(ActorLocation, MouseWorldLocation);
            TargetLookRotation = (Rotator + DefaultMeshRotation);
            //GetMesh()->SetRelativeRotation((Rotator + DefaultMeshRotation).Quaternion());
        }
    }
}

bool ABaseCharacter::PlayerIsBoosting() const
{
    return IsBoosting;
}

void ABaseCharacter::CombatDirectionPressed (EMovementDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = true;
    UpdateMovementForcesForDirectionKey (direction, true);
}

void ABaseCharacter::CombatDirectionReleased (EMovementDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = false;
    UpdateMovementForcesForDirectionKey (direction, false);
}

void ABaseCharacter::CombatForwardPressed()     { CombatDirectionPressed  (EMovementDirectionType::Forward); }
void ABaseCharacter::CombatForwardReleased()    { CombatDirectionReleased (EMovementDirectionType::Forward); }
void ABaseCharacter::CombatBackwardsPressed()   { CombatDirectionPressed  (EMovementDirectionType::Backwards); }
void ABaseCharacter::CombatBackwardsReleased()  { CombatDirectionReleased (EMovementDirectionType::Backwards); }
void ABaseCharacter::CombatRightPressed()       { CombatDirectionPressed  (EMovementDirectionType::Right); }
void ABaseCharacter::CombatRightReleased()      { CombatDirectionReleased (EMovementDirectionType::Right); }
void ABaseCharacter::CombatLeftPressed()        { CombatDirectionPressed  (EMovementDirectionType::Left); }
void ABaseCharacter::CombatLeftReleased()       { CombatDirectionReleased (EMovementDirectionType::Left); }

void ABaseCharacter::ExploreDirectionPressed (EMovementDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = true;
    UpdateMovementForcesForDirectionKey (direction, true);
    UpdateMeshRotationForExploreDirection();
}

void ABaseCharacter::BoostPressed()
{
    BoostStartPos = GetActorLocation();
    BoostDestPos = GetActorLocation() + GetMovementVector() * BoostDistMultiplier;
    UE_LOG(LogTemp, Warning, TEXT("Boost Start: %s | Boost End: %s"), *BoostStartPos.ToString(), *BoostDestPos.ToString());
    IsBoosting = true;
    TimeSinceBoost = 0.0f;
    OnBoostStarted.Broadcast();
}

void ABaseCharacter::ExploreDirectionReleased (EMovementDirectionType direction)
{
    MovementKeysPressedState.DirectionStates[(int) direction] = false;
    UpdateMovementForcesForDirectionKey (direction, false);

    if (MovementKeysPressedState.AreAnyKeysPressed())
        UpdateMeshRotationForExploreDirection();
}

void ABaseCharacter::ItemHotkeyPressed(int itemNumber)
{
    BuildableItem = (EBuildableActorType)itemNumber;
    OnPlayerBuildItemChanged();
}

void ABaseCharacter::UpdateMeshRotationForExploreDirection()
{
    FRotator newNormalizedMeshRotation = GetNormalizedMovementRotation() + DefaultMeshRotation;
    newNormalizedMeshRotation.Normalize();
    TargetLookRotation = newNormalizedMeshRotation;
}

void ABaseCharacter::UpdateMovementForcesForDirectionKey (EMovementDirectionType direction, bool pressed)
{
    switch (direction)
    {
        case EMovementDirectionType::Forward:
        {
            FrontBackMovementForce = pressed ? 1.0f : IsDirectionPressed (EMovementDirectionType::Backwards) ? -1.0f : 0.0f; 
            break;
        }
        case EMovementDirectionType::Backwards: 
        { 
            FrontBackMovementForce = pressed ? -1.0f : IsDirectionPressed (EMovementDirectionType::Forward) ? 1.0f : 0.0f; 
            break; 
        }
        case EMovementDirectionType::Right:     
        { 
            RightLeftMovementForce = pressed ? 1.0f  : IsDirectionPressed (EMovementDirectionType::Left) ? -1.0f : 0.0f; 
            break; 
        }
        case EMovementDirectionType::Left:      
        { 
            RightLeftMovementForce = pressed ? -1.0f : IsDirectionPressed (EMovementDirectionType::Right) ? 1.0f : 0.0f; 
            break; 
        }
    }
}

void ABaseCharacter::CheckForPressedKeys()
{
    for (int d = 0; d < (int) EMovementDirectionType::NumMovementDirections; d++)
    {
        if (MovementKeysPressedState.DirectionStates[d])
        {
            ExploreDirectionPressed ((EMovementDirectionType) d);
            return;
        }
    }
}

void ABaseCharacter::ExploreForwardPressed()    { ExploreDirectionPressed  (EMovementDirectionType::Forward); }
void ABaseCharacter::ExploreForwardReleased()   { ExploreDirectionReleased (EMovementDirectionType::Forward); }
void ABaseCharacter::ExploreBackwardsPressed()  { ExploreDirectionPressed  (EMovementDirectionType::Backwards); }
void ABaseCharacter::ExploreBackwardsReleased() { ExploreDirectionReleased (EMovementDirectionType::Backwards); }
void ABaseCharacter::ExploreRightPressed()      { ExploreDirectionPressed  (EMovementDirectionType::Right); }
void ABaseCharacter::ExploreRightReleased()     { ExploreDirectionReleased (EMovementDirectionType::Right); }
void ABaseCharacter::ExploreLeftPressed()       { ExploreDirectionPressed  (EMovementDirectionType::Left); }
void ABaseCharacter::ExploreLeftReleased()      { ExploreDirectionReleased (EMovementDirectionType::Left); }

void ABaseCharacter::PlayerFired() { OnPlayerFired.Broadcast(); }
void ABaseCharacter::FireReleased() { OnFireReleased.Broadcast(); }
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
bool ABaseCharacter::IsDirectionPressed (EMovementDirectionType direction)
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
    FRotator newRotation = UKismetMathLibrary::RInterpTo (GetNormalizedLookRotation(), TimelineTargetLookRotation, deltaTime, interpSpeed);
    
    //VerticalLookRotation = DefaultLookCombatPitch;
    HorizontalLookRotation = newRotation.Yaw;
    LimitVerticalLookRotation();
    LimitHorizontalLookRotation();
    UpdateControlRotation();

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

