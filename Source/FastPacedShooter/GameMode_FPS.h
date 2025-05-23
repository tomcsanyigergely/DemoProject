// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameMode_FPS.generated.h"

struct FShootData
{
	
};

UCLASS(minimalapi)
class AGameMode_FPS : public AGameMode
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	class AGameState_FPS* GameState_FPS;

public:	
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnTick(float DeltaTime);
};



