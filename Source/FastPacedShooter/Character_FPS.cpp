// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character_FPS.h"

#include "AnimInstance_FPS.h"
#include "CharacterMovementComponent_FPS.h"
#include "CollisionChannels.h"
#include "DrawDebugHelpers.h"
#include "GameMode_FPS.h"
#include "GameState_FPS.h"
#include "MeshAttributes.h"
#include "PlayerController_FPS.h"
#include "PlayerState_FPS.h"
#include "SocketSubsystem.h"
#include "Weapon_FPS.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

//////////////////////////////////////////////////////////////////////////
// AFastPacedShooterCharacter

ACharacter_FPS::ACharacter_FPS(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer.SetDefaultSubobjectClass<UCharacterMovementComponent_FPS>(ACharacter::CharacterMovementComponentName))
{
	LagCompMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LagCompMesh"));
	LagCompMesh->SetupAttachment(RootComponent);

	LagCompMesh->SetVisibility(false);
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	Camera->bUsePawnControlRotation = true;

	FirstPersonArmMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmMesh"));
	FirstPersonArmMesh->SetupAttachment(Camera);
	FirstPersonArmMesh->SetRelativeLocation(FVector(19.148914f, -26.427397f, -143.53006f));
	FirstPersonArmMesh->SetRelativeRotation(FRotator(9.978858f, 22.324265f, 9.034965f));
	FirstPersonArmMesh->SetCastShadow(false);
	FirstPersonArmMesh->SetOnlyOwnerSee(true);
	FirstPersonArmMesh->AddTickPrerequisiteComponent(CharacterMovementComponent_FPS);
	
	CharacterMovementComponent_FPS = Cast<UCharacterMovementComponent_FPS>(GetCharacterMovement());
}

void ACharacter_FPS::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACharacter_FPS, InterpolationID);

	DOREPLIFETIME_CONDITION(ACharacter_FPS, CurrentHealth, COND_OwnerOnly);
	DOREPLIFETIME(ACharacter_FPS, bKilled);
	DOREPLIFETIME(ACharacter_FPS, KilledByPawn);
}

void ACharacter_FPS::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	{
		static const FName ReplicatedPropertyName(TEXT("ReplicatedMovement"));
		static FProperty* ReplicatedProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedPropertyName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedProperty->RepIndex, false);
	}

	{
		static const FName ReplicatedPropertyName(TEXT("ReplicatedBasedMovement"));
		static FProperty* ReplicatedProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedPropertyName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedProperty->RepIndex, false);
	}

	{
		static const FName ReplicatedPropertyName(TEXT("ReplicatedMovementMode"));
		static FProperty* ReplicatedProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedPropertyName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedProperty->RepIndex, false);
	}

	{
		static const FName ReplicatedServerLastTransformUpdateTimeStampName(TEXT("ReplicatedServerLastTransformUpdateTimeStamp"));
		static FProperty* ReplicatedServerLastTransformUpdateTimeStampProperty = GetReplicatedProperty(StaticClass(), ACharacter::StaticClass(), ReplicatedServerLastTransformUpdateTimeStampName);
		ChangedPropertyTracker.SetCustomIsActiveOverride(this, ReplicatedServerLastTransformUpdateTimeStampProperty->RepIndex, false);
	}

	DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, RemoteViewPitch, false);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, bProxyIsJumpForceApplied, false);
	DOREPLIFETIME_ACTIVE_OVERRIDE(ACharacter, RepRootMotion, false);
}

void ACharacter_FPS::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (PreviousController != Controller)
	{
		if (IsValid(PreviousController) && !IsValid(Controller))
		{
			PreviousController->SetPawnFromRep(nullptr);
		}

		PreviousController = Controller;
	}
}

void ACharacter_FPS::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		GetMesh()->Deactivate();
		GetMesh()->SetHiddenInGame(true, true);
		GetMesh()->SetCastHiddenShadow(false);
		LagCompMesh->SetHiddenInGame(false, false);
		LagCompMesh->SetVisibility(true);
		bInterpolate = true;
		CharacterMovementComponent_FPS->SetComponentTickEnabled(false);
	}

	GameState = GetWorld()->GetGameState<AGameState_FPS>();

	while (!Delayed_OnRep_InterpolationID_Queue.IsEmpty())
	{
		uint8 NewInterpolationID = *Delayed_OnRep_InterpolationID_Queue.Peek();

		Delayed_OnRep_InterpolationID_Queue.Pop();

		InterpolationID = NewInterpolationID;
		
		Handle_OnRep_InterpolationID();
	}

	for(int i = 0; i < WeaponTypes.Num(); i++)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWeapon_FPS* Weapon = GetWorld()->SpawnActor<AWeapon_FPS>(WeaponTypes[i].Get(), FTransform{ FRotator::ZeroRotator, FVector::ZeroVector, FVector{1, 1, 1} }, Params);

		Weapon->GetWeaponMesh()->SetVisibility(i == SelectedWeaponIndex);
		
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Weapon->SetToFirstPersonMode();
			Weapon->AttachToComponent(FirstPersonArmMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName("hand_r"));
		}
		else
		{			
			Weapon->SetToThirdPersonMode();
			Weapon->AttachToComponent(LagCompMesh, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName("hand_r"));
		}
		
		Weapons.Add(Weapon);
	}

	if (GetWorld()->IsServer())
	{
		OnTakeAnyDamage.AddDynamic(this, &ACharacter_FPS::OnDamageTaken);
		GameState->AddToInterpolatedCharacters(this);
	}

	SetCurrentHealth(MaxHealth);

	SetSelectedWeapon(SelectedWeaponIndex);

	OnWeaponChanged.Broadcast();
}

void ACharacter_FPS::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		if (!bIronsights && bIronsightsPressed && !CharacterMovementComponent_FPS->IsReloading() && !CharacterMovementComponent_FPS->IsSwitching())
		{
			bIronsights = true;
			OnIronsightsStarted.Broadcast();
		}
		else if (bIronsights && !(bIronsightsPressed && !CharacterMovementComponent_FPS->IsReloading() && !CharacterMovementComponent_FPS->IsSwitching()))
		{
			bIronsights = false;
			OnIronsightsEnded.Broadcast();
		}

		if (bIronsights)
		{
			IronsightsAlpha = FMath::Clamp(IronsightsAlpha + DeltaSeconds / IronsightsTransitionDuration, 0.0f, 1.0f);
		}
		else
		{
			IronsightsAlpha = FMath::Clamp(IronsightsAlpha - DeltaSeconds / IronsightsTransitionDuration, 0.0f, 1.0f);
		}

		bool bScopeViewCondition = (GetSelectedWeapon()->GetAimType() == AIM_SCOPE) && (IronsightsAlpha == 1.0f);

		if (!bInScopeView && bScopeViewCondition)
		{
			bInScopeView = true;
			Camera->SetFieldOfView(GetController<APlayerController_FPS>()->GetScopeFOV());
			GetSelectedWeapon()->GetWeaponMesh()->SetVisibility(false);
			FirstPersonArmMesh->SetVisibility(false);
			OnScopeViewChanged.Broadcast();
		}
		else if (bInScopeView && !bScopeViewCondition)
		{
			bInScopeView = false;
			GetSelectedWeapon()->GetWeaponMesh()->SetVisibility(true);
			FirstPersonArmMesh->SetVisibility(true);
			OnScopeViewChanged.Broadcast();
		}

		if (!bInScopeView)
		{
			Camera->SetFieldOfView(FMath::Lerp(DefaultFOV, IronsightsFOV, IronsightsAlpha));
		}
	}
}

