// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAM312_FinalCharacter.h"
#include "GAM312_FinalProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/Actor.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "Engine.h"
#include "CameraDirector.h" // Added access to camera director for toggle - LS 11/30/22
#include "UsableItem.h" // Added usable item access - LS 12/4/22
#include "Components/PrimitiveComponent.h" // Added primitive component for highlight material - LS 12/5/22
#include "Animation/AnimInstance.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AGAM312_FinalCharacter

AGAM312_FinalCharacter::AGAM312_FinalCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void AGAM312_FinalCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}

	// Heath and HUD
	FullHealth = 1000.0f;
	Health = FullHealth;
	HealthPercentage = 1.0f;
	PreviousHealth = HealthPercentage;
	bCanBeDamaged = true;
	redFlash = false;
}

// Called every frame
void AGAM312_FinalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AUsableItem* itemSeen = GetItemFocus();
	static AUsableItem* oldFocus = NULL;

	oldFocus = ApplyPostProcessing(itemSeen, oldFocus);

}

//////////////////////////////////////////////////////////////////////////
// Input

void AGAM312_FinalCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// setup gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AGAM312_FinalCharacter::OnFire);

	// Bind Camera event
	PlayerInputComponent->BindAction("CameraToggle", IE_Released, this, &AGAM312_FinalCharacter::OnCameraSwitch);

	// Bind display raycast
	PlayerInputComponent->BindAction("Raycast", IE_Pressed, this, &AGAM312_FinalCharacter::DisplayRaycast);

	// Bind use item
	PlayerInputComponent->BindAction("Use", IE_Pressed, this, &AGAM312_FinalCharacter::Use);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AGAM312_FinalCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AGAM312_FinalCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGAM312_FinalCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGAM312_FinalCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGAM312_FinalCharacter::LookUpAtRate);
}

// Added use functionality to item being seen - LS 12/4/22
void AGAM312_FinalCharacter::Use() {
	UE_LOG(LogTemp, Warning, TEXT("Use activated."));
	if (GetItemFocus()) 
	{
		GetItemFocus()->GetStaticMeshComponent()->DestroyComponent();
	}
}

void AGAM312_FinalCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<AGAM312_FinalProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				const FRotator SpawnRotation = GetControlRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<AGAM312_FinalProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AGAM312_FinalCharacter::OnCameraSwitch()
{
	cameraToggle();
}

void AGAM312_FinalCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AGAM312_FinalCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AGAM312_FinalCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void AGAM312_FinalCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void AGAM312_FinalCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AGAM312_FinalCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AGAM312_FinalCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AGAM312_FinalCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AGAM312_FinalCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AGAM312_FinalCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AGAM312_FinalCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AGAM312_FinalCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}

// Added function to temporarily display raycast line on button press 'R' - LS 11/30/22
void AGAM312_FinalCharacter::DisplayRaycast()
{
	FHitResult* HitResult = new FHitResult();
	FVector StartTrace = GetActorLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	StartTrace = FVector(StartTrace.X + (ForwardVector.X * 100), StartTrace.Y + (ForwardVector.Y * 100), StartTrace.Z + (ForwardVector.Z * 100));
	FVector EndTrace = ((ForwardVector * 6000) + StartTrace);
	FCollisionQueryParams* TraceParams = new FCollisionQueryParams();

	if (GetWorld()->LineTraceSingleByChannel(*HitResult, StartTrace, EndTrace, ECC_Visibility, *TraceParams))
	{
		DrawDebugLine(GetWorld(), StartTrace, EndTrace, FColor(255, 0, 0), false, 2.f, 0.f, 10.f);

		if (HitResult->GetActor())
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("You hit: %s"), *HitResult->Actor->GetName()));
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("You did not hit an actor")));
		}
	}
}

// Toggle active camera on button press 'C' - LS 11/30/22
void AGAM312_FinalCharacter::cameraToggle()
{
	UE_LOG(LogTemp, Warning, TEXT("Pressed C Button: Activated by character pawn."));
	ACameraDirector* CameraDirectorPointer = Cast<ACameraDirector>(CameraDirector);
	CameraDirectorPointer->cameraGo();
}

