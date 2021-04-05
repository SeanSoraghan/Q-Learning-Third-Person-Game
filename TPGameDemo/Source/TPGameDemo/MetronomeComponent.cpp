// Fill out your copyright notice in the Description page of Project Settings.


#include "MetronomeComponent.h"
#include "MetronomeResponderComponent.h"

UMetronomeComponent::UMetronomeComponent(const FObjectInitializer& ObjectInitializer)
{
	for (ETimeSynthEventQuantization subdivision = ETimeSynthEventQuantization::Bars8;
		subdivision < ETimeSynthEventQuantization::Count;
		subdivision = (ETimeSynthEventQuantization)((int)subdivision + 1))
	{
		QuantizationCounts.Add(subdivision, 0);
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
	for (ETimeSynthEventQuantization subdivision = ETimeSynthEventQuantization::Bars8;
		subdivision < ETimeSynthEventQuantization::Count;
		subdivision = (ETimeSynthEventQuantization)((int)subdivision + 1))
	{
		TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(subdivision);
		if (responders != nullptr)
		{
			responders->Empty();
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
	TArray<UMetronomeResponderComponent*>& responders = MetronomeResponders.FindOrAdd(responder->Quantization);
	responders.Add(responder);
	responder->SetMetronome(this);
}

void UMetronomeComponent::RemoveMetronomeResponder(UMetronomeResponderComponent* responder)
{
	TArray<UMetronomeResponderComponent*>& responders = MetronomeResponders.FindOrAdd(responder->Quantization);
	responders.Remove(responder);
}

void UMetronomeComponent::OnQuantizationEvent(ETimeSynthEventQuantization quantizationType, int32 numBars, float beat)
{
	QuantizationCounts[quantizationType] = (*QuantizationCounts.Find(quantizationType) + 1) % GetNumSubdivisionsPer8Bar(quantizationType);
	const int nextQuantixationIndex = QuantizationCounts[quantizationType];
	int currentQuantizationIndex = nextQuantixationIndex - 1;
	if (currentQuantizationIndex < 0)
	{
		currentQuantizationIndex += UMetronomeComponent::GetNumSubdivisionsPer8Bar(quantizationType);
	}
	TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(quantizationType);
	if (responders != nullptr)
	{
		for (UMetronomeResponderComponent*& responder : *responders)
		{
			if (IsValid(responder))
			{
				if (responder->GetShouldTriggerAudio() && responder->TimeSynthClip != nullptr)
					if (responder->ShouldRespondToQuantizationIndex(nextQuantixationIndex))
						responder->LastClipTriggerHandle = TimeSynthComponent->PlayClip(responder->TimeSynthClip, responder->VolumeGroup);
				if (responder->ShouldRespondToQuantizationIndex(currentQuantizationIndex))
					responder->MetronomeTick();
			}
		}
	}
}

void UMetronomeComponent::ResponderAudioStateChanged(UMetronomeResponderComponent* responder)
{
	TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(responder->Quantization);
	const int nextQuantixationIndex = QuantizationCounts[responder->Quantization];
	if (responders != nullptr && responders->Contains(responder))
	{
		if (responder->GetShouldTriggerAudio() && responder->TimeSynthClip != nullptr)
			if (responder->ShouldRespondToQuantizationIndex(nextQuantixationIndex))
				responder->LastClipTriggerHandle = TimeSynthComponent->PlayClip(responder->TimeSynthClip, responder->VolumeGroup);
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

int UMetronomeComponent::GetNumSubdivisionsPer8Bar(ETimeSynthEventQuantization quantizationType)
{
	switch (quantizationType)
	{
	case ETimeSynthEventQuantization::None: return 0;
	case ETimeSynthEventQuantization::Bars8: return 1;
	case ETimeSynthEventQuantization::Bars4: return 2;
	case ETimeSynthEventQuantization::Bars2: return 4;
	case ETimeSynthEventQuantization::Bar: return 8;
	case ETimeSynthEventQuantization::HalfNote: return 16;
	case ETimeSynthEventQuantization::HalfNoteTriplet: return 24;
	case ETimeSynthEventQuantization::QuarterNote: return 32;
	case ETimeSynthEventQuantization::QuarterNoteTriplet: return 48;
	case ETimeSynthEventQuantization::EighthNote: return 64;
	case ETimeSynthEventQuantization::EighthNoteTriplet: return 96;
	case ETimeSynthEventQuantization::SixteenthNote: return 128;
	case ETimeSynthEventQuantization::SixteenthNoteTriplet: return 192;
	case ETimeSynthEventQuantization::ThirtySecondNote: return 256;
	case ETimeSynthEventQuantization::Count: return 0;
	default: return 0;
	}
}


