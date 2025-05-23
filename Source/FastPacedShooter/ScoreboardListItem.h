#pragma once

#include "CoreMinimal.h"

#include "ScoreboardListItem.generated.h"

UCLASS(BlueprintType)
class FASTPACEDSHOOTER_API UScoreboardListItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	bool bLocalPlayerState = false;

	UPROPERTY(BlueprintReadOnly)
	FString PlayerName;
	
	UPROPERTY(BlueprintReadOnly)
	int Kills = 0;

	UPROPERTY(BlueprintReadOnly)
	int Deaths = 0;
};