void ACharacter_FPS::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (HasValidInterpolationID())
	{
		GameState->RemoveFromInterpolatedCharacters(this);
	}

	for(AWeapon_FPS* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ACharacter_FPS::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACharacter_FPS::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACharacter_FPS::MoveRight);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::JumpPressed);
	PlayerInputComponent->BindAction("Jump", IE_Released, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::JumpReleased);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ACharacter_FPS::SprintPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ACharacter_FPS::SprintReleased);

	PlayerInputComponent->BindAction("Ironsights", IE_Pressed, this, &ACharacter_FPS::IronsightsPressed);
	PlayerInputComponent->BindAction("Ironsights", IE_Released, this, &ACharacter_FPS::IronsightsReleased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::FirePressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::FireReleased);

	PlayerInputComponent->BindAction("Reload", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::ReloadPressed);
	PlayerInputComponent->BindAction("SwitchWeapon", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::SwitchWeaponPressed);
	PlayerInputComponent->BindAction("SelectWeaponOne", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::SelectWeaponOnePressed);
	PlayerInputComponent->BindAction("SelectWeaponTwo", IE_Pressed, CharacterMovementComponent_FPS, &UCharacterMovementComponent_FPS::SelectWeaponTwoPressed);
}

void ACharacter_FPS::SetSelectedWeapon(uint8 NewSelectedWeaponIndex)
{
	if (SelectedWeaponIndex != NewSelectedWeaponIndex)
	{
		Weapons[SelectedWeaponIndex]->GetWeaponMesh()->SetVisibility(false);
		Weapons[NewSelectedWeaponIndex]->GetWeaponMesh()->SetVisibility(true);
		
		SelectedWeaponIndex = NewSelectedWeaponIndex;
	
		OnWeaponChanged.Broadcast();
	}
}

void ACharacter_FPS::IronsightsPressed()
{
	bIronsightsPressed = true;
	CharacterMovementComponent_FPS->EndSprinting();
}

void ACharacter_FPS::IronsightsReleased()
{
	bIronsightsPressed = false;

	if (bSprintPressed)
	{
		CharacterMovementComponent_FPS->StartSprinting();
	}
}

void ACharacter_FPS::SprintPressed()
{
	bSprintPressed = true;

	if (!bIronsightsPressed)
	{
		CharacterMovementComponent_FPS->StartSprinting();
	}
}

void ACharacter_FPS::SprintReleased()
{
	bSprintPressed = false;
	CharacterMovementComponent_FPS->EndSprinting();
}

void ACharacter_FPS::ReliableShootRPC_Implementation(float InterpolationTime)
{
	CharacterMovementComponent_FPS->HandleReliableShoot(InterpolationTime);
}

void ACharacter_FPS::SwitchToThirdPersonView()
{
	bool bKilledByPawn = IsValid(KilledByPawn);

	if (bKilledByPawn)
	{
		GetMesh()->SetVisibility(true);			
	}
	
	for(int i = 0; i < Weapons.Num(); i++)
	{
		Weapons[i]->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName("hand_r"));
		Weapons[i]->SetToThirdPersonMode();
		Weapons[i]->GetWeaponMesh()->SetVisibility(bKilledByPawn && i == SelectedWeaponIndex);
	}
}

uint8 ACharacter_FPS::GetCurrentAmmo() const
{
	return Weapons[SelectedWeaponIndex]->GetCurrentAmmo();
}

uint8 ACharacter_FPS::GetMaxAmmo() const
{
	return Weapons[SelectedWeaponIndex]->GetMaxAmmo();
}

void ACharacter_FPS::OnDamageTaken(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (GetWorld()->IsServer())
	{
		if (!bKilled)
		{						
			SetCurrentHealth(CurrentHealth - Damage);

			LastDamageTime = GetWorld()->GetTimeSeconds();

			if (APlayerController_FPS* InstigatedByPlayerController = Cast<APlayerController_FPS>(InstigatedBy))
			{
				uint8 HitBoneIndex = InstigatedByPlayerController->LastHitBoneIndex;
				float HitRewindTime = InstigatedByPlayerController->LastHitRewindTime;
				uint8 HitWeaponTypeIndex = InstigatedByPlayerController->LastHitWeaponTypeIndex;
				uint8 LastClientHitInterpID = InstigatedByPlayerController->LastClientHitInterpID;
				uint8 LastClientHitBoneIndex = InstigatedByPlayerController->LastClientHitBoneIndex;
			
				InstigatedByPlayerController->HitFeedback(GetPlayerState<APlayerState>(), HitBoneIndex, HitWeaponTypeIndex, CurrentHealth <= 0, HitRewindTime, LastClientHitInterpID, LastClientHitBoneIndex);

				if (CurrentHealth <= 0)
				{
					bKilled = true;

					if (APawn* DamageCauserPawn = Cast<APawn>(DamageCauser))
					{					
						KilledByPawn = DamageCauserPawn;
					}

					GetPlayerState<APlayerState_FPS>()->IncreaseDeaths();

					InstigatedBy->GetPlayerState<APlayerState_FPS>()->IncreaseKills();

					GetController<APlayerController_FPS>()->OnPlayerKilled(InstigatedByPlayerController, HitWeaponTypeIndex, GetWorld()->GetGameState<AGameState_FPS>()->IsHeadBoneIndex(HitBoneIndex));
				}
			}
		}
	}
}

void ACharacter_FPS::SetCurrentHealth(float NewHealth)
{
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast();
}

int ACharacter_FPS::CalculateShotCount()
{
	float DeltaTime = 1.0f / GameState->GetSnapshotsPerSecond();
	
	bool bProcessShots;
	float FireInterval = 0;

	if (ShootCommandQueue.IsEmpty())
	{
		bProcessShots = false;
	}
	else
	{
		const FShootCommand* NextShootCommand = ShootCommandQueue.Peek();

		FireInterval = GameState->GetWeaponFireInterval(NextShootCommand->WeaponIndex);
		
		float TimeSinceLastShot = (GameState->GetServerTick() - LastShotServerTick) * DeltaTime;

		if (TimeSinceLastShot + SMALL_NUMBER >= FireInterval)
		{
			bProcessShots = true;
		}
		else
		{
			bProcessShots = false;
		}
	}

	if (!bProcessShots)
	{
		if (BurstCounter == 0)
		{
			BurstCapacity = 0;
		}
		else
		{
			BurstCounter--;
			BurstCapacity = FMath::Min(BurstCapacity + DeltaTime, BurstLimit);
		}

		return 0;
	}
	else
	{
		BurstCapacity = FMath::Min(BurstCapacity + DeltaTime, BurstLimit);

		if (BurstCounter > 0)
		{
			return BurstCapacity / FireInterval;
		}
		else
		{
			int Ret = BurstCapacity / FireInterval;
			
			if (Ret > 1)
			{
				return Ret;
			}
			else
			{
				return 1;
			}
		}
	}
}

void ACharacter_FPS::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast();
}

