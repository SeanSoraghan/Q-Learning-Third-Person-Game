// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/ActorComponent.h"
#include "ScannerComponent.generated.h"

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
Implements a line scan around a circle. Can be broken into multiple segments.
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPGAMEDEMO_API UScannerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UScannerComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void StartScanner(int segment = 0);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void StopScanner();

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	float GetScanLineRotation() const;

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	EScannerState GetScannerState() const;

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void RegisterScannerStateChangedCallback(const FOnScannerStateChanged& Callback);

	UFUNCTION(BlueprintCallable, Category = "Scanner State")
	void RegisterScanSegmentEndingCallback(const FOnScanSegmentEnding& Callback);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scan Segments")
	TArray<float> Segments;
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

private:
	EScannerState ScannerState = EScannerState::Stopped;
	float CurrentRotation = 0.0f;
	int CurrentSegment;
	// Angle of the current segment
	float CurrentSegmentAngle = 1.0f; // init 1 to avoid divide by 0.
	float CurrentSegmentDuration = 0.0f;
	// Rotation covered up to the start of the current segment
	float CurrentSegmentStartRotation = 0.0f;

	float CurrentPauseDurationSeconds = 0.0f;
	float CurrentPauseCounterSeconds = 0.0f;

	ScannerStateChangedEvent OnScannerStateChanged;
	ScanSegmentEnding OnScanSegmentEnding;
	bool SegmentEnding = false;
	void BroadcastSegmentEnding();

	void UpdateScanLine(float DeltaTime);
	void SetScannerState(EScannerState state);
	void SetCurrentSegment(int segment);
	void PauseAndIncrementSegment();
	void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};