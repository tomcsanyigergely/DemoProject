// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterMovementComponent_FPS.h"

#include "Character_FPS.h"
#include "CollisionChannels.h"
#include "DrawDebugHelpers.h"
#include "GameState_FPS.h"
#include "PlayerController_FPS.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

DECLARE_CYCLE_STAT(TEXT("Char Update Acceleration"), STAT_CharUpdateAcceleration, STATGROUP_Character);

FNetworkPredictionData_Client_FPS::FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClientMovement)
: Super(ClientMovement)
{
}

FSavedMovePtr FNetworkPredictionData_Client_FPS::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_FPS());
}

void FCharacterNetworkMoveData_FPS::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	Super::ClientFillNetworkMoveData(ClientMove, MoveType);

	const FSavedMove_FPS& ClientMove_FPS = static_cast<const FSavedMove_FPS&>(ClientMove);

	InterpolationTime = ClientMove_FPS.InterpolationTime;
	
	CurrentAmmo = ClientMove_FPS.SavedCurrentAmmo;
	bIsReloading = ClientMove_FPS.SavedIsReloading;
	bIsSwitching = ClientMove_FPS.SavedIsSwitching;
	SelectedWeaponIndex = ClientMove_FPS.SavedSelectedWeaponIndex;
	bForceShooting = ClientMove_FPS.SavedForceShooting;
	LastHitInterpolationID = ClientMove_FPS.LastHitInterpolationID;
	LastHitBoneIndex = ClientMove_FPS.LastHitBoneIndex;
}

bool FCharacterNetworkMoveData_FPS::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar,	UPackageMap* PackageMap, ENetworkMoveType MoveType)
{
	Super::Serialize(CharacterMovement, Ar, PackageMap, MoveType);

	Ar << InterpolationTime;

	Ar.SerializeBits(&CurrentAmmo, 5);
	Ar.SerializeBits(&bIsReloading, 1);
	Ar.SerializeBits(&bIsSwitching, 1);
	Ar.SerializeBits(&SelectedWeaponIndex, 1);
	Ar.SerializeBits(&bForceShooting, 1);

	Ar << LastHitInterpolationID;
	if (LastHitInterpolationID != 0)
	{
		Ar << LastHitBoneIndex;
	}
	
	return !Ar.IsError();
}

FCharacterNetworkMoveDataContainer_FPS::FCharacterNetworkMoveDataContainer_FPS()
{
	NewMoveData = &MoveData[0];
	PendingMoveData = &MoveData[1];
	OldMoveData = &MoveData[2];
}

bool UCharacterMovementComponent_FPS::ServerCheckClientError(float ClientTimeStamp, float DeltaTime,
	const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation,
	UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode)
{
	if (Super::ServerCheckClientError(ClientTimeStamp, DeltaTime, Accel, ClientWorldLocation, RelativeClientLocation, ClientMovementBase, ClientBaseBoneName, ClientMovementMode))
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: default."))
		return true;
	}

	const FCharacterNetworkMoveData_FPS* MoveData = static_cast<const FCharacterNetworkMoveData_FPS*>(GetCurrentNetworkMoveData());
	
	if (GetCurrentAmmo() != MoveData->CurrentAmmo)
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: GetCurrentAmmo(): %d, MoveData->CurrentAmmo: %d"), GetCurrentAmmo(), MoveData->CurrentAmmo)
		return true;
	}

	if (bIsReloading != MoveData->bIsReloading)
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: bIsReloading: %d, MoveData->bIsReloading: %d"), bIsReloading, MoveData->bIsReloading)
		return true;
	}

	if (bIsSwitching != MoveData->bIsSwitching)
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: bIsSwitching: %d, MoveData->bIsSwitching: %d"), bIsSwitching, MoveData->bIsSwitching)
		return true;
	}

	if (GetSelectedWeaponIndex() != MoveData->SelectedWeaponIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: GetSelectedWeaponIndex(): %d, MoveData->SelectedWeaponIndex: %d"), GetSelectedWeaponIndex(), MoveData->SelectedWeaponIndex)
		return true;
	}

	if (bForceShooting != MoveData->bForceShooting)
	{
		UE_LOG(LogTemp, Warning, TEXT("Correction: bForceShooting: %d, MoveData->bForceShooting: %d"), bForceShooting, MoveData->bForceShooting)
		return true;
	}
	
	return false;
}

