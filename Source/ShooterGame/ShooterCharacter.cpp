// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Item.h"
#include "Components/WidgetComponent.h"
#include "Weapon.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Ammo.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ShooterGame.h"
#include "BulletHitInterface.h"
#include "Enemy.h"
#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"

// Sets default values
AShooterCharacter::AShooterCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	bAiming(false),

	CameraDefaultFOV(0.f),
	CameraZoomedFOV(35.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.f),

	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),

	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.6f),
	MouseAimingLookUpRate(0.6f),

	CrosshairSpreadMultiplier(0.f),
	CrosshairVelocityFactor(0.f),
	CrosshairInAirFactor(0.f),
	CrosshairAimFactor(0.f),
	CrosshairShootingFactor(0.f),

	ShootTimeDuration(0.05f),
	bFiringBullet(false),
	bShouldFire(true),
	bFireButtonPressed(false),

    bShouldTraceForItems(false),
	OverlappedItemCount(0),
	CameraInterpDistance(250.f),
	CameraInterpElevation(65.f),

	Starting9mmmAmmo(80),
	StartingARAmmo(120),
	bCrouching(false),
	CombatState(ECombatState::ECS_Unoccupied),
	BaseMovementSpeed(600.f),
	CrouchMovementSpeed(300.f),
	StandingCapsuleHalfHeight(88.f),
	CrouchingCapsuleHalfHeight(44.f),
	BaseGroundFriction(2.f),
	CrouchingGroundFriction(100.f),
	bAimingButtonPressed(false),
	bShouldPlayPickupSound(true),
	bShouldPlayEquipSound(true),
	PickupSoundResetTime(0.2f),
	EquipSoundResetTime(0.2f),
	HighlightedSlot(-1),
	Health(100.f),
	MaxHealth(100.f),
	StunChance(.25f),
	bDead(false)

{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f,35.f,60.f);

	//Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));

	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponInterpolation Component"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component1"));
	InterpComp1->SetupAttachment(GetFollowCamera());

	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component2"));
	InterpComp2->SetupAttachment(GetFollowCamera());

	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component3"));
	InterpComp3->SetupAttachment(GetFollowCamera());

	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component4"));
	InterpComp4->SetupAttachment(GetFollowCamera());

	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component5"));
	InterpComp5->SetupAttachment(GetFollowCamera());

	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Innterpolation Component6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

}


float AShooterCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();
		auto EnemyController = Cast<AEnemyController>(EventInstigator);
		if (EnemyController) {
			EnemyController->GetBlackboardComponent()->SetValueAsBool(
				FName(TEXT("CharacterDead")),
				true
			);
		}
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

void AShooterCharacter::Die()
{
	bDead = true;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
		AnimInstance->Montage_JumpToSection(FName("DeathA"), DeathMontage);
	}

	// Start bullet fire timer for crosshairs
	StartCrosshairBulletFire();
}

void AShooterCharacter::FinishDeath()
{
	GetMesh()->bPauseAnims = true;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		DisableInput(PC);
	}
}


// Called when the game starts or when spawned
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FollowCamera) {
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);

	EquippedWeapon->SetSlotIndex(0);
	InitializeAmmoMap();

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	InitializeInterpLocations();

	EquippedWeapon->SetCharacter(this);
}

void AShooterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f)) {
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f)) {
		//find out which way is right
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());

}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor{};
	if (bAiming) {
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else {
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value * TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if (bAiming) {
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else {
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpScaleFactor);
}

void AShooterCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr)return;
	if (CombatState != ECombatState::ECS_Unoccupied)return;
	if (WeaponHasAmmo()) {
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();
		StartCrosshairBulletFire();
		EquippedWeapon->DecrementAmmo();
		StartFireTimer();
		if (EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
		{
			// Start moving slide timer
			EquippedWeapon->StartSlideTimer();
		}
	}
}

bool AShooterCharacter::GetBeamEndLocation(
	const FVector& MuzzleSocketLocation,
	FHitResult& OutHitResult)
{
	FVector OutBeamLocation;
	// Check for crosshair trace hit
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		// Tentative beam location - still need to trace from gun
		OutBeamLocation = CrosshairHitResult.Location;
	}
	else // no crosshair trace hit
	{
		// OutBeamLocation is the End location for the line trace
	}

	// Perform a second trace, this time from the gun barrel
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - WeaponTraceStart };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		WeaponTraceStart,
		WeaponTraceEnd,
		ECollisionChannel::ECC_Visibility);
	if (!OutHitResult.bBlockingHit) // object between barrel and BeamEndPoint?
	{
		OutHitResult.Location = OutBeamLocation;
		return false;
	}

	return true;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading && CombatState != ECombatState::ECS_Equipping && CombatState != ECombatState::ECS_Stunned)
	{
		Aim();
	}
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	if (bAiming) {
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraZoomedFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	else {
		CameraCurrentFOV = FMath::FInterpTo(
			CameraCurrentFOV,
			CameraDefaultFOV,
			DeltaTime,
			ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::SetLookRates()
{
	if (bAiming) {
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else {
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.f,600.f };
	FVector2D VelocityMultiplierRange{ 0.f,1.f };
	FVector Velocity{ GetVelocity() };
	Velocity.Z = 0.f;
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(
		WalkSpeedRange,
		VelocityMultiplierRange,
		Velocity.Size());

	if (GetCharacterMovement()->IsFalling()) { //是否在空中？
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else {
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if (bAiming) { //是否在瞄准？
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.6f,
			DeltaTime,
			30.f);
	}
	else {
		CrosshairAimFactor = FMath::FInterpTo(
			CrosshairAimFactor,
			0.f,
			DeltaTime,
			30.f);
	}

	if (bFiringBullet) {
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.3f,
			DeltaTime,
			60.f);
	}
	else {
		CrosshairShootingFactor = FMath::FInterpTo(
			CrosshairShootingFactor,
			0.f,
			DeltaTime,
			60.f);
	}

	CrosshairSpreadMultiplier = 
		0.5f + 
		CrosshairVelocityFactor + 
		CrosshairInAirFactor - 
		CrosshairAimFactor + 
		CrosshairShootingFactor;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;

	GetWorldTimerManager().SetTimer(
		CrosshairShooterTimer,
		this,
		&AShooterCharacter::FinishCrosshairBulletFire,
		ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();	
}

void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	CombatState = ECombatState::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(
			AutoFireTimer,
			this,
			&AShooterCharacter::AutoFireReset,
		EquippedWeapon->GetAutoFireRate());
}

void AShooterCharacter::AutoFireReset()
{
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;

	if (EquippedWeapon == nullptr) return;
	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	}
	else
	{
		// Reload Weapon
		ReloadWeapon();
	}
	
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult,FVector& OutHitLocation)
{
	//获取当前视口大小 
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport) {
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	//获取准星位置
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= -50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	//获取准星朝向
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld) {
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ Start + CrosshairWorldDirection * 50000.f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(
			OutHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility);
		if (OutHitResult.bBlockingHit) {
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}
	return false;
}

void AShooterCharacter::TraceForItems()
{
	//if (bShouldTraceForItems)
	//{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
			if (TraceHitWeapon)
			{
				if (HighlightedSlot == -1)
				{
					// Not currently highlighting a slot; highlight one
					HighlightInventorySlot();
				}
			}
			else
			{
				// Is a slot being highlight?
				if (HighlightedSlot != -1)
				{
					// UnHighlight the slot
					UnHighlightInventorySlot();
				}
			}

			if (TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}

			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				// Show Item's Pickup Widget
				TraceHitItem->GetPickupWidget()->SetVisibility(true);

				if (Inventory.Num() >= INVENTORY_CAPACITY)
				{
					// Inventory is full
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					// Inventory has room
					TraceHitItem->SetCharacterInventoryFull(false);
				}
			}

			// We hit an AItem last frame
			if (TraceHitItemLastFrame)
			{
				if (TraceHitItem != TraceHitItemLastFrame)
				{
					// We are hitting a different AItem this frame from last frame
					// Or AItem is null.
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
				}
			}
			// Store a reference to HitItem for next frame
			TraceHitItemLastFrame = TraceHitItem;
		}
	//}
	//else if (TraceHitItemLastFrame)
	//{
		// No longer overlapping any items,
		// Item last frame should not show widget
	//	TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);

	//}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass) {
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip, bool bSwapping)
{
	if (WeaponToEquip) {

		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			// Attach the Weapon to the hand socket RightHandSocket
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}

		if (EquippedWeapon == nullptr)
		{
			// -1 == no EquippedWeapon yet. No need to reverse the icon animation
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}

		// Set EquippedWeapon to the newly spawned Weapon
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon()
{
	if (EquippedWeapon) {
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::SelectButtonPressed()
{
	if (CombatState != ECombatState::ECS_Unoccupied) return;
	if (TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
		TraceHitItem = nullptr;
	}
}

void AShooterCharacter::SelectButtonReleased()
{
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if (Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	DropWeapon();
	EquipWeapon(WeaponToSwap,true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm,Starting9mmmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr)return false;

	return EquippedWeapon->GetAmmo() > 0;
}

void AShooterCharacter::PlayFireSound()
{
	//--------开枪声音
	if (EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AShooterCharacter::SendBullet()
{
	//--------发射子弹
	const USkeletalMeshSocket* BarrelSocket =
		EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket) {
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		//开枪粒子效果
		if (EquippedWeapon->GetMuzzleFlash()) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), SocketTransform);
		}

		//开枪弹道
		FHitResult BeamHitResult;
		bool bBeamEnd = GetBeamEndLocation(
			SocketTransform.GetLocation(),
			BeamHitResult);
		if (bBeamEnd) {
			// Does hit Actor implement BulletHitInterface?
			if (BeamHitResult.GetActor())
			{
				IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.GetActor());
				if (BulletHitInterface)
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult);
				}

				AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.GetActor());
				if (HitEnemy)
				{
					int32 Damage{};
					if (BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadBone())
					{
						// Head shot
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, true);
					}
					else
					{
						// Body shot
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(
							BeamHitResult.GetActor(),
							Damage,
							GetController(),
							this,
							UDamageType::StaticClass());
						HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, false);
					}
				}
			}
			else {
				if (ImpactParticles) {
					UGameplayStatics::SpawnEmitterAtLocation(
						GetWorld(),
						ImpactParticles,
						BeamHitResult.Location);
				}
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					BeamParticles,
					SocketTransform);
				if (Beam) {
					Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
				}
			}
		}
	}
}

