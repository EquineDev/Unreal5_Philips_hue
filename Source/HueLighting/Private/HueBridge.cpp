/*
MIT License Modified See LICENSE Files for more details
Copyright (c) 2022 Scott Tongue all rights reversed 
*/

#include "HueBridge.h"

#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"

//const FSTRING names 
const static FString CONFIG_FILE= TEXT("/Config/HueConfig.json");
const static FString VERB_GET = TEXT("GET");
const static FString VERB_POST = TEXT("POST");
const static FString EXTEND = TEXT("Extended color light");
const static FString TYPE = TEXT("type");
const static FString STATE = TEXT("/state");
const static FString USERNAME = TEXT("username");
const static FString NAME = TEXT("name");
// Sets default values
AHueBridge::AHueBridge()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AHueBridge::BeginPlay()
{
	Super::BeginPlay();
	
}



// Called every frame
void AHueBridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHueBridge::OnResponseReceivedDiscover(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	const FString Data  = Response->GetContentAsString();
	if(Data.Contains(TEXT("Error")))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"),*Data);
		UE_LOG(LogTemp, Warning, TEXT("USER DOES NOT EXIST!"));
		bInUse = false;
		UserConfiguredCorrectly(false);
		return;
	}
	
	TSharedPtr<FJsonObject> ResponseObj;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Data);
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	TMap<FString, TSharedPtr<FJsonValue>> JsonValues = ResponseObj->Values;
	
	int32 KeyCounter = 1;
	//Setup URL path for Device; 
	const FString URL = TEXT("http://")+
		HueBridgeConfig.HostName +
		TEXT("/api/") + HueBridgeConfig.UserName +
		TEXT("/lights");
	FString Device;
	for (const auto& Element : JsonValues)
	{

		FString FieldName = Element.Key;

		const TSharedPtr<FJsonValue> FieldValue = Element.Value;
		
		if(FieldName.Compare(TYPE))
		{
			//Complete URL path to hue bridge for hue lamp
			Device = URL + TEXT("/") + FString::FromInt(KeyCounter)+ STATE;
			UE_LOG(LogTemp,Warning, TEXT("%s"), *Device);
		}
		if(FieldName.Compare(NAME))
		{
			FString LampName;
			GetStringName(FieldValue->AsObject(),NAME, LampName);
			TObjectPtr<AHueLamp> Lamp = GetWorld()->SpawnActor<AHueLamp>();
			Lamp->SetupLamp(Device, FString::FromInt(KeyCounter), LampName);
			HueLamps.Add(LampName, Lamp);
			UE_LOG(LogTemp,Warning, TEXT("%s"), *LampName);
			KeyCounter++;
		}
		
	}
	bInUse = false;
	FoundDiscoverableLights.Broadcast();
}

void AHueBridge::OnResponseReceivedNewUser(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	 const FString Data = Response->GetContentAsString();
	
	if(Data.Contains(TEXT("Error")))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"),*Data);
		UE_LOG(LogTemp, Warning, TEXT("USER DOES NOT EXIST!"));
		bInUse = false;
		UserConfiguredCorrectly(false);
		return;
	}
	
	TSharedPtr<FJsonObject> ResponseObj;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Data);
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	TMap<FString, TSharedPtr<FJsonValue>> JsonValues = ResponseObj->Values;
	
	for (const auto& Element : JsonValues)
	{
		FString FieldName = Element.Key;

		const TSharedPtr<FJsonValue> FieldValue = Element.Value;
		
		if(FieldName.Compare(USERNAME))
		{
			FString UserName;
			GetStringName(FieldValue->AsObject(),USERNAME, UserName);
			HueBridgeConfig.UserName = UserName;
			UE_LOG(LogTemp, Warning, TEXT("USER: %s Created"), *HueBridgeConfig.UserName);
			
			bInUse = false;
			UserConfiguredCorrectly(true);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT(" User Couldnt be Created please press link on Hue Hub!!"));
	bInUse = false;
	UserConfiguredCorrectly(false);
}

void AHueBridge::OnResponseReceivedUserExist(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString Data = Response->GetContentAsString();
	if(Data.Contains(HueBridgeConfig.UserName))
	{
		bUserExist = true;
	}
	else
	{
		bUserExist = false;
	}

	HasUserBeenConfigured.Broadcast(bUserExist);
}



bool AHueBridge::CheckIfBusy()
{
	if(bInUse)
	{
		PleaseWaitingForBridgeRespond();
		RequestsBusy.Broadcast();
		return true;
	}
	return false;
}


//Handles Covverting Data from FJsonObject Back to to FLightInfo Struct
void  AHueBridge::GetLightInfo(TSharedPtr<FJsonObject> JsonObject,  FLightInfo& LightInfoOut)
{
	LightInfoOut.Name = JsonObject->GetStringField("LightName");
	LightInfoOut.bUseLight = JsonObject->GetBoolField("bUseLight");
}

//Handles Converting Data from FJsonObject back to a FString
void AHueBridge::GetStringName(TSharedPtr<FJsonObject> JsonObject, const FString& Field, FString& NameOut)
{
	NameOut = JsonObject->GetStringField(Field);
}

