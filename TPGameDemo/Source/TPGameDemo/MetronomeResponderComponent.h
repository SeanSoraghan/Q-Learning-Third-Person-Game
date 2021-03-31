// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MetronomeResponderComponent.generated.h"

DECLARE_EVENT(UMetronomeResponderComponent, MetronomeTickEvent);
DECLARE_DYNAMIC_DELEGATE(FOnMetronomeTick);

class UTimeSynthClip;

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

public:
	UFUNCTION(BlueprintCallable, Category = "Level Training")
	void RegisterMetronomeTickCallback(const FOnMetronomeTick& Callback);


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UTimeSynthClip* TimeSynthClip;

	void MetronomeTick();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	MetronomeTickEvent OnMetronomeTick;

		
};
