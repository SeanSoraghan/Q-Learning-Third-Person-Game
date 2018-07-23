// Fill out your copyright notice in the Description page of Project Settings.

#include "TPGameDemo.h"
#include "BuildableActor.h"

bool ABuildableActor::IsItemBuilt() const
{
    return IsBuilt;
}

void ABuildableActor::PlaceItem()
{
    IsBuilt = true;
    ItemWasPlaced();
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


