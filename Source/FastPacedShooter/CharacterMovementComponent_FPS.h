// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character_FPS.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CharacterMovementComponent_FPS.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDynamicDelegate);

struct FASTPACEDSHOOTER_API FCharacterNetworkMoveData_FPS : public FCharacterNetworkMoveData
{
	typedef FCharacterNetworkMoveData Super;

	float InterpolationTime;
	
	uint8 CurrentAmmo;
	bool bIsReloading;
	bool bIsSwitching;
	uint8 SelectedWeaponIndex;
	bool bForceShooting;
	uint8 LastHitInterpolationID;
	uint8 LastHitBoneIndex;

	FCharacterNetworkMoveData_FPS() : FCharacterNetworkMoveData(), InterpolationTime(0.0f), CurrentAmmo(0), bIsReloading(false), bIsSwitching(false), SelectedWeaponIndex(0), bForceShooting(false), LastHitInterpolationID(0), LastHitBoneIndex(0) {}

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;
};

struct FASTPACEDSHOOTER_API FCharacterNetworkMoveDataContainer_FPS : public FCharacterNetworkMoveDataContainer
{
	FCharacterNetworkMoveDataContainer_FPS();

private:	
	FCharacterNetworkMoveData_FPS MoveData[3];
};

struct FASTPACEDSHOOTER_API FCharacterMoveResponseDataContainer_FPS : public FCharacterMoveResponseDataContainer
{
	using Super = FCharacterMoveResponseDataContainer;

	virtual void ServerFillResponseData(const UCharacterMovementComponent& CharacterMovement, const FClientAdjustment& PendingAdjustment) override;
	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap) override;

	uint8 CurrentAmmo;
	float ShootCooldownTime;
	bool bIsReloading;
	float ReloadTimeRemaining;
	bool bIsSwitching;
	float SwitchingTimeRemaining;
	uint8 SelectedWeaponIndex;
	float SprintingAlpha;
	bool bForceShooting;
};

UCLASS()
class FASTPACEDSHOOTER_API UCharacterMovementComponent_FPS : public UCharacterMovementComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegate);

	friend class FSavedMove_FPS;

public:
	UPROPERTY(BlueprintAssignable)
	FDynamicDelegate OnShootDelegate;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnAmmoChanged;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnReloadStarted;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnReloadFinished;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnSwitchingStarted;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnSwitchingFinished;

	FDelegate OnSprintStarted;
	FDelegate OnSprintFinished;
	
	uint8 LastHitInterpolationID = 0;
	uint8 LastHitBoneIndex = 0;

