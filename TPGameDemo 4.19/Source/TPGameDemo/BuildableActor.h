// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MazeActor.h"
#include "BuildableActor.generated.h"

/**
 * 
 */
UCLASS()
class TPGAMEDEMO_API ABuildableActor : public AMazeActor
{
	GENERATED_BODY()
	
public:
    UFUNCTION(BlueprintCallable, Category = "Buildable Items")
        bool IsItemBuilt() const;

    UFUNCTION(BlueprintCallable, Category = "Buildable Items Placement")
        void PlaceItem();

    UFUNCTION(BlueprintImplementableEvent, Category = "Buildable Items Placement")
        void ItemWasPlaced();

private:
	bool IsBuilt = false;
};
