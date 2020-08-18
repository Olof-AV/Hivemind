// Fill out your copyright notice in the Description page of Project Settings.


#include "UFO.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

#include "../../Characters/Aliens/Character/AlienCharacter.h"

#include "../../Game_Rules/Hivemind_GameState.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AUFO::AUFO()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	//Setup trigger
	TriggerEscape = CreateDefaultSubobject<USphereComponent>("TriggerEscape");
	TriggerEscape->SetSphereRadius(500.f);

	TriggerEscape->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TriggerEscape->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerEscape->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(TriggerEscape);

	//Setup mesh
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
	MeshComp->SetupAttachment(TriggerEscape);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	
	//Setup enter
	{
		FScriptDelegate ScriptDelegate{};
		ScriptDelegate.BindUFunction(this, L"OnTriggerEnter");
		TriggerEscape->OnComponentBeginOverlap.AddUnique(ScriptDelegate);
	}

	//Setup exit
	{
		FScriptDelegate ScriptDelegate{};
		ScriptDelegate.BindUFunction(this, L"OnTriggerExit");
		TriggerEscape->OnComponentEndOverlap.AddUnique(ScriptDelegate);
	}
}

// Called when the game starts or when spawned
void AUFO::BeginPlay()
{
	Super::BeginPlay();
	
}

void AUFO::OnTriggerEnter_Implementation(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//When a player enters, increase amount
	AAlienCharacter* const AlienChar = Cast<AAlienCharacter>(OtherActor);
	if (AlienChar)
	{
		UE_LOG(LogTemp, Warning, L"Alien entered the UFO trigger");
		AHivemind_GameState* const State = Cast<AHivemind_GameState>(UGameplayStatics::GetGameState(this));
		if (State)
		{
			State->IncreaseCurrentAmountEscapingPlayers(1);
		}
	}
}

void AUFO::OnTriggerExit_Implementation(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//When a player exits, decrease amount
	AAlienCharacter* const AlienChar = Cast<AAlienCharacter>(OtherActor);
	if (AlienChar)
	{
		UE_LOG(LogTemp, Warning, L"Alien left the UFO trigger");
		AHivemind_GameState* const State = Cast<AHivemind_GameState>(UGameplayStatics::GetGameState(this));
		if (State)
		{
			State->IncreaseCurrentAmountEscapingPlayers(-1);
		}
	}
}

// Called every frame
void AUFO::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector AUFO::GetBeamLocation() const
{
	//Determine the offset transform (only location change)
	FTransform OffsetTransform = FTransform::Identity;
	OffsetTransform.SetLocation(BeamLocation);

	//Combine transforms to obtain final beam location
	return (OffsetTransform * GetActorTransform()).GetLocation();
}

void AUFO::ToggleTriggerEscape(bool bValue)
{
	//Enable/disable depending on input
	TriggerEscape->SetCollisionEnabled(bValue ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

