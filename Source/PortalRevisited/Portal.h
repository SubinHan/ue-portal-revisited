// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Portal.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;

UCLASS()
class PORTALREVISITED_API APortal : public AActor
{
	GENERATED_BODY()

	/**
	 * 
	 */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	UStaticMeshComponent* MeshPortalHole;

	/**
	 * If a dynamic mesh has overlapped this mask, then collision channel
	 * will be changed to enter the portal ignoring static mesh behind.
	 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> PortalMask;
	
public:	
	// Sets default values for this actor's properties
	APortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);



};
