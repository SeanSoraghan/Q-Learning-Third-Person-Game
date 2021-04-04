// Fill out your copyright notice in the Description page of Project Settings.
#include "TPGameDemo.h"
#include "TPGameDemoGameState.h"
#include "MetronomeComponent.h"
#include "MazeActor.h"


// Sets default values
AMazeActor::AMazeActor (const FObjectInitializer& ObjectInitializer) : Super (ObjectInitializer) 
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    MetronomeTickCallback.BindUFunction(this, "OnMetronomeTick");
}

// Called when the game starts or when spawned
void AMazeActor::BeginPlay()
{
	Super::BeginPlay();
    
    MaxHealth = 100.0f;

    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    ATPGameDemoGameMode* gameMode = (ATPGameDemoGameMode*) GetWorld()->GetAuthGameMode();
    if (gameMode != nullptr && gameState != nullptr)
    {
        gameState->OnMazeDimensionsChanged.AddLambda([this]()
        {
            UpdateMazeDimensions();
        });
        CurrentLevelNumGridUnitsX   = gameState->NumGridUnitsX;
        CurrentLevelNumGridUnitsY   = gameState->NumGridUnitsY;
        CurrentLevelGridUnitLengthXCM = gameState->GridUnitLengthXCM;
        CurrentLevelGridUnitLengthYCM = gameState->GridUnitLengthYCM;
        MaxHealth = gameMode->DefaultMaxHealth;
    }
    
    Health = MaxHealth;

    UpdatePosition (false);
}

void AMazeActor::BeginDestroy()
{
    Super::BeginDestroy();
    MetronomeTickCallback.Unbind();
}

void AMazeActor::SetOccupyCells(bool bShouldOccupy)
{
    bOccupyCells = bShouldOccupy;
}

void AMazeActor::InitialisePosition(FIntPoint roomCoords)
{ 
    CurrentRoomCoords = roomCoords;
}

float AMazeActor::GetHealthPercentage()
{
    return Health / MaxHealth;
}

void AMazeActor::TakeDamage (float damageAmount)
{
    Health -= damageAmount;
    CheckDeath();
}

void AMazeActor::CheckDeath()
{
    if (Health <= 0.0f)
    {
        Health = 0.0f;
        IsAlive = false;
        ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
        if (gameState != nullptr && bOccupyCells)
        {
            gameState->ActorExitedTilePosition(CurrentRoomCoords, FIntPoint(GridXPosition, GridYPosition));
        }
        ActorDied();
        OnActorDied.Broadcast();
    }
}

void AMazeActor::ActorDied()
{}

void AMazeActor::UpdateMazeDimensions()
{
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState();
    if (gameState != nullptr)
    {
        CurrentLevelNumGridUnitsX   = gameState->NumGridUnitsX;
        CurrentLevelNumGridUnitsY   = gameState->NumGridUnitsY;
        CurrentLevelGridUnitLengthXCM = gameState->GridUnitLengthXCM;
        CurrentLevelGridUnitLengthYCM = gameState->GridUnitLengthYCM;
    }
}

void AMazeActor::UpdatePosition (bool broadcastChange)
{
    if (ShouldUpdatePosition)
    {
        FVector worldPosition = GetActorLocation();
        const int totalGridLengthCMX = CurrentLevelGridUnitLengthXCM * CurrentLevelNumGridUnitsX;
        const int totalGridLengthCMY = CurrentLevelGridUnitLengthYCM * CurrentLevelNumGridUnitsY;
        const int overlappingGridLengthCMX = totalGridLengthCMX - CurrentLevelGridUnitLengthXCM;
        const int overlappingGridLengthCMY = totalGridLengthCMY - CurrentLevelGridUnitLengthYCM;
        // map -gridLength/2 > gridLength/2 to 0 > gridLength.
        const int mappedX = worldPosition.X + overlappingGridLengthCMX / 2;
        const int mappedY = worldPosition.Y + overlappingGridLengthCMY / 2;
        // get current room coords. negative values should start indexed from -1, not 0 (hence the ternary addition).
        const int roomX = (FMath::Abs(mappedX / overlappingGridLengthCMX) + (mappedX < 0 ? 1 : 0)) * FMath::Sign(mappedX);
        const int roomY = (FMath::Abs(mappedY / overlappingGridLengthCMY) + (mappedY < 0 ? 1 : 0)) * FMath::Sign(mappedY);
        CurrentRoomCoords = FIntPoint(roomX, roomY);
        // divide mappedX and mappedY to get individual cell coordinates within room. If negative, should index backwards.
        const int numUnitsX = (int)(mappedX /*- CurrentLevelGridUnitLengthXCM * 0.5f*/) / (CurrentLevelGridUnitLengthXCM);
        const int numUnitsY = (int)(mappedY /*- CurrentLevelGridUnitLengthYCM * 0.5f*/) / (CurrentLevelGridUnitLengthYCM);
        GridXPosition = numUnitsX % (CurrentLevelNumGridUnitsX - 1);
        GridYPosition = numUnitsY % (CurrentLevelNumGridUnitsY - 1);
        if (mappedX < 0)
            GridXPosition = (CurrentLevelNumGridUnitsX - 2) - FMath::Abs(GridXPosition);
        if (mappedY < 0)
            GridYPosition = (CurrentLevelNumGridUnitsY - 2) - FMath::Abs(GridYPosition);

        if (broadcastChange && (GridYPosition != PreviousGridYPosition || GridXPosition != PreviousGridXPosition))
        {
            ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState();
            if (gameState != nullptr && bOccupyCells)
            {
                gameState->ActorExitedTilePosition(PreviousRoomCoords, FIntPoint(PreviousGridXPosition, PreviousGridYPosition));
                gameState->ActorEnteredTilePosition(CurrentRoomCoords, FIntPoint(GridXPosition, GridYPosition));
            }

            const bool roomChanged = PreviousRoomCoords != CurrentRoomCoords;
            if (roomChanged)
            {
                RoomCoordsChanged();
                RoomCoordsChangedEvent.Broadcast();
                PreviousRoomCoords = CurrentRoomCoords;
            }
            PositionChanged();
            GridPositionChangedEvent.Broadcast();
            PreviousGridXPosition = GridXPosition;
            PreviousGridYPosition = GridYPosition;
        }
    }
}

