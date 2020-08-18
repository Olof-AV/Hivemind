// Fill out your copyright notice in the Description page of Project Settings.


#include "NodeAI.h"
#include "Components/BillboardComponent.h"

#include "Kismet/KismetMathLibrary.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

// Sets default values
ANodeAI::ANodeAI()
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
	
	//Initialise
	SetRootComponent(CreateDefaultSubobject<UBillboardComponent>("Root"));
}

bool ANodeAI::ShouldTickIfViewportsOnly() const
{
#if WITH_EDITOR
	//Draw arrows indicating next nodes
	if (NextNodes.Num() > 0)
	{
		UWorld* const World = GetWorld();
		if (World)
		{
			const FVector ActorPos = GetActorLocation();
			for (AActor* const Actor : NextNodes)
			{
				if (Actor)
				{
					//DrawDebugLine(World, ActorPos, Actor->GetActorLocation(), FColor::Blue, false, -1.f, '\000', 4.f);
					DrawDebugDirectionalArrow(World, ActorPos, Actor->GetActorLocation(), 1000.f, FColor::Blue, false, -1.f, '\000', 4.f);
				}
			}
		}
	}

	return true;
#else
	return false;
#endif
}

ANodeAI* ANodeAI::GetNextNode() const
{
	if (NextNodes.Num() > 0)
	{
		//Get random node in array
		const int32 Index = UKismetMathLibrary::RandomIntegerInRange(0, NextNodes.Num() - 1);

		//Return it
		if (NextNodes[Index])
		{
			return NextNodes[Index];
		}
	}

	//Something went wrong
	return nullptr;
}

// Called when the game starts or when spawned
void ANodeAI::BeginPlay()
{
	Super::BeginPlay();
	
	//Drops down to the floor at beginning
	UWorld* const World = GetWorld();
	if (World)
	{
		FHitResult HitResult{};
		const bool Result = World->LineTraceSingleByChannel(HitResult, GetActorLocation(), GetActorLocation() + -FVector::UpVector * 9999.f, ECollisionChannel::ECC_Visibility);

		if (Result)
		{
			SetActorLocation(HitResult.Location);
		}
	}
}
