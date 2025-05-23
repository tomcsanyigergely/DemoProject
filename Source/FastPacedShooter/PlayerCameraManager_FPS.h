// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "PlayerCameraManager_FPS.generated.h"

/**
 * 
 */
UCLASS()
class FASTPACEDSHOOTER_API APlayerCameraManager_FPS : public APlayerCameraManager
{
	GENERATED_BODY()

	FMinimalViewInfo LastViewInfo;

protected:
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;
};
