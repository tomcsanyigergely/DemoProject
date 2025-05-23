// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon_FPS.generated.h"

UENUM(BlueprintType)
enum EAimType
{	
	AIM_None	UMETA(Hidden),
	AIM_ADS		UMETA(DisplayName="ADS"),
	AIM_SCOPE	UMETA(DisplayName="Scope"),
	AIM_MAX		UMETA(Hidden),
};

UCLASS(Blueprintable)
class FASTPACEDSHOOTER_API AWeapon_FPS : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USceneComponent* FirstPersonTransform;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USceneComponent* ThirdPersonTransform;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	bool bAutomatic;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TEnumAsByte<EAimType> AimType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 MaxAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float FireInterval;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float ReloadTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USoundCue* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USoundCue* RemoteFireSound;	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class UAnimMontage* FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class UTexture* WeaponImage;

	uint8 CurrentAmmo = 0;
	
public:	
	// Sets default values for this actor's properties
	AWeapon_FPS();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	FORCEINLINE uint8 GetCurrentAmmo() const { return CurrentAmmo; }
	FORCEINLINE void DecrementAmmo() { if (CurrentAmmo > 0) CurrentAmmo--; }
	FORCEINLINE void RestoreAmmo() { CurrentAmmo = MaxAmmo; }
	FORCEINLINE void SetAmmo(uint8 NewAmmo) { CurrentAmmo = FMath::Clamp(NewAmmo, (uint8)0, MaxAmmo); }
	FORCEINLINE uint8 GetMaxAmmo() const { return MaxAmmo; }
	FORCEINLINE float GetReloadTime() const { return ReloadTime; }
	FORCEINLINE float GetFireInterval() const { return FireInterval; }
	FORCEINLINE float GetBaseDamage() const { return BaseDamage; }
	FORCEINLINE UTexture* GetWeaponImage() const { return WeaponImage; }
	FORCEINLINE EAimType GetAimType() const { return AimType; }
	FORCEINLINE bool IsAutomatic() const { return bAutomatic; }

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	void SetToFirstPersonMode();
	void SetToThirdPersonMode();
};