void FCharacterMoveResponseDataContainer_FPS::ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment)
{
	Super::ServerFillResponseData(CharacterMovement, PendingAdjustment);

	const UCharacterMovementComponent_FPS* CharacterMovementComponent_FPS = Cast<UCharacterMovementComponent_FPS>(&CharacterMovement);
	CurrentAmmo = CharacterMovementComponent_FPS->GetCurrentAmmo();
	ShootCooldownTime = CharacterMovementComponent_FPS->GetShootCooldownTime();
	bIsReloading = CharacterMovementComponent_FPS->IsReloading();
	ReloadTimeRemaining = CharacterMovementComponent_FPS->GetReloadTimeRemaining();
	bIsSwitching = CharacterMovementComponent_FPS->IsSwitching();
	SwitchingTimeRemaining = CharacterMovementComponent_FPS->GetSwitchingTimeRemaining();
	SelectedWeaponIndex = CharacterMovementComponent_FPS->GetSelectedWeaponIndex();
	SprintingAlpha = CharacterMovementComponent_FPS->GetSprintingAlpha();
	bForceShooting = CharacterMovementComponent_FPS->GetForceShooting();
}

bool FCharacterMoveResponseDataContainer_FPS::Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap)
{
	FCharacterMoveResponseDataContainer::Serialize(CharacterMovement, Ar, PackageMap);

	if (IsCorrection())
	{
		Ar << CurrentAmmo;
		Ar << ShootCooldownTime;
		
		Ar.SerializeBits(&bIsReloading, 1);
		if (bIsReloading)
		{
			Ar << ReloadTimeRemaining;
		}

		Ar.SerializeBits(&bIsSwitching, 1);
		if (bIsSwitching)
		{
			Ar << SwitchingTimeRemaining;
		}

		Ar.SerializeBits(&SelectedWeaponIndex, 1);

		SerializeOptionalValue<float>(Ar.IsSaving(), Ar, SprintingAlpha, 0.0f);

		Ar.SerializeBits(&bForceShooting, 1);
	}
	
	return !Ar.IsError();
}

void UCharacterMovementComponent_FPS::ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_CharUpdateAcceleration);

		// We need to check the jump state before adjusting input acceleration, to minimize latency
		// and to make sure acceleration respects our potentially new falling state.
		CharacterOwner->CheckJumpInput(DeltaSeconds);

		// apply input to acceleration
		Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(InputVector.RotateAngleAxis(CharacterOwner->GetControlRotation().Yaw, FVector::UpVector)));
		AnalogInputModifier = ComputeAnalogInputModifier();
	}

	if (CharacterOwner->GetLocalRole() == ROLE_Authority)
	{
		PerformMovement(DeltaSeconds);
	}
	else if (CharacterOwner->GetLocalRole() == ROLE_AutonomousProxy && IsNetMode(NM_Client))
	{
		ReplicateMoveToServer(DeltaSeconds, Acceleration);
	}
}

void UCharacterMovementComponent_FPS::ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse)
{
	if (!MoveResponse.IsGoodMove())
	{
		const FCharacterMoveResponseDataContainer_FPS& MoveResponseDataContainer_FPS = static_cast<const FCharacterMoveResponseDataContainer_FPS&>(MoveResponse);
		
		CharacterOwner_FPS->SetSelectedWeapon(MoveResponseDataContainer_FPS.SelectedWeaponIndex);
		CharacterOwner_FPS->SetWeaponAmmo(MoveResponseDataContainer_FPS.CurrentAmmo);
		ShootCooldownTime = MoveResponseDataContainer_FPS.ShootCooldownTime;
		bIsReloading = MoveResponseDataContainer_FPS.bIsReloading;
		ReloadTimeRemaining = MoveResponseDataContainer_FPS.ReloadTimeRemaining;
		bIsSwitching = MoveResponseDataContainer_FPS.bIsSwitching;
		SwitchingTimeRemaining = MoveResponseDataContainer_FPS.SwitchingTimeRemaining;
		SprintingAlpha = MoveResponseDataContainer_FPS.SprintingAlpha;
		bForceShooting = MoveResponseDataContainer_FPS.bForceShooting;
	}
	
	Super::ClientHandleMoveResponse(MoveResponse);
}

