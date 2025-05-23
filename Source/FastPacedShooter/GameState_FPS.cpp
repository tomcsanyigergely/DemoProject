// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState_FPS.h"

#include "Character_FPS.h"
#include "PlayerController_FPS.h"
#include "PlayerState_FPS.h"
#include "privablic.h"
#include "ScoreboardListItem.h"
#include "SnapshotReplicator.h"
#include "SocketSubsystem.h"
#include "Net/UnrealNetwork.h"

struct NetConnection_LastOSReceiveTime { typedef FPacketTimestamp(UNetConnection::*type); };
template struct privablic::private_member<NetConnection_LastOSReceiveTime, &UNetConnection::LastOSReceiveTime>;

struct NetConnection_IsOSReceiveTimeLocal { typedef bool(UNetConnection::*type); };
template struct privablic::private_member<
	NetConnection_IsOSReceiveTimeLocal, &UNetConnection::bIsOSReceiveTimeLocal>;

AGameState_FPS::AGameState_FPS()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGameState_FPS::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->IsServer())
	{
		SnapshotReplicator = GetWorld()->SpawnActor<ASnapshotReplicator>();

		for(int i = 1; i <= 255; i++)
		{
			InterpolationIDPool.Enqueue(i);
		}
	}

	for(int i = 0; i < Hitbox.Num(); i++)
	{
		HitboxIndices.Add(Hitbox[i].BoneName, i);

		if (Hitbox[i].BoneName == "head")
		{
			HeadBoneIndex = i;
		}
	}

	RewindDelta = RewindDeltaInSeconds * SnapshotsPerSecond;
	HardSmoothnessRange = HardSmoothnessRangeSeconds * SnapshotsPerSecond;
	SoftSmoothnessRange = SoftSmoothnessRangeSeconds * SnapshotsPerSecond;
}

void AGameState_FPS::Tick(float DeltaSeconds)
{	
	Super::Tick(DeltaSeconds);

	if (GetWorld()->IsServer())
	{
		ServerTick++;
		LastTickPlatformTime = FPlatformTime::Seconds();

		OnServerTick.Broadcast(DeltaSeconds);

		FGameSnapshot SnapshotPackedBits;

		CreateSnapshot(SnapshotPackedBits);

		SendSnapshot(SnapshotPackedBits);
	}
}

void AGameState_FPS::SendSnapshot(const FGameSnapshot& PackedBits)
{
	SnapshotReplicator->SendSnapshot(PackedBits);
}

void AGameState_FPS::ReceiveSnapshot(const FGameSnapshot& PackedBits)
{
	if (GetWorld()->IsClient())
	{
		ConsumeSnapshot(PackedBits);

		if (ServerTick > SnapshotTimeBuffer[LastSnapshotIndex].ServerTick)
		{
			SnapshotTimeBufferElement NewSnapshotTimeBufferElement;
			NewSnapshotTimeBufferElement.LocalTime = GetWorld()->GetTimeSeconds();
			NewSnapshotTimeBufferElement.ServerTick = ServerTick;

			SnapshotTimeBuffer[++LastSnapshotIndex] = NewSnapshotTimeBufferElement;
		
			UpdateInterpolationParameters();
		}
	}
}

