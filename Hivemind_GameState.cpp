// Fill out your copyright notice in the Description page of Project Settings.


#include "Hivemind_GameState.h"

//Added
#include "GameFramework/PlayerState.h"
#include "HivemindGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "../Characters/Aliens/Character/AlienCharacter.h"
#include "../Components/HealthComponent.h"
#include "../Characters/Aliens/Link_System/AlienPowerSource.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

#include "Components/SkeletalMeshComponent.h"

AHivemind_GameState::AHivemind_GameState()
{
	//Initialise
	CurrentAmountTime = 9'999.f;
	CurrentAmountCollectedParts = 0;
	CurrentState = FGameStates::Start;
	CurrentAmountRespawnsLeft = 0;

	//Disable damage
	bCanBeDamaged = false;

	//Disable tick
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;

	//Audio
	MusicFadeLevel = 0.f;
}

void AHivemind_GameState::HandleBeginPlay()
{
	//Get players
	{
		//Get player states from which we will get players
		TArray<APlayerState*> PlayerStates = PlayerArray;

		//Go over collected states
		for (APlayerState* const State : PlayerStates)
		{
			if (State)
			{
				//If the pawn is valid, add it to our saved array
				APawn* const Pawn = State->GetPawn();
				if (Pawn)
				{
					AAlienCharacter* const Alien = Cast<AAlienCharacter>(Pawn);
					AlienArray.AddUnique(Alien);
				}
			}
		}
	}

	//Set player colours
	for (int32 i{}; i < AlienArray.Num(); ++i)
	{
		AlienArray[i]->GetMesh()->SetCustomDepthStencilValue(i + 1);

		switch (i)
		{
		case 0:
			AlienArray[i]->SetPlayerColour(FColor::Red);
			break;

		case 1:
			AlienArray[i]->SetPlayerColour(FColor::Green);
			break;

		case 2:
			AlienArray[i]->SetPlayerColour(FColor::Blue);
			break;

		case 3:
			AlienArray[i]->SetPlayerColour(FColor::Purple);
			break;

		default:
			AlienArray[i]->SetPlayerColour(FColor::MakeRandomColor());
			break;
		}
	}

	//Call super after getting players to make sure everything is valid
	Super::HandleBeginPlay();

	//Set state to begin
	SetCurrentState(FGameStates::Start);

	//Log
	UE_LOG(LogTemp, Warning, L"HandleBeginPlay called");
}

#pragma region GettersSetters
FGameStates AHivemind_GameState::GetCurrentState() const
{
	return CurrentState;
}

void AHivemind_GameState::SetCurrentState(FGameStates NewState)
{
	//Remember to set new state
	CurrentState = NewState;

	//Ask the gamemode to handle the change of states
	AHivemindGameModeBase* const GameMode = Cast<AHivemindGameModeBase>(AuthorityGameMode);
	if (GameMode)
	{
		GameMode->HandleStateChange(NewState);
	}
}

float AHivemind_GameState::GetCurrentAmountTime() const
{
	return CurrentAmountTime;
}

void AHivemind_GameState::SetCurrentAmountTime(float Value)
{
	CurrentAmountTime = Value;
}

float AHivemind_GameState::IncreaseCurrentTime(float Value)
{
	//Add to time and return after incremented amount
	CurrentAmountTime += Value;
	return CurrentAmountTime;
}

int AHivemind_GameState::GetCurrentAmountCollectedParts() const
{
	return CurrentAmountCollectedParts;
}

void AHivemind_GameState::SetCurrentAmountCollectedParts(int Amount)
{
	CurrentAmountCollectedParts = Amount;

	//Since the amount of parts was updated, check if it would be equal/superior to required amount
	CheckCurrentAmountCollectedParts();
}

int AHivemind_GameState::IncreaseCurrentAmountCollectedParts(int Amount)
{
	//Increase amount of collected parts
	CurrentAmountCollectedParts += Amount;

	//Since the amount of parts was updated, check if it would be equal/superior to required amount
	CheckCurrentAmountCollectedParts();

	//Return new amount
	return CurrentAmountCollectedParts;
}