void FSavedMove_FPS::Clear()
{
	Super::Clear();

	bWantsToSprint = 0;
	bHoldingJump = 0;
	bWantsToShoot = 0;

	bPrevHoldingJump = 0;
	ControlRotation = FRotator::ZeroRotator;	
	bWantsToReload = 0;
	bWantsToSwitch = 0;

	InterpolationTime = 0;

	StartForceShooting = 0;
	StartSelectedWeaponIndex = 0;
	StartShootCooldownTime = 0;
	StartSprintingAlpha = 0;
	StartIsReloading = 0;
	StartReloadTimeRemaining = 0;
	StartIsSwitching = 0;
	StartSwitchingTimeRemaining = 0;
	
	SavedIsReloading = 0;
	SavedIsSwitching = 0;
	SavedSelectedWeaponIndex = 0;
	SavedCurrentAmmo = 0;
	SavedForceShooting = 0;
}

uint8 FSavedMove_FPS::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();

	if (bWantsToSprint) Result |= FLAG_WantsToSprint;
	if (bHoldingJump) Result |= FLAG_HoldingJump;
	if (bWantsToShoot) Result |= FLAG_WantsToShoot;

	return Result;
}

void FSavedMove_FPS::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UCharacterMovementComponent_FPS* CharacterMovement = Cast<UCharacterMovementComponent_FPS>(C->GetCharacterMovement());

	InterpolationTime = C->GetWorld()->GetGameState<AGameState_FPS>()->GetLastInterpolationTime();

	ControlRotation = C->GetController()->GetControlRotation();

	bWantsToReload = CharacterMovement->bWantsToReload;
	bWantsToSwitch = CharacterMovement->bWantsToSwitch;

	//UE_LOG(LogTemp, Warning, TEXT("SAVING MOVE (t=%f), MovementMode: %d, CustomMovementMode: %d"), TimeStamp, C->GetCharacterMovement()->MovementMode.GetValue(), C->GetCharacterMovement()->CustomMovementMode)
}

void FSavedMove_FPS::SetInitialPosition(ACharacter* C)
{
	FSavedMove_Character::SetInitialPosition(C);

	if (const UCharacterMovementComponent_FPS* MoveComp = C ? Cast<UCharacterMovementComponent_FPS>(C->GetCharacterMovement()) : nullptr)
	{
		bWantsToSprint = MoveComp->bWantsToSprint;
		bHoldingJump = MoveComp->bHoldingJump;
		bWantsToShoot = MoveComp->bWantsToShoot;

		bPrevHoldingJump = MoveComp->bPrevHoldingJump;

		StartForceShooting = MoveComp->bForceShooting;
		StartSelectedWeaponIndex = MoveComp->GetSelectedWeaponIndex();
		StartShootCooldownTime = MoveComp->GetShootCooldownTime();
		StartSprintingAlpha = MoveComp->GetSprintingAlpha();
		StartIsReloading = MoveComp->IsReloading();
		StartReloadTimeRemaining = MoveComp->GetReloadTimeRemaining();
		StartIsSwitching = MoveComp->IsSwitching();
		StartSwitchingTimeRemaining = MoveComp->GetSwitchingTimeRemaining();
	}
}

void FSavedMove_FPS::PostUpdate(ACharacter* C, EPostUpdateMode PostUpdateMode)
{
	FSavedMove_Character::PostUpdate(C, PostUpdateMode);

	if (UCharacterMovementComponent_FPS* MovementComponent = C ? Cast<UCharacterMovementComponent_FPS>(C->GetCharacterMovement()) : nullptr)
	{
		SavedIsReloading = MovementComponent->IsReloading();
		SavedIsSwitching = MovementComponent->IsSwitching();
		SavedSelectedWeaponIndex = MovementComponent->GetSelectedWeaponIndex();
		SavedCurrentAmmo = MovementComponent->GetCurrentAmmo();
		SavedForceShooting = MovementComponent->bForceShooting;
		LastHitInterpolationID = MovementComponent->LastHitInterpolationID;
		LastHitBoneIndex = MovementComponent->LastHitBoneIndex;
	}
}

