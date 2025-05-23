// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController_FPS.h"

#include "CharacterMovementComponent_FPS.h"
#include "Character_FPS.h"
#include "GameState_FPS.h"
#include "GameUserSettings_FPS.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

APlayerController_FPS::APlayerController_FPS()
{
	DeathCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("DeathCameraSpringArm"));
	DeathCameraSpringArm->SetupAttachment(GetTransformComponent());
	DeathCameraSpringArm->TargetArmLength = 300.0f;
	DeathCameraSpringArm->bUsePawnControlRotation = false;
	DeathCameraSpringArm->ProbeChannel = ECC_Visibility;

	DeathCameraTransform = CreateDefaultSubobject<USceneComponent>(TEXT("DeathCameraTransform"));
	DeathCameraTransform->SetupAttachment(DeathCameraSpringArm);
	DeathCameraTransform->SetTickGroup(TG_PostPhysics);
	DeathCameraTransform->AddTickPrerequisiteComponent(DeathCameraSpringArm);
}

void APlayerController_FPS::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		GameUserSettings = UGameUserSettings_FPS::GetGameUserSettings_FPS();

		HipFireMouseSensitivity = GameUserSettings->GetHipFireMouseSensitivity();
		IronsightsMouseSensitivity = GameUserSettings->GetIronsightsMouseSensitivity();
		ScopeMouseSensitivity = GameUserSettings->GetScopeMouseSensitivity();
		ScopeFOV = GameUserSettings->GetScopeFOV();
		
		UUserWidget* PingWidget = CreateWidget(this, PingWidgetClass);
		if (IsValid(PingWidget))
		{
			PingWidget->AddToViewport();
		}

		CrosshairWidget = CreateWidget(this, CrosshairWidgetClass);
		if (IsValid(CrosshairWidget))
		{
			CrosshairWidget->AddToViewport();
		}

		HealthWidget = CreateWidget(this, HealthWidgetClass);
		if (IsValid(HealthWidget))
		{
			HealthWidget->AddToViewport();
			HealthWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		AmmoWidget = CreateWidget(this, AmmoWidgetClass);
		if (IsValid(AmmoWidget))
		{
			AmmoWidget->AddToViewport();
			AmmoWidget->SetVisibility(ESlateVisibility::Visible);
		}

		ScoreWidget = CreateWidget(this, ScoreWidgetClass);
		if (IsValid(ScoreWidget))
		{
			ScoreWidget->AddToViewport();
			ScoreWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		InGameMenuWidget = CreateWidget(this, InGameMenuWidgetClass);
		if (IsValid(InGameMenuWidget))
		{
			InGameMenuWidget->AddToViewport(1);
			InGameMenuWidget->SetVisibility(ESlateVisibility::Hidden);			
		}

		UUserWidget* KillFeedWidget = CreateWidget(this, KillFeedWidgetClass);
		if (IsValid(KillFeedWidget))
		{
			KillFeedWidget->AddToViewport();
		}

		UUserWidget* HitMarkerWidget = CreateWidget(this, HitMarkerWidgetClass);
		if (IsValid(HitMarkerWidget))
		{
			HitMarkerWidget->AddToViewport();
		}

		WinnerWidget = CreateWidget(this, WinnerWidgetClass);
		if (IsValid(WinnerWidget))
		{
			WinnerWidget->AddToViewport();
			WinnerWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		ScopeWidget = CreateWidget(this, ScopeWidgetClass);
		if (IsValid(ScopeWidget))
		{
			ScopeWidget->AddToViewport(-1);
			ScopeWidget->SetVisibility(ESlateVisibility::Hidden);
		}

		StatisticsWidget = CreateWidget(this, StatisticsWidgetClass);
		if (IsValid(StatisticsWidget))
		{
			StatisticsWidget->AddToViewport();
			StatisticsWidget->SetVisibility(ESlateVisibility::Hidden);
		}
			
		bBeginPlayCalled = true;

		OnPawnChanged();
	}
}

void APlayerController_FPS::Tick(float DeltaSeconds)
{	
	Super::Tick(DeltaSeconds);

	if (!IsValid(GetPawn()) && IsValid(LastOwnedCharacter))
	{
		if (APawn* KilledByPawn = LastOwnedCharacter->GetKilledByPawn())
		{
			DeathCameraSpringArm->SetWorldRotation((KilledByPawn->GetActorLocation() - LastOwnedCharacter->GetActorLocation()).Rotation());
		}
	}
}

void APlayerController_FPS::PlayerTick(float DeltaTime)
{
	if (GetWorld()->IsClient() && IsValid(GetWorld()->GetGameState()))
	{
		if (AGameState_FPS* GameState = Cast<AGameState_FPS>(GetWorld()->GetGameState()))
		{
			GameState->UpdateSimulatedProxies(DeltaTime);
		}
	}
	
	Super::PlayerTick(DeltaTime);
}

void APlayerController_FPS::UpdateRotation(float DeltaTime)
{
	if (bAimbotActive && GetWorld()->IsClient() && IsValid(GetPawn<ACharacter_FPS>()) && GetPawn<ACharacter_FPS>()->GetCharacterMovement_FPS()->WantsToShoot())
	{
		FString LocalPlayerNickname;

		ACharacter_FPS* TargetCharacter = nullptr;
		float MaxDot = -1;

		if (IsValid(GetWorld()->GetGameState()) && IsValid(PlayerState))
		{
			for(APlayerState* OtherPlayerState : GetWorld()->GetGameState()->PlayerArray)
			{
				if (OtherPlayerState != PlayerState)
				{
					if (IsValid(OtherPlayerState))
					{
						ACharacter_FPS* OtherCharacter = OtherPlayerState->GetPawn<ACharacter_FPS>();

						if (IsValid(OtherCharacter) && !OtherCharacter->IsKilled())
						{
							float Dot = (ControlRotation.RotateVector(FVector::ForwardVector) | (OtherCharacter->GetActorLocation() - GetPawn()->GetActorLocation()).GetSafeNormal());
							
							if (TargetCharacter == nullptr || Dot > MaxDot)
							{
								TargetCharacter = OtherCharacter;
								MaxDot = Dot;
							}
							else if (Dot > MaxDot)
							{
								TargetCharacter = OtherCharacter;
								MaxDot = Dot;
							}
						}
					}
				}
			}
		}

		if (IsValid(TargetCharacter))
		{
			FVector TargetLocation = TargetCharacter->GetLagCompMesh()->GetBoneLocation("head");	

			FVector StartLocation = GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f) + GetPawn()->GetVelocity() * DeltaTime;

			ControlRotation = (TargetLocation - StartLocation).Rotation();
			
			RotationInput = FRotator::ZeroRotator;
		}
	}	
	
	Super::UpdateRotation(DeltaTime);
}

