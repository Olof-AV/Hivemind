// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSpawner.h"

//Added
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "BaseCollectible.h"
#include "Kismet/GameplayStatics.h"

#include "Materials/MaterialInstanceDynamic.h"

// Sets default values
ABaseSpawner::ABaseSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//Setup mesh
	SpawnerMesh = CreateDefaultSubobject<UStaticMeshComponent>("SpawnerMesh");
	SpawnerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SpawnerMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SpawnerMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore); //Power source can't crash into this
	SetRootComponent(SpawnerMesh);

	//Setup light
	SpawnerLight = CreateDefaultSubobject<UPointLightComponent>("SpawnerLight");
	SpawnerLight->SetupAttachment(SpawnerMesh);
	SpawnerLight->SetMobility(EComponentMobility::Movable);
	SpawnerLight->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	SpawnerLight->SetCastShadows(false);

	//Setup attach point
	AttachPoint = CreateDefaultSubobject<USceneComponent>("AttachPoint");
	AttachPoint->SetupAttachment(SpawnerMesh);

	//Setup
	bActive = false;
}

// Called when the game starts or when spawned
void ABaseSpawner::BeginPlay()
{
	//Call super
	Super::BeginPlay();

	//Move attach
	AttachPoint->SetRelativeLocation(AttachPointInitialPos);

	//Save max light intensity
	MaxLightIntensity = SpawnerLight->Intensity;

	//Make material instance dynamic
	MaterialInstDyn = SpawnerMesh->CreateAndSetMaterialInstanceDynamic(1);

	//Get max
	FMaterialParameterInfo ParamInfo{};
	ParamInfo.Name = "Emissive";
	MaterialInstDyn->GetScalarParameterDefaultValue(ParamInfo, MaterialMaxEmissive);
}

// Called every frame
void ABaseSpawner::Tick(float DeltaTime)
{
	//Call super
	Super::Tick(DeltaTime);

	//Emissive on mat inst dynamic
	float Intensity;
	FMaterialParameterInfo ParamInfo{};
	ParamInfo.Name = "Emissive";
	MaterialInstDyn->GetScalarParameterValue(ParamInfo, Intensity);

	//Some extra behaviour when active/inactive
	if (bActive)
	{
		//Rotate a little bit 
		AttachPoint->AddLocalRotation(FRotator(0.f, DeltaTime * 10.f, 0.f));

		//Light intensity goes towards max
		SpawnerLight->SetIntensity( FMath::Min(FMath::Lerp(SpawnerLight->Intensity, MaxLightIntensity, DeltaTime * 10.f), MaxLightIntensity));
	}
	else
	{
		//Light intensity goes towards 0
		SpawnerLight->SetIntensity( FMath::Max(FMath::Lerp(SpawnerLight->Intensity, 0.f, DeltaTime * 10.f), 0.f));
	}
	
	//Lerp mat inst emissive intensity
	Intensity = FMath::Lerp(Intensity, (bActive) ? MaterialMaxEmissive : 0.f, DeltaTime * 10.f);

	//Set mat inst emissive intensity
	MaterialInstDyn->SetScalarParameterValue("Emissive", Intensity);
}

bool ABaseSpawner::IsSpawnerActive() const
{
	return bActive;
}

void ABaseSpawner::SetSpawnerActive(bool bValue)
{
	//Setup
	bActive = bValue;

	//Determining what to do with linked object
	if (bActive)
	{
		//Nothing linked, spawn a new one
		if (!LinkedCollectible)
		{
			UWorld* const World = GetWorld();
			if(World)
			{
				const int32 RandomIndex = FMath::RandRange(0, SpawnableObjects.Num() - 1);
				LinkedCollectible = World->SpawnActor<ABaseCollectible>(SpawnableObjects[RandomIndex]);
				LinkedCollectible->SetLinkedSpawner(this);
				LinkedCollectible->AttachToComponent(AttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
		}
	}
	else
	{
		//Remove link
		if (LinkedCollectible)
		{
			LinkedCollectible->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			LinkedCollectible->SetLinkedSpawner(nullptr);
		}
	}
}