void FSavedMove_FPS::PrepMoveFor(ACharacter* Character)
{
	Character->GetCharacterMovement()->bForceMaxAccel = bForceMaxAccel;
	Character->bWasJumping = bWasJumping;
	Character->JumpKeyHoldTime = JumpKeyHoldTime;
	Character->JumpForceTimeRemaining = JumpForceTimeRemaining;
	Character->JumpMaxCount = JumpMaxCount;
	Character->JumpCurrentCount = JumpCurrentCount;
	Character->JumpCurrentCountPreJump = JumpCurrentCount;

	StartPackedMovementMode = Character->GetCharacterMovement()->PackNetworkMovementMode();

	UCharacterMovementComponent_FPS* CharacterMovement = Cast<UCharacterMovementComponent_FPS>(Character->GetCharacterMovement());

	CharacterMovement->bWantsToReload = bWantsToReload;
	CharacterMovement->bWantsToSwitch = bWantsToSwitch;

	CharacterMovement->bPrevHoldingJump = bPrevHoldingJump;

	Character->GetController()->SetControlRotation(ControlRotation);

	//UE_LOG(LogTemp, Error, TEXT("REPLAYING MOVE (t=%f), MovementMode: %d, CustomMovementMode: %d"), TimeStamp, Character->GetCharacterMovement()->MovementMode.GetValue(), Character->GetCharacterMovement()->CustomMovementMode)
}

bool FSavedMove_FPS::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	FSavedMove_FPS* NewMove_FPS = static_cast<FSavedMove_FPS*>(NewMove.Get());

	if (bWantsToSprint != NewMove_FPS->bWantsToSprint)
	{
		return false;
	}

	if (bHoldingJump != NewMove_FPS->bHoldingJump)
	{
		return false;
	}

	if (bWantsToShoot || NewMove_FPS->bWantsToShoot || StartForceShooting || NewMove_FPS->StartForceShooting)
	{
		return false;
	}

	if (bWantsToReload != NewMove_FPS->bWantsToReload)
	{
		return false;
	}

	if (bWantsToSwitch != NewMove_FPS->bWantsToSwitch)
	{
		return false;
	}

	if (StartSelectedWeaponIndex != NewMove_FPS->StartSelectedWeaponIndex)
	{
		return false;
	}
	
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void FSavedMove_FPS::CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation)
{
	FSavedMove_Character::CombineWith(OldMove, InCharacter, PC, OldStartLocation);

	const FSavedMove_FPS* SavedOldMove = static_cast<const FSavedMove_FPS*>(OldMove);

	if (UCharacterMovementComponent_FPS* MoveComp = InCharacter ? Cast<UCharacterMovementComponent_FPS>(InCharacter->GetCharacterMovement()) : nullptr)
	{		
		MoveComp->bPrevHoldingJump = (SavedOldMove->bPrevHoldingJump);
			
		MoveComp->SetShootCooldownTime(SavedOldMove->StartShootCooldownTime);
		MoveComp->SprintingAlpha = SavedOldMove->StartSprintingAlpha;
		MoveComp->bIsReloading = (SavedOldMove->StartIsReloading);
		MoveComp->ReloadTimeRemaining = SavedOldMove->StartReloadTimeRemaining;
		MoveComp->bIsSwitching = SavedOldMove->StartIsSwitching;
		MoveComp->SwitchingTimeRemaining = SavedOldMove->StartSwitchingTimeRemaining;
	}
}

void UCharacterMovementComponent_FPS::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	bWantsToSprint = (Flags & FSavedMove_FPS::CompressedFlags::FLAG_WantsToSprint) != 0;
	bHoldingJump = (Flags & FSavedMove_FPS::CompressedFlags::FLAG_HoldingJump) != 0;
	bWantsToShoot = (Flags & FSavedMove_FPS::CompressedFlags::FLAG_WantsToShoot) != 0;
}

bool UCharacterMovementComponent_FPS::CanShoot() const
{
	return !bIsReloading && !bIsSwitching && GetCurrentAmmo() > 0;
}

void UCharacterMovementComponent_FPS::StartShooting()
{
	bWantsToShoot = true;
}

void UCharacterMovementComponent_FPS::StopShooting()
{
	bWantsToShoot = false;
}

UCharacterMovementComponent_FPS::UCharacterMovementComponent_FPS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetNetworkMoveDataContainer(MoveDataContainer);
	SetMoveResponseDataContainer(MoveResponseDataContainer);
}

void UCharacterMovementComponent_FPS::InitializeComponent()
{
	Super::InitializeComponent();

	CharacterOwner_FPS = Cast<ACharacter_FPS>(GetOwner());
}

