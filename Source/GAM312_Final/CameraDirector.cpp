// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraDirector.h"
#include "Kismet/GameplayStatics.h"	// Added for player controller access

// Sets default values
ACameraDirector::ACameraDirector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACameraDirector::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.bCanEverTick = true;

	// Added camera director begin play - LS 11/30/22
	cameraChange = false;

	UE_LOG(LogTemp, Warning, TEXT("Camera Director BeginPlay"));	
}

// Called every frame
void ACameraDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Added Two Camera Component -LS 11/29/22
	const float TimeBetweenCameraChange = 2.0f;
	const float SmoothBlendTime = 0.75f;

	TimeToNextCameraChange -= DeltaTime;

	// This portion replaces the automatic toggle commented below in favor of a key press toggle
	if (cameraChange)
	{
		if (TimeToNextCameraChange <= 0.0f) {
			TimeToNextCameraChange += TimeBetweenCameraChange;

			APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);

			if (OurPlayerController) {
				if (CameraTwo && (OurPlayerController->GetViewTarget() == CameraOne)) {
					OurPlayerController->SetViewTargetWithBlend(CameraTwo, SmoothBlendTime);
				}
				else if (CameraOne) {
					OurPlayerController->SetViewTargetWithBlend(CameraOne, SmoothBlendTime);
				}
			}
		}
		else {
			cameraChange = false;
			TimeToNextCameraChange = 0.0f;
		}
	}
}

	/*
	APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (OurPlayerController)
	{
		if (TimeToNextCameraChange <= 0.0f)
		{
			TimeToNextCameraChange += TimeBetweenCameraChange;

			if (CameraTwo && (OurPlayerController->GetViewTarget() == CameraOne))
			{
				OurPlayerController->SetViewTargetWithBlend(CameraTwo, SmoothBlendTime);
			}
			else if (CameraOne)
			{
				OurPlayerController->SetViewTarget(CameraOne);
			}
		}
	}
	*/

void ACameraDirector::cameraGo()
{
	cameraChange = true;
	TimeToNextCameraChange = 0.0f;
	
	UE_LOG(LogTemp, Warning, TEXT("cameraGo function activated"));
}


