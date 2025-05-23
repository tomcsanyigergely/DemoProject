// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FastPacedShooter.h"
#include "Weapon_FPS.h"
#include "GameFramework/Character.h"
#include "GameSnapshot.h"
#include "Character_FPS.generated.h"

UCLASS(config=Game)
class ACharacter_FPS : public ACharacter
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegate);

	struct FStoredSnapshot
	{
		ServerTime ServerTick;
		FVector Position;
		FRotator Rotation;
		FAnimData AnimData;
	};

	struct FShootCommand
	{
		float RewindTime;
		uint8 WeaponIndex;
		FVector CameraLocation;
		FVector LookDirection;
		float Error;
		uint8 ClientHitInterpolationID;
		uint8 ClientHitBoneIndex;
	}; 
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess=true))
	class UCharacterMovementComponent_FPS* CharacterMovementComponent_FPS;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess=true))
	class USkeletalMeshComponent* LagCompMesh;
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class UCameraComponent* Camera;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class USkeletalMeshComponent* FirstPersonArmMesh;

	UPROPERTY(EditDefaultsOnly)
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly)
	float DefaultFOV = 90.0f;

	UPROPERTY(EditDefaultsOnly)
	float IronsightsFOV = 60.0f;

	UPROPERTY(EditDefaultsOnly)
	float ScopeFOV = 5.0f;

	UPROPERTY(EditDefaultsOnly)
	float IronsightsTransitionDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<class AWeapon_FPS>> WeaponTypes;

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* SwitchingMontage;

	UPROPERTY(EditDefaultsOnly)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequence* UpperBodyDefaultSequence;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequence* UpperBodySprintingSequence;

	UPROPERTY(EditDefaultsOnly)
	class UBlendSpace1D* IdleRunBlendSpace;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequence* JumpStartSequence;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequence* JumpLoopSequence;

	UPROPERTY(EditDefaultsOnly)
	class UAnimSequence* JumpEndSequence;

	UPROPERTY(EditDefaultsOnly)
	float BurstLimit = 0.2;

	UPROPERTY(EditDefaultsOnly)
	uint8 BurstCounterMax = 1;
	
private:
	UPROPERTY(Transient)
	class AGameState_FPS* GameState;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_InterpolationID)
	uint8 InterpolationID = 0;
	
	uint8 PreviousInterpolationID = 0;

	TQueue<uint8> Delayed_OnRep_InterpolationID_Queue;

	FStoredSnapshot SnapshotBuffer[256];
	
	ServerTime FirstSnapshotServerTick = 0;
	ServerTime LastSnapshotServerTick = 0;
	ServerTime InterpFromSnapshotServerTick = 0;
	ServerTime InterpToSnapshotServerTick = 0;

	float LastInterpolationTime = 0;

	bool bInterpolate = false;
	bool bInterpolateAnimationPose = true;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FAnimData InterpolatedAnim;

	bool bDuringRewind = false;
	
	FVector RealLocation;
	FRotator RealRotation;
	
public:
	FDelegate OnHealthChanged;

	FDelegate OnIronsightsStarted;
	FDelegate OnIronsightsEnded;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnRemotePlayerShoot;

	FDelegate OnScopeViewChanged;

	FDelegate OnWeaponChanged;
	
private:
	UPROPERTY(Transient)
	TArray<class AWeapon_FPS*> Weapons;

	UPROPERTY(Transient)
	AController* PreviousController;
	
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Killed, BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	bool bKilled = false;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_KilledByPawn)
	APawn* KilledByPawn;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_CurrentHealth)
	float CurrentHealth;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	bool bIronsights = false;

	UPROPERTY(Transient, BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	float IronsightsAlpha = 0.0f;
	
	bool bIronsightsPressed = false;
	bool bSprintPressed = false;	

	float LastDamageTime = 0;

	uint8 SelectedWeaponIndex = 0;

	bool bInScopeView = false;

	TQueue<FShootCommand> ShootCommandQueue;
	
	ServerTime LastShotServerTick = 0;
	float BurstCapacity = 0;
	uint8 BurstCounter = 0;
	
	bool bWasShootingLastTick = false;
	
public:
	ACharacter_FPS(const FObjectInitializer& ObjectInitializer);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	void AssignInterpolationID(uint8 InInterpolationID);
	void RemoveInterpolationID();

	void CreatePlayerSnapshot(ServerTime ServerTick, FPlayerSnapshot& PlayerSnapshot);
	void ConsumePlayerSnapshot(ServerTime ServerTick, const FPlayerSnapshot& PlayerSnapshot);

	void CreateAnimData(FAnimData& AnimData);

	void UpdateSimulatedProxy(float InterpolationTime);
	
	void Rewind(float RewindTime);
	void Restore();	
	
	void DisableAnimationInterpolation();
	
	UFUNCTION(Client, Reliable)
	void DebugHitboxRPC(const TArray<FVector>& HitboxPositions, const TArray<FQuat>& HitboxRotations, const FVector& StartLoc, const FVector& EndLoc, float Error, bool bHit);

private:	
	void AddSnapshot(ServerTime ServerTick, FVector Position, FRotator Rotation, const FAnimData& AnimData);

	void GetInterpolatedSnapshots(float InterpolationTime, uint8& FromSnapshotIndex, uint8& ToSnapshotIndex, float& Alpha);

	void InterpolateCharacter(uint8 FromSnapshotIndex, uint8 ToSnapshotIndex, float Alpha);

	void InterpolateAnimationParameters(uint8 FromSnapshotIndex, uint8 ToSnapshotIndex, float Alpha);

	UFUNCTION()
	void OnRep_InterpolationID();

	void Handle_OnRep_InterpolationID();

public:
	FORCEINLINE bool HasValidInterpolationID() const { return InterpolationID != 0; }
	FORCEINLINE uint8 GetInterpolationID() const { return InterpolationID; }
	FORCEINLINE bool CanRewindTo(float RewindTime) const { return LastSnapshotServerTick != 0 && RewindTime <= LastSnapshotServerTick * 1.0f && RewindTime >= FMath::Max(FirstSnapshotServerTick * 1.0f, LastSnapshotServerTick * 1.0f - 255); }

public:
	virtual void OnRep_Controller() override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);

