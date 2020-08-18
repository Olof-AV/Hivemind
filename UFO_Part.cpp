// Fill out your copyright notice in the Description page of Project Settings.


#include "UFO_Part.h"

//Added
#include "Components/StaticMeshComponent.h"
#include "UFO_Part.h"
#include "Kismet/GameplayStatics.h"
#include "../UFO/UFO.h"

#include "../../Game_Rules/Hivemind_GameState.h"

#include "Sound/SoundCue.h"

// Sets default values
AUFO_Part::AUFO_Part()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	//No damage
	bCanBeDamaged = false;

	//The trigger zone of the power source will automatically collect this
	//Make sure that it can actually respond to ECC_GameTraceChannel1 though
	Mesh->SetCollisionObjectType(ECC_GameTraceChannel1);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//Player can't walk through it
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Mesh->CanCharacterStepUpOn = ECB_No;

	//Setup variables
	HealthBonus = 25.f;

	//Set root
	SetRootComponent(Mesh);

	//Is an autoaim target
	Tags.Add("AutoAimTarget");
}

void AUFO_Part::Collect()
{
	//Call super
	Super::Collect();

	//Specific stuff for UFO part
	UWorld* const World = GetWorld();
	if (World)
	{
		//Physics
		Mesh->SetSimulatePhysics(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);

		//Also change collision types when beaming
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		//Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

		//Notify we got a part
		AHivemind_GameState* const GameState = World->GetGameState<AHivemind_GameState>();
		if (GameState)
		{
			GameState->NotifyPartCollected(HealthBonus);
		}
	}

	//Not autoaim target anymore
	Tags.Remove("AutoAimTarget");

	//Play sound
	UGameplayStatics::SpawnSoundAtLocation(this, Sound_Collect, GetActorLocation());

	//Teleport (beam particles are played by power source)
	SetActorLocation(BeamTarget);
}

// Called when the game starts or when spawned
void AUFO_Part::BeginPlay()
{
	Super::BeginPlay();
	
	//Retrieve beam target
	AUFO* const UFO = Cast<AUFO>(UGameplayStatics::GetActorOfClass(this, AUFO::StaticClass()));
	BeamTarget = UFO->GetBeamLocation();
}