FRoomPositionPair AMazeActor::GetRoomAndPosition()
{
    return {CurrentRoomCoords, {GridXPosition, GridYPosition}};
}

// Called every frame
void AMazeActor::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );
    UpdatePosition();

    if (IsPulsing)
    {
        PulseLerpLinear = TimeSincePulse / PulseLengthSeconds;
        float PulseLog = FMath::Loge(9.0f * (PulseLerpLinear)+1.0f);
        FVector newLocation = FMath::Lerp(PulseStartPos, PulseDestPos, PulseLog);
        FHitResult hitResult;
        SetActorLocation(newLocation, true, &hitResult, ETeleportType::TeleportPhysics);
        TimeSincePulse += DeltaTime;
        if (TimeSincePulse >= PulseLengthSeconds || hitResult.bBlockingHit)
        {
            IsPulsing = false;
        }
    }

    if (UndergoingImpulse())
    {
        UpdateImpulseStrength(DeltaTime);
        AddMovementInput(ImpulseDirection, CurrentImpulseStrength);
    }
}

bool AMazeActor::IsOnGridEdge() const
{
    if (ATPGameDemoGameState* gameState = (ATPGameDemoGameState*) GetWorld()->GetGameState())
        return gameState->IsOnGridEdge(FIntPoint(GridXPosition, GridYPosition));

    return false;
}

bool AMazeActor::WasOnGridEdge() const
{
    if (ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)GetWorld()->GetGameState())
        return gameState->IsOnGridEdge(FIntPoint(PreviousGridXPosition, PreviousGridYPosition));
    
    return false;
}

void AMazeActor::SetupMetronomicMovement(UMetronomeComponent* metronome, UMetronomeResponderComponent* metronomeResponder)
{
    if (metronomeResponder == nullptr)
        metronomeResponder = Cast<UMetronomeResponderComponent>(GetComponentByClass(UMetronomeResponderComponent::StaticClass()));
    if (metronomeResponder != nullptr)
    {
        metronomeResponder->SecondsPerQuantization = metronome->GetSecondsPerMetronomeQuantization(metronomeResponder->Quantization);
        SetPulseLengthSeconds(metronomeResponder->SecondsPerQuantization);
        metronome->AddMetronomeResponder(metronomeResponder);
        metronomeResponder->RegisterMetronomeTickCallback(MetronomeTickCallback);
    }
}

bool AMazeActor::ActorAboutToPulse() const
{
    UMetronomeResponderComponent* metronomeResponder = Cast<UMetronomeResponderComponent>(GetComponentByClass(UMetronomeResponderComponent::StaticClass()));
    if (metronomeResponder == nullptr)
        return false;
    return TimeSincePulse >= metronomeResponder->SecondsPerQuantization * 0.75f;
}

void AMazeActor::SetPulseLengthSeconds(float pulseLength)
{
    PulseLengthSeconds = pulseLength;
}

void AMazeActor::BeginPulse(FVector direction)
{
    PulseStartPos = GetActorLocation();
    PulseDestPos = GetActorLocation() + direction * PulseDistMultiplier;
    IsPulsing = true;
    TimeSincePulse = 0.0f;
}

void AMazeActor::AddImpulseForce(FVector direction, float duration, float normedForce)
{
    GetCharacterMovement()->SetMovementMode(ImpulseMovement);
    ImpulseDirection = direction;
    CurrentImpulseDuration = duration;
    InitialImpulseStrength = FMath::Clamp(normedForce, -1.0f, 1.0f);
    CurrentImpulseStrength = InitialImpulseStrength;
    SecondsSinceLastImpulse = 0.0f;
}

void AMazeActor::UpdateImpulseStrength(float deltaTime)
{
    SecondsSinceLastImpulse = SecondsSinceLastImpulse + deltaTime;
    float animPos = FMath::Loge(9.0f * (SecondsSinceLastImpulse / CurrentImpulseDuration) + 1.0f);
    if (animPos >= 1.0f)
    {
        CurrentImpulseStrength = 0.0f;
        GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
    }
    else
    {
        CurrentImpulseStrength = FMath::Lerp(InitialImpulseStrength, 0.0f, animPos);
    }
}

bool AMazeActor::UndergoingImpulse() const { return GetCharacterMovement()->MovementMode == ImpulseMovement; }

void AMazeActor::PositionChanged(){}
void AMazeActor::RoomCoordsChanged(){}