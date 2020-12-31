#include "ScannerComponent.h"

UScannerComponent::UScannerComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UScannerComponent::StartScanner(int segment /*= 0*/)
{
	SetCurrentSegment(segment);
	SetScannerState(EScannerState::Scanning); 
}

void UScannerComponent::StopScanner() { SetScannerState(EScannerState::Stopped); }

float UScannerComponent::GetScanLineRotation() const { return CurrentRotation; }

EScannerState UScannerComponent::GetScannerState() const { return ScannerState; }

void UScannerComponent::RegisterScannerStateChangedCallback(const FOnScannerStateChanged& Callback)
{
	OnScannerStateChanged.AddLambda([Callback]()
	{
			Callback.ExecuteIfBound();
	});
}

void UScannerComponent::RegisterScanSegmentEndingCallback(const FOnScanSegmentEnding& Callback)
{
	OnScanSegmentEnding.AddLambda([Callback]()
	{
		Callback.ExecuteIfBound();
	});
}

void UScannerComponent::SetScannerState(EScannerState state)
{
	ScannerState = state;
	if (ScannerState == EScannerState::Paused)
	{
		CurrentPauseCounterSeconds = 0.0f;
		if (CurrentSegment < (Segments.Num() - 1))
			CurrentPauseDurationSeconds = SecondsBetweenSegments;
		else
			CurrentPauseDurationSeconds = SecondsBetweenScans;
	}
	OnScannerStateChanged.Broadcast();
	SegmentEnding = false;
}

void UScannerComponent::SetCurrentSegment(int segment)
{
	ensure(segment < Segments.Num());
	CurrentSegment = segment;
	CurrentSegmentStartRotation = 0.0f;
	for (int i = 0; i < CurrentSegment; ++i)
		CurrentSegmentStartRotation += Segments[i];
	CurrentSegmentAngle = Segments[CurrentSegment];
	CurrentRotation = CurrentSegmentStartRotation;
	CurrentSegmentDuration = CurrentSegmentAngle / RotationDegreesPerSecond;
}

void UScannerComponent::BroadcastSegmentEnding()
{
	SegmentEnding = true;
	OnScanSegmentEnding.Broadcast();
}

void UScannerComponent::UpdateScanLine(float DeltaTime)
{
	const float degreesSoFar = CurrentRotation - CurrentSegmentStartRotation;
	const float currentSegmentProgress = degreesSoFar / CurrentSegmentAngle;
	
	if (!SegmentEnding)
	{
		const float degreesRemaining = CurrentSegmentAngle - degreesSoFar;
		if (degreesRemaining / RotationDegreesPerSecond <= SegmentEndDurationSeconds)
			BroadcastSegmentEnding();
	}

	CurrentRotation += DeltaTime * RotationDegreesPerSecond;

	if (currentSegmentProgress >= 1.0f)
	{
		PauseAndIncrementSegment();
	}
}

void UScannerComponent::PauseAndIncrementSegment()
{
	SetScannerState(EScannerState::Paused);
	SetCurrentSegment((CurrentSegment + 1) % Segments.Num());
}

void UScannerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (ScannerState)
	{
	case EScannerState::Paused:
		CurrentPauseCounterSeconds += DeltaTime;
		if (CurrentPauseCounterSeconds >= CurrentPauseDurationSeconds)
		{
			SetScannerState(EScannerState::Scanning);
		}
		break;

	case EScannerState::Scanning:
		UpdateScanLine(DeltaTime);
		break;

	default: break;
	}
}