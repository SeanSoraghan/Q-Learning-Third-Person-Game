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
		QuantizationEvents.Add(subdivision);
		FOnQuantizationEventBP& quantizationDelegate = QuantizationEvents.FindOrAdd(subdivision);
		quantizationDelegate.BindUFunction(this, FName(FString("OnQuantizationEvent")));
	}
}

UMetronomeComponent::~UMetronomeComponent()
{

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

void UMetronomeComponent::AddMetronomeResponder(ETimeSynthEventQuantization quantizationType, UMetronomeResponderComponent* responder)
{
	TArray<UMetronomeResponderComponent*>& responders = MetronomeResponders.FindOrAdd(quantizationType);
	responders.Add(responder);
}

void UMetronomeComponent::OnQuantizationEvent(ETimeSynthEventQuantization quantizationType, int32 numBars, float beat)
{
	TArray<UMetronomeResponderComponent*>* responders = MetronomeResponders.Find(quantizationType);
	if (responders != nullptr)
	{
		for (UMetronomeResponderComponent*& responder : *responders)
		{
			if (responder != nullptr)
			{
				if (responder->TimeSynthClip != nullptr)
					TimeSynthComponent->PlayClip(responder->TimeSynthClip);
				responder->MetronomeTick();
			}
		}
	}
}