void APlayerController_FPS::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerController_FPS, AllShots)
	DOREPLIFETIME(APlayerController_FPS, MatchedShots)
	DOREPLIFETIME(APlayerController_FPS, MatchedHitShots)
	DOREPLIFETIME(APlayerController_FPS, MatchedMissedShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedEqualDamageShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalDamageShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalMoreDamageShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalMoreDamageClientHitShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalMoreDamageClientMissedShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalLessDamageShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalLessDamageServerHitShots)
	DOREPLIFETIME(APlayerController_FPS, MismatchedUnequalLessDamageServerMissedShots)

	DOREPLIFETIME(APlayerController_FPS, HeadshotTruePositives)
	DOREPLIFETIME(APlayerController_FPS, HeadshotFalsePositives)
	DOREPLIFETIME(APlayerController_FPS, HeadshotFalseNegatives)
}

void APlayerController_FPS::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAxis("Turn", this, &APlayerController::AddYawInput);
	InputComponent->BindAxis("LookUp", this, &APlayerController::AddPitchInput);

	InputComponent->BindAction("Scoreboard", IE_Pressed, this, &APlayerController_FPS::ShowScoreboard);
	InputComponent->BindAction("Scoreboard", IE_Released, this, &APlayerController_FPS::HideScoreboard);

	InputComponent->BindAction("Statistics", IE_Pressed, this, &APlayerController_FPS::StatisticsPressed);

	InputComponent->BindAction("Escape", IE_Pressed, this, &APlayerController_FPS::EscapePressed);
	InputComponent->BindAction("Info", IE_Pressed, this, &APlayerController_FPS::InfoPressed);

	InputComponent->BindAction("Aimbot", IE_Pressed, this, &APlayerController_FPS::AimbotPressed);
}

void APlayerController_FPS::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);

	if (bBeginPlayCalled)
	{
		OnPawnChanged();
	}
}

int APlayerController_FPS::GetPing() const
{
	if (IsValid(PlayerState))
	{
		return FMath::FloorToInt(PlayerState->ExactPingV2);
	}

	return 0;
}

