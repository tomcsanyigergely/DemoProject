#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameSnapshot.h"

#include "SnapshotReplicator.generated.h"

UCLASS()
class FASTPACEDSHOOTER_API ASnapshotReplicator : public AActor
{	
	GENERATED_BODY()

	UPROPERTY(Transient)
	class AGameState_FPS* GameState;
	
public:
	ASnapshotReplicator();

	virtual void BeginPlay() override;

	UFUNCTION(NetMulticast, Unreliable)
	void SendSnapshot(const FGameSnapshot& PackedBits);
};
