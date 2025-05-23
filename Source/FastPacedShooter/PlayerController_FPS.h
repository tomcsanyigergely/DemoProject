// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerController_FPS.generated.h"

USTRUCT(BlueprintType)
struct FASTPACEDSHOOTER_API FKillFeedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString LeftPlayerName;

	UPROPERTY(BlueprintReadOnly)
	FString RightPlayerName;

	UPROPERTY(BlueprintReadOnly)
	uint8 WeaponTypeIndex;
	
	UPROPERTY(BlueprintReadOnly)
	bool bHeadshot;
};

/**
 * 
 */
UCLASS()
class FASTPACEDSHOOTER_API APlayerController_FPS : public APlayerController
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegate);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHitEventDelegate, bool, bKilled, bool, bHeadshot);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStringDelegate, FString, Winner);

	UPROPERTY(BlueprintAssignable);
	FDelegate OnUpdatePing;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnUpdateHealth;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnAmmoChanged;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnWeaponChanged;

	UPROPERTY(BlueprintAssignable)
	FDelegate OnNewKillFeedEvent;

	UPROPERTY(BlueprintAssignable)
	FOnHitEventDelegate OnHitEvent;
	
	UPROPERTY(BlueprintAssignable)
	FStringDelegate OnWinnerEvent;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> PingWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> CrosshairWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> HealthWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> AmmoWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> ScoreWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> InGameMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> KillFeedWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> HitMarkerWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> WinnerWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> ScopeWidgetClass;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UUserWidget> StatisticsWidgetClass;

	UPROPERTY()
	class ACharacter_FPS* LastOwnedCharacter = nullptr;

	UPROPERTY()
	UUserWidget* CrosshairWidget;

	UPROPERTY()
	UUserWidget* HealthWidget;
	
	UPROPERTY()
	UUserWidget* AmmoWidget;

	UPROPERTY()
	UUserWidget* ScoreWidget;

	UPROPERTY()
	UUserWidget* InGameMenuWidget;

	UPROPERTY()
	UUserWidget* WinnerWidget;

	UPROPERTY()
	UUserWidget* ScopeWidget;

	UPROPERTY()
	UUserWidget* StatisticsWidget;

	bool bBeginPlayCalled = false;

	FTimerHandle RespawnTimer;

	float HipFireMouseSensitivity;
	float IronsightsMouseSensitivity;
	float ScopeMouseSensitivity;
	float ScopeFOV;

	UPROPERTY()
	class UGameUserSettings_FPS* GameUserSettings;

	bool bInGameMenuVisible = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class USpringArmComponent* DeathCameraSpringArm;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	class USceneComponent* DeathCameraTransform;
	
	TQueue<FKillFeedEvent> KillFeedEventQueue;

	bool bAimbotActive = false;

	bool bStatisticsVisible = false;