float APlayerController_FPS::GetHealthNormalized() const
{
	if (IsValid(GetPawn()))
	{
		return GetPawn<ACharacter_FPS>()->GetHealthNormalized();
	}

	return 0.0f;
}

void APlayerController_FPS::OnPlayerKilled(APlayerController* KilledBy, uint8 WeaponTypeIndex, bool bHeadshot)
{
	if (GetWorld()->IsServer())
	{
		ACharacter_FPS* KilledCharacter = GetPawn<ACharacter_FPS>();

		GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &APlayerController_FPS::RespawnPlayer, KilledCharacter), 5.0f, false);

		GetWorld()->GetGameState<AGameState_FPS>()->PlayerKilled(KilledBy->PlayerState, PlayerState, WeaponTypeIndex, bHeadshot);
	}
}

void APlayerController_FPS::AddYawInput(float Val)
{
	if (!bInGameMenuVisible)
	{
		float InputMultiplier = 1.0f;

		ACharacter_FPS* PlayerCharacter = GetPawn<ACharacter_FPS>();

		if (IsValid(PlayerCharacter))
		{
			switch (PlayerCharacter->GetAimType())
			{
			case AIM_ADS:
				{
					InputMultiplier = FMath::Lerp(HipFireMouseSensitivity, IronsightsMouseSensitivity, GetPawn<ACharacter_FPS>()->GetIronsightsAlpha());
				}
				break;
			case AIM_SCOPE:
				{
					InputMultiplier = FMath::Lerp(HipFireMouseSensitivity, ScopeMouseSensitivity, GetPawn<ACharacter_FPS>()->GetIronsightsAlpha());
				}
				break;
			}
		}
	
		Super::AddYawInput(Val * InputMultiplier);
	}
}

void APlayerController_FPS::AddPitchInput(float Val)
{
	if (!bInGameMenuVisible)
	{
		float InputMultiplier = 1.0f;

		ACharacter_FPS* PlayerCharacter = GetPawn<ACharacter_FPS>();

		if (IsValid(PlayerCharacter))
		{
			switch (PlayerCharacter->GetAimType())
			{
			case AIM_ADS:
				{
					InputMultiplier = FMath::Lerp(HipFireMouseSensitivity, IronsightsMouseSensitivity, GetPawn<ACharacter_FPS>()->GetIronsightsAlpha());
				}
				break;
			case AIM_SCOPE:
				{
					InputMultiplier = FMath::Lerp(HipFireMouseSensitivity, ScopeMouseSensitivity, GetPawn<ACharacter_FPS>()->GetIronsightsAlpha());
				}
				break;
			}
		}
	
		Super::AddPitchInput(Val * InputMultiplier);
	}
}

int APlayerController_FPS::GetCurrentAmmo() const
{
	ACharacter_FPS* OwnedCharacter = GetPawn<ACharacter_FPS>();
	
	if (IsValid(OwnedCharacter))
	{
		return OwnedCharacter->GetCurrentAmmo();
	}

	return 0;
}

int APlayerController_FPS::GetMaxAmmo() const
{
	ACharacter_FPS* OwnedCharacter = GetPawn<ACharacter_FPS>();
	
	if (IsValid(OwnedCharacter))
	{
		return OwnedCharacter->GetMaxAmmo();
	}

	return 0;
}

UTexture* APlayerController_FPS::GetSelectedWeaponImage() const
{
	ACharacter_FPS* OwnedCharacter = GetPawn<ACharacter_FPS>();
	
	if (IsValid(OwnedCharacter) && IsValid(OwnedCharacter->GetSelectedWeapon()))
	{
		return OwnedCharacter->GetSelectedWeapon()->GetWeaponImage();
	}

	return nullptr;
}

float APlayerController_FPS::GetInPacketLoss() const
{
	if (const UNetConnection* MyNetConnection = GetNetConnection())
	{
		return MyNetConnection->GetInLossPercentage().GetAvgLossPercentage();
	}
	return 0;
}

float APlayerController_FPS::GetOutPacketLoss() const
{
	if (const UNetConnection* MyNetConnection = GetNetConnection())
	{
		return MyNetConnection->GetOutLossPercentage().GetAvgLossPercentage();
	}
	return 0;
}

