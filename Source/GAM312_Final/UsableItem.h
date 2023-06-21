// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "Engine/StaticMeshActor.h"
#include "UsableItem.generated.h"

UCLASS()
class AUsableItem : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AUsableItem(const class FObjectInitializer& PCIP);
};