void AHueBridge::UserConfiguredCorrectly(bool Value)
{
	bUserExist = Value;
	HasUserBeenConfigured.Broadcast(bUserExist);
}


void AHueBridge::DiscoverLamps()
{
	if(CheckIfBusy())
	{
		return;
	}
	//Setup HTTP REST CALL and Completed Request Delegate 
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AHueBridge::OnResponseReceivedDiscover);
	const FString URL = TEXT("http://")+
		HueBridgeConfig.HostName +
		TEXT("/api/") + HueBridgeConfig.UserName +
		TEXT("/lights");

	//Fill out JSON DATA
	TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();
	RequestOBJ->SetStringField(TEXT("devicetype"), HueBridgeConfig.AppName);

	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);
	
	Request->SetURL(URL);
	Request->SetVerb(VERB_GET);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	bInUse = true;
}

void AHueBridge::HueBridgeSetupTimer()
{
	if(GetWorldTimerManager().IsTimerActive(LinkBridgeTimer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Hue Bridge Link already being requested"));	
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(LinkBridgeTimer);
	GetWorld()->GetTimerManager().SetTimer(LinkBridgeTimer, this, &AHueBridge::SetupNewUser,DelayTimerForBridgePress, false);
	RequestUser.Broadcast(DelayTimerForBridgePress);
	HueBringTimerStarted_Implementation(DelayTimerForBridgePress);
	
}


void AHueBridge::SetupNewUser()
{
	if(CheckIfBusy())
	{
		return;
	}
	
	//Setup HTTP REST CALL and Completed Request Delegate 
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AHueBridge::OnResponseReceivedNewUser);
	const FString URL = TEXT("http://") +
		HueBridgeConfig.HostName +
		TEXT("/api/");
	
	//Fill out JSON DATA
	TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();
	RequestOBJ->SetStringField(TEXT("devicetype"), HueBridgeConfig.AppName);

	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);

	Request->SetURL(URL); 
	Request->SetVerb(VERB_POST);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	bInUse = true;
	
	
}


void AHueBridge::SaveConfig()
{
	SaveWarning.Broadcast();
	for (const auto&  Element: HueLamps)
	{
 
		FLightUse Light;
		Light.LightName = Element.Key;
		Light.bUseLight = HueLamps[Element.Key]->LampToBeUsed();
		HueBridgeConfig.Lights.Add(Light);
	}

	//Write Data out to file in JSON format 
	FString JsonData;
	FJsonObjectConverter::UStructToJsonObjectString(HueBridgeConfig, JsonData);
	FFileHelper::SaveStringToFile(*JsonData, *(FPaths::ProjectContentDir()+CONFIG_FILE));
	UE_LOG(LogTemp, Warning, TEXT("HueConfig SAVED!"));
	
}

void AHueBridge::LoadConfig()
{

	//Load in Data from JSON file
	FString JsonData;
	FFileHelper::LoadFileToString(JsonData, *(FPaths::ProjectContentDir()+CONFIG_FILE));
	const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JsonData);
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	if(FJsonSerializer::Deserialize(JsonReader,JsonObject) && JsonObject.IsValid())
	{
		//Setup our Config Data
		HueBridgeConfig.HostName = JsonObject->GetStringField("HostName");
		HueBridgeConfig.UserName = JsonObject->GetStringField("UserName");
		TArray<TSharedPtr<FJsonValue>> LightsJson =  JsonObject->GetArrayField("Lights");
		
		//Setup all our Hue Lamps
		DiscoverLamps();
		for (const auto& Element : LightsJson)
		{
			FLightInfo Info;
			GetLightInfo(Element->AsObject(), Info);
			if(HueLamps.Contains(Info.Name))
			{
				HueLamps[Info.Name]->UseLampLight(Info.bUseLight);
			}
		}
	
		UE_LOG(LogTemp, Warning, TEXT("HueConfig LOADED!"));	
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HueConfig Not Found Creating New Save Config"));	
		SaveConfig();
	}
}

AHueLamp* AHueBridge::GetLamp(const FString& LampName)
{
	if(HueLamps.Contains(LampName))
	{
		return HueLamps[LampName];
	}
	return nullptr;
	
}

bool AHueBridge::DoesHueUserExist()
{
	//Setup HTTP REST CALL and Completed Request Delegate 
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AHueBridge::OnResponseReceivedNewUser);
	const FString URL = TEXT("http://") +
		HueBridgeConfig.HostName +
		TEXT("/api/");
	
	//Fill out JSON DATA
	TSharedRef<FJsonObject> RequestOBJ = MakeShared<FJsonObject>();
	RequestOBJ->SetStringField(TEXT("devicetype"), HueBridgeConfig.AppName);

	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);

	Request->SetURL(URL);
	Request->SetVerb(VERB_POST);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	bInUse = true;
	return bUserExist;
	
}

void AHueBridge::ClearOutLights()
{
	for (const auto&  Element: HueLamps)
	{
		FString Key = Element.Key;
		if(HueLamps.Contains(Key))
		{
			AHueLamp* Lamp = GetLamp(Key);
			Lamp->Delete();
		}
	}
	HueLamps.Empty();
}

void AHueBridge::PleaseWaitingForBridgeRespond_Implementation()
{
}

void AHueBridge::HueBringTimerStarted_Implementation(float timer)
{
}