const TArray<AAlienCharacter*>& AHivemind_GameState::GetAlienArray() const
{
	return AlienArray;
}
int AHivemind_GameState::GetAlienAliveCount() const
{
	//Retrive amount of aliens that are not dead, but can be downed
	int Count = 0;
	for (AAlienCharacter* const Char : AlienArray)
	{
		if (Char)
		{
			UHealthComponent* const Health = Cast<UHealthComponent>(Char->GetComponentByClass(UHealthComponent::StaticClass()));
			if (Health)
			{
				if (!Health->IsDead())
				{
					++Count;
				}
			}
		}
	}

	return Count;
}

int AHivemind_GameState::GetAlienUpCount() const
{
	//Retrive amount of aliens that are still running around, not dead or downed
	int Count = 0;
	for (AAlienCharacter* const Char : AlienArray)
	{
		if (Char)
		{
			UHealthComponent* const Health = Cast<UHealthComponent>(Char->GetComponentByClass(UHealthComponent::StaticClass()));
			if (Health)
			{
				if ( !(Health->IsDead() || Health->IsDowned()) )
				{
					++Count;
				}
			}
		}
	}

	return Count;
}

void AHivemind_GameState::NotifyAlienDowned(AAlienCharacter* const Char)
{
	//Can only notify when playing, doesn't matter otherwise
	if (CurrentState == FGameStates::Play)
	{
		if(GetAlienUpCount() == 0)
		{
			SetCurrentState(FGameStates::Loss);
		}
	}
}

void AHivemind_GameState::NotifyAlienDeath(AAlienCharacter* const Char)
{
	//Can only notify when playing, doesn't matter otherwise
	if (CurrentState == FGameStates::Play)
	{
		//Remove time
		const AHivemindGameModeBase* const Mode = GetDefaultGameMode<AHivemindGameModeBase>();
		if (Mode)
		{
			IncreaseCurrentTime(-Mode->GetDeathTimePenalty());
			OnTimeLostDelegate.Broadcast(-Mode->GetDeathTimePenalty());
		}

		//Respawn feature down below, will need to be tested if it should stay in or not
		if (CurrentAmountRespawnsLeft > 0)
		{
			if (Char)
			{
				Char->Respawn();
			}
			
			CurrentAmountRespawnsLeft--;

			//Notify delegates
			OnRespawnUseDelegate.Broadcast();
		}

		//If no players left, game over
		if (GetAlienAliveCount() == 0)
		{
			SetCurrentState(FGameStates::Loss);
		}
	}
}

int AHivemind_GameState::GetCurrentAmountRespawnsLeft() const
{
	return CurrentAmountRespawnsLeft;
}

void AHivemind_GameState::SetCurrentAmountRespawnsLeft(int Amount)
{
	CurrentAmountRespawnsLeft = Amount;
}

int AHivemind_GameState::GetCurrentAmountEscapingPlayers() const
{
	return CurrentAmountEscapingPlayers;
}

int AHivemind_GameState::IncreaseCurrentAmountEscapingPlayers(int Amount)
{
	//Increase and return new amount
	CurrentAmountEscapingPlayers += Amount;

	//Limit was reached, we won
	if (CurrentAmountEscapingPlayers >= GetAlienAliveCount())
	{
		SetCurrentState(FGameStates::Win);
	}

	return CurrentAmountEscapingPlayers;
}

bool AHivemind_GameState::IsEnemyAlerted() const
{
	return CurrentAmountAlertGuards != 0;
}