void UCharacterMovementComponent_FPS::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(GetWorld()->GetGameState()))
	{
		GameState = GetWorld()->GetGameState<AGameState_FPS>();
	}
	else
	{
		UE_LOG(LogTemp, Fatal, TEXT("Error: GameState not available."))
	}
}

FNetworkPredictionData_Client* UCharacterMovementComponent_FPS::GetPredictionData_Client() const
{
	check(PawnOwner != nullptr)

	if (ClientPredictionData == nullptr)
	{
		UCharacterMovementComponent_FPS* MutableThis = const_cast<UCharacterMovementComponent_FPS*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_FPS(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
		MutableThis->ClientPredictionData->MaxSavedMoveCount = 300;
	} 
	
	return ClientPredictionData;
}

float UCharacterMovementComponent_FPS::GetMaxSpeed() const
{
	if (IsWalking())
	{
		return IsSprinting() ? Sprint_MaxWalkSpeed : Walk_MaxWalkSpeed;
	}
	else
	{
		return Super::GetMaxSpeed();
	}
}

void UCharacterMovementComponent_FPS::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	//UE_LOG(LogTemp, Warning, TEXT("CMC::Tick() begin"))

	//UE_LOG(LogTemp, Warning, TEXT("CMC::Tick() before super"))
	//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::White, FString::Printf(TEXT("Tick start. CurrentAmmo: %d, CooldownTime: %f"), CurrentAmmo, ShootCooldownTime));
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::White, FString::Printf(TEXT("Tick end. CurrentAmmo: %d, CooldownTime: %f"), CurrentAmmo, ShootCooldownTime));
	//UE_LOG(LogTemp, Warning, TEXT("CMC::Tick() after super"))
	
	if (GetWorld()->IsClient())
	{
		if (!bWasReloading && bIsReloading)
		{
			OnReloadStarted.Broadcast();
		}
		else if (bWasReloading && !bIsReloading)
		{
			OnReloadFinished.Broadcast();
		}

		bWasReloading = bIsReloading;

		if (!bWasSwitching && bIsSwitching)
		{
			OnSwitchingStarted.Broadcast();
		}
		else if (bWasSwitching && !bIsSwitching)
		{
			OnSwitchingFinished.Broadcast();
		}

		bWasSwitching = bIsSwitching;

		if (bIsSwitching && SwitchingTimeRemaining >= FirstPersonArmTransitionDuration)
		{
			FirstPersonArmOffsetAlpha = FMath::Clamp(FirstPersonArmOffsetAlpha + DeltaTime / FirstPersonArmTransitionDuration, 0.0f, 1.0f);
		}
		else
		{
			FirstPersonArmOffsetAlpha = FMath::Clamp(FirstPersonArmOffsetAlpha - DeltaTime / FirstPersonArmTransitionDuration, 0.0f, 1.0f);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("CMC::Tick() end"))
}

bool UCharacterMovementComponent_FPS::ClientUpdatePositionAfterServerUpdate()
{	
	const bool bReal_WantsToSprint = bWantsToSprint;
	const bool bReal_HoldingJump = bHoldingJump;
	const bool bReal_PrevHoldingJump = bPrevHoldingJump;
	const bool bReal_WantsToShoot = bWantsToShoot;
	const FRotator Real_ControlRotation = CharacterOwner->GetController()->GetControlRotation();
	const bool Real_bWantsToReload = bWantsToReload;
	const bool Real_bWantsToSwitch = bWantsToSwitch;
	
	const bool bResult = Super::ClientUpdatePositionAfterServerUpdate();

	bWantsToSwitch = Real_bWantsToSwitch;
	bWantsToReload = Real_bWantsToReload;
	CharacterOwner->GetController()->SetControlRotation(Real_ControlRotation);
	bWantsToSprint = bReal_WantsToSprint;
	bHoldingJump = bReal_HoldingJump;
	bPrevHoldingJump = bReal_PrevHoldingJump;
	bWantsToShoot = bReal_WantsToShoot;

	OnAmmoChanged.Broadcast();

	return bResult;
}

