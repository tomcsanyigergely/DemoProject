// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCameraManager_FPS.h"

#include "CharacterMovementComponent_FPS.h"
#include "Character_FPS.h"
#include "PlayerController_FPS.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

void APlayerCameraManager_FPS::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{	
	Super::UpdateViewTarget(OutVT, DeltaTime);

	if (APlayerController_FPS* PlayerController = Cast<APlayerController_FPS>(GetOwningPlayerController()))
	{
		if (ACharacter_FPS* Character = GetOwningPlayerController()->GetPawn<ACharacter_FPS>())
		{			
			LastViewInfo = OutVT.POV;

			return;
		}
	}
	
	if (APlayerController_FPS* PlayerController = Cast<APlayerController_FPS>(GetOwningPlayerController()))
	{
		if (ACharacter_FPS* LastOwnedCharacter = PlayerController->GetLastOwnedCharacter())
		{			
			if (APawn* KilledByPawn = LastOwnedCharacter->GetKilledByPawn())
			{
				PlayerController->GetDeathCameraTransform()->UpdateComponentToWorld();
				OutVT.POV.Location = PlayerController->GetDeathCameraTransform()->GetComponentLocation();
				OutVT.POV.Rotation = PlayerController->GetDeathCameraTransform()->GetComponentRotation();

				LastViewInfo = OutVT.POV;

				return;
			}
		}
	}
	
	OutVT.POV = LastViewInfo;
}
