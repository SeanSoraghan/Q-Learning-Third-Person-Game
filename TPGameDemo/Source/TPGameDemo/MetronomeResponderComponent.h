// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimeSynthComponent.h"
#include "MetronomeResponderComponent.generated.h"

DECLARE_EVENT(UMetronomeResponderComponent, MetronomeTickEvent);
DECLARE_DYNAMIC_DELEGATE(FOnMetronomeTick);

class UTimeSynthClip;
class UMetronomeComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPGAMEDEMO_API UMetronomeResponderComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMetronomeResponderComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Metronome Responder")
	void RegisterMetronomeTickCallback(const FOnMetronomeTick& callback);

	UFUNCTION(BlueprintCallable, Category = "Metronome Responder")
	void SetShouldTriggerAudio(bool shouldTriggerAudio);

	UFUNCTION(BlueprintCallable, Category = "Metronome Responder")
	bool GetShouldTriggerAudio() { return ShouldTriggerAudio; }

	UFUNCTION(BlueprintCallable, Category = "Metronome Responder")
	void SetMetronome(UMetronomeComponent* metronome);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTimeSynthClip* TimeSynthClip;

	void MetronomeTick();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Metronome Responder")
	ETimeSynthEventQuantization Quantization;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Metronome Responder")
	int QuantizationLoopLength = 1;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Metronome Responder")
	float SecondsPerQuantization;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Metronome Responder")
	FTimeSynthClipHandle LastClipTriggerHandle;

private:
	MetronomeTickEvent OnMetronomeTick;

	int QuantizationCount = 0;

	UMetronomeComponent* Metronome = nullptr;

	UPROPERTY(VisibleAnywhere)
	bool ShouldTriggerAudio = false;
};