private:
	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;

	UPROPERTY(EditDefaultsOnly) float AirJumpHorizontalForce;

	UPROPERTY(EditDefaultsOnly) float FirstPersonArmTransitionDuration;
	UPROPERTY(EditDefaultsOnly) float WeaponSwitchingTime = 2.0f;

	// Flags (sent to server, not corrected, restored in UpdateFromCompressedFlags)
	bool bWantsToSprint;
	bool bHoldingJump;
	bool bWantsToShoot;

	// Input-derived state (not sent to server, not corrected, restored in PrepMoveFor)
	bool bPrevHoldingJump;
	bool bWantsToSwitch;
	bool bWantsToReload;

	// Internal state (not sent to server, corrected)	
	float ReloadTimeRemaining;
	float SwitchingTimeRemaining;	

	// Output state (sent to server, corrected)	
	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess=true)) bool bIsReloading = false;
	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess=true)) bool bIsSwitching = false;
	// SelectedWeaponIndex from Character
	// CurrentAmmo from weapon

	float ShootCooldownTime = 0;

	UPROPERTY(Transient) class ACharacter_FPS* CharacterOwner_FPS;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	bool bFirePressed = false;

	UPROPERTY(Transient, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
	bool bPendingFirstQuantizationEvent = false;

	FCharacterNetworkMoveDataContainer_FPS MoveDataContainer;
	FCharacterMoveResponseDataContainer_FPS MoveResponseDataContainer;
		
	bool bWasReloading = false;
	bool bWasSwitching = false;

	UPROPERTY()
	class AGameState_FPS* GameState;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float FirstPersonArmOffsetAlpha = 0.0f;

	bool bAcceleratingForward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float SprintTransitionDuration = 0.25f;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float SprintingAlpha = 0.0f;

	bool bForceShooting = false;

	bool bReliableShoot = false;
	float ReliableShootInterpolationTime = 0;

	float CurrentInterpolationTime;
	FRotator CurrentControlRotation;

public:	
	UCharacterMovementComponent_FPS(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeComponent() override;

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void ControlledCharacterMove(const FVector& InputVector, float DeltaSeconds) override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	virtual float GetMaxSpeed() const override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

	void StartSprinting();
	void EndSprinting();

	void JumpPressed();
	void JumpReleased();

	void FirePressed();
	void FireReleased();

	void ReloadPressed();
	void SwitchWeaponPressed();
	void SelectWeaponOnePressed();
	void SelectWeaponTwoPressed();

	virtual void SimulatedTick(float DeltaSeconds) override;

	virtual bool ClientUpdatePositionAfterServerUpdate() override;

	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation, const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;
		
	virtual void ClientHandleMoveResponse(const FCharacterMoveResponseDataContainer& MoveResponse) override;

	UFUNCTION(BlueprintPure) EMovementMode GetMovementMode() const { return MovementMode; }

	UFUNCTION(BlueprintPure) bool IsSprinting() const;

	void HandleReliableShoot(float InterpolationTime);
	
	FORCEINLINE float GetShootCooldownTime() const { return ShootCooldownTime; }
	FORCEINLINE void SetShootCooldownTime(float NewShootCooldownTime) { ShootCooldownTime = NewShootCooldownTime; }

	FORCEINLINE bool IsReloading() const { return bIsReloading; }
	FORCEINLINE float GetReloadTimeRemaining() const { return ReloadTimeRemaining; }
	FORCEINLINE bool IsSwitching() const { return bIsSwitching; }
	FORCEINLINE float GetSwitchingTimeRemaining() const { return SwitchingTimeRemaining; }
	UFUNCTION(BlueprintPure) bool CanShoot() const;
	FORCEINLINE uint8 GetSelectedWeaponIndex() const { return CharacterOwner_FPS->GetSelectedWeaponIndex(); }
	FORCEINLINE uint8 GetCurrentAmmo() const { return CharacterOwner_FPS->GetCurrentAmmo(); }
	FORCEINLINE uint8 GetMaxAmmo() const { return CharacterOwner_FPS->GetMaxAmmo(); }
	FORCEINLINE float GetWeaponReloadTime() const { return CharacterOwner_FPS->GetWeaponReloadTime(); }
	FORCEINLINE float GetWeaponFireInterval() const { return CharacterOwner_FPS->GetWeaponFireInterval(); }
	FORCEINLINE float GetWeaponBaseDamage() const { return CharacterOwner_FPS->GetWeaponBaseDamage(); }
	FORCEINLINE bool IsWeaponAutomatic() const { return CharacterOwner_FPS->IsWeaponAutomatic(); }
	FORCEINLINE void DecrementWeaponAmmo() { CharacterOwner_FPS->DecrementWeaponAmmo(); }
	FORCEINLINE void RestoreWeaponAmmo() { CharacterOwner_FPS->RestoreWeaponAmmo(); }

	FORCEINLINE float GetSprintingAlpha() const { return SprintingAlpha; }
	FORCEINLINE bool GetForceShooting() const { return bForceShooting; }

	FORCEINLINE bool WantsToShoot() const { return bWantsToShoot; }

	FTransform RootMotionTransformCopy;

	UFUNCTION(BlueprintCallable) void StartShooting();
	UFUNCTION(BlueprintCallable) void StopShooting();

	UFUNCTION(Server, Reliable)
	void ReloadWeaponRPC();

	UFUNCTION(Server, Reliable)
	void SwitchWeaponRPC();
	
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

private:
	void GenerateShootCommand();
};

class FSavedMove_FPS : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;
		
public:
	enum CompressedFlags
	{
		FLAG_WantsToSprint = 0x10,
		FLAG_HoldingJump = 0x20,
		FLAG_WantsToShoot = 0x40,
	};
	
	// Flags (restored in UpdateFromCompressedFlags)
	uint8 bWantsToSprint:1;
	uint8 bHoldingJump:1;
	uint8 bWantsToShoot:1;

	// Input-derived state (not sent to server, not corrected, restored in PrepMoveFor)
	uint8 bPrevHoldingJump:1;
	FRotator ControlRotation;
	uint8 bWantsToReload:1;
	uint8 bWantsToSwitch:1;

	// Additional input (sent to server, not corrected)
	float InterpolationTime;

	// Internal state (used for move combining, set in SetInitialPosition(), not sent to server)
	uint8 StartForceShooting:1;
	uint8 StartSelectedWeaponIndex;
	float StartShootCooldownTime;
	float StartSprintingAlpha;
	uint8 StartIsReloading:1;
	float StartReloadTimeRemaining;
	uint8 StartIsSwitching:1;
	float StartSwitchingTimeRemaining;

	// Output state (used for correction, set in PostUpdate(), sent to server)
	uint8 SavedIsReloading:1;
	uint8 SavedIsSwitching:1;
	uint8 SavedSelectedWeaponIndex;
	uint8 SavedCurrentAmmo;
	uint8 SavedForceShooting:1;

	// Output state (used for statistics)
	uint8 LastHitInterpolationID;
	uint8 LastHitBoneIndex;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
	virtual void CombineWith(const FSavedMove_Character* OldMove, ACharacter* InCharacter, APlayerController* PC, const FVector& OldStartLocation) override;
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
	virtual void SetInitialPosition(ACharacter* C) override;
	virtual void PostUpdate(ACharacter* C, EPostUpdateMode PostUpdateMode) override;
};

class FNetworkPredictionData_Client_FPS : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_FPS(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	virtual FSavedMovePtr AllocateNewMove() override;
};