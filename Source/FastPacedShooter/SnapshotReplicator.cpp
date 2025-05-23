#include "SnapshotReplicator.h"

#include "GameState_FPS.h"

ASnapshotReplicator::ASnapshotReplicator()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ASnapshotReplicator::BeginPlay()
{
	Super::BeginPlay();

	GameState = GetWorld()->GetGameState<AGameState_FPS>();
}

void ASnapshotReplicator::SendSnapshot_Implementation(const FGameSnapshot& PackedBits)
{
	if (IsValid(GameState))
	{
		GameState->ReceiveSnapshot(PackedBits);
	}
}
