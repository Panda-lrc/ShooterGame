// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterPlayerController.h"
#include "Blueprint/UserWidget.h"

AShooterPlayerController::AShooterPlayerController() {

}

void AShooterPlayerController::BeginPlay() {
	Super::BeginPlay();
	if (HUDOverlayClass) {
		HUDDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if (HUDDOverlay) {
			HUDDOverlay->AddToViewport();
			HUDDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}