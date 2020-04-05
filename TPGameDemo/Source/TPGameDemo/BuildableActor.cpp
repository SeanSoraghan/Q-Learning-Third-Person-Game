// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "BuildableActor.h"
#include "TPGameDemoGameState.h"

void ABuildableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    if (IsPlaced)
        UnplaceItem();
}

bool ABuildableActor::IsItemPlaced() const
{
    return IsPlaced;
}

void ABuildableActor::PlaceItem()
{
    IsPlaced = true;
    UWorld* world = GetWorld();
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(world->GetGameState());
    if (gameState != nullptr)
    {
        gameState->SetBuildableItemPlaced(AttachmentRoomAndPosition, AttachmentDirection, true);
    }
    ItemWasPlaced();
}

void ABuildableActor::UnplaceItem()
{
    IsPlaced = false;
    UWorld* world = GetWorld();
    ATPGameDemoGameState* gameState = (ATPGameDemoGameState*)(world->GetGameState());
    if (gameState != nullptr)
    {
        gameState->SetBuildableItemPlaced(AttachmentRoomAndPosition, AttachmentDirection, false);
    }
}

bool ABuildableActor::CanItemBePlaced() const
{
    return CanBePlaced;
}

void ABuildableActor::SetCanBePlaced(bool itemCanBePlaced)
{
    bool placementStatusChanged = itemCanBePlaced != CanBePlaced;
    CanBePlaced = itemCanBePlaced;
    if (placementStatusChanged)
        PlacementStatusChanged();
}


