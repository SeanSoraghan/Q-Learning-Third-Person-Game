// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/ActorComponent.h"
#include "ScannerComponent.generated.h"

UENUM(BlueprintType)
enum class EScanSegment : uint8
{
	// scanning back to front, along right side of the box extent
	Right UMETA(DisplayName = "Right"),
	// scanning right to left, along front side of the box extent
	Forward  UMETA(DisplayName = "Forward"),
	// scanning front to back, along left side of the box extent
	Left UMETA(DisplayName = "Left"),
	NumSegments  UMETA(DisplayName = "Num Segments"),
};

UENUM(BlueprintType)
enum class EScannerState : uint8
{
	Scanning UMETA(DisplayName = "Scanning"),
	Paused  UMETA(DisplayName = "Paused"),
	Stopped UMETA(DisplayName = "Stopped")
};

DECLARE_EVENT(UScannerComponent, ScannerStateChangedEvent);
DECLARE_DYNAMIC_DELEGATE(FOnScannerStateChanged);

DECLARE_EVENT(UScannerComponent, ScanSegmentEnding);
DECLARE_DYNAMIC_DELEGATE(FOnScanSegmentEnding);

/*
Implements a line scan around half the inner area of a rectangle.
The scan starts from 'east' (directly along the local Y axis) and scans around to west (along negative local Y axis).
The scan line length and rotation can be queried using GetScanLineLength() and GetScanLineRotation().
This could be used to drive the scaling, positioning, and rotating of a vertical plane with constant height, for example, to implement a scan effect.
Used by the turrets.
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPGAMEDEMO_API UScannerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UScannerComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void StartScanner(EScanSegment segment = EScanSegment::Right);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void StopScanner();

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	float GetScanLineLength() const;

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	float GetScanLineRotation() const;

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	FVector2D GetScanLineMidPoint() const;

	// Takes the un-rotated, un-translated bounds of the given component and uses that as the scanner extent
	UFUNCTION(BlueprintCallable, Category = "Scanner Extent")
	void SetExtentFromSceneComponent(const USceneComponent* sceneComponent);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	EScannerState GetScannerState() const;

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void RegisterScannerStateChangedCallback(const FOnScannerStateChanged& Callback);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void RegisterScanSegmentEndingCallback(const FOnScanSegmentEnding& Callback);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float RotationDegreesPerSecond = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SecondsBetweenSegments = 0.15f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SecondsBetweenScans = 0.3f;
	// The ScanSegmentEnding event will be broadcast this many seconds before the end of a scan segment.
	// This can be used, for example, to trigger a timeline that updates a dynamic material's parameters.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SegmentEndDurationSeconds = 0.5f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Extent")
	FBox ScannerExtent = FBox();

private:
	EScannerState ScannerState = EScannerState::Stopped;
	// The current point on the outer scan extent
	FVector2D ScanEndPoint = FVector2D::ZeroVector;
	// Current rotation value from 0 - 180
	float CurrentRotation = 0.0f;
	// Current rotation segment
	EScanSegment CurrentSegment = EScanSegment::Right;
	// Angle of the current segment
	float CurrentSegmentAngle = 1.0f; // init 1 to avoid divide by 0.
	float CurrentSegmentDuration = 0.0f;
	// Rotation covered up to the start of the current segment
	float CurrentSegmentStartRotation = 0.0f;
	float SideSegmentsAngle = 0.0f;
	float FrontSegmentAngle = 0.0f;
	float ScanLineLength = 0.0f;

	float CurrentPauseDurationSeconds = 0.0f;
	float CurrentPauseCounterSeconds = 0.0f;

	ScannerStateChangedEvent OnScannerStateChanged;
	ScanSegmentEnding OnScanSegmentEnding;
	bool SegmentEnding = false;
	void BroadcastSegmentEnding();

	void UpdateScanLine(float DeltaTime);
	void SetCurrentSegment(EScanSegment segment);
	void SetScannerState(EScannerState state);
	void PauseAndIncrementSegment();
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};