UTexture* APlayerController_FPS::GetWeaponImage(uint8 WeaponTypeIndex) const
{
	AGameState_FPS* GameState = GetWorld()->GetGameState<AGameState_FPS>();

	if (IsValid(GameState) && IsValid(GameState->GameModeClass))
	{
		return GameState->GetWeaponImage(WeaponTypeIndex);
	}

	return nullptr;
}

bool APlayerController_FPS::GetNextKillFeedEvent(FKillFeedEvent& NextKillFeedEvent)
{
	return KillFeedEventQueue.Dequeue(NextKillFeedEvent);
}

void APlayerController_FPS::AddKillFeedEvent(FString LeftPlayerName, FString RightPlayerName, uint8 WeaponTypeIndex, bool bHeadshot)
{
	KillFeedEventQueue.Enqueue(FKillFeedEvent{LeftPlayerName, RightPlayerName, WeaponTypeIndex, bHeadshot});
	OnNewKillFeedEvent.Broadcast();
}

void APlayerController_FPS::SetWinner(APlayerState* WinnerPlayerState)
{
	OnWinnerEvent.Broadcast(WinnerPlayerState->GetPlayerName());
	WinnerWidget->SetVisibility(ESlateVisibility::Visible);
}

void APlayerController_FPS::OnPawnChanged()
{
	if (IsLocalPlayerController())
	{
		ACharacter_FPS* PlayerCharacter = GetPawn<ACharacter_FPS>();
		
		if (IsValid(PlayerCharacter) && PlayerCharacter != LastOwnedCharacter)
		{			
			PlayerCharacter->OnHealthChanged.AddDynamic(this, &APlayerController_FPS::OnUpdateHealthBroadcast);
			PlayerCharacter->OnIronsightsStarted.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->OnIronsightsEnded.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnReloadStarted.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnReloadFinished.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnSwitchingStarted.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnSwitchingFinished.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnSprintStarted.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->GetCharacterMovement_FPS()->OnSprintFinished.AddDynamic(this, &APlayerController_FPS::OnUpdateCrosshair);
			PlayerCharacter->MovementModeChangedDelegate.AddDynamic(this, &APlayerController_FPS::OnMovementModeChanged);
			PlayerCharacter->GetCharacterMovement_FPS()->OnAmmoChanged.AddDynamic(this, &APlayerController_FPS::OnAmmoChangedEvent);
			PlayerCharacter->OnScopeViewChanged.AddDynamic(this, &APlayerController_FPS::OnScopeViewChanged);
			PlayerCharacter->OnWeaponChanged.AddDynamic(this, &APlayerController_FPS::OnWeaponChangedEvent);

			HealthWidget->SetVisibility(ESlateVisibility::Visible);
			AmmoWidget->SetVisibility(ESlateVisibility::Visible);

			OnUpdateHealth.Broadcast();
			
			LastOwnedCharacter = PlayerCharacter;
		}
		else if (!IsValid(PlayerCharacter))
		{
			HealthWidget->SetVisibility(ESlateVisibility::Hidden);
			AmmoWidget->SetVisibility(ESlateVisibility::Hidden);
			ScopeWidget->SetVisibility(ESlateVisibility::Hidden);

			if (IsValid(LastOwnedCharacter))
			{
				LastOwnedCharacter->SwitchToThirdPersonView();
				GetTransformComponent()->SetWorldLocation(LastOwnedCharacter->GetActorLocation());
			}
		}

		OnUpdateCrosshair();
	}
}

void APlayerController_FPS::OnUpdateHealthBroadcast()
{
	OnUpdateHealth.Broadcast();
}

void APlayerController_FPS::RespawnPlayer(ACharacter_FPS* KilledCharacter)
{
	KilledCharacter->Destroy();
	GetWorld()->GetAuthGameMode()->RestartPlayer(this);
}

