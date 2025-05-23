#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"

#include "AnimInstance_FPS.generated.h"

struct FLocomotionStateMachineData
{
	float IdleRunStateWeight;
	float JumpStartStateWeight;
	float JumpLoopStateWeight;
	float JumpEndStateWeight;
	
	float IdleRunBlendSpaceTime;
	float Speed;
	
	float JumpStartSequenceTime;
	
	float JumpLoopSequenceTime;
	
	float JumpEndSequenceTime;
};

struct FUpperBodyStateMachineData
{
	float AimingStateWeight;
	float SprintingStateWeight;
	
	float Pitch;
	float IdleRifleSequenceTime;
	
	float SprintingSequenceTime;
};

UCLASS()
class FASTPACEDSHOOTER_API UAnimInstance_FPS : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	FLocomotionStateMachineData GetLocomotionSMData();

	FUpperBodyStateMachineData GetUpperBodySMData();

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

USTRUCT()
struct FASTPACEDSHOOTER_API FAnimInstanceProxy_FPS : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FAnimInstanceProxy_FPS()
		: FAnimInstanceProxy()
	{
	}

	FAnimInstanceProxy_FPS(UAnimInstance* Instance)
		: FAnimInstanceProxy(Instance)
	{
	}

	FLocomotionStateMachineData GetLocomotionSMData();

	FUpperBodyStateMachineData GetUpperBodySMData();
};