// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDAttributeBar.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API UHUDAttributeBar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
		void Initialize(UImage* inBar, UImage* inTrack, UImage* inStartCap, UImage* inEndCap);
	UFUNCTION(BlueprintCallable)
		void SetBarPercent(float percentage);
	UFUNCTION(BlueprintCallable)
		void UpdateBar();
	UFUNCTION(BlueprintCallable)
		void Test();
	UPROPERTY(EditAnywhere)
		int FullBarWidth_NoScaling;
private:
	float barPercentage = 0.0f;
	FVector2D barFull = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere)
		UImage* bar;
	UPROPERTY(EditAnywhere)
		UImage* track;
	UPROPERTY(EditAnywhere)
		UImage* startCap;
	UPROPERTY(EditAnywhere)
		UImage* endCap;
};