// Highlight seen usable item with post processing volume - LS 12/4/22
AUsableItem* AGAM312_FinalCharacter::ApplyPostProcessing(AUsableItem* itemSeen, AUsableItem* oldFocus) {
	if (itemSeen) {
		// An item is currently being looked at
		if (itemSeen == oldFocus || oldFocus == NULL) {
			//The item being looked at is the same as the one on the last tick
			UStaticMeshComponent* mesh = itemSeen->GetStaticMeshComponent();
			mesh->SetRenderCustomDepth(true);
			UE_LOG(LogTemp, Warning, TEXT("Same item, only new post processing applied."));

		}
		else if (oldFocus != NULL) {
			// An item is being looked at and the old focus was not null (and not the same as the one on the last tick)
			UStaticMeshComponent* mesh = itemSeen->GetStaticMeshComponent();
			mesh->SetRenderCustomDepth(true);

			UStaticMeshComponent* oldMesh = oldFocus->GetStaticMeshComponent();
			oldMesh->SetRenderCustomDepth(false);
			UE_LOG(LogTemp, Warning, TEXT("New item, old and new post processing applied."));
		}

		return oldFocus = itemSeen;
	}
	else {
		// No item currectly being looked at
		if (oldFocus != NULL) {
			//An item was looked at last tick but isn't being looked at anymore
			UStaticMeshComponent* mesh = oldFocus->GetStaticMeshComponent();
			mesh->SetRenderCustomDepth(false);
			UE_LOG(LogTemp, Warning, TEXT("No new item, old post processing applied."));
		}

		return oldFocus = NULL;
	}

}

// Get item player is looking at - LS 12/5/22
AUsableItem* AGAM312_FinalCharacter::GetItemFocus() {
	FHitResult* HitResult = new FHitResult();
	FVector StartTrace = GetActorLocation();
	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	StartTrace = FVector(StartTrace.X + (ForwardVector.X * 100), StartTrace.Y + (ForwardVector.Y * 100), StartTrace.Z + (ForwardVector.Z * 100));
	FVector EndTrace = ((ForwardVector * 6000) + StartTrace);
	FCollisionQueryParams* TraceParams = new FCollisionQueryParams();

	GetWorld()->LineTraceSingleByChannel(*HitResult, StartTrace, EndTrace, ECC_Visibility, *TraceParams);

	return Cast<AUsableItem>(HitResult->GetActor());
}


// Health and HUD ~ LS 12.15.22
float AGAM312_FinalCharacter::GetHealth()
{
	return HealthPercentage;
}

FText AGAM312_FinalCharacter::GetHealthIntText()
{
	int32 HP = FMath::RoundHalfFromZero(HealthPercentage * 100);
	FString HPS = FString::FromInt(HP);
	FString HealthHUD = HPS + FString(TEXT("%"));
	FText HPText = FText::FromString(HealthHUD);
	return HPText;
}

// Toggle player's invincibility state ~ LS 12.15.22
void AGAM312_FinalCharacter::SetDamageState()
{
	bCanBeDamaged = true;
}

void AGAM312_FinalCharacter::DamageTimer()
{
	GetWorldTimerManager().SetTimer(MemberTimerHandle, this, &AGAM312_FinalCharacter::SetDamageState, 2.0f, false);
}

float AGAM312_FinalCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	bCanBeDamaged = false;
	redFlash = true;
	UpdateHealth(-DamageAmount);
	DamageTimer();
	return DamageAmount;
}

// Update health ~ LS 12.15.22
void AGAM312_FinalCharacter::UpdateHealth(float HealthChange)
{
	Health += HealthChange;
	Health = FMath::Clamp(Health, 0.0f, FullHealth);
	PreviousHealth = HealthPercentage;
	HealthPercentage = Health / FullHealth;
}

bool AGAM312_FinalCharacter::PlayFlash()
{
	if (redFlash)
	{
		redFlash = false;
		return true;
	}
	return false;
}

// Receive damage ~ LS 12.15.22
//void AGAM312_FinalCharacter::ReceivePointDamage(float Damage, const UDamageType* DamageType, FVector HitLocation, FVector HitNormal, UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, AController* InstigatedBy, AActor* DamageCauser, const FHitResult& HitInfo)

	//bCanBeDamaged = false;
	//UpdateHealth(-Damage);
	//DamageTimer();