void UCharacterMovementComponent_FPS::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (bReliableShoot)
	{
		if (GetWorld()->IsServer())
		{
			FCharacterNetworkMoveData_FPS* MoveData = static_cast<FCharacterNetworkMoveData_FPS*>(GetCurrentNetworkMoveData());

			if (MoveData != nullptr && MoveData->InterpolationTime >= ReliableShootInterpolationTime)
			{
				bReliableShoot = false;
				bWantsToShoot = true;
			}
		}
	}
	
	bAcceleratingForward = (FRotator(0, CharacterOwner->GetControlRotation().Yaw, 0).Quaternion().GetForwardVector() | GetCurrentAcceleration().GetSafeNormal2D()) >= 0.01f;

	float PrevSprintingAlpha = SprintingAlpha;

	if (IsSprinting())
	{
		SprintingAlpha = FMath::Clamp(SprintingAlpha + DeltaSeconds / SprintTransitionDuration, 0.0f, 1.0f);
	}
	else
	{
		SprintingAlpha = FMath::Clamp(SprintingAlpha - DeltaSeconds / SprintTransitionDuration, 0.0f, 1.0f);
	}

	if (PrevSprintingAlpha == 0.0f && SprintingAlpha > 0)
	{
		OnSprintStarted.Broadcast();
	}
	else if (PrevSprintingAlpha > 0 && SprintingAlpha == 0)
	{
		OnSprintFinished.Broadcast();
	}
	
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		if (!bPrevHoldingJump && bHoldingJump)
		{
			if (IsMovingOnGround())
			{
				Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);

				SetMovementMode(MOVE_Falling);
			}
		}

		bPrevHoldingJump = bHoldingJump;
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);

	if (bWantsToSwitch)
	{
		if (!CharacterOwner->HasAuthority() && !CharacterOwner->bClientUpdating) { SwitchWeaponRPC(); }

		if (!bIsSwitching && !((bWantsToShoot || bForceShooting) && CanShoot()))
		{
			bIsSwitching = true;
			SwitchingTimeRemaining = WeaponSwitchingTime;

			if (!CharacterOwner->bClientUpdating)
			{
				OnSwitchingStarted.Broadcast();
			}

			if (bIsReloading)
			{
				bIsReloading = false;
				OnReloadFinished.Broadcast();
			}			
		}
	}
	
	bWantsToSwitch = false;

	if (bWantsToReload)
	{
		if (!CharacterOwner->HasAuthority() && !CharacterOwner->bClientUpdating) { ReloadWeaponRPC(); }

		if (!bIsReloading && !bIsSwitching && GetCurrentAmmo() < GetMaxAmmo() && !((bWantsToShoot || bForceShooting) && GetCurrentAmmo() > 0))
		{
			bIsReloading = true;
			ReloadTimeRemaining = GetWeaponReloadTime();

			if (!CharacterOwner->bClientUpdating)
			{
				OnReloadStarted.Broadcast();
			}
		}
	}
	
	bWantsToReload = false;	

	//UE_LOG(LogTemp, Warning, TEXT("CMC::UpdateCharacterStateBeforeMovement() end"))
}

