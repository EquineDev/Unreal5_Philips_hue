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




void AHueBridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

/**
 * @brief Callback for HUE API Response for discover of all lights connected to the Hue Bridge
 * @param Request Signature for callback 
 * @param Response Signature for callback 
 * @param bWasSuccessful  Signature for callback 
 */
void AHueBridge::OnResponseReceivedDiscover(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	//Get Respond as string of data and if there is any Error in the string break out early
	const FString Data  = Response->GetContentAsString();
	if(Data.Contains(TEXT("Error")))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"),*Data);
		UE_LOG(LogTemp, Warning, TEXT("USER DOES NOT EXIST!"));
		bInUse = false;
		UserConfiguredCorrectly(false);
		return;
	}

	//Deserilize the Json object 
	TSharedPtr<FJsonObject> ResponseObj;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Data);

	if(! FJsonSerializer::Deserialize(JsonReader, ResponseObj))
	{
		UE_LOG(LogTemp, Warning, TEXT("FAILED TO Deserialize Find Lights Responds"));
	}
	else
	{
		//Setup a Tmap of all keys and Json data
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

			//Check if the field is Lamp
			if(FieldName.Compare(TYPE))
			{
				//Complete URL path to hue bridge for hue lamp
				Device = URL + TEXT("/") + FString::FromInt(KeyCounter) + STATE;
				UE_LOG(LogTemp,Warning, TEXT("%s"), *Device);
			}
			//Setup the Name of the Lamp
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
	}
	
	bInUse = false;
	FoundDiscoverableLights.Broadcast();
}


/**
 * @brief Callback for HUE API Response for New user Setup
 * @param Request Signature for callback 
 * @param Response Signature for callback 
 * @param bWasSuccessful Signature for callback 
 */
void AHueBridge::OnResponseReceivedNewUser(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	//Get Respond as string of data and if there is any Error in the string break out early
	FString Data = Response->GetContentAsString();
	if(Data.Contains(TEXT("Error")))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"),*Data);
		UE_LOG(LogTemp, Warning, TEXT("USER DOES NOT EXIST!"));
		bInUse = false;
		UserConfiguredCorrectly(false);
		return;
	}

	int32 index = Data.Find(USERNAME)+11;
	Data = Data.RightChop(index);
	Data = Data.LeftChop(4);
	HueBridgeConfig.UserName = Data;
	UE_LOG(LogTemp, Warning, TEXT("USER: %s Created"), *HueBridgeConfig.UserName);
	bInUse = false;
	UserConfiguredCorrectly(true);
	//Deserilize the Json object JSON OBJECT FAIL TO Deserialize so chopping the string instead
	

	/*TSharedPtr<FJsonObject> ResponseObj = MakeShareable(new FJsonObject);
//	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	
	if(! FJsonSerializer::Deserialize(JsonReader, ResponseObj))
	{
		UE_LOG(LogTemp, Warning, TEXT("FAILED TO Deserialize NEWUSER RESPOND!"));
		
	}
	else
	{
		//Setup a Tmap of all keys and Json data
		TMap<FString, TSharedPtr<FJsonValue>> JsonValues = ResponseObj->Values;
		for (const auto& Element : JsonValues)
		{
			FString FieldName = Element.Key;

			const TSharedPtr<FJsonValue> FieldValue = Element.Value;

			//Find our Username field and setup the user name properly 
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
	}
	UE_LOG(LogTemp, Warning, TEXT(" User Couldnt be Created please press link on Hue Hub!!"));
	bInUse = false;
	UserConfiguredCorrectly(false);
	*/
	

    
	
/*	 if(FString UserName; ResponseObj->TryGetStringField(USERNAME,UserName ))
	{
		UE_LOG(LogTemp, Warning, TEXT("FOUND:: %s"),*UserName)
		HueBridgeConfig.UserName = UserName;
		
	} */
	
	
}

/**
 * @brief Callback for HUE API Response for if user exist yet or not on huebridge
 * @param Request Signature for callback 
 * @param Response Signature for callback 
 * @param bWasSuccessful Signature for callback 
 */
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


/**
 * @brief Check to see if we are making Rest API call to the hue bridge to prevent flooding the hue bridge
 * @return Returns false if we aren't busy making a REST Call
 */
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


/**
 * @brief  Converts JasonObject data to Lightinfo Struct
 * @param JsonObject Jsonobject to be used FJsonObject
 * @param LightInfoOut Flightinfo struct out param to be filled FLightInfo
 */
void  AHueBridge::GetLightInfo(TSharedPtr<FJsonObject> JsonObject,  FLightInfo& LightInfoOut)
{
	LightInfoOut.Name = JsonObject->GetStringField("LightName");
	LightInfoOut.bUseLight = JsonObject->GetBoolField("bUseLight");
}

/**
 * @brief Converts JasonObject data to a string 
 * @param JsonObject 
 * @param Field Name of json field to used as a string
 * @param NameOut String that is used for Out param to be filled
 */
void AHueBridge::GetStringName(TSharedPtr<FJsonObject> JsonObject, const FString& Field, FString& NameOut)
{
	NameOut = JsonObject->GetStringField(Field);
}

/**
 * @brief Set if our user has been configured properly, use to broadcast for UI elements 
 * @param Value Bool if used is configured properly
 */
void AHueBridge::UserConfiguredCorrectly(bool Value)
{
	bUserExist = Value;
	HasUserBeenConfigured.Broadcast(bUserExist);
}

/**
 * @brief Creates REST API call to the Hue bridge too find all lamp connected to the bridge. User needs to be created
 * in order to find all lamps 
 */
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
	//RequestOBJ->SetStringField(TEXT("devicetype"), HueBridgeConfig.AppName);

	//Serialize Data
	FString RequestBody;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RequestOBJ, Writer);
	
	Request->SetURL(URL);
	Request->SetVerb(VERB_GET);
	Request->SetHeader("Content-Type", TEXT("application/json"));
	//Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	bInUse = true;
}

/**
 * @brief Setups a timer to create to ask the user to press the hue bridge link button
 */
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

/**
 * @brief Creates REST API call to the Hue bridge to create a new user
 */
void AHueBridge::SetupNewUser()
{
	GetWorld()->GetTimerManager().ClearTimer(LinkBridgeTimer);
	if(CheckIfBusy())
	{
		return;
	}
	
	//Setup HTTP REST CALL and Completed Request Delegate 
	const TSharedRef<IHttpRequest> Request = HTTPHandler->Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AHueBridge::OnResponseReceivedNewUser);
	const FString URL = TEXT("http://") +
		HueBridgeConfig.HostName +
		TEXT("/api");
	
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

/**
 * @brief Save Hue bridge config out to Json file
 */
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

/**
 * @brief Load Hue Bridge config from json file
 */
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

/**
 * @brief Get a pointer to a Hue Lamp from our TMap of Lamps
 * @param LampName const String name of lamp to get
 * @return Point to our Hue Lamp
 */
AHueLamp* AHueBridge::GetLamp(const FString& LampName)
{
	if(HueLamps.Contains(LampName))
	{
		return HueLamps[LampName];
	}
	return nullptr;
	
}

/**
 * @brief Creates REST API call to the Hue bridge to see if user already exists
 * @return True if Hue user exist
 */
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

/**
 * @brief Clear out all hue Lamps
 */
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

