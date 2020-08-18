// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerSpawnGroup.h"
#include "GameFramework/PlayerStart.h"

#include "Components/BillboardComponent.h"

#include "../../Environment/UFO/UFO.h"

#if WITH_EDITOR
	#include "DrawDebugHelpers.h"
#endif

// Sets default values
APlayerSpawnGroup::APlayerSpawnGroup()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.

#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
#else
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
#endif

	//Disable damage
	bCanBeDamaged = false;

	//Set root
	SetRootComponent(CreateDefaultSubobject<UBillboardComponent>("Root"));
}

bool APlayerSpawnGroup::ShouldTickIfViewportsOnly() const
{
#if WITH_EDITOR
	//Draw arrows indicating next nodes
	if (SpawnPoints.Num() > 0)
	{
		UWorld* const World = GetWorld();
		if (World)
		{
			const FVector ActorPos = GetActorLocation();
			for (AActor* const Actor : SpawnPoints)
			{
				if (Actor)
				{
					//DrawDebugLine(World, ActorPos, Actor->GetActorLocation(), FColor::Blue, false, -1.f, '\000', 4.f);
					DrawDebugDirectionalArrow(World, ActorPos, Actor->GetActorLocation(), 1000.f, FColor::Green, false, -1.f, '\000', 4.f);
				}
			}

			if (LinkedUFO)
			{
				DrawDebugDirectionalArrow(World, ActorPos, LinkedUFO->GetActorLocation(), 1000.f, FColor::Green, false, -1.f, '\000', 4.f);
			}
		}
	}

	return true;
#else
	return false;
#endif
}

TArray<APlayerStart*> APlayerSpawnGroup::GetSpawnPoints() const
{
	return SpawnPoints;
}

AUFO* const APlayerSpawnGroup::GetLinkedUFO() const
{
	return LinkedUFO;
}
