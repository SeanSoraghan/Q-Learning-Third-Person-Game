// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Components/ActorComponent.h"
#include "TimelineContainerComponent.generated.h"

/*
A simple container class that can be added as a component to a game object.
Game objects that make use of this component should bind responder functions to
the TimelineInterpFunction and TimelineFinishedFunction delegates.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TPGAMEDEMO_API UTimelineContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTimelineContainerComponent (const FObjectInitializer& ObjectInitializer);

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

	UTimelineComponent* Timeline;
    UCurveFloat*        FloatCurve;
    FOnTimelineFloat    TimelineInterpFunction  {};
    FOnTimelineEvent    TimelineFinishedFunction{};	

    //Debugging
    FString CurveLoadedResponse = "";
};
