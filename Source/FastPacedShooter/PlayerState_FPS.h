#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"

#include "PlayerState_FPS.generated.h"

UCLASS()
class FASTPACEDSHOOTER_API APlayerState_FPS : public APlayerState
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegate);

	bool bBeginPlayCalled = false;

	bool bPlayerNameReceived = false;
	
public:
	FDelegate OnPlayerScoreChanged;

	UPROPERTY(Transient, BlueprintReadOnly)
	int RemoteAudioIndex = 0;
	
private:
	UPROPERTY(ReplicatedUsing=OnRep_Kills)
	int Kills = 0;

	UPROPERTY(ReplicatedUsing=OnRep_Deaths)
	int Deaths = 0;
	
public:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_PlayerName() override;

	FORCEINLINE int GetKills() const { return Kills; }
	FORCEINLINE int GetDeaths() const { return Deaths; }

	void IncreaseKills();
	void IncreaseDeaths();

private:
	UFUNCTION()
	void OnRep_Kills();

	UFUNCTION()
	void OnRep_Deaths();

	void GetRemoteAudioIndex();
	
};