void APlayerController_FPS::ShowScoreboard()
{
	if (IsValid(ScoreWidget))
	{
		ScoreWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void APlayerController_FPS::HideScoreboard()
{
	if (IsValid(ScoreWidget))
	{
		ScoreWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerController_FPS::OnUpdateCrosshair()
{
	ACharacter_FPS* PlayerCharacter = GetPawn<ACharacter_FPS>();
	
	if (IsValid(PlayerCharacter) && !PlayerCharacter->GetIronsights() && !PlayerCharacter->GetCharacterMovement_FPS()->IsReloading() && !PlayerCharacter->GetCharacterMovement_FPS()->IsSwitching() && PlayerCharacter->GetCharacterMovement_FPS()->GetSprintingAlpha() == 0.0f)
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerController_FPS::OnScopeViewChanged()
{
	ACharacter_FPS* PlayerCharacter = GetPawn<ACharacter_FPS>();

	if (IsValid(PlayerCharacter) && PlayerCharacter->IsInScopeView())
	{
		ScopeWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		ScopeWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerController_FPS::OnMovementModeChanged(class ACharacter* OwnedCharacter, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	OnUpdateCrosshair();
}

void APlayerController_FPS::EscapePressed()
{
	if (!bInGameMenuVisible)
	{
		bInGameMenuVisible = true;
		
		InGameMenuWidget->SetVisibility(ESlateVisibility::Visible);
		if (IsValid(GetPawn()))
		{
			GetPawn()->DisableInput(this);
			GetPawn<ACharacter_FPS>()->IronsightsReleased();
			GetPawn<ACharacter_FPS>()->GetCharacterMovement_FPS()->JumpReleased();
			GetPawn<ACharacter_FPS>()->IronsightsReleased();
			GetPawn<ACharacter_FPS>()->SprintReleased();
			GetPawn<ACharacter_FPS>()->GetCharacterMovement_FPS()->FireReleased();
		}

		SetShowMouseCursor(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetWidgetToFocus(InGameMenuWidget->TakeWidget());
		SetInputMode(InputMode);
	}
	else
	{
		bInGameMenuVisible = false;
		
		InGameMenuWidget->SetVisibility(ESlateVisibility::Hidden);

		if (IsValid(GetPawn()))
		{
			GetPawn()->EnableInput(this);
		}
		
		GameUserSettings->SaveSettings();
		HipFireMouseSensitivity = GameUserSettings->GetHipFireMouseSensitivity();
		IronsightsMouseSensitivity = GameUserSettings->GetIronsightsMouseSensitivity();
		ScopeMouseSensitivity = GameUserSettings->GetScopeMouseSensitivity();
		ScopeFOV = GameUserSettings->GetScopeFOV();

		SetShowMouseCursor(false);

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void APlayerController_FPS::InfoPressed()
{
	bHitscanDebugInfoActive = !bHitscanDebugInfoActive;

	if (bHitscanDebugInfoActive)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::White, TEXT("Hitscan debug info ENABLED."));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::White, TEXT("Hitscan debug info DISABLED."));
	}

	SwitchDebugInfo(bHitscanDebugInfoActive);
}

void APlayerController_FPS::AimbotPressed()
{
	bAimbotActive = !bAimbotActive;

	if (bAimbotActive)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Magenta, TEXT("AIMBOT ENABLED"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Magenta, TEXT("AIMBOT DISABLED"));
	}
}

void APlayerController_FPS::StatisticsPressed()
{
	bStatisticsVisible = !bStatisticsVisible;

	if (bStatisticsVisible)
	{
		StatisticsWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		StatisticsWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void APlayerController_FPS::SwitchDebugInfo_Implementation(bool bNewHitscanDebugInfoActive)
{
	bHitscanDebugInfoActive = bNewHitscanDebugInfoActive;
}

void APlayerController_FPS::OnAmmoChangedEvent()
{
	OnAmmoChanged.Broadcast();
}

void APlayerController_FPS::OnWeaponChangedEvent()
{
	OnWeaponChanged.Broadcast();
}

void APlayerController_FPS::HitFeedback_Implementation(APlayerState* DamagedPlayer, uint8 BoneIndex, uint8 WeaponTypeIndex, bool bKilled, float RewindTime, uint8 ClientInterpID, uint8 ClientHitBoneIndex)
{
	AGameState_FPS* GameState = GetWorld()->GetGameState<AGameState_FPS>();

	if (IsValid(GameState) && IsValid(GameState->GameModeClass))
	{
		bool bHeadshot = GameState->IsHeadBoneIndex(BoneIndex);

		float Damage = GameState->GetWeaponBaseDamage(WeaponTypeIndex) * GameState->GetBoneDamageMultiplierByIndex(BoneIndex);

		if (IsValid(DamagedPlayer))
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("[Server] Hit: %s, damage: %.0f, bone: %s, time: %.3f"), *DamagedPlayer->GetPlayerName(), Damage, *GameState->GetBoneNameByIndex(BoneIndex).ToString(), RewindTime));
		}

		OnHitEvent.Broadcast(bKilled, bHeadshot);
	}
}