void AGameState_FPS::AddToInterpolatedCharacters(ACharacter_FPS* Character)
{
	if (GetWorld()->IsServer())
	{
		if (Character->HasValidInterpolationID())
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): Character already has a valid InterpolationID."))
			return;
		}

		if (InterpolationIDPool.IsEmpty())
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): InterpolationID pool is empty."))
			return;
		}

		uint8 InterpolationID = *InterpolationIDPool.Peek();

		if (InterpolatedCharactersMap.Contains(InterpolationID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): InterpolatedCharacters map already contains a character with the same InterpolationID."))
			return;
		}

		if (InterpolatedCharactersSet.Contains(Character))
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): InterpolatedCharacters set already contains this character."))
			return;
		}
	
		InterpolationIDPool.Pop();

		Character->AssignInterpolationID(InterpolationID);

		InterpolatedCharactersMap.Add(InterpolationID, Character);
		InterpolatedCharactersSet.Add(Character);

		UE_LOG(LogTemp, Warning, TEXT("Added to interpolated characters: %s"), *Character->GetName())
	}
	else
	{
		if (!Character->HasValidInterpolationID())
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): Character has no valid InterpolationID."))
			return;
		}

		uint8 InterpolationID = Character->GetInterpolationID();

		if (InterpolatedCharactersMap.Contains(InterpolationID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): InterpolatedCharacters map already contains a character with the same InterpolationID: %d"), InterpolationID)
			return;
		}

		if (InterpolatedCharactersSet.Contains(Character))
		{
			UE_LOG(LogTemp, Fatal, TEXT("AddToInterpolatedCharacters(): InterpolatedCharacters set already contains this character."))
			return;
		}

		InterpolatedCharactersMap.Add(InterpolationID, Character);
		InterpolatedCharactersSet.Add(Character);

		//GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Green, FString::Printf(TEXT("Character %s added to interpolated characters with ID %d"), *Character->GetName(), InterpolationID));
	}
}

void AGameState_FPS::RemoveFromInterpolatedCharacters(ACharacter_FPS* Character)
{
	if (GetWorld()->IsServer())
	{
		if (!Character->HasValidInterpolationID())
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): Character has no valid InterpolationID."))
			return;
		}

		uint8 InterpolationID = Character->GetInterpolationID();

		if (!InterpolatedCharactersMap.Contains(InterpolationID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): InterpolatedCharacters map does not contain the removed InterpolationID."))
			return;
		}

		if (!InterpolatedCharactersSet.Contains(Character))
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): InterpolatedCharacters set does not contain the removed character."))
			return;
		}

		InterpolatedCharactersMap.Remove(InterpolationID);
		InterpolatedCharactersSet.Remove(Character);

		Character->RemoveInterpolationID();

		InterpolationIDPool.Enqueue(InterpolationID);

		UE_LOG(LogTemp, Warning, TEXT("Removed from interpolated characters: %s"), *Character->GetName())
	}
	else
	{
		if (!Character->HasValidInterpolationID())
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): Character has no valid InterpolationID."))
			return;
		}

		uint8 InterpolationID = Character->GetInterpolationID();

		if (!InterpolatedCharactersMap.Contains(InterpolationID))
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): InterpolatedCharacters map does not contain the removed InterpolationID: %d"), InterpolationID)
			return;
		}

		if (!InterpolatedCharactersSet.Contains(Character))
		{
			UE_LOG(LogTemp, Fatal, TEXT("RemoveFromInterpolatedCharacters(): InterpolatedCharacters set does not contain the removed character."))
			return;
		}

		InterpolatedCharactersMap.Remove(InterpolationID);
		InterpolatedCharactersSet.Remove(Character);

		//GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Purple, FString::Printf(TEXT("Character %s removed from interpolated characters with ID %d"), *Character->GetName(), InterpolationID));
	}
}

bool AGameState_FPS::IsInterpolated(ACharacter_FPS* Character)
{
	return InterpolatedCharactersSet.Contains(Character);
}

TSet<ACharacter_FPS*> AGameState_FPS::RewindAllCharactersExcept(float RewindTime, ACharacter_FPS* SkipRewindCharacter)
{
	TSet<ACharacter_FPS*> LagCompensatedCharacters;
	
	for(auto Character : InterpolatedCharactersSet)
	{
		if (Character != SkipRewindCharacter && Character->CanRewindTo(RewindTime))
		{
			Character->Rewind(RewindTime);

			LagCompensatedCharacters.Add(Character);
		}
	}

	return LagCompensatedCharacters;
}

