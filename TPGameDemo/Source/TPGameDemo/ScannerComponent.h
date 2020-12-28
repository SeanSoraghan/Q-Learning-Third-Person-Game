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

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	float GetScanLineLength() const;

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	float GetScanLineRotation() const;

	UFUNCTION(BlueprintCallable, Category = "Scan State")
	FVector2D GetScanLineMidPoint() const;

	// Takes the un-rotated, un-translated bounds of the given component and uses that as the scanner extent
	UFUNCTION(BlueprintCallable, Category = "Scanner Extent")
	void SetExtentFromSceneComponent(const USceneComponent* sceneComponent);

	// By default, scan at '~100 bpm' where each scan segment is a 16th note long.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SecondsPerSegment = 0.415f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SecondsBetweenSegments = 0.415f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Animation")
	float SecondsBetweenScans = 0.83;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Scanner Extent")
	FBox ScannerExtent = FBox();

private:
	// The current point on the outer scan extent
	FVector2D ScanEndPoint = FVector2D::ZeroVector;
	float CurrentScanTime = 0.0f;
	float ScanLineRotation = 0.0f;
	float ScanLineLength = 0.0f;

	void UpdateScanLine(float DeltaTime);
};