void AShooterCharacter::PlayGunFireMontage()
{
	//----------播放动画
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage) {
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if (CombatState != ECombatState::ECS_Unoccupied)return;
	if (bAiming) {
		StopAiming();
	}
	if (EquippedWeapon == nullptr) return;
	if (CarryingAmmo() &&  !EquippedWeapon->ClipIsFull()) {
		if (bAiming) {
			StopAiming();
		}

		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (ReloadMontage && AnimInstance) {
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

bool AShooterCharacter::CarryingAmmo()
{
	if (EquippedWeapon == nullptr) return false;
	auto AmmoType = EquippedWeapon->GetAmmoType();
	if (AmmoMap.Contains(AmmoType)) {
		return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AShooterCharacter::FinishReloading()
{
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed) {
		Aim();
	}

	if (EquippedWeapon == nullptr)return;
	const auto AmmoType{ EquippedWeapon->GetAmmoType() };
	if (AmmoMap.Contains(AmmoType)) {
		int32 CarriedAmmo = AmmoMap[AmmoType];
		const int32 MagEmptySpace =
			EquippedWeapon->GetMagazineCapacity() - 
			EquippedWeapon->GetAmmo();
		if (MagEmptySpace > CarriedAmmo) {
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else {
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AShooterCharacter::CrouchButtonPressed() {
	if (!GetCharacterMovement()->IsFalling()) {
		bCrouching = !bCrouching;
	}
	if (bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}

}

void AShooterCharacter::Jump()
{
	if (bCrouching) {
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else {
		ACharacter::Jump();
	}
}

void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight{};
	if (bCrouching) {
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else {
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}

	const float InterpHalfHeight{ 
		FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),TargetCapsuleHalfHeight,DeltaTime,20.f) };

	const float DeltaCapsuleHalfHeight{ InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() };

	const FVector MeshOffset{ 0.f,0.f,-DeltaCapsuleHalfHeight };

	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AShooterCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;
	if (!bCrouching) {
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AShooterCharacter::PickupAmmo(AAmmo* Ammo)
{
	if (AmmoMap.Find(Ammo->GetAmmoType())) {
		int32 AmmoCount{ AmmoMap[Ammo->GetAmmoType()] };
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType()) {
		if (EquippedWeapon->GetAmmo() == 0) {
			ReloadWeapon();
		}
	}
	Ammo->Destroy();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation{ WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1, 0 };
	InterpLocations.Add(InterpLoc1);

	FInterpLocation InterpLoc2{ InterpComp2, 0 };
	InterpLocations.Add(InterpLoc2);

	FInterpLocation InterpLoc3{ InterpComp3, 0 };
	InterpLocations.Add(InterpLoc3);

	FInterpLocation InterpLoc4{ InterpComp4, 0 };
	InterpLocations.Add(InterpLoc4);

	FInterpLocation InterpLoc5{ InterpComp5, 0 };
	InterpLocations.Add(InterpLoc5);

	FInterpLocation InterpLoc6{ InterpComp6, 0 };
	InterpLocations.Add(InterpLoc6);
}

void AShooterCharacter::FKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::OneKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::TwoKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::ThreeKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 3) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::FourKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 4) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::FiveKeyPressed()
{
	if (EquippedWeapon->GetSlotIndex() == 5) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::ExchangeInventoryItems(int32 CurrentItemIndex, int32 NewItemIndex)
{
	const bool bCanExchangeItems =
		(CurrentItemIndex != NewItemIndex) &&
		(NewItemIndex < Inventory.Num()) &&
		(CombatState == ECombatState::ECS_Unoccupied || CombatState == ECombatState::ECS_Equipping);

	if (bCanExchangeItems)
	{
		if (bAiming)
		{
			StopAiming();
		}

		auto OldEquippedWeapon = EquippedWeapon;
		auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);

		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);

		CombatState = ECombatState::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.0f);
			AnimInstance->Montage_JumpToSection(FName("Equip"));
		}
		NewWeapon->PlayEquipSound(true);
	}
}
void AShooterCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr) return;
	if (HandSceneComponent == nullptr) return;

	// Index for the clip bone on the Equipped Weapon
	int32 ClipBoneIndex{ EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName()) };
	// Store the transform of the clip
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

int32 AShooterCharacter::GetEmptyInventorySlot()
{
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if (Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}

	return -1; // Inventory is full!
}

void AShooterCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot{ GetEmptyInventorySlot() };
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

void AShooterCharacter::UnHighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}


EPhysicalSurface AShooterCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start{ GetActorLocation() };
	const FVector End{ Start + FVector(0.f, 0.f, -400.f) };
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility,
		QueryParams);
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AShooterCharacter::Stun()
{
	if (Health <= 0.f) return;

	CombatState = ECombatState::ECS_Stunned;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

void AShooterCharacter::EndStun()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (bAimingButtonPressed)
	{
		Aim();
	}
}


