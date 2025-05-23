// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon_FPS.h"

// Sets default values
AWeapon_FPS::AWeapon_FPS()
{
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	FirstPersonTransform = CreateDefaultSubobject<USceneComponent>(TEXT("FirstPersonTransform"));
	FirstPersonTransform->SetupAttachment(RootSceneComponent);

	ThirdPersonTransform = CreateDefaultSubobject<USceneComponent>(TEXT("ThirdPersonTransform"));
	ThirdPersonTransform->SetupAttachment(RootSceneComponent);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWeapon_FPS::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MaxAmmo;
}

void AWeapon_FPS::SetToFirstPersonMode()
{
	WeaponMesh->SetCastShadow(false);
	WeaponMesh->AttachToComponent(FirstPersonTransform, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
}

void AWeapon_FPS::SetToThirdPersonMode()
{
	WeaponMesh->SetCastShadow(true);
	WeaponMesh->AttachToComponent(ThirdPersonTransform, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
}

