// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode_FPS.h"

#include "CharacterMovementComponent_FPS.h"
#include "Character_FPS.h"
#include "GameState_FPS.h"
#include "PlayerState_FPS.h"
#include "GameFramework/GameState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void AGameMode_FPS::BeginPlay()
{
	Super::BeginPlay();

	GameState_FPS = Cast<AGameState_FPS>(GameState);

	if (IsValid(GameState_FPS))
	{
		GameState_FPS->OnServerTick.AddDynamic(this, &AGameMode_FPS::OnTick);
	}
}

void AGameMode_FPS::OnTick(float DeltaTime)
{
	TArray<ACharacter_FPS*> PlayerCharacters;

	for (APlayerState* Player : GameState->PlayerArray)
	{
		if (ACharacter_FPS* PlayerCharacter = Player->GetPawn<ACharacter_FPS>())
		{
			if (!PlayerCharacter->IsKilled())
			{
				PlayerCharacters.Add(PlayerCharacter);
			}
		}
	}

	for (ACharacter_FPS* PlayerCharacter : PlayerCharacters)
	{
	    PlayerCharacter->ApplyShootCommands();
	}

	GetGameState<AGameState_FPS>()->RestoreAllCharacters();

	for (ACharacter_FPS* PlayerCharacter : PlayerCharacters)
	{
		if (IsValid(PlayerCharacter->GetController()) && PlayerCharacter->IsKilled())
		{
			PlayerCharacter->GetController()->UnPossess();
		}
	}

	float Now = GetWorld()->GetTimeSeconds();

	for (ACharacter_FPS* PlayerCharacter : PlayerCharacters)
	{
		if (!PlayerCharacter->IsKilled() && Now - PlayerCharacter->GetLastDamageTime() >= 6.0f)
		{
			PlayerCharacter->Heal();
		}
	}

	if (IsValid(GameState_FPS) && !GameState_FPS->IsGameFinished())
	{
		int MaxKills = 0;
		int NumPlayersWithMaxKills = 0;
		APlayerState* PlayerWithMaxKills = nullptr;

		for (APlayerState* Player : GameState->PlayerArray)
		{
			APlayerState_FPS* PlayerState = Cast<APlayerState_FPS>(Player);

			if (IsValid(PlayerState))
			{
				if (PlayerState->GetKills() > MaxKills)
				{
					MaxKills = PlayerState->GetKills();
					NumPlayersWithMaxKills = 0;
					PlayerWithMaxKills = PlayerState;
				}

				if (PlayerState->GetKills() == MaxKills)
				{
					NumPlayersWithMaxKills++;
				}
			}
		}
	
		if (MaxKills >= 30 && NumPlayersWithMaxKills == 1)
		{
			GameState_FPS->WinnerPlayerState = PlayerWithMaxKills;
			GameState_FPS->FinishGame();
		}
	}
}
