// Fill out your copyright notice in the Description page of Project Settings.

#include "MetronomeComponent.h"
#include "MetronomeResponderComponent.h"

// Sets default values for this component's properties
UMetronomeResponderComponent::UMetronomeResponderComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UMetronomeResponderComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UMetronomeResponderComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	OnMetronomeTick.Clear();
	if (Metronome != nullptr)
		Metronome->RemoveMetronomeResponder(this);
}

// Called every frame
void UMetronomeResponderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMetronomeResponderComponent::SetMetronome(UMetronomeComponent* metronome)
{
	Metronome = metronome;
}

bool UMetronomeResponderComponent::ShouldRespondToQuantizationIndex(int quantizationIndex) const
{
	return (quantizationIndex % QuantizationLoopLength) == QuantizationLoopOffset;
}

void UMetronomeResponderComponent::MetronomeTick()
{
	OnMetronomeTick.Broadcast();
}

void UMetronomeResponderComponent::RegisterMetronomeTickCallback(const FOnMetronomeTick& Callback)
{
	OnMetronomeTick.AddLambda([this, Callback]()
		{
			Callback.ExecuteIfBound();
		});
}

void UMetronomeResponderComponent::SetShouldTriggerAudio(bool shouldTriggerAudio)
{
	if (ShouldTriggerAudio != shouldTriggerAudio)
	{
		ShouldTriggerAudio = shouldTriggerAudio;
		if (IsValid(Metronome))
		{
			Metronome->ResponderAudioStateChanged(this);
		}
	}
}


