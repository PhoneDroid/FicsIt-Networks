﻿#include "FINScreenHolo.h"

#include "FGBuildableFoundation.h"
#include "FGBuildableWall.h"
#include "FINScreen.h"
#include "UnrealNetwork.h"

AFINScreenHolo::AFINScreenHolo() {
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void AFINScreenHolo::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	if ((OldScreenHeight != ScreenHeight || OldScreenWidth != ScreenWidth)) {
		OldScreenHeight = ScreenHeight;
		OldScreenWidth = ScreenWidth;
		
		// Clear Components
		for (UStaticMeshComponent* comp : Parts) {
			comp->UnregisterComponent();
			comp->SetActive(false);
			comp->DestroyComponent();
		}
		Parts.Empty();

		// Create Components
		UStaticMesh* MiddlePartMesh = Cast<AFINScreen>(mBuildClass->GetDefaultObject())->ScreenMiddle;
		UStaticMesh* EdgePartMesh = Cast<AFINScreen>(mBuildClass->GetDefaultObject())->ScreenEdge;
		UStaticMesh* CornerPartMesh = Cast<AFINScreen>(mBuildClass->GetDefaultObject())->ScreenCorner;
		AFINScreen::SpawnComponents(ScreenWidth, ScreenHeight, MiddlePartMesh, EdgePartMesh, CornerPartMesh, this, RootComponent, Parts);
		for (UStaticMeshComponent* Part : Parts) {
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void AFINScreenHolo::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	SetActorTickEnabled(false);
	
	for (UStaticMeshComponent* Part : Parts) {
		Part->UnregisterComponent();
		Part->SetActive(false);
		Part->DestroyComponent();
	}
	Parts.Empty();
	SetActorHiddenInGame(true);
	SML::Logging::error("EndPlay");
}

void AFINScreenHolo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFINScreenHolo, ScreenWidth);
	DOREPLIFETIME(AFINScreenHolo, ScreenHeight);
}

bool AFINScreenHolo::DoMultiStepPlacement(bool isInputFromARelease) {
	if (bSnapped) {
		return true;
	} else {
		bSnapped = true;
		return false;
	}
}

int32 AFINScreenHolo::GetBaseCostMultiplier() const {
	return FMath::Abs(ScreenWidth) * FMath::Abs(ScreenHeight);
}

bool AFINScreenHolo::IsValidHitResult(const FHitResult& hitResult) const {
	if (bSnapped) return true;
	return hitResult.GetActor() && (hitResult.GetActor()->GetClass()->IsChildOf<AFGBuildableWall>() || hitResult.GetActor()->GetClass()->IsChildOf<AFGBuildableFoundation>());
}

void AFINScreenHolo::SetHologramLocationAndRotation(const FHitResult& hitResult) {
	if (bSnapped) {
		// Calculate new Screen Size
		FVector PlaneNormal = Normal;
		PlaneNormal.Normalize(0.01);
		FVector PlanePosition = GetActorLocation();
		FVector LineDirection = (hitResult.TraceEnd - hitResult.TraceStart);
		LineDirection.Normalize(0.01);
		FVector LinePosition = hitResult.TraceStart;
		float PlaneDotLine = FVector::DotProduct(PlaneNormal, LineDirection);
		if (PlaneDotLine == 0.0) {
			ScreenHeight = 1;
			ScreenWidth = 1;
		} else {
			double t = (FVector::DotProduct(PlaneNormal, PlanePosition) - FVector::DotProduct(PlaneNormal, LinePosition)) / PlaneDotLine;
			FVector ViewPostion = LinePosition + (LineDirection * t);
			FVector ScreenDiagonal = ViewPostion - PlanePosition;
			FVector TestDiagonal = GetActorRotation().UnrotateVector(ScreenDiagonal);
			ScreenHeight = FMath::Clamp(FMath::RoundToInt((TestDiagonal.Z + (TestDiagonal.Z < 0 ? -100.0 : 100.0)) / 100.0), -10, 10);
			ScreenWidth = FMath::Clamp(FMath::RoundToInt((TestDiagonal.Y + (TestDiagonal.Y < 0 ? -100.0 : 100.0)) / 100.0), -10, 10);
			if (ScreenHeight == 0) ScreenHeight = 1;
			if (ScreenWidth == 0) ScreenWidth = 1;
		}
	} else {
		Normal = hitResult.ImpactNormal;
		ScreenHeight = 1;
		ScreenWidth = 1;
		FRotator SnappedActorRotation = FRotator::ZeroRotator;
		if (AActor* SnappedActor = hitResult.GetActor()) {
			SnappedActorRotation = FRotator(0,0,SnappedActor->GetActorRotation().Roll);
		}
		SetActorLocationAndRotation(hitResult.ImpactPoint, Normal.Rotation() + SnappedActorRotation + FRotator(0,0, GetScrollRotateValue()));
	}
}

AActor* AFINScreenHolo::Construct(TArray<AActor*>& out_children, FNetConstructionID netConstructionID) {
	bSnapped = false;

	return Super::Construct(out_children, netConstructionID);
}

void AFINScreenHolo::CheckValidFloor() {}

void AFINScreenHolo::ConfigureActor(AFGBuildable* inBuildable) const {
	Super::ConfigureActor(inBuildable);
	
	AFINScreen* Screen = Cast<AFINScreen>(inBuildable);
	Screen->ScreenHeight = ScreenHeight;
	Screen->ScreenWidth = ScreenWidth;
}
