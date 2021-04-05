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

	UFUNCTION(BlueprintCallable, Category = "Metronome Responder")
	bool ShouldRespondToQuantizationIndex(int quantizationIndex) const;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTimeSynthClip* TimeSynthClip;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTimeSynthVolumeGroup* VolumeGroup = nullptr;

	void MetronomeTick();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Metronome Responder")
	ETimeSynthEventQuantization Quantization;

	/* The number of subdivisions to count between metronome tick callbacks */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Metronome Responder")
	int QuantizationLoopLength = 1;

	/* The subdivision index within QuantizationLoopLength that should trigger the callback. 
		For example, if Quantization = 16th, QuantizationLoopLength = 4, and QuantizationLoopOffset = 1, 
		this will trigger a callback on every 2nd 16th note of a beat.
		Note: if QuantizationLoopOffset > QuantizationLoopLength - 1, the callback will never be triggered.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Metronome Responder")
	int QuantizationLoopOffset = 0;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Metronome Responder")
	float SecondsPerQuantization;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Metronome Responder")
	FTimeSynthClipHandle LastClipTriggerHandle;

private:
	MetronomeTickEvent OnMetronomeTick;

	UMetronomeComponent* Metronome = nullptr;

	UPROPERTY(VisibleAnywhere)
	bool ShouldTriggerAudio = false;
};
