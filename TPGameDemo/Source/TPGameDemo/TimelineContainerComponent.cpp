// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "TimelineContainerComponent.h"


// Sets default values for this component's properties
UTimelineContainerComponent::UTimelineContainerComponent (const FObjectInitializer& ObjectInitializer)
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = false;

    static ConstructorHelpers::FObjectFinder<UCurveFloat> LinearTime (TEXT("/Game/Utils/Timelines/LinearTime.LinearTime"));
    
    if (LinearTime.Object)
    {
        CurveLoadedResponse = "Loaded curve";
        FloatCurve = LinearTime.Object;
    } 
    else
        CurveLoadedResponse = "Cant find curve!";

    Timeline = ObjectInitializer.CreateDefaultSubobject<UTimelineComponent> (this, TEXT("Timeline"));
}


// Called when the game starts
void UTimelineContainerComponent::BeginPlay()
{
	Super::BeginPlay();
    Timeline->AddInterpFloat          (FloatCurve, TimelineInterpFunction);
    Timeline->SetTimelineFinishedFunc (TimelineFinishedFunction);	
}


// Called every frame
void UTimelineContainerComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );
}