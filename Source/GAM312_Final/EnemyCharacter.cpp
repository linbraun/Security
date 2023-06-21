// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "GAM312_FinalCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "EnemyController.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PlayerCollisionDetection =
		CreateDefaultSubobject<USphereComponent>(TEXT("Player Collision Detection"));

	PlayerCollisionDetection->SetupAttachment(RootComponent);

	PlayerAttackCollisionDetection =
		CreateDefaultSubobject<USphereComponent>(TEXT("Player Attack Collision Detection"));

	PlayerAttackCollisionDetection->SetupAttachment(RootComponent);

	DamageCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Damage Collision"));
	DamageCollision->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));

	CanApplyDamage = false;
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	EnemyController = Cast<AEnemyController>(GetController());
	EnemyController->GetPathFollowingComponent()->OnRequestFinished.AddUObject
	(this, &AEnemyCharacter::OnAIMoveCompleted);

	PlayerCollisionDetection->OnComponentBeginOverlap.AddDynamic(this,
		&AEnemyCharacter::OnPlayerDetectedOverlapBegin);

	PlayerCollisionDetection->OnComponentEndOverlap.AddDynamic(this,
		&AEnemyCharacter::OnPlayerDetectedOverlapEnd);

	PlayerAttackCollisionDetection->OnComponentBeginOverlap.AddDynamic(this,
		&AEnemyCharacter::OnPlayerAttackOverlapBegin);

	PlayerAttackCollisionDetection->OnComponentEndOverlap.AddDynamic(this,
		&AEnemyCharacter::OnPlayerAttackOverlapEnd);

	DamageCollision->OnComponentBeginOverlap.AddDynamic(this,
		&AEnemyCharacter::OnDealDamageOverlapBegin);

	DamageCollision->OnComponentEndOverlap.AddDynamic(this,
		&AEnemyCharacter::OnDealDamageOverlapEnd);

	AnimInstance = GetMesh()->GetAnimInstance();	
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::OnAIMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (!PlayerDetected)
	{
		EnemyController->RandomPatrol();
	}
	else if (PlayerDetected && CanAttackPlayer)
	{
		StopSeekingPlayer();

		// attack player
		AnimInstance->Montage_Play(EnemyAttackAnimation);
	}
}

void AEnemyCharacter::MoveToPlayer()
{
	if (PlayerREF && EnemyController)
	{
		EnemyController->MoveToLocation(PlayerREF->GetActorLocation(), StoppingDistance, true);
	}
}

void AEnemyCharacter::SeekPlayer()
{
	MoveToPlayer();
	GetWorld()->GetTimerManager().SetTimer(SeekPlayerTimerHandle, this,
		&AEnemyCharacter::SeekPlayer, 0.25f, true);
}

void AEnemyCharacter::StopSeekingPlayer()
{
	GetWorld()->GetTimerManager().ClearTimer(SeekPlayerTimerHandle);
}

void AEnemyCharacter::OnPlayerDetectedOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{

	PlayerREF = Cast<AGAM312_FinalCharacter>(OtherActor);
	if (PlayerREF)
	{
		PlayerDetected = true;
		SeekPlayer();
	}
}

void AEnemyCharacter::OnPlayerDetectedOverlapEnd(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	PlayerREF = Cast<AGAM312_FinalCharacter>(OtherActor);
	if (PlayerREF)
	{
		PlayerDetected = false;
		StopSeekingPlayer();
		EnemyController->RandomPatrol();
	}
}

void AEnemyCharacter::OnPlayerAttackOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	PlayerREF = Cast<AGAM312_FinalCharacter>(OtherActor);
	if (PlayerREF)
	{
		CanAttackPlayer = true;
	}
}

void AEnemyCharacter::OnPlayerAttackOverlapEnd(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	PlayerREF = Cast<AGAM312_FinalCharacter>(OtherActor);
	if (PlayerREF)
	{
		CanAttackPlayer = false;

		// stop the attack animation and chase the player
		AnimInstance->Montage_Stop(0.0f, EnemyAttackAnimation);

		SeekPlayer();
	}
}

// Deal damage to player ~ LS 12.15.22
void AEnemyCharacter::OnDealDamageOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	PlayerREF = Cast<AGAM312_FinalCharacter>(OtherActor);
	if (PlayerREF && CanDealDamage)
	{
		// Post to log
		UE_LOG(LogTemp, Warning, TEXT("Player Damaged"));

		CanApplyDamage = true;
		MyHit = SweepResult;
		GetWorldTimerManager().SetTimer(EnemyDealDamageTimerHandle, this, &AEnemyCharacter::ApplyHitDamage, 2.2f, true, 0.0f);
	}
}

// Stop dealing damage to player ~ LS 12.15.22
void AEnemyCharacter::OnDealDamageOverlapEnd(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	CanApplyDamage = false;

}

void AEnemyCharacter::AttackAnimationEnded()
{
	if (CanAttackPlayer)
	{
		AnimInstance->Montage_Play(EnemyAttackAnimation);
	}
}

void AEnemyCharacter::ApplyHitDamage()
{
	if (CanApplyDamage)
	{
		UGameplayStatics::ApplyPointDamage(PlayerREF, 200.0f, GetActorLocation(), MyHit, nullptr, this, HitDamageType);
	}
}