void AShooterCharacter::Footstep()
{
	FHitResult HitResult;
	const FVector Start{ GetActorLocation() };
	const FVector End{ Start + FVector(0.f, 0.f, -400.f) };
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECollisionChannel::ECC_Visibility,
		QueryParams);
	auto HitSurface = HitResult.PhysMaterial->SurfaceType;
	if (HitSurface == EPS_Grass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hit Grass surface type!"));
	}
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for (int32 i = 1; i < InterpLocations.Num(); i++) {
		if (InterpLocations[i].ItemCount < LowestCount) {
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}

	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if (Amount < -1 || Amount > 1)return;
	if (InterpLocations.Num() >= Index) {
		InterpLocations[Index].ItemCount += Amount;
	}
}


// Called every frame
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CameraInterpZoom(DeltaTime);
	SetLookRates();
	CalculateCrosshairSpread(DeltaTime);
	TraceForItems();
	InterpCapsuleHalfHeight(DeltaTime);

}

// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);
	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);
	
	PlayerInputComponent->BindAction("FKey", IE_Pressed, this,
		&AShooterCharacter::FKeyPressed);
	PlayerInputComponent->BindAction("1Key", IE_Pressed, this,
		&AShooterCharacter::OneKeyPressed);
	PlayerInputComponent->BindAction("2Key", IE_Pressed, this,
		&AShooterCharacter::TwoKeyPressed);
	PlayerInputComponent->BindAction("3Key", IE_Pressed, this,
		&AShooterCharacter::ThreeKeyPressed);
	PlayerInputComponent->BindAction("4Key", IE_Pressed, this,
		&AShooterCharacter::FourKeyPressed);
	PlayerInputComponent->BindAction("5Key", IE_Pressed, this,
		&AShooterCharacter::FiveKeyPressed);
}

void AShooterCharacter::FinishEquipping()
{
	if (CombatState == ECombatState::ECS_Stunned) return;

	CombatState = ECombatState::ECS_Unoccupied;
	if (bAimingButtonPressed)
	{
		Aim();
	}
}


void AShooterCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AShooterCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}


float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if (OverlappedItemCount + Amount <= 0) {
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else {
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AShooterCharacter::GetPickupItem(AItem* Item)
{
	Item->PlayEquipSound();
	auto Weapon = Cast<AWeapon>(Item);
	if (Weapon) {
		if (Inventory.Num() < INVENTORY_CAPACITY)
		{
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else // Inventory is full! Swap with EquippedWeapon
		{
			SwapWeapon(Weapon);
		}
	}
	auto Ammo = Cast<AAmmo>(Item);
	if (Ammo) {
		PickupAmmo(Ammo);
	}
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
	if (Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

void AShooterCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(
		PickupSoundTimer,
		this,
		&AShooterCharacter::ResetPickupSoundTimer,
		PickupSoundResetTime);
}

void AShooterCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(
		PickupSoundTimer,
		this,
		&AShooterCharacter::ResetEquipSoundTimer,
		EquipSoundResetTime);
}
