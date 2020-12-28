#include "ScannerComponent.h"

UScannerComponent::UScannerComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}

float UScannerComponent::GetScanLineLength() const { return ScanLineLength; }
float UScannerComponent::GetScanLineRotation() const { return ScanLineRotation; }
FVector2D UScannerComponent::GetScanLineMidPoint() const { return ScanEndPoint / 2.0f; }

void UScannerComponent::SetExtentFromSceneComponent(const USceneComponent* sceneComponent)
{
	FTransform transform = sceneComponent->GetComponentTransform();
	transform.SetRotation(FQuat::Identity);
	transform.SetLocation(FVector::ZeroVector);
	ScannerExtent = sceneComponent->CalcBounds(transform).GetBox();
}

void UScannerComponent::UpdateScanLine(float DeltaTime)
{
	const float totalSegmentTime = SecondsPerSegment + SecondsBetweenSegments;
	EScanSegment scanSegment = (EScanSegment)FMath::Min(FMath::FloorToInt(CurrentScanTime / totalSegmentTime), (int)EScanSegment::NumSegments - 1);
	const float currentSegmentTime = FMath::Min((CurrentScanTime - (int)scanSegment * totalSegmentTime) / SecondsPerSegment, 1.0f);
	cases forward and left are producing incorrect rotation values.
	switch (scanSegment)
	{
	case EScanSegment::Right:
		ScanEndPoint.X = ScannerExtent.GetExtent().X * currentSegmentTime;
		ScanEndPoint.Y = ScannerExtent.GetExtent().Y;
		ScanLineLength = ScanEndPoint.Size();
		if (ScanLineLength > 0.0f)
			ScanLineRotation = FMath::RadiansToDegrees(FMath::Asin(ScanEndPoint.X / ScanLineLength));
		break;
	case EScanSegment::Forward:
		ScanEndPoint.X = ScannerExtent.GetExtent().X;
		ScanEndPoint.Y = (currentSegmentTime * 2.0f - 1.0f) * ScannerExtent.GetExtent().Y * -1.0f;
		ScanLineLength = ScanEndPoint.Size();
		if (ScanLineLength > 0.0f)
			ScanLineRotation = 90.0f + (FMath::Sign(ScanEndPoint.Y) * FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(ScanEndPoint.Y) / ScanLineLength)));
		break;
	case EScanSegment::Left:
		ScanEndPoint.X = ScannerExtent.GetExtent().X * (1.0f - currentSegmentTime);
		ScanEndPoint.Y = -ScannerExtent.GetExtent().Y;
		ScanLineLength = ScanEndPoint.Size();
		if (ScanLineLength > 0.0f)
			ScanLineRotation = 180.0f - FMath::RadiansToDegrees(FMath::Asin(ScanEndPoint.X / ScanLineLength));
		break;

	default: break;
	}

	CurrentScanTime += DeltaTime;
	const float totalScanTime = totalSegmentTime * 3.0f + SecondsBetweenScans;
	if (CurrentScanTime > totalScanTime)
		CurrentScanTime = 0.0f;
}

// Called every frame
void UScannerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UpdateScanLine(DeltaTime);
}