public:
	void IronsightsPressed();
	void IronsightsReleased();

	void SprintPressed();
	void SprintReleased();

	UFUNCTION(Server, Reliable)
	void ReliableShootRPC(float InterpolationTime);

	void AddShootCommand(float InterpolationTime, FRotator ControlRotation);
	void ApplyShootCommands();
	
	bool FindDamagedBone(const TSet<ACharacter_FPS*>& LagCompensatedCharacters, const TArray<FHitResult>& HitResults, const FVector& StartLocation, ACharacter_FPS*& HitCharacter, FName& HitBoneName);

private:	
	UFUNCTION()
	void OnRep_Killed();

	UFUNCTION()
	void OnRep_KilledByPawn();

	UFUNCTION()
	void OnRep_CurrentHealth();

	UFUNCTION()
	void OnDamageTaken(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	void SetCurrentHealth(float NewHealth);

	int CalculateShotCount();
	
public:
	void SwitchToThirdPersonView();
	
	FORCEINLINE USkeletalMeshComponent* GetLagCompMesh() const { return LagCompMesh; }
	FORCEINLINE UCharacterMovementComponent_FPS* GetCharacterMovement_FPS() const { return CharacterMovementComponent_FPS; }

	FORCEINLINE UCameraComponent* GetCameraComponent() const { return Camera; }

	FORCEINLINE TArray<TSubclassOf<AWeapon_FPS>> GetWeaponTypes() const { return WeaponTypes; }

	void SetSelectedWeapon(uint8 NewSelectedWeaponIndex);
	UFUNCTION(BlueprintPure) AWeapon_FPS* GetSelectedWeapon() const;
	FORCEINLINE uint8 GetSelectedWeaponIndex() const { return SelectedWeaponIndex; }
	FORCEINLINE float GetWeaponReloadTime() const { return Weapons[SelectedWeaponIndex]->GetReloadTime(); }
	FORCEINLINE float GetWeaponFireInterval() const { return Weapons[SelectedWeaponIndex]->GetFireInterval(); }
	FORCEINLINE float GetWeaponBaseDamage() const { return Weapons[SelectedWeaponIndex]->GetBaseDamage(); }
	UFUNCTION(BlueprintPure) bool IsWeaponAutomatic() const { return Weapons[SelectedWeaponIndex]->IsAutomatic(); }
	
	FORCEINLINE void DecrementWeaponAmmo() { Weapons[SelectedWeaponIndex]->DecrementAmmo(); }
	FORCEINLINE void RestoreWeaponAmmo() { Weapons[SelectedWeaponIndex]->RestoreAmmo(); }
	FORCEINLINE void SetWeaponAmmo(uint8 Ammo) { Weapons[SelectedWeaponIndex]->SetAmmo(Ammo); }
	UFUNCTION(BlueprintPure) uint8 GetCurrentAmmo() const;
	UFUNCTION(BlueprintPure) uint8 GetMaxAmmo() const;

	FORCEINLINE bool IsKilled() const { return bKilled; }
	FORCEINLINE APawn* GetKilledByPawn() const { return KilledByPawn; }

	FORCEINLINE void Heal() { CurrentHealth = MaxHealth; }
	FORCEINLINE float GetHealthNormalized() const { return CurrentHealth / MaxHealth; }

	FORCEINLINE float GetLastDamageTime() const { return LastDamageTime; }
			
	FORCEINLINE float GetIronsightsAlpha() const { return IronsightsAlpha; }
	FORCEINLINE bool GetIronsights() const { return bIronsights; }

	FORCEINLINE bool IsInScopeView() const { return bInScopeView; }
	
	FORCEINLINE EAimType GetAimType() const
	{
		if (IsValid(GetSelectedWeapon()))
		{
			return GetSelectedWeapon()->GetAimType();
		}
		
		return EAimType::AIM_None;
	}
};

