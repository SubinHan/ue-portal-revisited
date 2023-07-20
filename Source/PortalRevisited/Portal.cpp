// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"
#include "Private/DebugHelper.h"
#include <stdexcept>

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"

#define PORTAL_COLLISION_PROFILE_NAME "Pawn_Hole"
#define STANDARD_COLLISION_PROFILE_NAME "Pawn"

template<class T>
using Asset = ConstructorHelpers::FObjectFinder<T>;

constexpr uint8 DEFAULT_STENCIL_VALUE = 1;

void APortal::InitMeshPortalHole()
{
	MeshPortalHole =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("PortalHole"));
	MeshPortalHole->CastShadow = false;
	MeshPortalHole->SetupAttachment(RootComponent);
	//MeshPortalHole->SetVisibility(false);

	// TODO: Hard coded object type.
	MeshPortalHole->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2);

	Asset<UStaticMesh> PortalHoleMesh(
		TEXT("StaticMesh'/Game/SM_PortalRectangle.SM_PortalRectangle'"));

	if (PortalHoleMesh.Object)
	{
		MeshPortalHole->SetStaticMesh(PortalHoleMesh.Object);
	}
}

void APortal::InitPortalEnterMask()
{
	PortalEnterMask =
		CreateDefaultSubobject<UCapsuleComponent>("PortalEnterMask");
	PortalEnterMask->InitCapsuleSize(55.f, 96.0f);
	PortalEnterMask->SetupAttachment(MeshPortalHole);
	
	// TODO: Hard coded rotation, scale
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f));
	PortalEnterMask->SetRelativeRotation(Rotator);
	PortalEnterMask->SetRelativeScale3D(FVector(2.0f, 1.0f, 1.5f));

	PortalEnterMask->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
	PortalEnterMask->OnComponentEndOverlap.AddDynamic(this, &APortal::OnOverlapEnd);
}

void APortal::InitPortalEntranceDirection()
{
	PortalEntranceDirection =
		CreateDefaultSubobject<UArrowComponent>("PortalEntranceDirection");
	PortalEntranceDirection->SetupAttachment(MeshPortalHole);
	
	// TODO: Hard coded rotation
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0f, 90.0f, 0.0f));
	PortalEntranceDirection->SetRelativeRotation(Rotator);
}

void APortal::InitPortalInner()
{
	PortalInner =
		CreateDefaultSubobject<UStaticMeshComponent>("PortalInner");
	PortalInner->SetupAttachment(MeshPortalHole);
	PortalInner->SetCollisionProfileName("NoCollision");

	// TODO: Hard coded scale
	PortalInner->SetRelativeLocation(FVector(0.0f, 0.0f, 30.0f));
	PortalInner->SetRelativeScale3D(FVector(3.0f, 2.0f, 0.2f));

	Asset<UStaticMesh> PortalInnerMesh(
		TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));

	if (PortalInnerMesh.Object)
	{
		PortalInner->SetStaticMesh(PortalInnerMesh.Object);
	}

	Asset<UMaterial> PortalInnerMaterial(
		TEXT("Material'/Game/M_Portal.M_Portal'"));

	if (PortalInnerMaterial.Object)
	{
		PortalInner->SetMaterial(0, PortalInnerMaterial.Object);
	}

	PortalInner->SetRenderCustomDepth(true);
	PortalInner->SetCustomDepthStencilValue(DEFAULT_STENCIL_VALUE);
}

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetMobility(EComponentMobility::Movable);

	InitMeshPortalHole();
	InitPortalEnterMask();
	InitPortalEntranceDirection();
	InitPortalInner();

	DebugHelper::PrintText(L"PortalCreated");
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

