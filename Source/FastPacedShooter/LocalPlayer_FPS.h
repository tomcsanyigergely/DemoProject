// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LocalPlayer.h"
#include "LocalPlayer_FPS.generated.h"

/**
 * 
 */
UCLASS()
class FASTPACEDSHOOTER_API ULocalPlayer_FPS : public ULocalPlayer
{
	GENERATED_BODY()

public:
	virtual FString GetNickname() const override;
};