void UCharacterMovementComponent_FPS::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{	
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	if (bIsReloading)
	{
		float PrevReloadTimeRemaining = ReloadTimeRemaining;
		
		ReloadTimeRemaining -= DeltaSeconds;

		if (PrevReloadTimeRemaining > 0.5f && ReloadTimeRemaining <= 0.5f)
		{			
			RestoreWeaponAmmo();

			if (!CharacterOwner->bClientUpdating)
			{
				OnAmmoChanged.Broadcast();
			}
		}

		if (ReloadTimeRemaining <= 0.0f)
		{
			bIsReloading = false;

			if (!CharacterOwner->bClientUpdating)
			{
				OnReloadFinished.Broadcast();
			}
			
			//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::White, FString::Printf(TEXT("Reload finished")));
		}
	}

	if (bIsSwitching)
	{
		float PrevSwitchingTimeRemaining = SwitchingTimeRemaining;
		
		SwitchingTimeRemaining -= DeltaSeconds;

		if (PrevSwitchingTimeRemaining > WeaponSwitchingTime / 2.0f && SwitchingTimeRemaining <= WeaponSwitchingTime / 2.0f)
		{
			CharacterOwner_FPS->SetSelectedWeapon(1 - CharacterOwner_FPS->GetSelectedWeaponIndex());
		}

		if (SwitchingTimeRemaining <= 0.0f)
		{
			bIsSwitching = false;

			if (!CharacterOwner->bClientUpdating)
			{
				OnSwitchingFinished.Broadcast();
			}
		}
	}

	bool bShoot = false;

	if ((bWantsToShoot || bForceShooting) && ShootCooldownTime <= DeltaSeconds && CanShoot())
	{
		if (GetWorld()->IsClient() && !IsWeaponAutomatic() && bWantsToShoot && !bForceShooting)
		{
			bWantsToShoot = false;
			
			if (!CharacterOwner->bClientUpdating)
			{
				CharacterOwner_FPS->ReliableShootRPC(GameState->GetLastInterpolationTime());
			}
		}
		
		if (SprintingAlpha == 0.0f)
		{		
			bShoot = true;
			bForceShooting = false;
			ShootCooldownTime = FMath::Max(GetWeaponFireInterval() - (DeltaSeconds - ShootCooldownTime), 0.0f);
			DecrementWeaponAmmo();
		}
		else
		{
			bForceShooting = true;
			ShootCooldownTime = FMath::Max(ShootCooldownTime - DeltaSeconds, 0.0f);
		}		
	}
	else
	{
		ShootCooldownTime = FMath::Max(ShootCooldownTime - DeltaSeconds, 0.0f);
	}

	if (!CharacterOwner->bClientUpdating)
	{		
		if (bShoot)
		{
			if (GetWorld()->IsClient())
			{
				OnShootDelegate.Broadcast();
				OnAmmoChanged.Broadcast();

				FVector ForwardVector = CharacterOwner->GetControlRotation().Quaternion().GetForwardVector();
				
				FVector StartLocation = CharacterOwner->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
				FVector EndLocation = StartLocation + 10000.0f * ForwardVector;

				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(CharacterOwner);
				
				TArray<FHitResult> HitResults;

				GetWorld()->LineTraceMultiByChannel(HitResults, StartLocation, EndLocation, ECC_GameTraceChannel_WeaponTrace, QueryParams);

				bool bHitPlayer = false;
				bool bHeadshot = false;

				LastHitInterpolationID = 0;
				LastHitBoneIndex = 0;

				ACharacter_FPS* DamagedCharacter;
				FName DamagedBoneName;
				if (HitResults.Num() > 0 && CharacterOwner_FPS->FindDamagedBone(GameState->GetInterpolatedCharacters(), HitResults, StartLocation, DamagedCharacter, DamagedBoneName))
				{					
					float Damage = GetWeaponBaseDamage() * GameState->GetBoneDamageMultiplier(DamagedBoneName);

					if (IsValid(DamagedCharacter->GetPlayerState()))
					{
						uint8 BoneIndex = GameState->GetBoneIndex(DamagedBoneName);
						bHitPlayer = true;
						bHeadshot = GameState->IsHeadBoneIndex(BoneIndex);
						GEngine->AddOnScreenDebugMessage(-1, 10.0f, bHeadshot ? FColor::Orange : FColor::Yellow, FString::Printf(TEXT("[Client] Hit: %s, damage: %.0f, bone: %s, time: %.3f"), *DamagedCharacter->GetPlayerState()->GetPlayerName(), Damage, *DamagedBoneName.ToString(), GameState->GetLastInterpolationTime()));
						LastHitInterpolationID = DamagedCharacter->GetInterpolationID();
						LastHitBoneIndex = BoneIndex;
					}
				}

				if (APlayerController_FPS* PlayerController = CharacterOwner->GetController<APlayerController_FPS>())
				{
					if (PlayerController->bHitscanDebugInfoActive)
					{
						DrawDebugLine(GetWorld(), StartLocation, EndLocation, bHitPlayer ? (bHeadshot ? FColor::Orange : FColor::Yellow) : FColor::Blue, true);
						
						for(APlayerState* PlayerArrayElement : GetWorld()->GetGameState()->PlayerArray)
						{
							if (ACharacter_FPS* Character = PlayerArrayElement->GetPawn<ACharacter_FPS>())
							{
								if (Character != CharacterOwner && !Character->IsKilled())
								{
									for(USkeletalBodySetup* SkeletalBodySetup : Character->GetLagCompMesh()->GetPhysicsAsset()->SkeletalBodySetups)
									{
										FTransform BoneTransform = Character->GetLagCompMesh()->GetBoneTransform(Character->GetLagCompMesh()->GetBoneIndex(SkeletalBodySetup->BoneName));
					
										const FKAggregateGeom& AggGeom = SkeletalBodySetup->AggGeom;

										for(const FKBoxElem& BoxElem : AggGeom.BoxElems)
										{
											DrawDebugBox(GetWorld(), BoneTransform.TransformPosition(BoxElem.Center), FVector(BoxElem.X/2.0, BoxElem.Y/2.0, BoxElem.Z/2.0), BoneTransform.TransformRotation(BoxElem.Rotation.Quaternion()), FColor::Blue, true);
										}

										for(const FKSphylElem& SphylElem : AggGeom.SphylElems)
										{
											DrawDebugCapsule(GetWorld(), BoneTransform.TransformPosition(SphylElem.Center), SphylElem.GetScaledHalfLength(FVector(1.0f, 1.0f, 1.0f)), SphylElem.Radius, BoneTransform.TransformRotation(SphylElem.Rotation.Quaternion()), FColor::Blue, true);
										}
									}
								}
							}		
						}
					}
				}
			}
			else if (GetWorld()->IsServer())
			{				
				FCharacterNetworkMoveData_FPS* MoveData = static_cast<FCharacterNetworkMoveData_FPS*>(GetCurrentNetworkMoveData());

				if (MoveData != nullptr)
				{
					LastHitInterpolationID = MoveData->LastHitInterpolationID;
					LastHitBoneIndex = MoveData->LastHitBoneIndex;
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Missing MoveData!!!"))
				}
			
				GenerateShootCommand();
			}
		}
	}
}