public:	
	bool bHitscanDebugInfoActive = false;
	
	uint8 LastHitBoneIndex;
	float LastHitRewindTime;
	uint8 LastHitWeaponTypeIndex;
	
	float AuthRewindTime;

	uint8 NextMetricIndex = 0;
	float AbsRewindDiffValues[100];
	float AuthDiffValues[100];

	uint8 LastClientHitInterpID = 0;
	uint8 LastClientHitBoneIndex = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int AllShots = 0;

		UPROPERTY(Replicated, BlueprintReadOnly)
		int MatchedShots = 0;

			UPROPERTY(Replicated, BlueprintReadOnly)
			int MatchedHitShots = 0;

			UPROPERTY(Replicated, BlueprintReadOnly)
			int MatchedMissedShots = 0;

		UPROPERTY(Replicated, BlueprintReadOnly)
		int MismatchedShots = 0;

			UPROPERTY(Replicated, BlueprintReadOnly)
			int MismatchedEqualDamageShots = 0;

			UPROPERTY(Replicated, BlueprintReadOnly)
			int MismatchedUnequalDamageShots = 0;

				UPROPERTY(Replicated, BlueprintReadOnly)
				int MismatchedUnequalMoreDamageShots = 0;

					UPROPERTY(Replicated, BlueprintReadOnly)
					int MismatchedUnequalMoreDamageClientHitShots = 0;
					
					UPROPERTY(Replicated, BlueprintReadOnly)
					int MismatchedUnequalMoreDamageClientMissedShots = 0;

				UPROPERTY(Replicated, BlueprintReadOnly)
				int MismatchedUnequalLessDamageShots = 0;

					UPROPERTY(Replicated, BlueprintReadOnly)
					int MismatchedUnequalLessDamageServerHitShots = 0;

					UPROPERTY(Replicated, BlueprintReadOnly)
					int MismatchedUnequalLessDamageServerMissedShots = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int HeadshotTruePositives = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int HeadshotFalsePositives = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int HeadshotFalseNegatives = 0;
	
	APlayerController_FPS();

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerTick(float DeltaTime) override;

	virtual void UpdateRotation(float DeltaTime) override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual void SetupInputComponent() override;
	
	virtual void SetPawn(APawn* InPawn) override;
	
	UFUNCTION(BlueprintPure)
	int GetPing() const;

	UFUNCTION(BlueprintPure)
	float GetHealthNormalized() const;

	void OnPlayerKilled(APlayerController* KilledBy, uint8 WeaponTypeIndex, bool bHeadshot);

	virtual void AddYawInput(float Val) override;
	virtual void AddPitchInput(float Val) override;

	UFUNCTION(BlueprintPure)
	int GetCurrentAmmo() const;

	UFUNCTION(BlueprintPure)
	int GetMaxAmmo() const;

	UFUNCTION(BlueprintPure)
	UTexture* GetSelectedWeaponImage() const;

	UFUNCTION(BlueprintPure)
	float GetInPacketLoss() const;

	UFUNCTION(BlueprintPure)
	float GetOutPacketLoss() const;

	UFUNCTION(BlueprintPure)
	UTexture* GetWeaponImage(uint8 WeaponTypeIndex) const;

	FORCEINLINE USceneComponent* GetDeathCameraTransform() const { return DeathCameraTransform; }
	FORCEINLINE ACharacter_FPS* GetLastOwnedCharacter() const { return LastOwnedCharacter; }

	FORCEINLINE float GetScopeFOV() const { return ScopeFOV; }

	UFUNCTION(BlueprintCallable)
	bool GetNextKillFeedEvent(FKillFeedEvent& NextKillFeedEvent);

	UFUNCTION(BlueprintCallable)
	void AddKillFeedEvent(FString LeftPlayerName, FString RightPlayerName, uint8 WeaponTypeIndex, bool bHeadshot);
	
	UFUNCTION(Client, Reliable)
	void HitFeedback(APlayerState* DamagedPlayer, uint8 BoneIndex, uint8 WeaponTypeIndex, bool bKilled, float RewindTime, uint8 ClientInterpID, uint8 ClientHitBoneIndex);

	void SetWinner(APlayerState* WinnerPlayerState);

private:
	void OnPawnChanged();

	UFUNCTION()
	void OnUpdateHealthBroadcast();

	UFUNCTION()
	void RespawnPlayer(class ACharacter_FPS* KilledCharacter);

	void ShowScoreboard();
	void HideScoreboard();

	UFUNCTION()
	void OnUpdateCrosshair();

	UFUNCTION()
	void OnScopeViewChanged();

	UFUNCTION()
	void OnMovementModeChanged(ACharacter* OwnedCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	void EscapePressed();

	void InfoPressed();

	void AimbotPressed();

	void StatisticsPressed();

	UFUNCTION(Server, Reliable)
	void SwitchDebugInfo(bool bNewHitscanDebugInfoActive);

	UFUNCTION()
	void OnAmmoChangedEvent();

	UFUNCTION()
	void OnWeaponChangedEvent();

};