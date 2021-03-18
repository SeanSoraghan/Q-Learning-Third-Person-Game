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
	void Initialize(UImage* inBar, UImage* inStartCap, UImage* inEndCap);
	UFUNCTION(BlueprintCallable)
	void UpdateBar();
	UFUNCTION(BlueprintCallable)
	void UpdateCapDimensions();
	UFUNCTION(BlueprintCallable)
	void Test();

	UFUNCTION(BlueprintCallable)
	FVector2D BarDimensionsForCurrentViewSize() const;
	UFUNCTION(BlueprintCallable)
	FVector2D CapDimensionsForCurrentViewSize() const;

private:
	float barPercentage = 0.0f;

	UPROPERTY(EditAnywhere)
		FVector2D barScreenRatioXY = 1.0f;

	UPROPERTY(EditAnywhere)
		UImage* bar;
	UPROPERTY(EditAnywhere)
		UImage* startCap;
	UPROPERTY(EditAnywhere)
		UImage* endCap;
};
