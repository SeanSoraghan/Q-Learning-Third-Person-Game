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


