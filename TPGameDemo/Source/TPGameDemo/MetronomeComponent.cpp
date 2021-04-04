// Fill out your copyright notice in the Description page of Project Settings.


#include "MetronomeComponent.h"
#include "MetronomeResponderComponent.h"

UMetronomeComponent::UMetronomeComponent(const FObjectInitializer& ObjectInitializer)
{
	for (ETimeSynthEventQuantization subdivision = ETimeSynthEventQuantization::Bars8;
		subdivision < ETimeSynthEventQuantization::Count;
		subdivision = (ETimeSynthEventQuantization)((int)subdivision + 1))
	{
		MetronomeResponders.Add(subdivision, TArray<UMetronomeResponderComponent*>());
		FOnQuantizationEventBP& delegate = QuantizationEvents.Add(subdivision);
		delegate.BindUFunction(this, FName(FString("OnQuantizationEvent")));
	}
}

UMetronomeComponent::~UMetronomeComponent()
{

}

void UMetronomeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	FScopeLock lock(&RespondersMutex);
	for (ETimeSynthEventQuantization subdivision = ETimeSynthEventQuantization::Bars8;
		subdivision < ETimeSynthEventQuantization::Count;
		subdivision = (ETimeSynthEventQuantization)((int)subdivision + 1))
	{
		TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(subdivision);
		if (responders != nullptr)
		{
			for (UMetronomeResponderComponent*& responder : *responders)
			{
				if (IsValid(responder))
				{
					responder->RemoveFromRoot();
				}
			}
		}
	}
}

void UMetronomeComponent::SetTimeSynthComponent(UTimeSynthComponent* timeSynthComp)
{
	TimeSynthComponent = timeSynthComp;
	if (TimeSynthComponent != nullptr)
	{
		for (ETimeSynthEventQuantization subdivision = ETimeSynthEventQuantization::Bars8;
			subdivision < ETimeSynthEventQuantization::Count;
			subdivision = (ETimeSynthEventQuantization)((int)subdivision + 1))
		{
			FOnQuantizationEventBP* quantizationDelegate = QuantizationEvents.Find(subdivision);
			if (quantizationDelegate != nullptr)
				TimeSynthComponent->AddQuantizationEventDelegate(subdivision, *quantizationDelegate);
		}
	}
}

void UMetronomeComponent::AddMetronomeResponder(UMetronomeResponderComponent* responder)
{
	FScopeLock lock(&RespondersMutex);
	TArray<UMetronomeResponderComponent*>& responders = MetronomeResponders.FindOrAdd(responder->Quantization);
	responders.Add(responder);
	responder->SetMetronome(this);
	responder->AddToRoot();
}

void UMetronomeComponent::RemoveMetronomeResponder(UMetronomeResponderComponent* responder)
{
	FScopeLock lock(&RespondersMutex);
	TArray<UMetronomeResponderComponent*>& responders = MetronomeResponders.FindOrAdd(responder->Quantization);
	responders.Remove(responder);
}

void UMetronomeComponent::OnQuantizationEvent(ETimeSynthEventQuantization quantizationType, int32 numBars, float beat)
{
	FScopeLock lock(&RespondersMutex);
	TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(quantizationType);
	if (responders != nullptr)
	{
		for (UMetronomeResponderComponent*& responder : *responders)
		{
			if (IsValid(responder))
			{
				if (responder->GetShouldTriggerAudio() && responder->TimeSynthClip != nullptr)
					responder->LastClipTriggerHandle = TimeSynthComponent->PlayClip(responder->TimeSynthClip);
				responder->MetronomeTick();
			}
		}
	}
}

void UMetronomeComponent::ResponderAudioStateChanged(UMetronomeResponderComponent* responder)
{
	FScopeLock lock(&RespondersMutex);
	TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(responder->Quantization);
	if (responders != nullptr && responders->Contains(responder))
	{
		if (responder->GetShouldTriggerAudio() && responder->TimeSynthClip != nullptr)
			responder->LastClipTriggerHandle = TimeSynthComponent->PlayClip(responder->TimeSynthClip);
		if (!responder->GetShouldTriggerAudio())
			TimeSynthComponent->StopClip(responder->LastClipTriggerHandle, (ETimeSynthEventClipQuantization)((int)responder->Quantization + 1));
	}
}

float UMetronomeComponent::GetSecondsPerMetronomeQuantization(ETimeSynthEventQuantization quantizationType) const
{
	ensure(TimeSynthComponent != nullptr);
	if (TimeSynthComponent == nullptr)
		return -1.0f;

	/* 
	None					UMETA(DisplayName = "No Quantization"),
	Bars8					UMETA(DisplayName = "8 Bars"),1
	Bars4					UMETA(DisplayName = "4 Bars"),2
	Bars2					UMETA(DisplayName = "2 Bars"),4
	Bar						UMETA(DisplayName = "1 Bar"),8
	HalfNote				UMETA(DisplayName = "1/2"),16
	HalfNoteTriplet			UMETA(DisplayName = "1/2 T"),
	QuarterNote				UMETA(DisplayName = "1/4"),
	QuarterNoteTriplet		UMETA(DisplayName = "1/4 T"),
	EighthNote				UMETA(DisplayName = "1/8"),
	EighthNoteTriplet		UMETA(DisplayName = "1/8 T"),
	SixteenthNote			UMETA(DisplayName = "1/16"),
	SixteenthNoteTriplet	UMETA(DisplayName = "1/16 T"),
	ThirtySecondNote		UMETA(DisplayName = "1/32"),
	Count					UMETA(Hidden)
	*/

	ensure(quantizationType != ETimeSynthEventQuantization::None);
	if (quantizationType == ETimeSynthEventQuantization::None)
		return -1.0f;

	const float secondsPerBeat = 60.0f / (float)TimeSynthComponent->GetBPM();
	const float secondsPer16Bars = secondsPerBeat * 64.0f;

	if (quantizationType < ETimeSynthEventQuantization::HalfNoteTriplet)
		return secondsPer16Bars / ((float)quantizationType * 2.0f);

	if (quantizationType == ETimeSynthEventQuantization::HalfNoteTriplet)
		return (secondsPerBeat * 2.0f) / 3.0f;

	if (quantizationType < ETimeSynthEventQuantization::EighthNote)
		return secondsPerBeat / (1.0f + 2.0f * ((float)quantizationType - (float)ETimeSynthEventQuantization::QuarterNote));

	if (quantizationType < ETimeSynthEventQuantization::SixteenthNote)
		return (secondsPerBeat / 2.0f) / (1.0f + 2.0f * ((float)quantizationType - (float)ETimeSynthEventQuantization::EighthNote));

	if (quantizationType < ETimeSynthEventQuantization::ThirtySecondNote)
		return (secondsPerBeat / 4.0f) / (1.0f + 2.0f * ((float)quantizationType - (float)ETimeSynthEventQuantization::SixteenthNote));

	return secondsPerBeat / 8;
}