void AGameState_FPS::RestoreAllCharacters()
{
	for(auto Character : InterpolatedCharactersSet)
	{
		Character->Restore();
	}
}

/*static IConsoleVariable* UseRecvMultiCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.UseRecvMulti"));
	static IConsoleVariable* UseRecvTimestampsCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.UseRecvTimestamps"));*/

float AGameState_FPS::GetRewindTime(APlayerController* Player, float TargetRewindTime) const
{
	UNetConnection* NetConnection = Player->GetNetConnection();

	ISocketSubsystem* SocketSubsystem = NetConnection->GetDriver()->GetSocketSubsystem();
				
	float PacketDelta = 0;

	if (SocketSubsystem && SocketSubsystem->IsSocketRecvMultiSupported())
	{
		auto LastOSReceiveTime = NetConnection->*privablic::member<NetConnection_LastOSReceiveTime>::value;
		auto bIsOSReceiveTimeLocal = NetConnection->*privablic::member<NetConnection_IsOSReceiveTimeLocal>::value;

		double PacketReceiveTime;
		FTimespan& RecvTimespan = LastOSReceiveTime.Timestamp;

		if (bIsOSReceiveTimeLocal)
		{
			PacketReceiveTime = RecvTimespan.GetTotalSeconds();
		}
		else
		{
			PacketReceiveTime = SocketSubsystem->TranslatePacketTimestamp(LastOSReceiveTime, ETimestampTranslation::LocalTimestamp);
		}

		PacketDelta = PacketReceiveTime - LastTickPlatformTime;
	}

	float RTT = Player->PlayerState->ExactPingV2 * 0.001;
	
	float AuthRewindTime = ServerTick + PacketDelta * SnapshotsPerSecond - (RTT + InterpolationDelay) * SnapshotsPerSecond;

	Cast<APlayerController_FPS>(Player)->AuthRewindTime = AuthRewindTime;
	
	return FMath::Clamp(TargetRewindTime, AuthRewindTime - RewindDelta, AuthRewindTime + RewindDelta);
}

void AGameState_FPS::UpdateSimulatedProxies(float DeltaTime)
{
	UpdateInterpolationTime(DeltaTime);

	UE_LOG(LogTemp, Warning, TEXT("InterpolationTime: %f"), InterpolationTime)
	
	for(ACharacter_FPS* Character : InterpolatedCharactersSet)
	{
		if (Character->GetLocalRole() == ROLE_SimulatedProxy)
		{			
			Character->UpdateSimulatedProxy(InterpolationTime);
		}
	}
}

void AGameState_FPS::CreateSnapshot(FGameSnapshot& SnapshotPackedBits)
{	
	SnapshotPackedBits.ServerTick = ServerTick;
	
	for(auto Iterator = InterpolatedCharactersMap.CreateConstIterator(); Iterator; ++Iterator)
	{
		uint8 InterpolationID = Iterator.Key();
		ACharacter_FPS* Character = Iterator.Value();

		if (IsValid(Character))
		{
			FPlayerSnapshot PlayerSnapshot;
			PlayerSnapshot.InterpolationID = InterpolationID;
			Character->CreatePlayerSnapshot(ServerTick, PlayerSnapshot);
			SnapshotPackedBits.PlayerSnapshots.Add(PlayerSnapshot);
		}
	}
}

void AGameState_FPS::ConsumeSnapshot(const FGameSnapshot& SnapshotPackedBits)
{
	ServerTick = SnapshotPackedBits.ServerTick;
	
	if (APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController()) 
	{
		for(const FPlayerSnapshot& PlayerSnapshot : SnapshotPackedBits.PlayerSnapshots)
		{
			if (InterpolatedCharactersMap.Contains(PlayerSnapshot.InterpolationID))
			{
				ACharacter_FPS* Character = InterpolatedCharactersMap[PlayerSnapshot.InterpolationID];

				if (IsValid(Character) && Character != LocalPlayerController->GetPawn())
				{
					Character->ConsumePlayerSnapshot(ServerTick, PlayerSnapshot);
				}
			}
		}
	}
}

