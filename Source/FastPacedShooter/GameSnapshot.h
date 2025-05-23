#pragma once
#include "FastPacedShooter.h"

#include "GameSnapshot.generated.h"

USTRUCT(BlueprintType)
struct FAnimData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float IdleRunStateWeight;	
	UPROPERTY(BlueprintReadOnly) float JumpStartStateWeight;
	UPROPERTY(BlueprintReadOnly) float JumpLoopStateWeight;
	UPROPERTY(BlueprintReadOnly) float JumpEndStateWeight;
	
	UPROPERTY(BlueprintReadOnly) float IdleRunBlendSpaceTime;
	UPROPERTY(BlueprintReadOnly) float Speed;

	UPROPERTY(BlueprintReadOnly) float JumpStartSequenceTime;

	UPROPERTY(BlueprintReadOnly) float JumpLoopSequenceTime;

	UPROPERTY(BlueprintReadOnly) float JumpEndSequenceTime;

	UPROPERTY(BlueprintReadOnly) float SwitchingSlotWeight;
	UPROPERTY(BlueprintReadOnly) float SwitchingSlotTime;
	
	UPROPERTY(BlueprintReadOnly) float ReloadSlotWeight;
	UPROPERTY(BlueprintReadOnly) float ReloadSlotTime;
	
	UPROPERTY(BlueprintReadOnly) float AimingStateWeight;
	UPROPERTY(BlueprintReadOnly) float SprintingStateWeight;
	
	UPROPERTY(BlueprintReadOnly) float Pitch;
	UPROPERTY(BlueprintReadOnly) float IdleRifleSequenceTime;

	UPROPERTY(BlueprintReadOnly) float SprintingSequenceTime;

	void Serialize(FArchive& Ar);

	static void SerializeBlendAlpha(FArchive& Ar, float& Alpha);

	static void SerializeBlendWeights(FArchive& Ar, TArray<float*> Weights);
};

USTRUCT()
struct FPlayerSnapshot
{
	GENERATED_BODY()
	
	uint8 InterpolationID;
	FVector Position;
	FRotator Rotation;
	FAnimData AnimData;
	bool bShooting;
	uint8 SelectedWeaponIndex;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FPlayerSnapshot> : public TStructOpsTypeTraitsBase2<FPlayerSnapshot>
{
	enum
	{
		WithNetSerializer = true,
	};
};

USTRUCT()
struct FGameSnapshot
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 ServerTick;

	UPROPERTY()
	TArray<FPlayerSnapshot> PlayerSnapshots;

	//bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};