void ACharacter_FPS::AddShootCommand(float InterpolationTime, FRotator ControlRotation)
{
	APlayerController_FPS* PlayerController = GetController<APlayerController_FPS>();

	if (PlayerController)
	{
		float RewindTime = GameState->GetRewindTime(PlayerController, InterpolationTime);

		float AbsRewindDiff = FMath::Abs((RewindTime - InterpolationTime) / GameState->GetSnapshotsPerSecond());
		float AuthDiff = (PlayerController->AuthRewindTime - InterpolationTime) / GameState->GetSnapshotsPerSecond();

		PlayerController->AbsRewindDiffValues[PlayerController->NextMetricIndex] = AbsRewindDiff;
		PlayerController->AuthDiffValues[PlayerController->NextMetricIndex] = AuthDiff;

		PlayerController->NextMetricIndex++;

		if (PlayerController->NextMetricIndex == 100)
		{
			PlayerController->NextMetricIndex = 0;

			float AvgAbsRewindDiffValues = 0;
			float AvgAuthDiffValues = 0;
			int Accuracy = 0;

			for(int i = 0; i < 100; i++)
			{
				AvgAbsRewindDiffValues += PlayerController->AbsRewindDiffValues[i];
				AvgAuthDiffValues += PlayerController->AuthDiffValues[i];
				
				if (PlayerController->AbsRewindDiffValues[i] == 0)
				{
					Accuracy++;
				}
			}

			AvgAbsRewindDiffValues /= 100;
			AvgAuthDiffValues /= 100;

			UE_LOG(LogTemp, Warning, TEXT("Results: AbsRewindDiff: %f, AuthDiff: %f, Accuracy: %f"), AvgAbsRewindDiffValues, AvgAuthDiffValues, Accuracy / 100.0f)
		}

		FVector StartLocation = GetActorLocation() + Camera->GetRelativeLocation();
		FVector ForwardVector = FRotator(ControlRotation.Pitch, ControlRotation.Yaw, 0).Quaternion().GetForwardVector();
		
		ShootCommandQueue.Enqueue(FShootCommand{RewindTime, GetSelectedWeaponIndex(), StartLocation, ForwardVector, RewindTime - InterpolationTime, CharacterMovementComponent_FPS->LastHitInterpolationID, CharacterMovementComponent_FPS->LastHitBoneIndex});
			
		if (PlayerController->bHitscanDebugInfoActive)
		{					
			FVector ActorLocation = GetActorLocation();
				
			UE_LOG(LogTemp, Warning, TEXT("[Server] #%d Time: %.3f, Start: [%.2f, %.2f, %.2f], Dir: [%.2f, %.2f, %.2f], Pos: [%.2f, %.2f, %.2f], Yaw: %.2f, Pitch: %.2f"), (GetCurrentAmmo()+1), RewindTime, StartLocation.X, StartLocation.Y, StartLocation.Z, ForwardVector.X, ForwardVector.Y, ForwardVector.Z, ActorLocation.X, ActorLocation.Y, ActorLocation.Z, ControlRotation.Yaw, ControlRotation.Pitch);
		}

		//UE_LOG(LogTemp, Warning, TEXT("Shot generated"))
	}
}

void ACharacter_FPS::ApplyShootCommands()
{
	if (GetWorld()->IsServer())
	{
		bWasShootingLastTick = false;
		
		int ShotCount = CalculateShotCount();

		//UE_LOG(LogTemp, Warning, TEXT("BurstCapacity: %f, BurstCounter: %d"), BurstCapacity, BurstCounter)

		if (!ShootCommandQueue.IsEmpty())
		{
			uint8 FirstWeaponIndex = ShootCommandQueue.Peek()->WeaponIndex;			

			while (ShotCount > 0 && !ShootCommandQueue.IsEmpty() && ShootCommandQueue.Peek()->WeaponIndex == FirstWeaponIndex)
			{
				FShootCommand ShootCommand;

				ShootCommandQueue.Dequeue(ShootCommand);
				
				TSet<ACharacter_FPS*> LagCompensatedCharacters = GameState->RewindAllCharactersExcept(ShootCommand.RewindTime, this);

				FCollisionQueryParams QueryParams;
				QueryParams.AddIgnoredActor(this);
					
				TArray<FHitResult> HitResults;

				GetWorld()->LineTraceMultiByChannel(HitResults, ShootCommand.CameraLocation, ShootCommand.CameraLocation + 10000.0f * ShootCommand.LookDirection, ECC_GameTraceChannel_WeaponTrace, QueryParams);

				if (APlayerController_FPS* PlayerController = GetController<APlayerController_FPS>())
				{
					if (PlayerController->bHitscanDebugInfoActive)
					{
						for(const FHitResult& HitResult : HitResults)
						{
							if (IsValid(HitResult.GetActor()))
							{
								UE_LOG(LogTemp, Warning, TEXT("HitResult: %s"), *HitResult.GetActor()->GetName())
							}
						}
					}
				}

				uint8 ServerHitInterpolationID = 0;
				uint8 ServerHitBoneIndex = 0;

				ACharacter_FPS* DamagedCharacter;
				FName DamagedBoneName;
				bool bHitPlayer = false;
				if (HitResults.Num() > 0 && FindDamagedBone(LagCompensatedCharacters, HitResults, ShootCommand.CameraLocation, DamagedCharacter, DamagedBoneName))
				{
					float Damage = GameState->GetWeaponBaseDamage(ShootCommand.WeaponIndex) * GameState->GetBoneDamageMultiplier(DamagedBoneName);
						
					//UE_LOG(LogTemp, Warning, TEXT("Damaged Actor: %s, Bone: %s, Damage: %f"), *DamagedCharacter->GetName(), *DamagedBoneName.ToString(), Damage)

					GetController<APlayerController_FPS>()->LastHitBoneIndex = GameState->GetBoneIndex(DamagedBoneName);
					GetController<APlayerController_FPS>()->LastHitRewindTime = ShootCommand.RewindTime;
					GetController<APlayerController_FPS>()->LastHitWeaponTypeIndex = ShootCommand.WeaponIndex;
					GetController<APlayerController_FPS>()->LastClientHitInterpID = ShootCommand.ClientHitInterpolationID;
					GetController<APlayerController_FPS>()->LastClientHitBoneIndex = ShootCommand.ClientHitBoneIndex;

					UGameplayStatics::ApplyDamage(DamagedCharacter, Damage, GetController(), this, UDamageType::StaticClass());

					bHitPlayer = true;
					
					ServerHitInterpolationID = DamagedCharacter->GetInterpolationID();
					ServerHitBoneIndex = GameState->GetBoneIndex(DamagedBoneName);
				}

				if (APlayerController_FPS* PlayerController = GetController<APlayerController_FPS>())
				{
					uint8 ClientHitInterpolationID = ShootCommand.ClientHitInterpolationID;
					uint8 ClientHitBoneIndex = ShootCommand.ClientHitBoneIndex;

					ACharacter_FPS* ClientHitCharacter = GameState->GetInterpolatedCharacter(ClientHitInterpolationID);
					
					PlayerController->AllShots++;

					if ((ClientHitInterpolationID == 0 && ServerHitInterpolationID == 0) || (ClientHitInterpolationID != 0 && (ClientHitCharacter == nullptr || ClientHitCharacter->IsKilled() || (ClientHitInterpolationID == ServerHitInterpolationID && ClientHitBoneIndex == ServerHitBoneIndex))))
					{
						PlayerController->MatchedShots++;
						
						if (ServerHitInterpolationID != 0)
						{
							PlayerController->MatchedHitShots++;

							if (ServerHitBoneIndex == 10)
							{
								PlayerController->HeadshotTruePositives++;
							}
						}
						else
						{
							PlayerController->MatchedMissedShots++;
						}
					}
					else
					{
						PlayerController->MismatchedShots++;
						
						if (ServerHitInterpolationID != 0)
						{
							if (ClientHitInterpolationID == ServerHitInterpolationID)
							{
								float ClientHitBoneMultiplier = GameState->GetBoneDamageMultiplierByIndex(ClientHitBoneIndex);
								float ServerHitBoneMultiplier = GameState->GetBoneDamageMultiplierByIndex(ServerHitBoneIndex);

								if (ServerHitBoneMultiplier == ClientHitBoneMultiplier)
								{
									PlayerController->MismatchedEqualDamageShots++;
								}
								else
								{
									PlayerController->MismatchedUnequalDamageShots++;

									if (ServerHitBoneMultiplier < ClientHitBoneMultiplier)
									{
										PlayerController->MismatchedUnequalLessDamageShots++;
										PlayerController->MismatchedUnequalLessDamageServerHitShots++;

										if (ClientHitBoneIndex == 10)
										{
											PlayerController->HeadshotFalsePositives++;
										}
									}
									else
									{
										PlayerController->MismatchedUnequalMoreDamageShots++;
										PlayerController->MismatchedUnequalMoreDamageClientHitShots++;

										if (ServerHitBoneIndex == 10)
										{
											PlayerController->HeadshotFalseNegatives++;
										}
									}
								}
							}
							else
							{
								PlayerController->MismatchedUnequalDamageShots++;
								
								if (ClientHitInterpolationID != 0)
								{
									PlayerController->MismatchedUnequalLessDamageShots++;
									PlayerController->MismatchedUnequalLessDamageServerMissedShots++;

									if (ClientHitBoneIndex == 10)
									{
										PlayerController->HeadshotFalsePositives++;
									}
								}
								else
								{									
									PlayerController->MismatchedUnequalMoreDamageShots++;
									PlayerController->MismatchedUnequalMoreDamageClientMissedShots++;

									if (ServerHitBoneIndex == 10)
									{
										PlayerController->HeadshotFalseNegatives++;
									}

									//UE_LOG(LogTemp, Error, TEXT("ClientHitInterpID: %d, ClientHitBoneIdx: %d, ServerHitInterpID: %d, ServerHitBoneIdx: %d"), ClientHitInterpolationID, ClientHitBoneIndex, ServerHitInterpolationID, ServerHitBoneIndex)
								}								
							}
						}
						else
						{
							PlayerController->MismatchedUnequalDamageShots++;
							PlayerController->MismatchedUnequalLessDamageShots++;
							PlayerController->MismatchedUnequalLessDamageServerMissedShots++;

							if (ClientHitBoneIndex == 10)
							{
								PlayerController->HeadshotFalsePositives++;
							}
						}
					}
				}

				if (APlayerController_FPS* PlayerController = GetController<APlayerController_FPS>())
				{
					if (PlayerController->bHitscanDebugInfoActive)
					{
						TArray<FVector> HitboxPositions;
						TArray<FQuat> HitboxRotations;

						for(APlayerState* PlayerArrayElement : GetWorld()->GetGameState()->PlayerArray)
						{
							if (ACharacter_FPS* Character = PlayerArrayElement->GetPawn<ACharacter_FPS>())
							{
								if (Character != this)
								{
									for(USkeletalBodySetup* SkeletalBodySetup : Character->GetLagCompMesh()->GetPhysicsAsset()->SkeletalBodySetups)
									{
										FTransform BoneTransform = Character->GetLagCompMesh()->GetBoneTransform(Character->GetLagCompMesh()->GetBoneIndex(SkeletalBodySetup->BoneName));
						
										const FKAggregateGeom& AggGeom = SkeletalBodySetup->AggGeom;

										for(const FKBoxElem& BoxElem : AggGeom.BoxElems)
										{
											HitboxPositions.Add(BoneTransform.TransformPosition(BoxElem.Center));
											HitboxRotations.Add(BoneTransform.TransformRotation(BoxElem.Rotation.Quaternion()));
										}

										for(const FKSphylElem& SphylElem : AggGeom.SphylElems)
										{
											HitboxPositions.Add(BoneTransform.TransformPosition(SphylElem.Center));
											HitboxRotations.Add(BoneTransform.TransformRotation(SphylElem.Rotation.Quaternion()));
										}
									}
								}
							}
						}

						DebugHitboxRPC(HitboxPositions, HitboxRotations, ShootCommand.CameraLocation, ShootCommand.CameraLocation + 10000.0f * ShootCommand.LookDirection, ShootCommand.Error, bHitPlayer);
					}
				}
				
				ShotCount--;
				
				BurstCapacity -= GameState->GetWeaponFireInterval(FirstWeaponIndex);

				if (BurstCapacity < 0)
				{
					BurstCapacity = 0;
				}

				LastShotServerTick = GameState->GetServerTick();
				bWasShootingLastTick = true;
				BurstCounter = BurstCounterMax;
				//UE_LOG(LogTemp, Warning, TEXT("Processed"))
			}
		}
	}
}

