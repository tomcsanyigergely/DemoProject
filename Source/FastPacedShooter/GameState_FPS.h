// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FastPacedShooter.h"
#include "GameSnapshot.h"
#include "GameFramework/GameState.h"
#include "GameState_FPS.generated.h"

struct FGameSnapshot;

USTRUCT()
struct FASTPACEDSHOOTER_API FHitboxData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	FName BoneName;

	UPROPERTY(EditDefaultsOnly)
	float DamageMultiplier;
};

/**
 * 
 */
UCLASS()
class FASTPACEDSHOOTER_API AGameState_FPS : public AGameState
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegate);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerTickDelegate, float, DeltaTime);

	struct SnapshotTimeBufferElement
	{
		float LocalTime = 0;
		ServerTime ServerTick = 0;
	};

private:	
	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	uint8 SnapshotsPerSecond = 10;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float InterpolationDelay = 0.3;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float RewindDeltaInSeconds = 0.025;

	float RewindDelta;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	uint8 TimeSynchronizationWindow = 20;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float SoftSmoothnessRangeSeconds = 0.005f;

	float SoftSmoothnessRange;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float SoftSmoothnessFactor = 0.01;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float HardSmoothnessRangeSeconds = 0.100f;

	float HardSmoothnessRange;

	UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess=true))
	float HardSmoothnessFactor = 0.1;
	
public:	
	UPROPERTY(BlueprintAssignable)
	FOnServerTickDelegate OnServerTick;
	
private:
	UPROPERTY(Transient)
	class ASnapshotReplicator* SnapshotReplicator;
	
	SnapshotTimeBufferElement SnapshotTimeBuffer[256];

	float TimestampBufferLineSlope = 0;
	float TimestampBufferLineOffset = 0;

	uint8 LastSnapshotIndex = -1;

	ServerTime ServerTick = 0;
	double LastTickPlatformTime = 0;

	float InterpolationTime = 0;

	float LastUpdateTime = 0;

	TMap<uint8, class ACharacter_FPS*> InterpolatedCharactersMap;

	TSet<ACharacter_FPS*> InterpolatedCharactersSet;

	TQueue<uint8> InterpolationIDPool;

private:
	UPROPERTY(BlueprintAssignable)
	FDelegate OnScoreChanged;
	
	UPROPERTY(EditDefaultsOnly)
	TArray<FHitboxData> Hitbox;

	UPROPERTY(Transient)
	TMap<FName, uint8> HitboxIndices;

	uint8 HeadBoneIndex;
	
	bool bGameFinished = false;

	TArray<class AWeapon_FPS*> WeaponCDOs;

	bool RemoteAudioIndicesInitialized = false;

public:	
	TQueue<int> RemoteAudioIndices;
	
	UPROPERTY(Transient, ReplicatedUsing=OnRep_WinnerPlayerState)
	APlayerState* WinnerPlayerState;

public:
	AGameState_FPS();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;

	void SendSnapshot(const FGameSnapshot& PackedBits);
	void ReceiveSnapshot(const FGameSnapshot& PackedBits);

	FORCEINLINE float GetLastInterpolationTime() const { return InterpolationTime; }
	FORCEINLINE ServerTime GetServerTick() const { return ServerTick; }
	FORCEINLINE const TSet<ACharacter_FPS*>& GetInterpolatedCharacters() const { return InterpolatedCharactersSet; }
	FORCEINLINE ACharacter_FPS* GetInterpolatedCharacter(uint8 InterpolationID) const
	{
		if (InterpolatedCharactersMap.Contains(InterpolationID))
		{
			return InterpolatedCharactersMap[InterpolationID];
		}
		else
		{
			return nullptr;
		}
	}

	FORCEINLINE float GetSnapshotsPerSecond() const { return SnapshotsPerSecond; }

	void AddToInterpolatedCharacters(ACharacter_FPS* Character);
	void RemoveFromInterpolatedCharacters(ACharacter_FPS* Character);
	bool IsInterpolated(ACharacter_FPS* Character);

	TSet<ACharacter_FPS*> RewindAllCharactersExcept(float RewindTime, ACharacter_FPS* SkipRewindCharacter);
	void RestoreAllCharacters();

	float GetRewindTime(class APlayerController_FPS* Player, float TargetRewindTime) const;

	void UpdateSimulatedProxies(float DeltaTime);

private:
	void CreateSnapshot(FGameSnapshot& SnapshotPackedBits);
	void ConsumeSnapshot(const FGameSnapshot& SnapshotPackedBits);
	void UpdateInterpolationParameters();
	void UpdateInterpolationTime(float DeltaTime);

public:	
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	virtual void ReceivedGameModeClass() override;

	UFUNCTION(BlueprintPure)
	TArray<UObject*> GetScoreboardList();

	UFUNCTION(NetMulticast, Reliable)
	void PlayerKilled(APlayerState* DamageCauserPlayerState, APlayerState* KilledPlayerState, uint8 WeaponTypeIndex, bool bHeadshot);

	FORCEINLINE float GetBoneDamageMultiplier(FName BoneName) const { return Hitbox[HitboxIndices[BoneName]].DamageMultiplier; }
	FORCEINLINE float GetBoneDamageMultiplierByIndex(uint8 BoneIndex) const { return Hitbox[BoneIndex].DamageMultiplier; }
	FORCEINLINE uint8 GetBoneIndex(FName BoneName) const { return HitboxIndices[BoneName]; }
	FORCEINLINE FName GetBoneNameByIndex(uint8 BoneIndex) const { return Hitbox[BoneIndex].BoneName; }
	FORCEINLINE bool IsHeadBoneIndex(uint8 BoneIndex) const { return BoneIndex == HeadBoneIndex; }
	FORCEINLINE bool IsGameFinished() const { return bGameFinished; }
	FORCEINLINE void FinishGame() {	bGameFinished = true; }

	float GetWeaponBaseDamage(uint8 WeaponTypeIndex) const;
	float GetWeaponFireInterval(uint8 WeaponTypeIndex) const;
	UTexture* GetWeaponImage(uint8 WeaponTypeIndex) const;
	
	void InitializeRemoteAudioIndices();
	
private:
	UFUNCTION()
	void OnRep_WinnerPlayerState();
	
	UFUNCTION()
	void OnPlayerScoreChanged();

	void AddPlayerToScoreboardList(class APlayerState_FPS* PlayerState, bool bLocalPlayerState, TArray<UObject*>& ScoreboardList);	
};
