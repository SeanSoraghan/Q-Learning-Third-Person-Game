#include "ScannerComponent.h"

UScannerComponent::UScannerComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UScannerComponent::StartScanner(EScanSegment segment /*= EScanSegment::Right*/)
{
	SetCurrentSegment(segment);
	SetScannerState(EScannerState::Scanning); 
}

void UScannerComponent::StopScanner() { SetScannerState(EScannerState::Stopped); }

float UScannerComponent::GetScanLineLength() const { return ScanLineLength; }
float UScannerComponent::GetScanLineRotation() const { return CurrentRotation; }
FVector2D UScannerComponent::GetScanLineMidPoint() const { return ScanEndPoint / 2.0f; }

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

void UScannerComponent::SetExtentFromSceneComponent(const USceneComponent* sceneComponent)
{
	FTransform transform = sceneComponent->GetComponentTransform();
	transform.SetRotation(FQuat::Identity);
	transform.SetLocation(FVector::ZeroVector);
	ScannerExtent = sceneComponent->CalcBounds(transform).GetBox();
	float scanRadius = FVector2D(ScannerExtent.GetExtent().X, ScannerExtent.GetExtent().Y).Size();
	SideSegmentsAngle = FMath::RadiansToDegrees(FMath::Asin(ScannerExtent.GetExtent().X / scanRadius));
	FrontSegmentAngle = (90.0f - SideSegmentsAngle) * 2.0f;
}

void UScannerComponent::SetScannerState(EScannerState state)
{
	ScannerState = state;
	if (ScannerState == EScannerState::Paused)
	{
		CurrentPauseCounterSeconds = 0.0f;
		switch (CurrentSegment)
		{
		case EScanSegment::Right:
			CurrentPauseDurationSeconds = SecondsBetweenSegments;
			break;
		case EScanSegment::Forward:
			CurrentPauseDurationSeconds = SecondsBetweenSegments;
			break;
		case EScanSegment::Left:
			CurrentPauseDurationSeconds = SecondsBetweenScans;
			break;
		default: break;
		}
	}
	OnScannerStateChanged.Broadcast();
	SegmentEnding = false;
}

void UScannerComponent::SetCurrentSegment(EScanSegment segment)
{
	CurrentSegment = segment;
	switch (CurrentSegment)
	{
	case EScanSegment::Right:
		CurrentSegmentStartRotation = 0.0f;
		CurrentSegmentAngle = SideSegmentsAngle;
		break;

	case EScanSegment::Forward:
		CurrentSegmentStartRotation = SideSegmentsAngle;
		CurrentSegmentAngle = FrontSegmentAngle;
		break;

	case EScanSegment::Left:
		CurrentSegmentStartRotation = SideSegmentsAngle + FrontSegmentAngle;
		CurrentSegmentAngle = SideSegmentsAngle;
		break;

	default: break;
	}
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

	switch (CurrentSegment)
	{
	case EScanSegment::Right:
		ScanEndPoint.X = ScannerExtent.GetExtent().X * currentSegmentProgress;
		ScanEndPoint.Y = ScannerExtent.GetExtent().Y;
		break;

	case EScanSegment::Forward:
		ScanEndPoint.X = ScannerExtent.GetExtent().X;
		ScanEndPoint.Y = (currentSegmentProgress * 2.0f - 1.0f) * ScannerExtent.GetExtent().Y * -1.0f;
		break;

	case EScanSegment::Left:
		ScanEndPoint.X = ScannerExtent.GetExtent().X * (1.0f - currentSegmentProgress);
		ScanEndPoint.Y = -ScannerExtent.GetExtent().Y;
		break;

	default: break;
	}

	ScanLineLength = ScanEndPoint.Size();
	
	CurrentRotation += DeltaTime * RotationDegreesPerSecond;
	
	

	if (currentSegmentProgress >= 1.0f)
	{
		PauseAndIncrementSegment();
	}
}

void UScannerComponent::PauseAndIncrementSegment()
{
	SetScannerState(EScannerState::Paused);
	SetCurrentSegment((EScanSegment)(((int)CurrentSegment + 1) % (int)EScanSegment::NumSegments));
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