void ACharacter_FPS::OnRep_Killed()
{
	if (bKilled)
	{
		if (GetLocalRole() == ROLE_SimulatedProxy)
		{
			DisableAnimationInterpolation();
		}
	}
}

void ACharacter_FPS::OnRep_KilledByPawn()
{
	if (!IsValid(Controller) && IsValid(PreviousController))
	{
		SwitchToThirdPersonView();
	}
}

void ACharacter_FPS::DisableAnimationInterpolation()
{
	bInterpolateAnimationPose = false;
}

void ACharacter_FPS::UpdateSimulatedProxy(float InterpolationTime)
{
	if (bInterpolate)
	{
		if (InterpolationTime > LastSnapshotServerTick)
		{
			InterpolationTime = LastSnapshotServerTick;
		}

		if (InterpolationTime > LastInterpolationTime)
		{
			LastInterpolationTime = InterpolationTime;
		}

		uint8 FromSnapshotIdx;
		uint8 ToSnapshotIdx;
		float Alpha;
		GetInterpolatedSnapshots(InterpolationTime, FromSnapshotIdx, ToSnapshotIdx, Alpha);

		InterpolateCharacter(FromSnapshotIdx, ToSnapshotIdx, Alpha);
	}

	TArray<FVector> HitboxPositions;
	TArray<FQuat> HitboxRotations;

	for(USkeletalBodySetup* SkeletalBodySetup : GetLagCompMesh()->GetPhysicsAsset()->SkeletalBodySetups)
	{
		FTransform BoneTransform = GetLagCompMesh()->GetBoneTransform(GetLagCompMesh()->GetBoneIndex(SkeletalBodySetup->BoneName));
					
		const FKAggregateGeom& AggGeom = SkeletalBodySetup->AggGeom;

		for(const FKBoxElem& BoxElem : AggGeom.BoxElems)
		{
			HitboxPositions.Add(BoneTransform.TransformPosition(BoxElem.Center));
			HitboxRotations.Add(BoneTransform.TransformRotation(BoxElem.Rotation.Quaternion()));
		}

		for(const FKSphylElem& SphylElem : AggGeom.SphylElems)
		{
			HitboxPositions.Add(BoneTransform.TransformPosition(SphylElem.Center));
			HitboxRotations.Add(BoneTransform.TransformRotation(SphylElem.Rotation.Quaternion()));
		}
	}

	int c = 0;

	/*for(USkeletalBodySetup* SkeletalBodySetup : GetLagCompMesh()->GetPhysicsAsset()->SkeletalBodySetups)
	{
		const FKAggregateGeom& AggGeom = SkeletalBodySetup->AggGeom;

		for(const FKBoxElem& BoxElem : AggGeom.BoxElems)
		{
			DrawDebugBox(GetWorld(), HitboxPositions[c], FVector(BoxElem.X/2.0, BoxElem.Y/2.0, BoxElem.Z/2.0), HitboxRotations[c], FColor::Red, false);
			c++;
		}

		for(const FKSphylElem& SphylElem : AggGeom.SphylElems)
		{
			DrawDebugCapsule(GetWorld(), HitboxPositions[c], SphylElem.GetScaledHalfLength(FVector(1.0f, 1.0f, 1.0f)), SphylElem.Radius, HitboxRotations[c],  FColor::Red, false);				
			c++;
		}
	}*/
}

void ACharacter_FPS::Rewind(float RewindTime)
{
	if (!bDuringRewind)
	{
		RealLocation = GetActorLocation();
		RealRotation = GetActorRotation();
		
		bDuringRewind = true;
	}

	float MinRewindTime = FMath::Max(FirstSnapshotServerTick * 1.0f, LastSnapshotServerTick * 1.0f - 255);
	float MaxRewindTime = LastSnapshotServerTick;

	RewindTime = FMath::Clamp(RewindTime, MinRewindTime, MaxRewindTime);

	ServerTime FromServerTick = FMath::Floor(RewindTime);
	ServerTime ToServerTick = FromServerTick+1;
	float Alpha = RewindTime-FromServerTick;

	InterpolateCharacter(FromServerTick, ToServerTick, Alpha);
}

void ACharacter_FPS::InterpolateCharacter(uint8 FromSnapshotIdx, uint8 ToSnapshotIdx, float Alpha)
{	
	const FStoredSnapshot& FromSnapshot = SnapshotBuffer[FromSnapshotIdx];
	const FStoredSnapshot& ToSnapshot = SnapshotBuffer[ToSnapshotIdx];
	
	SetActorLocation(FMath::Lerp(FromSnapshot.Position, ToSnapshot.Position, Alpha));
	SetActorRotation(UKismetMathLibrary::RLerp(FromSnapshot.Rotation, ToSnapshot.Rotation, Alpha, true));

	if (bInterpolateAnimationPose)
	{
		InterpolateAnimationParameters(FromSnapshotIdx, ToSnapshotIdx, Alpha);
		LagCompMesh->GetAnimInstance()->UpdateAnimation(0.01, false);
		LagCompMesh->RefreshBoneTransforms();
	}
}

