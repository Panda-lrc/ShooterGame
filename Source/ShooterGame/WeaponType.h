#pragma once


UENUM(BlueprintType)
enum class EWeaponType :uint8
{
	EWT_SubmachineGun UMETA(DisplayName = "SubmachineGun"),
	EWT_SubmachineGun_Rare UMETA(DisplayName = "SubmachineGun_Rare"),
	EWT_AssaultRifle UMETA(DisplayName = "AssaultRifle"),
	EWT_Pistol UMETA(DisplayName = "Pistol"),
	EWT_Pistol_Uncommon UMETA(DisplayName = "Pistol_Uncommon"),
	EWT_MAX UMETA(DisplayName = "DefaultMAX")

};