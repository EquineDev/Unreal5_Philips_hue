/*
MIT License Modified See LICENSE Files for more details
Copyright (c) 2022 Scott Tongue all rights reversed 
*/
#include "HueLamp.h"

#include "HttpModule.h"
#include "Dom/JsonObject.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"

const static FString VERB_PUT = TEXT("PUT");

// Sets default values
AHueLamp::AHueLamp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AHueLamp::BeginPlay()
{
	Super::BeginPlay();
	
}

/**
 * @brief 
 * Converts RGB over int HSV Format for Hue lights :: Internal Call
 * Bri has a max value of 254
 * Hue has a max value of 65535
 * Sat has a max value of 254
 * FVector is used a X as Hue, Y as Sat, Z as Bri
 * @return
 */
FVector AHueLamp::CovertRGBToHSV(const FColor &RGB)
{
	const float Max = FMath::Max(RGB.R, FMath::Max(RGB.G, RGB.B));
	const float Min = FMath::Min(RGB.R, FMath::Min(RGB.G,RGB.B));
	const float Brightness = RGB.A;

	float Hue;
	float Saturation;

	if(Max == Min)
	{
		Hue = 0;
		Saturation = 0;
	}
	else
	{
		const float C = Max - Min;
		if(Max == RGB.R)
		{
			Hue = (RGB.G - RGB.B) / C;
		}
		else if(Max == RGB.G)
		{
			Hue = (RGB.B - RGB.R) / C + 2;
		}
		else
		{
			Hue = (RGB.R - RGB.G) / C + 4; 
		}

		Hue *= 60;
		if(Hue < 0)
		{
			Hue += 360;
		}

		Saturation = C / Max;
	}
	
	return  FVector(Hue, Saturation, Brightness);;
}

//Create Hue Light Lamp Brightness request  :: Internal Call
void AHueLamp::CreateRequestBrightness(int32 Bri)
{
	//if Bri is set to 0 light is off, and set data to be turn off only to save payload size
	if(Bri <= 0)
	{
		CreateRequestTurnLightOnOff(false);
		return; 
	}
	
	//Setup HTTP REST CALL 
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	const FString URL = DevicePath;
	const TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();
	
	//Fill out JSON DATA
	RequestOBJ->SetBoolField(TEXT("on"), true);
	RequestOBJ->SetNumberField(TEXT("bri"),Bri);
	
	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);
	
	Request->SetURL(URL);
	Request->SetVerb(VERB_PUT);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	
	bInUse = false;
}
//Create Hue Light Lamp Color request with HSV format :: Internal Call
void AHueLamp::CreateRequestColor(const FVector& HSV)
{
	//if Bri is set to 0 light is off, and set data to be turn off only to save payload size
	if(static_cast<int32>(HSV.Z * 255) <= 0)
	{
		CreateRequestTurnLightOnOff(false);
		return; 
	}
	
	//Setup HTTP REST CALL  
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	const FString URL = DevicePath;
	const TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();

	//Fill out JSON DATA
	RequestOBJ->SetBoolField(TEXT("on"), true);
	// Magic numbers are the hue bridge max values, Hue 65535,Sat 254 Bri 254
	RequestOBJ->SetNumberField(TEXT("hue"),static_cast<int32>(HSV.X / 360 * 65535));
	RequestOBJ->SetNumberField(TEXT("sat"),static_cast<int32>(HSV.Y * 254));
	RequestOBJ->SetNumberField(TEXT("bri"),static_cast<int32>(HSV.Z * 254));
	
	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);
	
	Request->SetURL(URL);
	Request->SetVerb(VERB_PUT);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	
	bInUse = false;
}
//Create Hue Light Lamp On/Off request :: Internal Call
void AHueLamp::CreateRequestTurnLightOnOff(bool bTurnOn)
{
	//Setup HTTP REST CALL  
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	const FString URL = DevicePath;
	const TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();
	
	//Fill out JSON DATA
	RequestOBJ->SetBoolField(TEXT("on"), bTurnOn);
	
	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);
	
	Request->SetURL(URL);
	Request->SetVerb(VERB_PUT);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	
	bInUse = false;
}

//Setup for testing Delegate Responds binds 
void AHueLamp::OnResponseTest(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	const FString Data = Response->GetContentAsString();
	UE_LOG(LogTemp, Warning, TEXT("%s"),*Data);
}


// Called every frame
void AHueLamp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

//Setup Hue Lamp Settings Path, Key, and device name
void AHueLamp::SetupLamp(const FString& Path, const FString& Key,const FString &Name)
{
	DevicePath = Path;
	DeviceKey = Key;
	LampName = Name;
	bHasBeenConfigured =true;
}

//Turn lamp on or off
void AHueLamp::TurnLightOnOff(bool bTurnOn)
{
	if(CheckInUse())
		return;
	
	bInUse = true;
	CreateRequestTurnLightOnOff(bTurnOn);
}

//set lamp color with FColor Gets converted to HSV
void AHueLamp::SetColor(const FColor &Color)
{
	if(CheckInUse())
		return;
	
	bInUse = true;
	CreateRequestColor(CovertRGBToHSV(Color));
}

//set lamp Brightness
void AHueLamp::SetBrightness(const int32 Brightness)
{
	if(CheckInUse())
		return;

	bInUse = true;
	CreateRequestBrightness(Brightness);
}

//Check to see if we are using lamp to prevent a flood of requests
bool AHueLamp::CheckInUse()
{
	if(bInUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("Device in Use!"));	
		return true;
	}
	return false;
}