int AHivemind_GameState::IncreaseCurrentAmountAlertGuards(int Amount)
{
	//If first guard to be alerted, broadcast an alert
	if (Amount > 0 && CurrentAmountAlertGuards == 0)
	{
		OnGuardsAlertDelegate.Broadcast();
		if (Sound_Alert) { UGameplayStatics::PlaySound2D(this, Sound_Alert); }
		UE_LOG(LogTemp, Warning, L"OnGuardsAlertDelegate broadcast...");
	}
	//If last guard to be alert and we go back to 0, then broadcast back to calm
	else if (Amount < 0 && CurrentAmountAlertGuards != 0 && (CurrentAmountAlertGuards + Amount) == 0)
	{
		OnGuardsLostDelegate.Broadcast();
		if (Sound_Lost) { UGameplayStatics::PlaySound2D(this, Sound_Lost); }
		UE_LOG(LogTemp, Warning, L"OnGuardsLostDelegate broadcast...");
	}

	//Change + return
	CurrentAmountAlertGuards += Amount;
	return CurrentAmountAlertGuards;
}

float AHivemind_GameState::GetMusicFadeLevel() const
{
	return MusicFadeLevel;
}

void AHivemind_GameState::SetMusicFadeLevel(float Value)
{
	MusicFadeLevel = Value;
}
int AHivemind_GameState::GetCurrentAmountGuardsKilled() const
{
	return CurrentAmountGuardsKilled;
}
void AHivemind_GameState::IncreaseCurrentAmountGuardsKilled(int Amount)
{
	CurrentAmountGuardsKilled += Amount;
}
float AHivemind_GameState::GetCurrentTotalTime() const
{
	return CurrentTotalTime;
}
void AHivemind_GameState::IncreaseCurrentTotalTime(float DeltaTime)
{
	CurrentTotalTime += DeltaTime;
}
#pragma endregion GettersSetters

void AHivemind_GameState::AddHealthToPlayers(float Amount)
{
	for (AAlienCharacter* const Char : AlienArray)
	{
		//Don't add health to dead/downed players, otherwise you risk flinging the power source hardcore and it's just weird 
		UHealthComponent* const HealthComp = Cast<UHealthComponent>(Char->GetComponentByClass(UHealthComponent::StaticClass()));
		if ( !(HealthComp->IsDead() || HealthComp->IsDowned()) )
		{
			HealthComp->AddHealth(Amount);
		}
	}
}

void AHivemind_GameState::TogglePause(APlayerController* const PController)
{
	bool NewValue = !UGameplayStatics::IsGamePaused(this);

	//Change state and broadcast
	UGameplayStatics::SetGamePaused(this, NewValue);
	OnGamePausedDelegate.Broadcast(NewValue, PController);

	//Sound
	if (Sound_Pause && Sound_Unpause)
	{
		UGameplayStatics::PlaySound2D(this, ((NewValue) ? Sound_Pause : Sound_Unpause));
	}

	AHivemindGameModeBase* const GameMode = Cast<AHivemindGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (GameMode) { GameMode->HandleMusicLowPass(NewValue); }
}

void AHivemind_GameState::NotifyPartCollected(float HealthBonus)
{
	//Get gamemode for time bonus
	const AHivemindGameModeBase* const GameMode = GetDefaultGameMode<AHivemindGameModeBase>();
	if (GameMode)
	{
		IncreaseCurrentAmountCollectedParts(1);
		IncreaseCurrentTime(GameMode->GetPartsTimeBonus());
		AddHealthToPlayers(HealthBonus);

		//Notify part collected + time obtained
		OnTimeGainedDelegate.Broadcast(GameMode->GetPartsTimeBonus());
		OnPartCollectedDelegate.Broadcast();
	}
}

void AHivemind_GameState::CheckCurrentAmountCollectedParts()
{
	//Get gamemode
	const AHivemindGameModeBase* const GameMode = GetDefaultGameMode<AHivemindGameModeBase>();
	if (GameMode)
	{
		//If amount is correct, players win
		if (GameMode->GetAmountPartsToRepair() <= CurrentAmountCollectedParts)
		{
			SetCurrentState(FGameStates::Escape);
		}
	}
}
