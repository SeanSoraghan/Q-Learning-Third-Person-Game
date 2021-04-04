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
	
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	UFUNCTION(BlueprintCallable)
		void AddMetronomeResponder(UMetronomeResponderComponent* responder);
	UFUNCTION(BlueprintCallable)
		void RemoveMetronomeResponder(UMetronomeResponderComponent* responder);
	UFUNCTION(BlueprintCallable)
		void SetTimeSynthComponent(UTimeSynthComponent* timeSynthComp);
	UFUNCTION()
		void OnQuantizationEvent(ETimeSynthEventQuantization quantizationType, int32 numBars, float beat);
	UFUNCTION(BlueprintPure)
		float GetSecondsPerMetronomeQuantization(ETimeSynthEventQuantization quantizationType) const;

	void ResponderAudioStateChanged(UMetronomeResponderComponent* responder);

private:
	TMap<ETimeSynthEventQuantization, TArray<UMetronomeResponderComponent*>> MetronomeResponders;
	UPROPERTY()
	TMap<ETimeSynthEventQuantization, FOnQuantizationEventBP> QuantizationEvents;
	UTimeSynthComponent* TimeSynthComponent;

	FCriticalSection RespondersMutex;
};
