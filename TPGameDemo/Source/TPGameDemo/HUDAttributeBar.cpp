// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDAttributeBar.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
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

void UHUDAttributeBar::Initialize(UImage* inBar, UImage* inStartCap, UImage* inEndCap)
{
	bar = inBar;
	startCap = inStartCap;
	endCap = inEndCap;
	UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(bar->Slot);
	if (slot != nullptr)
	{
		FVector2D barFull = slot->GetSize();
		barScreenRatioXY = barFull / FVector2D(DefaultScreenWidth, DefaultScreenHeight);
		FAnchors anchors = slot->GetAnchors();
	}
	UpdateCapDimensions();
	UpdateBar();
}

void UHUDAttributeBar::SetBarPercent(float percentage)
{
	barPercentage = percentage;
	UpdateBar();
}

FVector2D UHUDAttributeBar::BarDimensionsForCurrentViewSize() const
{
	return barScreenRatioXY * GetGameViewportSize();
}

FVector2D UHUDAttributeBar::CapDimensionsForCurrentViewSize() const
{
	return FVector2D(barScreenRatioXY.Y * GetGameViewportSize().Y);
}

void UHUDAttributeBar::UpdateBar()
{
	FVector2D fullBarDimensions = BarDimensionsForCurrentViewSize();
	float barX = 0.0f;
	float barWidth = 0.0f;
	UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(bar->Slot);
	if (slot != nullptr)
	{
		barWidth = barPercentage * fullBarDimensions.X;
		slot->SetSize(FVector2D(barWidth, fullBarDimensions.Y));
		barX = slot->GetPosition().X;
	}
	slot = Cast<UCanvasPanelSlot>(endCap->Slot);
	if (slot != nullptr)
	{
		float y = slot->GetPosition().Y;
		slot->SetPosition(FVector2D(barWidth, y));
	}
}

void UHUDAttributeBar::UpdateCapDimensions()
{
	UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(startCap->Slot);
	if (slot != nullptr)
	{
		slot->SetSize(CapDimensionsForCurrentViewSize());
	}
	slot = Cast<UCanvasPanelSlot>(endCap->Slot);
	if (slot != nullptr)
	{
		slot->SetSize(CapDimensionsForCurrentViewSize());
	}
}

void UHUDAttributeBar::Test()
{
	barPercentage = FMath::FRand();
	UpdateBar();
}