void ACharacter_FPS::Restore()
{
	if (bDuringRewind)
	{
		SetActorLocation(RealLocation);
		SetActorRotation(RealRotation);
		
		bDuringRewind = false;
	}	
}

void ACharacter_FPS::AssignInterpolationID(uint8 InInterpolationID)
{
	if (GetWorld()->IsServer())
	{
		InterpolationID = InInterpolationID;
	}
}

void ACharacter_FPS::RemoveInterpolationID()
{
	if (GetWorld()->IsServer())
	{
		InterpolationID = 0;
	}
}

void ACharacter_FPS::CreatePlayerSnapshot(ServerTime ServerTick, FPlayerSnapshot& PlayerSnapshot)
{	
	FVector PlayerLocation = GetActorLocation();
	PlayerSnapshot.Position = PlayerLocation;
	
	FRotator PlayerRotation = GetActorRotation();
	PlayerSnapshot.Rotation = PlayerRotation;

	CreateAnimData(PlayerSnapshot.AnimData);
	
	AddSnapshot(ServerTick, PlayerLocation, PlayerRotation, PlayerSnapshot.AnimData);

	PlayerSnapshot.bShooting = bWasShootingLastTick;
	PlayerSnapshot.SelectedWeaponIndex = SelectedWeaponIndex;
}

void ACharacter_FPS::ConsumePlayerSnapshot(ServerTime ServerTick, const FPlayerSnapshot& PlayerSnapshot)
{
	AddSnapshot(ServerTick, PlayerSnapshot.Position, PlayerSnapshot.Rotation, PlayerSnapshot.AnimData);
	
	if (!IsKilled())
	{
		if (PlayerSnapshot.SelectedWeaponIndex != SelectedWeaponIndex)
		{
			SetSelectedWeapon(PlayerSnapshot.SelectedWeaponIndex);
		}
		
		if (PlayerSnapshot.bShooting)
		{
			OnRemotePlayerShoot.Broadcast();
		}		
	}
}

void ACharacter_FPS::CreateAnimData(FAnimData& AnimData)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	AnimData.SwitchingSlotWeight = AnimInstance->GetSlotMontageLocalWeight("SwitchingSlot");

	if (AnimData.SwitchingSlotWeight > 0.0f)
	{
		if (AnimInstance->Montage_IsPlaying(SwitchingMontage))
		{
			AnimData.SwitchingSlotTime = AnimInstance->Montage_GetPosition(SwitchingMontage);
		}
		else
		{
			AnimData.SwitchingSlotTime = SwitchingMontage->GetPlayLength();
		}
	}

	if (AnimData.SwitchingSlotWeight < 1.0f)
	{
		AnimData.ReloadSlotWeight = AnimInstance->GetSlotMontageLocalWeight("ReloadSlot");

		if (AnimData.ReloadSlotWeight > 0.0f)
		{
			if (AnimInstance->Montage_IsPlaying(ReloadMontage))
			{
				AnimData.ReloadSlotTime = AnimInstance->Montage_GetPosition(ReloadMontage);
			}
			else
			{
				AnimData.ReloadSlotTime = ReloadMontage->GetPlayLength();
			}
		}

		if (AnimData.ReloadSlotWeight < 1.0f)
		{
			FUpperBodyStateMachineData UpperBodyStateMachineData = Cast<UAnimInstance_FPS>(GetMesh()->GetAnimInstance())->GetUpperBodySMData();

			AnimData.AimingStateWeight = UpperBodyStateMachineData.AimingStateWeight;
			AnimData.Pitch = UpperBodyStateMachineData.Pitch;
			AnimData.IdleRifleSequenceTime = UpperBodyStateMachineData.IdleRifleSequenceTime;

			AnimData.SprintingStateWeight = UpperBodyStateMachineData.SprintingStateWeight;
			AnimData.SprintingSequenceTime = UpperBodyStateMachineData.SprintingSequenceTime;
		}
	}

	FLocomotionStateMachineData LocomotionStateMachineData = Cast<UAnimInstance_FPS>(GetMesh()->GetAnimInstance())->GetLocomotionSMData();

	AnimData.IdleRunStateWeight = LocomotionStateMachineData.IdleRunStateWeight;
	AnimData.IdleRunBlendSpaceTime = LocomotionStateMachineData.IdleRunBlendSpaceTime;
	AnimData.Speed = LocomotionStateMachineData.Speed;

	AnimData.JumpStartStateWeight = LocomotionStateMachineData.JumpStartStateWeight;
	AnimData.JumpStartSequenceTime = LocomotionStateMachineData.JumpStartSequenceTime;

	AnimData.JumpLoopStateWeight = LocomotionStateMachineData.JumpLoopStateWeight;
	AnimData.JumpLoopSequenceTime = LocomotionStateMachineData.JumpLoopSequenceTime;

	AnimData.JumpEndStateWeight = LocomotionStateMachineData.JumpEndStateWeight;
	AnimData.JumpEndSequenceTime = LocomotionStateMachineData.JumpEndSequenceTime;
}

void ACharacter_FPS::GetInterpolatedSnapshots(float InterpolationTime, uint8& FromSnapshotIndex, uint8& ToSnapshotIndex, float& Alpha)
{
	if (InterpFromSnapshotServerTick <= InterpolationTime && InterpolationTime < InterpToSnapshotServerTick)
	{
		// do nothing
	}
	else if (InterpolationTime >= InterpToSnapshotServerTick)
	{
		InterpFromSnapshotServerTick = InterpToSnapshotServerTick;

		if (InterpToSnapshotServerTick < LastSnapshotServerTick)
		{
			do
			{
				InterpToSnapshotServerTick++;

				uint8 Index = static_cast<uint8>(InterpToSnapshotServerTick);

				if (SnapshotBuffer[Index].ServerTick == InterpToSnapshotServerTick)
				{
					if (InterpolationTime >= InterpToSnapshotServerTick)
					{
						InterpFromSnapshotServerTick = InterpToSnapshotServerTick;
					}
					else
					{
						break;
					}
				}
			}
			while (InterpToSnapshotServerTick < LastSnapshotServerTick);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetInterpolatedSnapshots(): This should not happen. InterpolationTime: %f, InterpFromSnapshotServerTick: %d"), InterpolationTime, InterpFromSnapshotServerTick)
	}

	FromSnapshotIndex = static_cast<uint8>(InterpFromSnapshotServerTick);
	ToSnapshotIndex = static_cast<uint8>(InterpToSnapshotServerTick);

	if (InterpFromSnapshotServerTick < InterpolationTime && InterpolationTime < InterpToSnapshotServerTick)
	{
		Alpha = (InterpolationTime - InterpFromSnapshotServerTick)/(InterpToSnapshotServerTick - InterpFromSnapshotServerTick);
	}
	else if (InterpolationTime >= InterpToSnapshotServerTick)
	{
		Alpha = 1.0f;
	}
	else
	{
		Alpha = 0.0f;
	}

	if (InterpolationTime > LastSnapshotServerTick)
	{
		static int asd = 0;
		GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Red, FString::Printf(TEXT("NO SNAPSHOTS. %f"), InterpolationTime/20.0f));
	}
}

