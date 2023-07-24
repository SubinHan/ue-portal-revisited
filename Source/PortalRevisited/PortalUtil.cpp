// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalUtil.h"

#include <stdexcept>

// Sets default values
APortalUtil::APortalUtil()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APortalUtil::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame




void APortalUtil::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
