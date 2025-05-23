#include "GameSnapshot.h"

bool FPlayerSnapshot::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << InterpolationID;
	Ar << Position;
	Ar << Rotation;
	AnimData.Serialize(Ar);
	Ar.SerializeBits(&bShooting, 1);
	Ar.SerializeBits(&SelectedWeaponIndex, 1);

	return !Ar.IsError();
}

void FAnimData::Serialize(FArchive& Ar)
{
	SerializeBlendWeights(Ar, { &IdleRunStateWeight, &JumpStartStateWeight, &JumpLoopStateWeight, &JumpEndStateWeight });
	SerializeBlendAlpha(Ar, SwitchingSlotWeight);

	if (IdleRunStateWeight > 0.0f)
	{
		Ar << IdleRunBlendSpaceTime;
		Ar << Speed;
	}

	if (JumpStartStateWeight > 0.0f)
	{
		Ar << JumpStartSequenceTime;
	}

	if (JumpLoopStateWeight > 0.0f)
	{
		Ar << JumpLoopSequenceTime;
	}

	if (JumpEndStateWeight > 0.0f)
	{
		Ar << JumpEndSequenceTime;
	}

	if (SwitchingSlotWeight > 0.0f)
	{
		Ar << SwitchingSlotTime;
	}

	if (SwitchingSlotWeight < 1.0f)
	{
		SerializeBlendAlpha(Ar, ReloadSlotWeight);

		if (ReloadSlotWeight > 0.0f)
		{
			Ar << ReloadSlotTime;
		}

		if (ReloadSlotWeight < 1.0f)
		{
			SerializeBlendWeights(Ar, { &AimingStateWeight, &SprintingStateWeight });

			if (AimingStateWeight > 0.0f)
			{
				Ar << IdleRifleSequenceTime;
				Ar << Pitch;
			}

			if (SprintingStateWeight > 0.0f)
			{
				Ar << SprintingSequenceTime;
			}
		}
	}
}

void FAnimData::SerializeBlendAlpha(FArchive& Ar, float& Alpha)
{
	uint8 CompressedFlag = (Alpha == 0.0f) || (Alpha == 1.0f);
	Ar.SerializeBits(&CompressedFlag, 1);

	if (CompressedFlag)
	{
		uint8 CompressedValue = (Alpha == 1.0f);
		Ar.SerializeBits(&CompressedValue, 1);
		Alpha = CompressedValue;
	}
	else
	{
		Ar << Alpha;
	}
}

void FAnimData::SerializeBlendWeights(FArchive& Ar, TArray<float*> Weights)
{
	float* FirstNonZeroWeightPtr = nullptr;
		
	float RemainingWeights = 1.0f;

	for(float* WeightPtr : Weights)
	{
		float& Weight = *WeightPtr;
		
		uint8 NonZeroWeight = (Weight > 0.0f);
		Ar.SerializeBits(&NonZeroWeight, 1);

		if (NonZeroWeight)
		{
			if (FirstNonZeroWeightPtr == nullptr)
			{
				FirstNonZeroWeightPtr = &Weight;
			}
			else // Note: Value of first non-zero weight is not serialized.
			{
				uint8 FullWeight = (Weight == 1.0f);
				Ar.SerializeBits(&FullWeight, 1);

				if (FullWeight)
				{
					Weight = 1.0f;
					RemainingWeights = 0.0f;
				}
				else
				{
					Ar << Weight;
					RemainingWeights -= Weight;
				}
			}
		}
		else
		{
			Weight = 0.0f;
		}
	}

	if (Ar.IsLoading())
	{
		if (FirstNonZeroWeightPtr != nullptr)
		{
			*FirstNonZeroWeightPtr = FMath::Clamp(RemainingWeights, 0.0f, 1.0f);
		}
	}
}