void AGameState_FPS::UpdateInterpolationParameters()
{
	float MeanX = 0;
	float MeanY = 0;

	uint8 NumSnapshots = TimeSynchronizationWindow;
	
	for(uint8 i = 0; i < NumSnapshots; i++)
	{
		uint8 Index = LastSnapshotIndex - i;
		MeanX += SnapshotTimeBuffer[Index].LocalTime;
		MeanY += SnapshotTimeBuffer[Index].ServerTick;
	}

	MeanX /= NumSnapshots;
	MeanY /= NumSnapshots;

	float Numerator = 0;
	float Denominator = 0;

	for(uint8 i = 0; i < NumSnapshots; i++)
	{
		uint8 Index = LastSnapshotIndex - i;
		float DiffX = SnapshotTimeBuffer[Index].LocalTime - MeanX;
		float DiffY = SnapshotTimeBuffer[Index].ServerTick - MeanY;
		Numerator += DiffX * DiffY;
		Denominator += DiffX * DiffX;
	}

	TimestampBufferLineSlope = FMath::Clamp(Numerator / Denominator, -1000.0f, 1000.0f); // prevent NaN
	TimestampBufferLineOffset = MeanY - TimestampBufferLineSlope * MeanX;
}

void AGameState_FPS::UpdateInterpolationTime(float DeltaTime)
{
	float LocalTime = GetWorld()->GetTimeSeconds();
	
	if (LastUpdateTime != LocalTime)
	{
		float TargetInterpolationTime = TimestampBufferLineSlope * (GetWorld()->GetTimeSeconds() - InterpolationDelay) + TimestampBufferLineOffset;
		
		InterpolationTime += SnapshotsPerSecond * DeltaTime;		

		float Error = TargetInterpolationTime - InterpolationTime;

		if (FMath::Abs(Error) <= SoftSmoothnessRange)
		{
			InterpolationTime += FMath::Clamp(Error, -SoftSmoothnessFactor * SnapshotsPerSecond * DeltaTime, SoftSmoothnessFactor * SnapshotsPerSecond * DeltaTime);
		}
		else if (FMath::Abs(Error) <= HardSmoothnessRange)
		{
			InterpolationTime += FMath::Clamp(Error, -HardSmoothnessFactor * SnapshotsPerSecond * DeltaTime, HardSmoothnessFactor * SnapshotsPerSecond * DeltaTime);
		}
		else
		{
			InterpolationTime = TargetInterpolationTime;
		}

		LastUpdateTime = LocalTime;
	}
}

void AGameState_FPS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGameState_FPS, WinnerPlayerState);
}

void AGameState_FPS::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	APlayerState_FPS* PlayerState_FPS = Cast<APlayerState_FPS>(PlayerState);

	if (IsValid(PlayerState_FPS))
	{
		PlayerState_FPS->OnPlayerScoreChanged.AddDynamic(this, &AGameState_FPS::OnPlayerScoreChanged);

		OnScoreChanged.Broadcast();
	}
}

void AGameState_FPS::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	OnScoreChanged.Broadcast();
}

void AGameState_FPS::ReceivedGameModeClass()
{
	Super::ReceivedGameModeClass();

	if (ACharacter_FPS* DefaultPawnCDO = Cast<ACharacter_FPS>(GetDefaultGameMode()->DefaultPawnClass.GetDefaultObject()))
	{
		const TArray<TSubclassOf<AWeapon_FPS>>& WeaponTypes = DefaultPawnCDO->GetWeaponTypes();

		for(int i = 0; i < WeaponTypes.Num(); i++)
		{
			WeaponCDOs.Add(WeaponTypes[i].GetDefaultObject());
		}
	}
}

