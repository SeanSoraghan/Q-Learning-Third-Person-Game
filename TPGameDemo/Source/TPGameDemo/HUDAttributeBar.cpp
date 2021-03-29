// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDAttributeBar.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Runtime/Engine/Classes/Engine/RendererSettings.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"

FVector2D GetGameViewportSize()
{
	FVector2D Result = FVector2D(1, 1);

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Result);
	}

	return Result;
}

FVector2D GameResolution()
{
	return FVector2D(GSystemResolution.ResX, GSystemResolution.ResY);
}

void UHUDAttributeBar::Initialize(UImage* inBar, UImage* inTrack, UImage* inStartCap, UImage* inEndCap)
{
	bar = inBar;
	track = inTrack;
	startCap = inStartCap;
	endCap = inEndCap;
	UpdateBar();
}

void UHUDAttributeBar::SetBarPercent(float percentage)
{
	barPercentage = percentage;
	UpdateBar();
}

void UHUDAttributeBar::UpdateBar()
{
	float barX = 0.0f;
	float barWidth = 0.0f;
	if (bar != nullptr && track != nullptr && endCap != nullptr)
	{
		UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(bar->Slot);
		if (slot != nullptr)
		{
			FVector2D viewportSize = GetGameViewportSize();
			const float viewportScale = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(viewportSize.X, viewportSize.Y));
			barWidth = barPercentage * FullBarWidth_NoScaling * viewportScale;
			slot->SetSize(FVector2D(barWidth, slot->GetSize().Y));
			barX = slot->GetPosition().X;
		}
		slot = Cast<UCanvasPanelSlot>(endCap->Slot);
		if (slot != nullptr)
		{
			float y = slot->GetPosition().Y;
			slot->SetPosition(FVector2D(barX + barWidth, y));
		}
		slot = Cast<UCanvasPanelSlot>(track->Slot);
		if (slot != nullptr)
		{
			FVector2D viewportSize = GetGameViewportSize();
			const float viewportScale = GetDefault<UUserInterfaceSettings>(UUserInterfaceSettings::StaticClass())->GetDPIScaleBasedOnSize(FIntPoint(viewportSize.X, viewportSize.Y));
			slot->SetSize(FVector2D(FullBarWidth_NoScaling * viewportScale, slot->GetSize().Y));
		}
	}
}

void UHUDAttributeBar::Test()
{
	barPercentage = FMath::FRand();
	UpdateBar();
}
