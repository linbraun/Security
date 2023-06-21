// Fill out your copyright notice in the Description page of Project Settings.


#include "UsableItem.h"

// Sets default values
AUsableItem::AUsableItem(const class FObjectInitializer& PCIP) : Super(PCIP)
{
	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;
	SetMobility(EComponentMobility::Movable);
}