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
FVector AHueLamp::CovertRGBToHSV(const FColor& RGB)
{
	const float Max = FMath::Max(RGB.R, FMath::Max(RGB.G, RGB.B));
	const float Min = FMath::Min(RGB.R, FMath::Min(RGB.G, RGB.B));
	const float Brightness = RGB.A;

	float Hue;
	float Saturation;

	if (Max == Min)
	{
		Hue = 0;
		Saturation = 0;
	}
	else
	{
		const float C = Max - Min;
		if (Max == RGB.R)
		{
			Hue = (RGB.G - RGB.B) / C;
		}
		else if (Max == RGB.G)
		{
			Hue = (RGB.B - RGB.R) / C + 2;
		}
		else
		{
			Hue = (RGB.R - RGB.G) / C + 4;
		}

		Hue *= 60;
		if (Hue < 0)
		{
			Hue += 360;
		}

		Saturation = C / Max;
	}

	return  FVector(Hue, Saturation, Brightness);;
}

/**
 * @brief Create Hue Light Lamp Brightness REST API Call request  :: Internal Call
 * @param Bri int32 brightness level
 */
void AHueLamp::CreateRequestBrightness(int32 Bri)
{
	//if Bri is set to 0 light is off, and set data to be turn off only to save payload size
	if (Bri <= 0)
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
	RequestOBJ->SetNumberField(TEXT("bri"), Bri);

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

/**
 * @brief Create Hue Light Lamp Color Rest API request with HSV format :: Internal Call
 * @param HSV FVector with color data formatted in HSV
 */
void AHueLamp::CreateRequestColor(const FVector& HSV)
{
	//if Bri is set to 0 light is off, and set data to be turn off only to save payload size
	if (static_cast<int32>(HSV.Z * 255) <= 0)
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
	RequestOBJ->SetNumberField(TEXT("hue"), static_cast<int32>(HSV.X / 360 * 65535));
	RequestOBJ->SetNumberField(TEXT("sat"), static_cast<int32>(HSV.Y * 254));
	RequestOBJ->SetNumberField(TEXT("bri"), static_cast<int32>(HSV.Z * 254));

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

/**
 * @brief Create HTTP REST API Call to turn Hue Light Lamp on or off
 * @param bTurnOn boolean set true to make Hue Light Lamp turn on
 */
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

/**
 * @brief Respond Test for REST API
 * @param Request Signature for callback
 * @param Response Signature for callback
 * @param bWasSuccessful Signature for callback
 */
void AHueLamp::OnResponseTest(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	const FString Data = Response->GetContentAsString();
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Data);
}


// Called every frame
void AHueLamp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * @brief Setup Hue Lamp Data
 * @param Path FString URL path to the lamp device on the bridge
 * @param Key  FString Key for the lamp Device
 * @param Name Fstring Name of the device
 */
void AHueLamp::SetupLamp(const FString& Path, const FString& Key, const FString& Name)
{
	DevicePath = Path;
	DeviceKey = Key;
	LampName = Name;
	bHasBeenConfigured = true;
}

/**
 * @brief Turn the Hue Light lamp on or off
 * @param bTurnOn boolean Turn light on or off true is on
 */
void AHueLamp::TurnLightOnOff(bool bTurnOn)
{
	if (CheckInUse())
		return;

	bInUse = true;
	CreateRequestTurnLightOnOff(bTurnOn);
}

/**
 * @brief Set the color of the Lamp
 * @param Color FColor of the color to be set
 */
void AHueLamp::SetColor(const FColor& Color)
{
	if (CheckInUse())
		return;

	bInUse = true;
	CreateRequestColor(CovertRGBToHSV(Color));
}

/**
 * @brief Check to see if we are using lamp to prevent a flood of requests
 * @param Brightness int32 brightness value
 */
void AHueLamp::SetBrightness(const int32 Brightness)
{
	if (CheckInUse())
		return;

	bInUse = true;
	CreateRequestBrightness(Brightness);
}

/**
 * @brief Check to see if we are using lamp to prevent a flood of requests
 * @return boolean false if we aren't in use
 */
bool AHueLamp::CheckInUse()
{
	if (bInUse)
	{
		UE_LOG(LogTemp, Warning, TEXT("Device in Use!"));
		return true;
	}
	return false;
}

