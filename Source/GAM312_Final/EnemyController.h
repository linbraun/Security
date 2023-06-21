// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyController.generated.h"

/**
 * 
 */
UCLASS()
class GAM312_FINAL_API AEnemyController : public AAIController
{
	GENERATED_BODY()

public:
    void BeginPlay() override;

private:

    class UNavigationSystemV1* NavArea;

    FVector RandomLocation;

public:

    UFUNCTION()
        void RandomPatrol();
};
