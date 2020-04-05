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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Buildable Items")
        bool IsItemPlaced() const;

    UFUNCTION(BlueprintCallable, Category = "Buildable Items Placement")
        void PlaceItem();

    UFUNCTION(BlueprintCallable, Category = "Buildable Items Placement")
        void UnplaceItem();

    UFUNCTION(BlueprintImplementableEvent, Category = "Buildable Items Placement")
        void ItemWasPlaced();

    UFUNCTION(BlueprintCallable, Category = "Buildable Items Placement")
        bool CanItemBePlaced() const;

    UFUNCTION(BlueprintCallable, Category = "Buildable Items Placement")
        void SetCanBePlaced(bool itemCanBePlaced);

    UFUNCTION(BlueprintImplementableEvent, Category = "Buildable Items Placement")
        void PlacementStatusChanged();

    UPROPERTY(BlueprintReadWrite, Category = "Buildable Items Resources Cost")
        int ResourceCost;

    UPROPERTY(BlueprintReadWrite, Category = "Buildable Items Resources Cost")
        FRoomPositionPair AttachmentRoomAndPosition;

    UPROPERTY(BlueprintReadWrite, Category = "Buildable Items Resources Cost")
        EDirectionType AttachmentDirection;

private:
	bool IsPlaced = false;
    bool CanBePlaced = true;
};
