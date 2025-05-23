#include "AnimInstance_FPS.h"

#include "privablic.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "AnimNodes/AnimNode_AimOffsetLookAt.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"


struct FAnimNode_BlendSpacePlayer_BlendFilter { typedef FBlendFilter(FAnimNode_BlendSpacePlayer::*type); };
template struct privablic::private_member<FAnimNode_BlendSpacePlayer_BlendFilter, &FAnimNode_BlendSpacePlayer::BlendFilter>;

FLocomotionStateMachineData UAnimInstance_FPS::GetLocomotionSMData()
{
	return GetProxyOnAnyThread<FAnimInstanceProxy_FPS>().GetLocomotionSMData();
}

FUpperBodyStateMachineData UAnimInstance_FPS::GetUpperBodySMData()
{
	return GetProxyOnAnyThread<FAnimInstanceProxy_FPS>().GetUpperBodySMData();
}

FAnimInstanceProxy* UAnimInstance_FPS::CreateAnimInstanceProxy()
{
	return new FAnimInstanceProxy_FPS(this);
}

FLocomotionStateMachineData FAnimInstanceProxy_FPS::GetLocomotionSMData()
{
	FLocomotionStateMachineData Data;
	
	int32 MachineIndex;
	const FBakedAnimationStateMachine* BakedAnimationStateMachine;
	GetStateMachineIndexAndDescription("Locomotion", MachineIndex, &BakedAnimationStateMachine);

	for(int StateIndex = 0; StateIndex < BakedAnimationStateMachine->States.Num(); StateIndex++)
	{
		const FBakedAnimationState& State = BakedAnimationStateMachine->States[StateIndex];

		if (State.PlayerNodeIndices.Num() > 0)
		{
			float StateWeight = GetInstanceStateWeight(MachineIndex, StateIndex);

			FAnimNode_AssetPlayerBase* PlayerNode = GetNodeFromIndex<FAnimNode_AssetPlayerBase>(State.PlayerNodeIndices[0]);
		
			float CurrentAssetTime = PlayerNode->GetCurrentAssetTimePlayRateAdjusted() / PlayerNode->GetCurrentAssetLength();

			if (State.StateName == "Idle/Run")
			{
				Data.IdleRunStateWeight = StateWeight;
				Data.IdleRunBlendSpaceTime = CurrentAssetTime;

				FAnimNode_BlendSpacePlayer* BlendSpaceNode = static_cast<FAnimNode_BlendSpacePlayer*>(PlayerNode);
				const FBlendFilter& BlendFilter = BlendSpaceNode->*privablic::member<FAnimNode_BlendSpacePlayer_BlendFilter>::value;

				Data.Speed = BlendFilter.FilterPerAxis[0].LastOutput;
			}
			else if (State.StateName == "JumpStart")
			{
				Data.JumpStartStateWeight = StateWeight;
				Data.JumpStartSequenceTime = CurrentAssetTime;
			}
			else if (State.StateName == "JumpLoop")
			{
				Data.JumpLoopStateWeight = StateWeight;
				Data.JumpLoopSequenceTime = CurrentAssetTime;
			}
			else if (State.StateName == "JumpEnd")
			{
				Data.JumpEndStateWeight = StateWeight;
				Data.JumpEndSequenceTime = CurrentAssetTime;
			}
		}
	}

	return Data;
}

FUpperBodyStateMachineData FAnimInstanceProxy_FPS::GetUpperBodySMData()
{
	FUpperBodyStateMachineData Data;
	
	int32 MachineIndex;
	const FBakedAnimationStateMachine* BakedAnimationStateMachine;
	GetStateMachineIndexAndDescription("UpperBody", MachineIndex, &BakedAnimationStateMachine);

	for(int StateIndex = 0; StateIndex < BakedAnimationStateMachine->States.Num(); StateIndex++)
	{
		const FBakedAnimationState& State = BakedAnimationStateMachine->States[StateIndex];

		float StateWeight = GetInstanceStateWeight(MachineIndex, StateIndex);

		if (State.StateName == "Aiming")
		{
			Data.AimingStateWeight = StateWeight;
			
			for(int AssetPlayerIndex : State.PlayerNodeIndices)
			{
				FAnimNode_AssetPlayerBase* PlayerNode = GetNodeFromIndex<FAnimNode_AssetPlayerBase>(AssetPlayerIndex);

				UAnimationAsset* AnimationAsset = PlayerNode->GetAnimAsset();

				if (UAimOffsetBlendSpace1D* AimOffsetAsset = Cast<UAimOffsetBlendSpace1D>(AnimationAsset))
				{
					Data.Pitch = static_cast<FAnimNode_AimOffsetLookAt*>(PlayerNode)->X;
				}
				else
				{						
					float CurrentAssetTime = PlayerNode->GetCurrentAssetTimePlayRateAdjusted() / PlayerNode->GetCurrentAssetLength();
						
					Data.IdleRifleSequenceTime = CurrentAssetTime;
				}
			}
		}
		else if (State.StateName == "Sprinting")
		{
			Data.SprintingStateWeight = StateWeight;
				
			FAnimNode_AssetPlayerBase* Node = GetNodeFromIndex<FAnimNode_AssetPlayerBase>(State.PlayerNodeIndices[0]);
		
			float NormalizedTime = Node->GetCurrentAssetTimePlayRateAdjusted() / Node->GetCurrentAssetLength();
								
			Data.SprintingSequenceTime = NormalizedTime;
		}
	}

	return Data;
}