void ACharacter_FPS::InterpolateAnimationParameters(uint8 FromSnapshotIndex, uint8 ToSnapshotIndex, float Alpha)
{
	const FAnimData& FromSnapshot = SnapshotBuffer[FromSnapshotIndex].AnimData;
	const FAnimData& ToSnapshot = SnapshotBuffer[ToSnapshotIndex].AnimData;

	bool bSwitchingSlot_PresentInFromSnapshot = true;
	bool bSwitchingSlot_PresentInToSnapshot = true;

	// Interpolate UpperBody part
	if (bSwitchingSlot_PresentInFromSnapshot || bSwitchingSlot_PresentInToSnapshot)
	{
		// Interpolate SwitchingSlot node
		
		float FromSwitchingSlotWeight = bSwitchingSlot_PresentInFromSnapshot ? FromSnapshot.SwitchingSlotWeight : ToSnapshot.SwitchingSlotWeight;
		float ToSwitchingSlotWeight = bSwitchingSlot_PresentInToSnapshot ? ToSnapshot.SwitchingSlotWeight : FromSnapshot.SwitchingSlotWeight;

		float FromSwitchingSlotTime = (bSwitchingSlot_PresentInFromSnapshot && FromSwitchingSlotWeight > 0.0f) ? FromSnapshot.SwitchingSlotTime : ToSnapshot.SwitchingSlotTime;
		float ToSwitchingSlotTime = (bSwitchingSlot_PresentInToSnapshot && ToSwitchingSlotWeight > 0.0f) ? ToSnapshot.SwitchingSlotTime : FromSnapshot.SwitchingSlotTime;

		InterpolatedAnim.SwitchingSlotWeight = FMath::Lerp(FromSwitchingSlotWeight, ToSwitchingSlotWeight, Alpha);
		InterpolatedAnim.SwitchingSlotTime = FMath::Lerp(FromSwitchingSlotTime, ToSwitchingSlotTime, Alpha);

		bool bReloadSlot_PresentInFromSnapshot = bSwitchingSlot_PresentInFromSnapshot && FromSnapshot.SwitchingSlotWeight < 1.0f;
		bool bReloadSlot_PresentInToSnapshot = bSwitchingSlot_PresentInToSnapshot && ToSnapshot.SwitchingSlotWeight < 1.0f;

		if (bReloadSlot_PresentInFromSnapshot || bReloadSlot_PresentInToSnapshot)
		{
			// Interpolate ReloadSlot node

			float FromReloadSlotWeight = bReloadSlot_PresentInFromSnapshot ? FromSnapshot.ReloadSlotWeight : ToSnapshot.ReloadSlotWeight;
			float ToReloadSlotWeight = bReloadSlot_PresentInToSnapshot ? ToSnapshot.ReloadSlotWeight : FromSnapshot.ReloadSlotWeight;

			float FromReloadSlotTime = (bReloadSlot_PresentInFromSnapshot && FromReloadSlotWeight > 0.0f) ? FromSnapshot.ReloadSlotTime : ToSnapshot.ReloadSlotTime;
			float ToReloadSlotTime = (bReloadSlot_PresentInToSnapshot && ToReloadSlotWeight > 0.0f) ? ToSnapshot.ReloadSlotTime : FromSnapshot.ReloadSlotTime;

			InterpolatedAnim.ReloadSlotWeight = FMath::Lerp(FromReloadSlotWeight, ToReloadSlotWeight, Alpha);
			InterpolatedAnim.ReloadSlotTime = FMath::Lerp(FromReloadSlotTime, ToReloadSlotTime, Alpha);

			bool bUpperBodyStateMachine_PresentInFromSnapshot = bReloadSlot_PresentInFromSnapshot && FromSnapshot.ReloadSlotWeight < 1.0f;
			bool bUpperBodyStateMachine_PresentInToSnapshot = bReloadSlot_PresentInToSnapshot && ToSnapshot.ReloadSlotWeight < 1.0f;

			if (bUpperBodyStateMachine_PresentInFromSnapshot || bUpperBodyStateMachine_PresentInToSnapshot)
			{
				// Interpolate UpperBody State Machine

				float FromUpperBodyDefaultStateWeight = bUpperBodyStateMachine_PresentInFromSnapshot ? FromSnapshot.AimingStateWeight : ToSnapshot.AimingStateWeight;
				float ToUpperBodyDefaultStateWeight = bUpperBodyStateMachine_PresentInToSnapshot ? ToSnapshot.AimingStateWeight : FromSnapshot.AimingStateWeight;

				float FromUpperBodySprintingStateWeight = bUpperBodyStateMachine_PresentInFromSnapshot ? FromSnapshot.SprintingStateWeight : ToSnapshot.SprintingStateWeight;
				float ToUpperBodySprintingStateWeight = bUpperBodyStateMachine_PresentInToSnapshot ? ToSnapshot.SprintingStateWeight : FromSnapshot.SprintingStateWeight;

				InterpolatedAnim.AimingStateWeight = FMath::Lerp(FromUpperBodyDefaultStateWeight, ToUpperBodyDefaultStateWeight, Alpha);
				InterpolatedAnim.SprintingStateWeight = FMath::Lerp(FromUpperBodySprintingStateWeight, ToUpperBodySprintingStateWeight, Alpha);

				bool bUpperBodyDefaultState_PresentInFromSnapshot = bUpperBodyStateMachine_PresentInFromSnapshot && FromSnapshot.AimingStateWeight > 0.0f;
				bool bUpperBodyDefaultState_PresentInToSnapshot = bUpperBodyStateMachine_PresentInToSnapshot && ToSnapshot.AimingStateWeight > 0.0f;

				bool bUpperBodySprintingState_PresentInFromSnapshot = bUpperBodyStateMachine_PresentInFromSnapshot && FromSnapshot.SprintingStateWeight > 0.0f;
				bool bUpperBodySprintingState_PresentInToSnapshot = bUpperBodyStateMachine_PresentInToSnapshot && ToSnapshot.SprintingStateWeight > 0.0f;

				if (bUpperBodyDefaultState_PresentInFromSnapshot || bUpperBodyDefaultState_PresentInToSnapshot)
				{
					// Interpolate UpperBody Default State
				
					float FromAimOffsetPitch = bUpperBodyDefaultState_PresentInFromSnapshot ? FromSnapshot.Pitch : ToSnapshot.Pitch;
					float ToAimOffsetPitch = bUpperBodyDefaultState_PresentInToSnapshot ? ToSnapshot.Pitch : FromSnapshot.Pitch;

					float FromIdleRifleSequenceTime = bUpperBodyDefaultState_PresentInFromSnapshot ? FromSnapshot.IdleRifleSequenceTime : ToSnapshot.IdleRifleSequenceTime;
					float ToIdleRifleSequenceTime = bUpperBodyDefaultState_PresentInToSnapshot ? ToSnapshot.IdleRifleSequenceTime : FromSnapshot.IdleRifleSequenceTime;

					if (ToIdleRifleSequenceTime < FromIdleRifleSequenceTime)
					{
						ToIdleRifleSequenceTime += 1.0f;
					}

					InterpolatedAnim.Pitch = FMath::Lerp(FromAimOffsetPitch, ToAimOffsetPitch, Alpha);					
					InterpolatedAnim.IdleRifleSequenceTime = FMath::Lerp(FromIdleRifleSequenceTime, ToIdleRifleSequenceTime, Alpha);
					
					if (InterpolatedAnim.IdleRifleSequenceTime > 1.0f)
					{
						InterpolatedAnim.IdleRifleSequenceTime -= 1.0f;
					}
				}

				if (bUpperBodySprintingState_PresentInFromSnapshot || bUpperBodySprintingState_PresentInToSnapshot)
				{
					// Interpolate UpperBody Sprinting State

					float FromSprintingSequenceTime = bUpperBodySprintingState_PresentInFromSnapshot ? FromSnapshot.SprintingSequenceTime : ToSnapshot.SprintingSequenceTime;
					float ToSprintingSequenceTime = bUpperBodySprintingState_PresentInToSnapshot ? ToSnapshot.SprintingSequenceTime : FromSnapshot.SprintingSequenceTime;

					if (ToSprintingSequenceTime < FromSprintingSequenceTime)
					{
						ToSprintingSequenceTime += 1.0f;
					}

					InterpolatedAnim.SprintingSequenceTime = FMath::Lerp(FromSprintingSequenceTime, ToSprintingSequenceTime, Alpha);

					if (InterpolatedAnim.SprintingSequenceTime > 1.0f)
					{
						InterpolatedAnim.SprintingSequenceTime -= 1.0f;
					}
				}
			}
		}
	}

	bool bLocomotionStateMachine_PresentInFromSnapshot = true;
	bool bLocomotionStateMachine_PresentInToSnapshot = true;

	// Interpolate Locomotion part
	if (bLocomotionStateMachine_PresentInFromSnapshot || bLocomotionStateMachine_PresentInToSnapshot)
	{
		// Interpolate Locomotion State Machine

		float FromLocomotionIdleRunStateWeight = bLocomotionStateMachine_PresentInFromSnapshot ? FromSnapshot.IdleRunStateWeight : ToSnapshot.IdleRunStateWeight;
		float ToLocomotionIdleRunStateWeight = bLocomotionStateMachine_PresentInToSnapshot ? ToSnapshot.IdleRunStateWeight : FromSnapshot.IdleRunStateWeight;

		float FromLocomotionJumpStartStateWeight = bLocomotionStateMachine_PresentInFromSnapshot ? FromSnapshot.JumpStartStateWeight : ToSnapshot.JumpStartStateWeight;
		float ToLocomotionJumpStartStateWeight = bLocomotionStateMachine_PresentInToSnapshot ? ToSnapshot.JumpStartStateWeight : FromSnapshot.JumpStartStateWeight;

		float FromLocomotionJumpLoopStateWeight = bLocomotionStateMachine_PresentInFromSnapshot ? FromSnapshot.JumpLoopStateWeight : ToSnapshot.JumpLoopStateWeight;
		float ToLocomotionJumpLoopStateWeight = bLocomotionStateMachine_PresentInToSnapshot ? ToSnapshot.JumpLoopStateWeight : FromSnapshot.JumpLoopStateWeight;

		float FromLocomotionJumpEndStateWeight = bLocomotionStateMachine_PresentInFromSnapshot ? FromSnapshot.JumpEndStateWeight : ToSnapshot.JumpEndStateWeight;
		float ToLocomotionJumpEndStateWeight = bLocomotionStateMachine_PresentInToSnapshot ? ToSnapshot.JumpEndStateWeight : FromSnapshot.JumpEndStateWeight;

		InterpolatedAnim.IdleRunStateWeight = FMath::Lerp(FromLocomotionIdleRunStateWeight, ToLocomotionIdleRunStateWeight, Alpha);
		InterpolatedAnim.JumpStartStateWeight = FMath::Lerp(FromLocomotionJumpStartStateWeight, ToLocomotionJumpStartStateWeight, Alpha);
		InterpolatedAnim.JumpLoopStateWeight = FMath::Lerp(FromLocomotionJumpLoopStateWeight, ToLocomotionJumpLoopStateWeight, Alpha);
		InterpolatedAnim.JumpEndStateWeight = FMath::Lerp(FromLocomotionJumpEndStateWeight, ToLocomotionJumpEndStateWeight, Alpha);

		bool bLocomotionIdleRunState_PresentInFromSnapshot = bLocomotionStateMachine_PresentInFromSnapshot && FromSnapshot.IdleRunStateWeight > 0.0f;
		bool bLocomotionIdleRunState_PresentInToSnapshot = bLocomotionStateMachine_PresentInToSnapshot && ToSnapshot.IdleRunStateWeight > 0.0f;

		bool bLocomotionJumpStartState_PresentInFromSnapshot = bLocomotionStateMachine_PresentInFromSnapshot && FromSnapshot.JumpStartStateWeight > 0.0f;
		bool bLocomotionJumpStartState_PresentInToSnapshot = bLocomotionStateMachine_PresentInToSnapshot && ToSnapshot.JumpStartStateWeight > 0.0f;

		bool bLocomotionJumpLoopState_PresentInFromSnapshot = bLocomotionStateMachine_PresentInFromSnapshot && FromSnapshot.JumpLoopStateWeight > 0.0f;
		bool bLocomotionJumpLoopState_PresentInToSnapshot = bLocomotionStateMachine_PresentInToSnapshot && ToSnapshot.JumpLoopStateWeight > 0.0f;

		bool bLocomotionJumpEndState_PresentInFromSnapshot = bLocomotionStateMachine_PresentInFromSnapshot && FromSnapshot.JumpEndStateWeight > 0.0f;
		bool bLocomotionJumpEndState_PresentInToSnapshot = bLocomotionStateMachine_PresentInToSnapshot && ToSnapshot.JumpEndStateWeight > 0.0f;

		if (bLocomotionIdleRunState_PresentInFromSnapshot || bLocomotionIdleRunState_PresentInToSnapshot)
		{
			// Interpolate Locomotion IdleRun State

			float FromIdleRunBlendSpaceSpeed = bLocomotionIdleRunState_PresentInFromSnapshot ? FromSnapshot.Speed : ToSnapshot.Speed;
			float ToIdleRunBlendSpaceSpeed = bLocomotionIdleRunState_PresentInToSnapshot ? ToSnapshot.Speed : FromSnapshot.Speed;
			
			float FromIdleRunBlendSpaceTime = bLocomotionIdleRunState_PresentInFromSnapshot ? FromSnapshot.IdleRunBlendSpaceTime : ToSnapshot.IdleRunBlendSpaceTime;
			float ToIdleRunBlendSpaceTime = bLocomotionIdleRunState_PresentInToSnapshot ? ToSnapshot.IdleRunBlendSpaceTime : FromSnapshot.IdleRunBlendSpaceTime;

			if (ToIdleRunBlendSpaceTime < FromIdleRunBlendSpaceTime)
			{
				ToIdleRunBlendSpaceTime += 1.0f;
			}

			InterpolatedAnim.Speed = FMath::Lerp(FromIdleRunBlendSpaceSpeed, ToIdleRunBlendSpaceSpeed, Alpha);
			InterpolatedAnim.IdleRunBlendSpaceTime = FMath::Lerp(FromIdleRunBlendSpaceTime, ToIdleRunBlendSpaceTime, Alpha);

			if (InterpolatedAnim.IdleRunBlendSpaceTime > 1.0f)
			{
				InterpolatedAnim.IdleRunBlendSpaceTime -= 1.0f;
			}
		}

		if (bLocomotionJumpStartState_PresentInFromSnapshot || bLocomotionJumpStartState_PresentInToSnapshot)
		{
			// Interpolate Locomotion JumpStart State

			float FromJumpStartSequenceTime = bLocomotionJumpStartState_PresentInFromSnapshot ? FromSnapshot.JumpStartSequenceTime : ToSnapshot.JumpStartSequenceTime;
			float ToJumpStartSequenceTime = bLocomotionJumpStartState_PresentInToSnapshot ? ToSnapshot.JumpStartSequenceTime : FromSnapshot.JumpStartSequenceTime;

			if (ToJumpStartSequenceTime < FromJumpStartSequenceTime)
			{
				ToJumpStartSequenceTime += 1.0f;
			}

			InterpolatedAnim.JumpStartSequenceTime = FMath::Lerp(FromJumpStartSequenceTime, ToJumpStartSequenceTime, Alpha);

			if (InterpolatedAnim.JumpStartSequenceTime > 1.0f)
			{
				InterpolatedAnim.JumpStartSequenceTime -= 1.0f;
			}
		}

		if (bLocomotionJumpLoopState_PresentInFromSnapshot || bLocomotionJumpLoopState_PresentInToSnapshot)
		{
			// Interpolate Locomotion JumpLoop State

			float FromJumpLoopSequenceTime = bLocomotionJumpLoopState_PresentInFromSnapshot ? FromSnapshot.JumpLoopSequenceTime : ToSnapshot.JumpLoopSequenceTime;
			float ToJumpLoopSequenceTime = bLocomotionJumpLoopState_PresentInToSnapshot ? ToSnapshot.JumpLoopSequenceTime : FromSnapshot.JumpLoopSequenceTime;

			if (ToJumpLoopSequenceTime < FromJumpLoopSequenceTime)
			{
				ToJumpLoopSequenceTime += 1.0f;
			}

			InterpolatedAnim.JumpLoopSequenceTime = FMath::Lerp(FromJumpLoopSequenceTime, ToJumpLoopSequenceTime, Alpha);

			if (InterpolatedAnim.JumpLoopSequenceTime > 1.0f)
			{
				InterpolatedAnim.JumpLoopSequenceTime -= 1.0f;
			}
		}

		if (bLocomotionJumpEndState_PresentInFromSnapshot || bLocomotionJumpEndState_PresentInToSnapshot)
		{
			// Interpolate Locomotion JumpEnd State

			float FromJumpEndSequenceTime = bLocomotionJumpEndState_PresentInFromSnapshot ? FromSnapshot.JumpEndSequenceTime : ToSnapshot.JumpEndSequenceTime;
			float ToJumpEndSequenceTime = bLocomotionJumpEndState_PresentInToSnapshot ? ToSnapshot.JumpEndSequenceTime : FromSnapshot.JumpEndSequenceTime;

			if (ToJumpEndSequenceTime < FromJumpEndSequenceTime)
			{
				ToJumpEndSequenceTime += 1.0f;
			}

			InterpolatedAnim.JumpEndSequenceTime = FMath::Lerp(FromJumpEndSequenceTime, ToJumpEndSequenceTime, Alpha);

			if (InterpolatedAnim.JumpEndSequenceTime > 1.0f)
			{
				InterpolatedAnim.JumpEndSequenceTime -= 1.0f;
			}
		}
	}
}