bool UCharacterMovementComponent_FPS::IsSprinting() const
{
	return IsMovingOnGround() && bWantsToSprint && bAcceleratingForward && !((bWantsToShoot || bForceShooting) && CanShoot());
}

void UCharacterMovementComponent_FPS::HandleReliableShoot(float InterpolationTime)
{
	if (GetWorld()->IsServer())
	{
		if (!IsWeaponAutomatic())
		{
			bReliableShoot = true;
			ReliableShootInterpolationTime = InterpolationTime;
		}
	}
}

void UCharacterMovementComponent_FPS::StartSprinting()
{
	bWantsToSprint = true;
}

void UCharacterMovementComponent_FPS::EndSprinting()
{
	bWantsToSprint = false;
}

void UCharacterMovementComponent_FPS::JumpPressed()
{
	bHoldingJump = true;
}

void UCharacterMovementComponent_FPS::JumpReleased()
{
	bHoldingJump = false;
}

void UCharacterMovementComponent_FPS::FirePressed()
{
	bFirePressed = true;
	StartShooting();
}

void UCharacterMovementComponent_FPS::FireReleased()
{
	bFirePressed = false;

	if (bPendingFirstQuantizationEvent || !CanShoot() || !IsWeaponAutomatic() || SprintingAlpha > 0)
	{
		StopShooting();
	}
}

void UCharacterMovementComponent_FPS::ReloadPressed()
{
	bWantsToReload = true;
}

void UCharacterMovementComponent_FPS::SwitchWeaponPressed()
{
	bWantsToSwitch = true;
}

void UCharacterMovementComponent_FPS::SelectWeaponOnePressed()
{
	if (GetSelectedWeaponIndex() != 0)
	{
		bWantsToSwitch = true;
	}
}

void UCharacterMovementComponent_FPS::SelectWeaponTwoPressed()
{
	if (GetSelectedWeaponIndex() != 1)
	{
		bWantsToSwitch = true;
	}
}

void UCharacterMovementComponent_FPS::SimulatedTick(float DeltaSeconds)
{
	UE_LOG(LogTemp, Warning, TEXT("SimulatedTick"))
}

void UCharacterMovementComponent_FPS::ReloadWeaponRPC_Implementation()
{
	bWantsToReload = true;
}

void UCharacterMovementComponent_FPS::SwitchWeaponRPC_Implementation()
{
	bWantsToSwitch = true;
}

void UCharacterMovementComponent_FPS::GenerateShootCommand()
{
	FCharacterNetworkMoveData_FPS* MoveData = static_cast<FCharacterNetworkMoveData_FPS*>(GetCurrentNetworkMoveData());

	if (MoveData != nullptr)
	{
		CurrentInterpolationTime = MoveData->InterpolationTime;
		CurrentControlRotation = MoveData->ControlRotation;
	}

	CharacterOwner_FPS->AddShootCommand(CurrentInterpolationTime, CurrentControlRotation);
}
