// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TimeSynthComponent.h"

#include "MetronomeComponent.generated.h"

class UMetronomeResponderComponent;
/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TPGAMEDEMO_API UMetronomeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMetronomeComponent(const FObjectInitializer& ObjectInitializer);
	~UMetronomeComponent();

	UFUNCTION(BlueprintCallable)
		void AddMetronomeResponder(ETimeSynthEventQuantization quantizationType, UMetronomeResponderComponent* responder);
	UFUNCTION(BlueprintCallable)
		void SetTimeSynthComponent(UTimeSynthComponent* timeSynthComp);
	UFUNCTION()
	void OnQuantizationEvent(ETimeSynthEventQuantization QuantizationType, int32 NumBars, float Beat);
private:
	TMap<ETimeSynthEventQuantization, TArray<UMetronomeResponderComponent*>> MetronomeResponders;
	TMap<ETimeSynthEventQuantization, FOnQuantizationEventBP> QuantizationEvents;
	UTimeSynthComponent* TimeSynthComponent;
};