TArray<UObject*> AGameState_FPS::GetScoreboardList()
{
	TArray<UObject*> ScoreboardList;
	
	struct FPlayerState
	{
		APlayerState_FPS* PlayerState;
	};

	TArray<FPlayerState> PlayerStates;

	for(auto PlayerArrayElement : PlayerArray)
	{
		APlayerState_FPS* PlayerState = Cast<APlayerState_FPS>(PlayerArrayElement);

		if (IsValid(PlayerState))
		{
			PlayerStates.Add(FPlayerState{PlayerState});
		}
	}

	PlayerStates.Sort([](const FPlayerState& LHS, const FPlayerState& RHS)
	{
		return LHS.PlayerState->GetKills() > RHS.PlayerState->GetKills();
	});

	for(auto PlayerArrayElement : PlayerStates)
	{
		AddPlayerToScoreboardList(PlayerArrayElement.PlayerState, false, ScoreboardList);
	}

	return ScoreboardList;
}

float AGameState_FPS::GetWeaponBaseDamage(uint8 WeaponTypeIndex) const
{	
	if (WeaponTypeIndex < WeaponCDOs.Num())
	{
		return WeaponCDOs[WeaponTypeIndex]->GetBaseDamage();
	}

	return 0;
}

float AGameState_FPS::GetWeaponFireInterval(uint8 WeaponTypeIndex) const
{
	if (WeaponTypeIndex < WeaponCDOs.Num())
	{
		return WeaponCDOs[WeaponTypeIndex]->GetFireInterval();
	}

	return 0;
}

UTexture* AGameState_FPS::GetWeaponImage(uint8 WeaponTypeIndex) const
{
	if (WeaponTypeIndex < WeaponCDOs.Num())
	{
		return WeaponCDOs[WeaponTypeIndex]->GetWeaponImage();
	}

	return nullptr;
}

void AGameState_FPS::OnRep_WinnerPlayerState()
{
	if (GetWorld()->IsClient())
	{
		if (IsValid(WinnerPlayerState))
		{
			GetWorld()->GetFirstPlayerController<APlayerController_FPS>()->SetWinner(WinnerPlayerState);
		}
	}
}

void AGameState_FPS::OnPlayerScoreChanged()
{
	OnScoreChanged.Broadcast();
}

void AGameState_FPS::AddPlayerToScoreboardList(APlayerState_FPS* PlayerState, bool bLocalPlayerState, TArray<UObject*>& ScoreboardList)
{
	UScoreboardListItem* ScoreboardListItem = NewObject<UScoreboardListItem>(this, UScoreboardListItem::StaticClass());

	ScoreboardListItem->bLocalPlayerState = bLocalPlayerState;
	ScoreboardListItem->PlayerName = PlayerState->GetPlayerName();
	ScoreboardListItem->Kills = PlayerState->GetKills();
	ScoreboardListItem->Deaths = PlayerState->GetDeaths();

	ScoreboardList.Add(ScoreboardListItem);
}

void AGameState_FPS::InitializeRemoteAudioIndices()
{
	if (!RemoteAudioIndicesInitialized)
	{
		RemoteAudioIndices.Enqueue(1);
		RemoteAudioIndices.Enqueue(2);
		RemoteAudioIndices.Enqueue(3);

		RemoteAudioIndicesInitialized = true;
	}
}

void AGameState_FPS::PlayerKilled_Implementation(APlayerState* DamageCauserPlayerState, APlayerState* KilledPlayerState, uint8 WeaponTypeIndex, bool bHeadshot)
{
	if (GetWorld()->IsClient() && IsValid(DamageCauserPlayerState) && IsValid(KilledPlayerState) && IsValid(GameModeClass))
	{		
		GetWorld()->GetFirstPlayerController<APlayerController_FPS>()->AddKillFeedEvent(DamageCauserPlayerState->GetPlayerName(), KilledPlayerState->GetPlayerName(), WeaponTypeIndex, bHeadshot);
	}
}

