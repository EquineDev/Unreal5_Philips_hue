/*
MIT License Modified See LICENSE Files for more details
Copyright (c) 2022 Scott Tongue all rights reversed 
*/

#pragma once

#include "CoreMinimal.h"
#include "HueLamp.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "HueBridge.generated.h"


class FHttpModule;

//const FSTRING names 
const static FString CONFIG_FILE= TEXT("/Config/HueConfig.json");
const static FString VERB_GET = TEXT("GET");
const static FString VERB_POST = TEXT("POST");
const static FString TYPE = TEXT("type");
const static FString STATE = TEXT("/state");
const static FString USERNAME = TEXT("username");
const static FString NAME = TEXT("name");

USTRUCT()
struct FLightInfo
{
	GENERATED_USTRUCT_BODY() 
public:
	UPROPERTY();
		FString Name;
	UPROPERTY();
		bool bUseLight;
};

USTRUCT(BlueprintType)
struct FLightUse
{
	GENERATED_USTRUCT_BODY() 
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Lamp")
		FString LightName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Lamp")
		bool bUseLight;
};

USTRUCT(BlueprintType)
struct FHueBridgeConfig
{
	GENERATED_USTRUCT_BODY() 
public:
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Bridge")
		FString UserName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Bridge")
		FString HostName;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Bridge")
		FString AppName = "MyUnrealApp";
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Hue Bridge")
		TArray<FLightUse> Lights;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveConfig );
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTooManyRequests );
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFoundLights);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNewUserRequest, float, Message );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUserConfigured, bool, Message );

UCLASS()
class HUELIGHTING_API AHueBridge : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHueBridge();

	TObjectPtr<FHttpModule> HTTPHandler;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TMap<FString, TObjectPtr<AHueLamp>> HueLamps;
	FTimerHandle LinkBridgeTimer;
	bool bUserExist = false;

	UPROPERTY(BlueprintGetter = BridgeInUse, Category = "Hue Bridge Config")
		bool bInUse = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hue Bridge Config")
		float  DelayTimerForBridgePress = 5.0;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hue Bridge Config")
		FHueBridgeConfig HueBridgeConfig;
	
	
	virtual void OnResponseReceivedDiscover( FHttpRequestPtr Request,  FHttpResponsePtr Response, bool bWasSuccessful);
	virtual void OnResponseReceivedNewUser( FHttpRequestPtr Request,  FHttpResponsePtr Response, bool bWasSuccessful);
	virtual void OnResponseReceivedUserExist( FHttpRequestPtr Request,  FHttpResponsePtr Response, bool bWasSuccessful);
	virtual bool CheckIfBusy();
	
	void GetLightInfo(TSharedPtr<FJsonObject> JsonObject,  FLightInfo& LightInfoOut);
	void GetStringName(TSharedPtr<FJsonObject> JsonObject,  const FString& Field, FString& NameOut );

	void UserConfiguredCorrectly(bool Value);
public:
	
	UPROPERTY(BlueprintAssignable,Category = "Hue Bridge || Warnings" )
		FSaveConfig SaveWarning;
	
	UPROPERTY(BlueprintAssignable,Category = "Hue Bridge || Warnings" )
		FTooManyRequests RequestsBusy;
	
	UPROPERTY(BlueprintAssignable,Category = "Hue Bridge || Warnings" )
		FNewUserRequest RequestUser;
	
	UPROPERTY(BlueprintAssignable,Category = "Hue Bridge || Warnings" )
		FUserConfigured HasUserBeenConfigured;
	
	UPROPERTY(BlueprintAssignable,Category = "Hue Bridge || Warnings" )
		FFoundLights FoundDiscoverableLights;
	
	
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void DiscoverLamps();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void HueBridgeSetupTimer();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void SetupNewUser();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void SaveConfig();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void LoadConfig();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void SetHostName(const FString &Host)  { HueBridgeConfig.HostName = Host;}
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual AHueLamp* GetLamp(const FString &LampName);
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual bool DoesHueUserExist();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual void ClearOutLights();
	
	UFUNCTION(BlueprintCallable, Category = "Hue Bridge")
		virtual TArray<FString> GetAllLampNames(){  TArray<FString> Names; HueLamps.GetKeys(Names); return Names;  }
	
	UFUNCTION(BlueprintPure, Category = "Hue Bridge")
		virtual bool DoesLampExist(const FString &LampName) {return HueLamps.Contains(LampName);}
	
	UFUNCTION(BlueprintPure, Category = "Hue Bridge")
		virtual bool BridgeInUse(){return bInUse;}
	
	UFUNCTION(BlueprintNativeEvent, Category = "Hue Bridge")
		void HueBringTimerStarted(float timer);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Hue Bridge")
		void PleaseWaitingForBridgeRespond();
};