void ACharacter_FPS::AddSnapshot(ServerTime ServerTick, FVector Position, FRotator Rotation, const FAnimData& AnimData)
{
	if (LastSnapshotServerTick == 0)
	{
		FirstSnapshotServerTick = ServerTick;
	}
	
	FStoredSnapshot Snapshot{ServerTick, Position, Rotation, AnimData};
	
	uint8 BufferIndex = Snapshot.ServerTick;

	if (Snapshot.ServerTick > SnapshotBuffer[BufferIndex].ServerTick)
	{
		SnapshotBuffer[BufferIndex] = Snapshot;

		if (Snapshot.ServerTick - InterpFromSnapshotServerTick >= 256)
		{
			InterpFromSnapshotServerTick = Snapshot.ServerTick;
			InterpToSnapshotServerTick = Snapshot.ServerTick;
		}
		else
		{
			if (InterpFromSnapshotServerTick < Snapshot.ServerTick && Snapshot.ServerTick < InterpToSnapshotServerTick)
			{
				if (LastInterpolationTime < Snapshot.ServerTick)
				{
					InterpToSnapshotServerTick = Snapshot.ServerTick;
				}
				else
				{
					InterpFromSnapshotServerTick = Snapshot.ServerTick;
				}
			}
			else if (LastInterpolationTime >= LastSnapshotServerTick && Snapshot.ServerTick > LastSnapshotServerTick)
			{
				InterpFromSnapshotServerTick = LastSnapshotServerTick;
				InterpToSnapshotServerTick = Snapshot.ServerTick;
			}
		}

		if (Snapshot.ServerTick > LastSnapshotServerTick)
		{
			LastSnapshotServerTick = Snapshot.ServerTick;
		}
	}
}

