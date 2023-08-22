// Copyright Epic Games, Inc. All Rights Reserved.

#include "PortalRevisitedProjectile.h"

#include <stdexcept>

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"

DEFINE_LOG_CATEGORY(PortalProjectile);

APortalRevisitedProjectile::APortalRevisitedProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(0.01f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &APortalRevisitedProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// Die after 3 seconds by default
	InitialLifeSpan = 1.0f;
}

void APortalRevisitedProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor) && (OtherActor != this) && (OtherComp))
	{
		UE_LOG(PortalProjectile, Log, TEXT("PortalProjectile: Projectile destroyed."));
		Destroy();
	}
}

void APortalRevisitedProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void APortalRevisitedProjectile::SetPrjectileDestination(const FVector NewDestination)
{
	Destination = NewDestination;
}
