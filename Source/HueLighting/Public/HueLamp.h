/*
MIT License Modified See LICENSE Files for more details
Copyright (c) 2022 Scott Tongue all rights reversed 
*/


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "HueLamp.generated.h"


class FHttpModule;
UCLASS()
class HUELIGHTING_API AHueLamp : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHueLamp();
	
	TObjectPtr<FHttpModule> HTTPHandler;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintGetter = CheckInUse)
		bool bUseLamp = true;
	
	UPROPERTY(BlueprintGetter = LampToBeUsed)
		bool bInUse = false;
	
	UPROPERTY(BlueprintGetter = HasLampBeenConfigured)
		bool bHasBeenConfigured = false;
	
	UPROPERTY(BlueprintGetter = GetLampName)
		FString LampName;
	
	FString DevicePath;
	FString DeviceKey;

	
	FVector CovertRGBToHSV(const FColor &RGB);
	
	virtual void CreateRequestBrightness(int32 Bri);
	virtual void CreateRequestColor(const FVector &HSV);
	virtual void CreateRequestTurnLightOnOff(bool bTurnOn);

	virtual void OnResponseTest( FHttpRequestPtr Request,  FHttpResponsePtr Response, bool bWasSuccessful);
public:
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupLamp(const FString &Path, const FString &Key, const FString &Name);
	virtual void Delete(){Destroy();}
	
	UFUNCTION(BlueprintCallable, Category = "Hue Light")
		virtual void TurnLightOnOff(bool bTurnOn);
	
	UFUNCTION(BlueprintCallable, Category = "Hue Light")
		virtual void SetColor(const FColor &Color);
	
	UFUNCTION(BlueprintCallable, Category = "Hue Light")
		virtual void SetBrightness(const int32 Brightness);
	
	UFUNCTION(BlueprintCallable, Category = "Hue Light" )
		virtual	void UseLampLight(bool bUse) {bUseLamp = bUse;};
	
	UFUNCTION(BlueprintPure, Category = "Hue Light")
		virtual bool CheckInUse();
	
	UFUNCTION(BlueprintPure, Category = "Hue Light")
		virtual bool HasLampBeenConfigured(){return bHasBeenConfigured;}
	
	UFUNCTION(BlueprintPure, Category = "Hue Light")
		virtual bool LampToBeUsed(){return bUseLamp;}
	
	UFUNCTION(BlueprintPure, Category = "Hue Light")
		virtual FString& GetLampName(){return LampName;}
};