void ACharacter_FPS::OnRep_InterpolationID()
{
	if (!HasActorBegunPlay())
	{
		Delayed_OnRep_InterpolationID_Queue.Enqueue(InterpolationID);
	}
	else
	{
		Handle_OnRep_InterpolationID();
	}
}

void ACharacter_FPS::Handle_OnRep_InterpolationID()
{
	if (PreviousInterpolationID != InterpolationID)
	{
		if (PreviousInterpolationID != 0)
		{
			GameState->RemoveFromInterpolatedCharacters(this);
		}

		if (InterpolationID != 0)
		{
			GameState->AddToInterpolatedCharacters(this);
		}
		
		PreviousInterpolationID = InterpolationID;
	}
}

void ACharacter_FPS::DebugHitboxRPC_Implementation(const TArray<FVector>& HitboxPositions, const TArray<FQuat>& HitboxRotations, const FVector& StartLoc, const FVector& EndLoc, float Error, bool bHit)
{
	DrawDebugLine(GetWorld(), StartLoc, EndLoc, bHit ? FColor::Green : FColor::Red, true);

	if (Error == 0.0f)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Green, TEXT("Hitbox rewind: OK"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(TEXT("Hitbox rewind error: %.2f ms"), Error * 100.0f));
	}
	
	
	int NumPlayers = HitboxPositions.Num() / 19;

	int c = 0;

	for(int i = 0; i < NumPlayers; i++)
	{
		for(USkeletalBodySetup* SkeletalBodySetup : GetLagCompMesh()->GetPhysicsAsset()->SkeletalBodySetups)
		{
			const FKAggregateGeom& AggGeom = SkeletalBodySetup->AggGeom;

			for(const FKBoxElem& BoxElem : AggGeom.BoxElems)
			{
				DrawDebugBox(GetWorld(), HitboxPositions[c], FVector(BoxElem.X/2.0, BoxElem.Y/2.0, BoxElem.Z/2.0), HitboxRotations[c], Error == 0.0f ? FColor::Green : FColor::Red, true);
				c++;
			}

			for(const FKSphylElem& SphylElem : AggGeom.SphylElems)
			{
				DrawDebugCapsule(GetWorld(), HitboxPositions[c], SphylElem.GetScaledHalfLength(FVector(1.0f, 1.0f, 1.0f)), SphylElem.Radius, HitboxRotations[c], Error == 0.0f ? FColor::Green : FColor::Red, true);				
				c++;
			}
		}
	}
}

AWeapon_FPS* ACharacter_FPS::GetSelectedWeapon() const
{
	if (SelectedWeaponIndex < Weapons.Num())
	{
		return Weapons[SelectedWeaponIndex];
	}
	else
	{
		return nullptr;
	}
}

void ACharacter_FPS::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void ACharacter_FPS::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		AddMovementInput(FVector::RightVector, Value);
	}
}

bool ACharacter_FPS::FindDamagedBone(const TSet<ACharacter_FPS*>& LagCompensatedCharacters, const TArray<FHitResult>& HitResults,	const FVector& StartLocation, ACharacter_FPS*& HitCharacter, FName& HitBoneName)
{
	TSet<ACharacter_FPS*> OverlapCharacters;

	for(const FHitResult& HitResult : HitResults)
	{
		if (ACharacter_FPS* PlayerCharacter = Cast<ACharacter_FPS>(HitResult.Actor))
		{
			if (LagCompensatedCharacters.Contains(PlayerCharacter) && !OverlapCharacters.Contains(PlayerCharacter) && !PlayerCharacter->IsKilled())
			{
				OverlapCharacters.Add(PlayerCharacter);
			}
		}
	}

	if (OverlapCharacters.Num() > 0)
	{
		ACharacter_FPS* ClosestCharacter = nullptr;
		float ClosestDistance;
						
		for (auto Iterator = OverlapCharacters.CreateConstIterator(); Iterator; ++Iterator)
		{
			ACharacter_FPS* PlayerCharacter = *Iterator;

			float Distance = (PlayerCharacter->GetActorLocation() - StartLocation).Size();

			if (ClosestCharacter == nullptr || (Distance < ClosestDistance))
			{
				ClosestCharacter = PlayerCharacter;
				ClosestDistance = Distance;
			}
		}

		FName HighestPriorityBoneName;
		float HighestDamageMultiplier = 0.0f;

		for(const FHitResult& HitResult : HitResults)
		{
			if (HitResult.Actor == ClosestCharacter)
			{
				float DamageMultiplier = GameState->GetBoneDamageMultiplier(HitResult.BoneName);

				if (DamageMultiplier > HighestDamageMultiplier)
				{
					HighestPriorityBoneName = HitResult.BoneName;
					HighestDamageMultiplier = DamageMultiplier;
				}
			}
		}

		HitCharacter = ClosestCharacter;
		HitBoneName = HighestPriorityBoneName;

		return true;
	}

	return false;
}