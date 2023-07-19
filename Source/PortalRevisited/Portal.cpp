// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include <stdexcept>

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"

#define PORTAL_COLLISION_PROFILE_NAME "Pawn_Hole"
#define STANDARD_COLLISION_PROFILE_NAME "Pawn"

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	PortalMask =
		CreateDefaultSubobject<UCapsuleComponent>("PortalMask");
	PortalMask->InitCapsuleSize(55.f, 96.0f);

	PortalMask->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
	PortalMask->OnComponentEndOverlap.AddDynamic(this, &APortal::OnOverlapEnd);

	MeshPortalHole = 
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("PortalHole")
		);
	MeshPortalHole->SetupAttachment(
		PortalMask
	);
	MeshPortalHole->CastShadow = false;
	// Hard coded object type.
	MeshPortalHole->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2);
	//MeshPortalHole->SetVisibility(false);

	PortalEntranceDirection =
		CreateDefaultSubobject<UArrowComponent>("PortalEntranceDirection");
}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortal::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (!OtherActor)
		return;

	if (OtherActor == this)
		return;

	if (!OtherComp)
		return;
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("OverlapBegin"));
		GEngine->AddOnScreenDebugMessage(
			-1, 5.f, FColor::Red,
			TEXT("OtherActor:") + OtherActor->GetName());
		GEngine->AddOnScreenDebugMessage(
			-1, 5.f, FColor::Red,
			TEXT("OverlappedComp:") + OverlappedComp->GetName());
		GEngine->AddOnScreenDebugMessage(
			-1, 5.f, FColor::Red,
			TEXT("OtherComp:") + OtherComp->GetName());
	}

	OtherComp->SetCollisionProfileName(TEXT(PORTAL_COLLISION_PROFILE_NAME));
}

void APortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
		return;

	if (OtherActor == this)
		return;

	if (!OtherComp)
		return;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("OverlapEnd"));
	}
	
	OtherComp->SetCollisionProfileName(TEXT(STANDARD_COLLISION_PROFILE_NAME));
}

