// MQ2AutoSize.cpp : Resize spawns by distance or whole zone (client only)
//
// 06/09/2009: Fixed for changes to the eqgame function
//				 added corpse option and autosave -pms
// 02/09/2009: added parameters, merc, npc & everything spawn options,
//				 bug fixes and code cleanup - pms
// 06/28/2008: finds its own offset - ieatacid
//
// version 0.9.3 (by Psycotic)
// v1.0 - Eqmule 07-22-2016 - Added string safety.
// v1.1 - hytiek 07-01-2024 - Fixed the experience, added MQ Settings ImGui panel

#include <mq/Plugin.h>

const char* MODULE_NAME = "MQ2AutoSize";
PreSetup(MODULE_NAME);
PLUGIN_VERSION(1.1);
// this controls how many pulses to perform a radius-based resize (bad performance hit)
const int SKIP_PULSES = 5;
// min and max size values
const int MIN_SIZE = 1;
const int MAX_SIZE = 250;
// maybe later this can link directly to the EQ Far Clip Plane value
int FAR_CLIP_PLANE = 1000;

// used by the plugin
const float OTHER_SIZE = 1.0f;
const float ZERO_SIZE = 0.0f;
unsigned int uiSkipPulse = 0;
char szTemp[MAX_STRING] = { 0 };
void SpawnListResize(bool bReset);

// added for MQ Settings imgui panel
bool group_control_checked = false;
bool group_control_enabled = false;
const char* comms = "None";
void DoGroupCommand(std::string_view command, bool includeSelf);
void ChooseInstructionPlugin();
void DrawAutoSize_MQSettingsPanel();
int optZonewide = 1;
int previousRangeDistance = 0;

// our configuration
class COurSizes
{
public:
	COurSizes()
	{
		// TODO: maybe remove later, for now commented out Zonewide
		// OptPC = OptByZone = true;
		OptPC = true;
		OptNPC = OptPet = OptMerc = OptMount = OptCorpse = OptSelf = OptEverything = OptByRange = OptAutoSave = false;
		ResizeRange = 50;
		SizeDefault = SizePC = SizeNPC = SizePet = SizeMerc = SizeTarget = SizeMount = SizeCorpse = SizeSelf = 1;
	};

	bool  OptAutoSave;
	bool  OptByRange;
	// TODO: maybe remove later, for now commented out Zonewide
	// bool  OptByZone;
	bool  OptEverything;
	bool  OptPC;
	bool  OptNPC;
	bool  OptPet;
	bool  OptMerc;
	bool  OptMount;
	bool  OptCorpse;
	bool  OptSelf;

	int ResizeRange;
	int SizeDefault;
	int SizePC;
	int SizeNPC;
	int SizePet;
	int SizeMerc;
	int SizeTarget;
	int SizeMount;
	int SizeCorpse;
	int SizeSelf;
};
COurSizes AS_Config;

// exposed TLO variables
class MQ2AutoSizeType* pAutoSizeType = 0;
class MQ2AutoSizeType : public MQ2Type
{
public:
	enum AutoSizeMembers
	{
		Active,
		AutoSave,
		ResizePC,
		ResizeNPC,
		ResizePets,
		ResizeMercs,
		ResizeAll,
		ResizeMounts,
		ResizeCorpse,
		ResizeSelf,
		SizeByRange,
		Range,
		SizeDefault,
		SizePC,
		SizeNPC,
		SizePets,
		SizeMercs,
		SizeTarget,
		SizeMounts,
		SizeCorpse,
		SizeSelf
	};

	MQ2AutoSizeType() :MQ2Type("AutoSize")
	{
		TypeMember(Active);
		TypeMember(AutoSave);
		TypeMember(ResizePC);
		TypeMember(ResizeNPC);
		TypeMember(ResizePets);
		TypeMember(ResizeMercs);
		TypeMember(ResizeAll);
		TypeMember(ResizeMounts);
		TypeMember(ResizeCorpse);
		TypeMember(ResizeSelf);
		TypeMember(SizeByRange);
		TypeMember(Range);
		TypeMember(SizeDefault);
		TypeMember(SizePC);
		TypeMember(SizeNPC);
		TypeMember(SizePets);
		TypeMember(SizeMercs);
		TypeMember(SizeTarget);
		TypeMember(SizeMounts);
		TypeMember(SizeCorpse);
		TypeMember(SizeSelf);
	}

	~MQ2AutoSizeType()
	{
	}

	bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest)
	{

		MQTypeMember* pMember = MQ2AutoSizeType::FindMember(Member);
		if (!pMember)
			return false;

		switch ((AutoSizeMembers)pMember->ID)
		{
		case Active:
			Dest.Int = AS_Config.OptPC || AS_Config.OptNPC || AS_Config.OptPet || AS_Config.OptMerc || AS_Config.OptMount || AS_Config.OptCorpse || AS_Config.OptSelf;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSave:
			Dest.Int = AS_Config.OptAutoSave;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizePC:
			Dest.Int = AS_Config.OptPC;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeNPC:
			Dest.Int = AS_Config.OptNPC;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizePets:
			Dest.Int = AS_Config.OptPet;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeMercs:
			Dest.Int = AS_Config.OptMerc;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeAll:
			Dest.Int = AS_Config.OptEverything;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeMounts:
			Dest.Int = AS_Config.OptMount;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeCorpse:
			Dest.Int = AS_Config.OptCorpse;
			Dest.Type = datatypes::pBoolType;
			return true;
		case ResizeSelf:
			Dest.Int = AS_Config.OptSelf;
			Dest.Type = datatypes::pBoolType;
			return true;
		case SizeByRange:
			Dest.Int = AS_Config.OptByRange;
			Dest.Type = datatypes::pBoolType;
			return true;
		case Range:
			Dest.Int = AS_Config.ResizeRange;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeDefault:
			Dest.Int = AS_Config.SizeDefault;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizePC:
			Dest.Int = AS_Config.SizePC;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeNPC:
			Dest.Int = AS_Config.SizeNPC;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizePets:
			Dest.Int = AS_Config.SizePet;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeMercs:
			Dest.Int = AS_Config.SizeMerc;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeTarget:
			Dest.Int = AS_Config.SizeTarget;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeMounts:
			Dest.Int = AS_Config.SizeMount;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeCorpse:
			Dest.Int = AS_Config.SizeCorpse;
			Dest.Type = datatypes::pIntType;
			return true;
		case SizeSelf:
			Dest.Int = AS_Config.SizeSelf;
			Dest.Type = datatypes::pIntType;
			return true;
		}
		return false;
	}

	bool ToString(MQVarPtr VarPtr, char* Destination)
	{
		return true;
	}
};

bool dataAutoSize(const char* szIndex, MQTypeVar& ret)
{
	ret.DWord = 1;
	ret.Type = pAutoSizeType;
	return true;
}

// class to access the ChangeHeight function
class PlayerZoneClient_Hook
{
public:
	DETOUR_TRAMPOLINE_DEF(void, ChangeHeight_Trampoline, (float, float, float, bool))
	void ChangeHeight_Detour(float newHeight, float cameraPos, float speedScale, bool unused)
	{
		ChangeHeight_Trampoline(newHeight, cameraPos, speedScale, unused);
	}

	// this assures valid function call
	void ChangeHeight_Wrapper(float fNewSize)
	{
		float fView = OTHER_SIZE;
		PlayerClient* pSpawn = reinterpret_cast<PlayerClient*>(this);

		if (pSpawn->SpawnID == pLocalPlayer->SpawnID)
		{
			fView = ZERO_SIZE;
		}

		ChangeHeight_Trampoline(fNewSize, fView, 1.0f, false);
	};
};

/**
 * This function reads a string value from a specified section and key in an INI file.
 * It then performs a case-insensitive comparison to check if the value is "on".
 *
 * @param section The section in the INI file to read from.
 * @param key The key in the section to read the value of.
 * @param defaultValue The default value to use if the key is not found.
 * @return true if the retrieved value is "on" (case-insensitively), false otherwise.
 */
static bool getOptionValue(const char* section, const char* key, const char* defaultValue) {
	char szTemp[MAX_STRING] = { 0 };
	GetPrivateProfileString(section, key, defaultValue, szTemp, MAX_STRING, INIFileName);
	return (ci_equals(szTemp, "on"));
}

/**
 * This function reads an integer value from a specified section and key in an INI file.
 * It then clamps the value to ensure it falls within the specified minimum and maximum size range.
 * If the pulled value is lower than MIN_SIZE then it will be set to MIN_SIZE value
 * If the pulled value is larger than MAX_SIZE then it will be set to MAX_SIZE value
 *
 * @param section The section in the INI file to read from.
 * @param key The key in the section to read the value of.
 * @param defaultValue The default value to use if the key is not found or the value is invalid.
 * @return An integer representing the size value, clamped to be within the range of MIN_SIZE and MAX_SIZE.
 */
static int getSaneSize(const std::string& section, const std::string& key, int defaultValue)
{
	int size = GetPrivateProfileInt(section.c_str(), key.c_str(), defaultValue, INIFileName);
	return std::clamp(size, MIN_SIZE, MAX_SIZE);
}

void LoadINI()
{
	// defaulted all to off
	AS_Config.OptAutoSave	= getOptionValue("Config", "AutoSave",		"off");
	AS_Config.OptPC			= getOptionValue("Config", "ResizePC",		"off");
	AS_Config.OptNPC		= getOptionValue("Config", "ResizeNPC",		"off");
	AS_Config.OptPet		= getOptionValue("Config", "ResizePets",	"off");
	AS_Config.OptMerc		= getOptionValue("Config", "ResizeMercs",	"off");
	// OptEverything didn't work. I've changed how "Everything" works and the
	// user experience is much better. I left this for now.
	//AS_Config.OptEverything	= getOptionValue("Config", "ResizeAll",		"off");
	AS_Config.OptMount		= getOptionValue("Config", "ResizeMounts",	"off");
	AS_Config.OptCorpse		= getOptionValue("Config", "ResizeCorpse",	"off");
	AS_Config.OptSelf		= getOptionValue("Config", "ResizeSelf",	"off");
	AS_Config.OptByRange	= getOptionValue("Config", "SizeByRange",	"off");

	AS_Config.ResizeRange	= getSaneSize("Config", "Range",		AS_Config.ResizeRange);
	AS_Config.SizeDefault	= getSaneSize("Config", "SizeDefault",	MIN_SIZE);
	AS_Config.SizePC		= getSaneSize("Config", "SizePC",		MIN_SIZE);
	AS_Config.SizeNPC		= getSaneSize("Config", "SizeNPC",		MIN_SIZE);
	AS_Config.SizePet		= getSaneSize("Config", "SizePets",		MIN_SIZE);
	AS_Config.SizeMerc		= getSaneSize("Config", "SizeMercs",	MIN_SIZE);
	AS_Config.SizeTarget	= getSaneSize("Config", "SizeTarget",	MIN_SIZE);
	AS_Config.SizeMount		= getSaneSize("Config", "SizeMounts",	MIN_SIZE);
	AS_Config.SizeCorpse	= getSaneSize("Config", "SizeCorpse",	MIN_SIZE);
	AS_Config.SizeSelf		= getSaneSize("Config", "SizeSelf",		MIN_SIZE);
	WriteChatf("\ay%s\aw:: Configuration file loaded.", MODULE_NAME);
	
	// apply new INI read
	// TODO: maybe remove later, for now commented out Zonewide
	// if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer && AS_Config.OptByZone) {
	if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer) {
		SpawnListResize(false);
	}
}

void SaveINI()
{
	WritePrivateProfileString("Config", "AutoSave",		AS_Config.OptAutoSave ? "on" : "off",	INIFileName);
	WritePrivateProfileString("Config", "ResizePC",		AS_Config.OptPC ? "on" : "off",			INIFileName);
	WritePrivateProfileString("Config", "ResizeNPC",	AS_Config.OptNPC ? "on" : "off",		INIFileName);
	WritePrivateProfileString("Config", "ResizePets",	AS_Config.OptPet ? "on" : "off",		INIFileName);
	WritePrivateProfileString("Config", "ResizeMercs",	AS_Config.OptMerc ? "on" : "off",		INIFileName);
	// no longer used since Everything feature was significantly changed
	//WritePrivateProfileString("Config", "ResizeAll",	AS_Config.OptEverything ? "on" : "off",	INIFileName);
	WritePrivateProfileString("Config", "ResizeMounts",	AS_Config.OptMount ? "on" : "off",		INIFileName);
	WritePrivateProfileString("Config", "ResizeCorpse",	AS_Config.OptCorpse ? "on" : "off",		INIFileName);
	WritePrivateProfileString("Config", "ResizeSelf",	AS_Config.OptSelf ? "on" : "off",		INIFileName);
	WritePrivateProfileString("Config", "SizeByRange",	AS_Config.OptByRange ? "on" : "off",	INIFileName);
	// this avoids writing 1000 to INI
	if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
		WritePrivateProfileString("Config", "Range", std::to_string(AS_Config.ResizeRange), INIFileName);
	}
	WritePrivateProfileString("Config", "SizeDefault",	std::to_string(AS_Config.SizeDefault),	INIFileName);
	WritePrivateProfileString("Config", "SizePC",		std::to_string(AS_Config.SizePC),		INIFileName);
	WritePrivateProfileString("Config", "SizeNPC",		std::to_string(AS_Config.SizeNPC),		INIFileName);
	WritePrivateProfileString("Config", "SizePets",		std::to_string(AS_Config.SizePet),		INIFileName);
	WritePrivateProfileString("Config", "SizeMercs",	std::to_string(AS_Config.SizeMerc),		INIFileName);
	WritePrivateProfileString("Config", "SizeTarget",	std::to_string(AS_Config.SizeTarget),	INIFileName);
	WritePrivateProfileString("Config", "SizeMounts",	std::to_string(AS_Config.SizeMount),	INIFileName);
	WritePrivateProfileString("Config", "SizeCorpse",	std::to_string(AS_Config.SizeCorpse),	INIFileName);
	WritePrivateProfileString("Config", "SizeSelf",		std::to_string(AS_Config.SizeSelf),		INIFileName);
	WriteChatf("\ay%s\aw:: Configuration file saved.", MODULE_NAME);
}

void ChangeSize(PlayerClient* pChangeSpawn, float fNewSize)
{
	if (pChangeSpawn)
	{
		reinterpret_cast<PlayerZoneClient_Hook*>(pChangeSpawn)->ChangeHeight_Wrapper(fNewSize);
	}
}

void SizePasser(PSPAWNINFO pSpawn, bool bReset)
{
	// TODO: maybe remove later, for now commented out Zonewide
	// if ((!bReset && !AS_Config.OptByZone && !AS_Config.OptByRange) || GetGameState() != GAMESTATE_INGAME) return;
	if (GetGameState() != GAMESTATE_INGAME) {
		return;
	}

	PSPAWNINFO pLPlayer = (PSPAWNINFO)pLocalPlayer;
	if (!pLPlayer || !pLPlayer->SpawnID || !pSpawn || !pSpawn->SpawnID) return;

	if (pSpawn->SpawnID == pLPlayer->SpawnID)
	{
		if (AS_Config.OptSelf) ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeSelf);
		return;
	}

	switch (GetSpawnType(pSpawn))
	{
	case PC:
		if (AS_Config.OptPC)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizePC);
			return;
		}
		break;
	case NPC:
		if (AS_Config.OptNPC)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeNPC);
			return;
		}
		break;
	case PET:
		if (AS_Config.OptPet)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizePet);
			return;
		}
		break;
	case MERCENARY:
		if (AS_Config.OptMerc)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeMerc);
			return;
		}
		break;
	case MOUNT:
		if (AS_Config.OptMount && pSpawn->SpawnID != pLPlayer->SpawnID)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeMount);
			return;
		}
		break;
	case CORPSE:
		if (AS_Config.OptCorpse)
		{
			ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeCorpse);
			return;
		}
		break;
	default:
		break;
	}

	// no longer used since Everything feature was changed to work
	//if (AS_Config.OptEverything && pSpawn->SpawnID != pLPlayer->SpawnID) {
	//	ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeDefault);
	//}
}

void ResetAllByType(eSpawnType OurType)
{
	PSPAWNINFO pSpawn = (PSPAWNINFO)pSpawnList;
	PSPAWNINFO pLPlayer = (PSPAWNINFO)pLocalPlayer;
	if (GetGameState() != GAMESTATE_INGAME || !pLPlayer || !pLPlayer->SpawnID || !pSpawn || !pSpawn->SpawnID) {
		return;
	}

	while (pSpawn)
	{
		if (pSpawn->SpawnID == pLPlayer->SpawnID)
		{
			pSpawn = pSpawn->pNext;
			continue;
		}

		eSpawnType ListType = GetSpawnType(pSpawn);
		if (ListType == OurType) ChangeSize(pSpawn, ZERO_SIZE);
		// Handle Everything resize all by using NONE
		// it's not a great option but we aren't using NONE 
		// for anything else so its available for use
		if (OurType == 0) {
			ChangeSize(pSpawn, ZERO_SIZE);
		}
		pSpawn = pSpawn->pNext;
	}
}

void SpawnListResize(bool bReset)
{
	if (GetGameState() != GAMESTATE_INGAME) return;
	PSPAWNINFO pSpawn = (PSPAWNINFO)pSpawnList;
	while (pSpawn)
	{
		SizePasser(pSpawn, bReset);
		pSpawn = pSpawn->pNext;
	}
}

PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	// TODO: maybe remove later, for now commented out Zonewide
	// if (AS_Config.OptByZone) {
		//SizePasser(pNewSpawn, false);
	//}
}

PLUGIN_API void OnEndZone()
{
	SpawnListResize(false);
}

PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME || !AS_Config.OptByRange) return;
	if (uiSkipPulse < SKIP_PULSES)
	{
		uiSkipPulse++;
		return;
	}

	// imgui panel related
	if (!group_control_enabled) {
		group_control_enabled = true;
		ChooseInstructionPlugin();
	}

	PSPAWNINFO pAllSpawns = (PSPAWNINFO)pSpawnList;
	float fDist = 0.0f;
	uiSkipPulse = 0;

	while (pAllSpawns)
	{
		fDist = GetDistance((PSPAWNINFO)pLocalPlayer, pAllSpawns);
		if (fDist < AS_Config.ResizeRange)
		{
			SizePasser(pAllSpawns, false);
		}
		else if (fDist < AS_Config.ResizeRange + 50)
		{
			SizePasser(pAllSpawns, true);
		}
		pAllSpawns = pAllSpawns->pNext;
	}
}

void OutputHelp()
{
	WriteChatf("\ay%s\aw:: Command Usage Help", MODULE_NAME);
	WriteChatf("  \ag/autosize\ax - Toggles zone-wide AutoSize on/off");
	WriteChatf("  \ag/autosize\ax \aydist\ax - Toggles distance-based AutoSize on/off");
	WriteChatf("  \ag/autosize\ax \ayrange #\ax - Sets range for distance checking");
	WriteChatf("--- Valid Resize Toggles ---");
	WriteChatf("  \ag/autosize\ax [ \aypc\ax | \aynpc\ax | \aypets\ax | \aymercs\ax | \aymounts\ax | \aycorpse\ax | \aytarget\ax | \ayeverything\ax | \ayself\ax ]");
	WriteChatf("--- Valid Size Syntax (1 to 250) ---");
	WriteChatf("  \ag/autosize\ax [ \aysize\ax | \aysizepc\ax | \aysizenpc\ax | \aysizepets\ax | \aysizemercs\ax | \aysizemounts\ax | \aysizecorpse\ax | \aysizetarget\ax | \aysizeself\ax ] [ \ay#\ax ]");
	WriteChatf("--- Other Valid Commands ---");
	WriteChatf("  \ag/autosize\ax [ \ayhelp\ax | \aystatus\ax | \ayautosave\ax | \aysave\ax | \ayload\ax ]");
}

void OutputStatus()
{
	char szMethod[100] = { 0 };
	char szOn[10] = "\agon\ax";
	char szOff[10] = "\aroff\ax";
	// TODO: maybe remove later, for now commented out Zonewide
	// if (AS_Config.OptByZone)
	//{
	//	sprintf_s(szMethod, "\ayZonewide\ax");
	//}
	//else if (AS_Config.OptByRange)
	//{
	//	sprintf_s(szMethod, "\ayRange\ax) RangeSize(\ag%d\ax", AS_Config.ResizeRange);
	//}
	//else
	//{
	//	sprintf_s(szMethod, "\arInactive\ax");
	//}

	// replaced above with a more direct feedback
	if (!AS_Config.OptPC && !AS_Config.OptNPC && !AS_Config.OptPet && !AS_Config.OptMerc && !AS_Config.OptCorpse && !AS_Config.OptSelf) {
		sprintf_s(szMethod, "\arInactive\ax");
	}
	else if (AS_Config.ResizeRange < 1000) {
		sprintf_s(szMethod, "\ayRange\ax) RangeSize(\ag%d\ax", AS_Config.ResizeRange);
	}
	else if (AS_Config.ResizeRange == 1000) {
		// covers the idea of Zonewide by using 1000 units (which is about 100% Far Cliping Plane)
		sprintf_s(szMethod, "\ayZonewide\ax");
	}
	
	WriteChatf("\ay%s\aw:: Current Status -- Method: (%s)%s", MODULE_NAME, szMethod, AS_Config.OptAutoSave ? " \agAUTOSAVING" : "");
	// leaving Everything in the output to avoid any script which might have read this line
	// forced the value to off though.
	WriteChatf("Toggles: PC(%s) NPC(%s) Pets(%s) Mercs(%s) Mounts(%s) Corpses(%s) Self(%s) Everything(%s) ", 
		AS_Config.OptPC ? szOn : szOff, 
		AS_Config.OptNPC ? szOn : szOff, 
		AS_Config.OptPet ? szOn : szOff, 
		AS_Config.OptMerc ? szOn : szOff, 
		AS_Config.OptMount ? szOn : szOff, 
		AS_Config.OptCorpse ? szOn : szOff, 
		AS_Config.OptSelf ? szOn : szOff, 
		//AS_Config.OptEverything ? szOn : szOff
		szOff);
	WriteChatf("Sizes: PC(\ag%d\ax) NPC(\ag%d\ax) Pets(\ag%d\ax) Mercs(\ag%d\ax) Mounts(\ag%d\ax) Corpses(\ag%d\ax) Target(\ag%d\ax) Self(\ag%d\ax) Everything(\ag%d\ax)", 
		AS_Config.SizePC, 
		AS_Config.SizeNPC, 
		AS_Config.SizePet, 
		AS_Config.SizeMerc, 
		AS_Config.SizeMount, 
		AS_Config.SizeCorpse, 
		AS_Config.SizeTarget, 
		AS_Config.SizeSelf, 
		AS_Config.SizeDefault);
}

bool ToggleOption(const char* pszToggleOutput, bool* pbOption)
{
	*pbOption = !*pbOption;
	WriteChatf("\ay%s\aw:: Option (\ay%s\ax) now %s\ax", MODULE_NAME, pszToggleOutput, *pbOption ? "\agenabled" : "\ardisabled");
	if (AS_Config.OptAutoSave) SaveINI();
	return *pbOption;
}

void SetSizeConfig(const char* pszOption, int iNewSize, int* iOldSize)
{
	// special handling for Range being set to Zonewide
	if (ci_equals(pszOption, "range") && iNewSize == FAR_CLIP_PLANE) {
		// set the pointer to the new value
		*iOldSize = iNewSize;
		WriteChatf("\ay%s\aw:: Range size is \agZonewide\ax - 1000", MODULE_NAME);
		// return early to avoid saving to INI
		return;
	}

	// make sure that we are setting values for a valid reason
	if ((iNewSize != *iOldSize) && (iNewSize >= MIN_SIZE && iNewSize <= MAX_SIZE))
	{
		int iPrevSize = *iOldSize;
		// set the pointer to the new value
		*iOldSize = iNewSize;
		WriteChatf("\ay%s\aw:: %s size changed from \ay%d\ax to \ag%d", MODULE_NAME, pszOption, iPrevSize, *iOldSize);
	}
	else
	{
		WriteChatf("\ay%s\aw:: %s size is \ag%d\ax (was not modified)", MODULE_NAME, pszOption, *iOldSize);
	}
	if (AS_Config.OptAutoSave) SaveINI();
}

// TODO: maybe remove later, for now commented out Zonewide
// void SetEnabled(bool bEnable) {
//	AS_Config.OptByZone = bEnable;
//	SpawnListResize(!bEnable);
//	WriteChatf("\ay%s\aw:: AutoSize (\ayZonewide\ax) now %s\ax!", MODULE_NAME, AS_Config.OptByZone ? "\agenabled" : "\ardisabled");
//	if (AS_Config.OptAutoSave)
//		SaveINI();
//}

void AutoSizeCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	char szCurArg[MAX_STRING] = { 0 };
	char szNumber[MAX_STRING] = { 0 };
	GetArg(szCurArg, szLine, 1);
	GetArg(szNumber, szLine, 2);
	int iNewSize = std::atoi(szNumber);

	if (!*szCurArg)
	{
		// TODO: maybe remove later, for now commented out Zonewide
		// SetEnabled(!AS_Config.OptByZone);
		
		// fake Zonewide with large value which is effectively Far Cliping Plane at 100%
		if (AS_Config.ResizeRange >= MIN_SIZE && AS_Config.ResizeRange < FAR_CLIP_PLANE) {
			// this means we are currently using Range distance normally
			// now we want look like we are toggling to Zonewide
			// we will do this by increasing the ResizeRange to FAR_CLIP_PLANE
			// SetSizeConfig() will mention Zonewide based on FAR_CLIP_PLANE value being used
			SetSizeConfig("range", FAR_CLIP_PLANE, &AS_Config.ResizeRange);
		}
		else if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
			// this means we are pretending to be Zonewide and need to revert to Range based
			// we will do this by reseting the ResizeRange to what is in INI
			AS_Config.ResizeRange = FAR_CLIP_PLANE;
			SetSizeConfig("range", 50, &AS_Config.ResizeRange);
		}
		return;
	}
	else if (ci_equals(szCurArg, "dist"))
	{
		// TODO: maybe remove later, for now commented out Zonewide
		// if (AS_Config.OptByRange)
		//{
		//	if (AS_Config.OptByZone)
		//	{
		//		ToggleOption("Zonewide", &AS_Config.OptByZone);
		//	}
		//}
		//else
		//{
		//	SpawnListResize(true);
		//}
		//ToggleOption("Range", &AS_Config.OptByRange);

		// TODO: write a new thing to act like range is toggling
		return;
	}
	else if (ci_equals(szCurArg, "save"))
	{
		SaveINI();
		return;
	}
	else if (ci_equals(szCurArg, "load"))
	{
		LoadINI();
		return;
	}
	else if (ci_equals(szCurArg, "autosave"))
	{
		ToggleOption("Autosave", &AS_Config.OptAutoSave);
		return;
	}
	else if (ci_equals(szCurArg, "range"))
	{
		SetSizeConfig("range", iNewSize, &AS_Config.ResizeRange);
		return;
	}
	else if (ci_equals(szCurArg, "size"))
	{
		SetSizeConfig("Default", iNewSize, &AS_Config.SizeDefault);
	}
	else if (ci_equals(szCurArg, "sizepc"))
	{
		SetSizeConfig("PC", iNewSize, &AS_Config.SizePC);
	}
	else if (ci_equals(szCurArg, "sizenpc"))
	{
		SetSizeConfig("NPC", iNewSize, &AS_Config.SizeNPC);
	}
	else if (ci_equals(szCurArg, "sizepets"))
	{
		SetSizeConfig("Pet", iNewSize, &AS_Config.SizePet);
	}
	else if (ci_equals(szCurArg, "sizemercs"))
	{
		SetSizeConfig("Mercs", iNewSize, &AS_Config.SizeMerc);
	}
	else if (ci_equals(szCurArg, "sizetarget"))
	{
		SetSizeConfig("Target", iNewSize, &AS_Config.SizeTarget);
	}
	else if (ci_equals(szCurArg, "sizemounts"))
	{
		SetSizeConfig("Mounts", iNewSize, &AS_Config.SizeMount);
	}
	else if (ci_equals(szCurArg, "sizecorpse"))
	{
		SetSizeConfig("Corpses", iNewSize, &AS_Config.SizeCorpse);
	}
	else if (ci_equals(szCurArg, "sizeself"))
	{
		SetSizeConfig("Self", iNewSize, &AS_Config.SizeSelf);
	}
	else if (ci_equals(szCurArg, "pc"))
	{
		if (!ToggleOption("PC", &AS_Config.OptPC)) {
			ResetAllByType(PC);
		}
	}
	else if (ci_equals(szCurArg, "npc"))
	{
		if (!ToggleOption("NPC", &AS_Config.OptNPC)) {
			ResetAllByType(NPC);
		}
	}
	else if (ci_equals(szCurArg, "everything"))
	{
		// this implementation doesn't work as-is
		//if (!ToggleOption("Everything", &AS_Config.OptEverything)) {
		//	SpawnListResize(true);
		//}

		// a different approach for a better user experience
		if (!AS_Config.OptSelf) {
			DoCommandf("/squelch /autosize self");
		}
		if (!AS_Config.OptCorpse) {
			DoCommandf("/squelch /autosize corpse");
		}
		if (!AS_Config.OptMerc) {
			DoCommandf("/squelch /autosize mercs");
		}
		if (!AS_Config.OptMount) {
			DoCommandf("/squelch /autosize mounts");
		}
		if (!AS_Config.OptNPC) {
			DoCommandf("/squelch /autosize npc");
		}
		if (!AS_Config.OptPC) {
			DoCommandf("/squelch /autosize pc");
		}
		if (!AS_Config.OptPet) {
			DoCommandf("/squelch /autosize pets");
		}		
	}
	else if (ci_equals(szCurArg, "pets"))
	{
		if (!ToggleOption("Pets", &AS_Config.OptPet)) {
			ResetAllByType(PET);
		}
	}
	else if (ci_equals(szCurArg, "mercs"))
	{
		if (!ToggleOption("Mercs", &AS_Config.OptMerc)) {
			ResetAllByType(MERCENARY);
		}
	}
	else if (ci_equals(szCurArg, "mounts"))
	{
		if (!ToggleOption("Mounts", &AS_Config.OptMount)) {
			ResetAllByType(MOUNT);
		}
	}
	else if (ci_equals(szCurArg, "corpse"))
	{
		if (!ToggleOption("Corpses", &AS_Config.OptCorpse)) {
			ResetAllByType(CORPSE);
		}
	}
	else if (ci_equals(szCurArg, "target"))
	{
		PSPAWNINFO pTheTarget = (PSPAWNINFO)pTarget;
		if (pTheTarget && GetGameState() == GAMESTATE_INGAME && pTheTarget->SpawnID)
		{
			ChangeSize(pTheTarget, static_cast<float>(AS_Config.SizeTarget));
			char szTarName[MAX_STRING] = { 0 };
			sprintf_s(szTarName, "%s", pTheTarget->DisplayedName);
			WriteChatf("\ay%s\aw:: Resized \ay%s\ax to \ag%d\ax", MODULE_NAME, szTarName, AS_Config.SizeTarget);
		}
		else
		{
			WriteChatf("\ay%s\aw:: \arYou must have a target to use this parameter.", MODULE_NAME);
		}
		return;
	}
	else if (ci_equals(szCurArg, "self"))
	{
		if (!ToggleOption("Self", &AS_Config.OptSelf))
		{
			if (((PSPAWNINFO)pLocalPlayer)->Mount) {
				ChangeSize((PSPAWNINFO)pLocalPlayer, ZERO_SIZE);
			}
			else {
				ChangeSize((PSPAWNINFO)pCharSpawn, ZERO_SIZE);
			}
		}
	}
	else if (ci_equals(szCurArg, "help"))
	{
		OutputHelp();
		return;
	}
	else if (ci_equals(szCurArg, "status"))
	{
		OutputStatus();
		return;
	}
	else if (ci_equals(szCurArg, "on"))
	{
		// TODO: maybe remove later, for now commented out Zonewide
		// SetEnabled(true);

		// TODO create something to look like zonewide is enabled
	}
	else if (ci_equals(szCurArg, "off"))
	{
		// TODO: maybe remove later, for now commented out Zonewide
		// SetEnabled(false);

		// TODO create something to look like zonewide is disabled
	}
	else
	{
		WriteChatf("\ay%s\aw:: \arInvalid command parameter.", MODULE_NAME);
		return;
	}

	// if size change or everything, pets, mercs,mounts toggled and won't be handled onpulse
	// TODO: maybe remove later, for now commented out Zonewide
	// if (AS_Config.OptByZone) {
	//	SpawnListResize(false);
	//}
}

PLUGIN_API void InitializePlugin()
{
	EzDetour(PlayerZoneClient__ChangeHeight, &PlayerZoneClient_Hook::ChangeHeight_Detour, &PlayerZoneClient_Hook::ChangeHeight_Trampoline);
	pAutoSizeType = new MQ2AutoSizeType;
	AddMQ2Data("AutoSize", dataAutoSize);
	AddCommand("/autosize", AutoSizeCmd);
	AddSettingsPanel("plugins/AutoSize", DrawAutoSize_MQSettingsPanel);
	LoadINI();
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveDetour(PlayerZoneClient__ChangeHeight);
	RemoveSettingsPanel("plugins/AutoSize");
	RemoveCommand("/autosize");
	SpawnListResize(true);
	SaveINI();
	RemoveMQ2Data("AutoSize");
	delete pAutoSizeType;
}

void DrawAutoSize_MQSettingsPanel()
{
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MQ2AutoSize");
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("AutoSizeTabBar", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Commands"))
		{
			ImGui::SeparatorText("Toggles");
			ImGui::Indent();
			if (ImGui::BeginTable("AutoSizeTogglesTable", 2, ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("");
				ImGui::TableNextColumn(); ImGui::Text("/autosize");
				ImGui::TableNextColumn(); ImGui::Text("Toggles zone-wide AutoSize on/off");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize dist");
				ImGui::TableNextColumn(); ImGui::Text("Toggles distance-based AutoSize on/off");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize pc");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize PC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize npc");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize NPC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize pets");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize pet spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize mercs");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize mercenary spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize mounts");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize mounted player spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize corpse");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize corpse spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize self");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize for your character");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize everything");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize all spawn types");
			ImGui::EndTable();
			ImGui::Unindent();
			}

			ImGui::SeparatorText("Size configuration (valid: 1 to 250)");
			ImGui::Indent();
			if (ImGui::BeginTable("AutoSizeSizeTable", 2, ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("");
				ImGui::TableNextColumn(); ImGui::Text("/autosize range #");
				ImGui::TableNextColumn(); ImGui::Text("Sets range for distance-based AutoSize");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize size #");
				ImGui::TableNextColumn(); ImGui::Text("Sets default size for everything");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizepc #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for PC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizenpc #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for NPC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizepets #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for pet spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizetarget #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for target parameter");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizemercs #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for mercenary spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizemounts #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for mounted player spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizecorpse #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for corpse spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizeself #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for your character");
			ImGui::EndTable();
			}
			ImGui::Unindent();

			ImGui::SeparatorText("Other commands");
			ImGui::Indent();
			if (ImGui::BeginTable("AutoSizeOtherCmdsTable", 2, ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("");
				ImGui::TableNextColumn(); ImGui::Text("/autosize status");
				ImGui::TableNextColumn(); ImGui::Text("Display current plugin settings to chatwnd");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize help");
				ImGui::TableNextColumn(); ImGui::Text("Display help menu to chatwnd");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize save");
				ImGui::TableNextColumn(); ImGui::Text("Save settings to INI file (auto on plugin unload)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize load");
				ImGui::TableNextColumn(); ImGui::Text("Load settings from INI file (auto on plugin load)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize target");
				ImGui::TableNextColumn(); ImGui::Text("Resizes your target to sizetarget size");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize autosave");
				ImGui::TableNextColumn(); ImGui::Text("Automatically save settings to INI file when an option is toggled or size is set");
			ImGui::EndTable();
			}
			ImGui::Unindent();

		ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Options"))
		{
			ImGui::SeparatorText("General");
			if (ImGui::RadioButton("Zonewide (really max clipping plane)", &optZonewide, 0)) {
				optZonewide = 0; // this is not a boolean, this indicates which radio button to enable
				previousRangeDistance = AS_Config.ResizeRange;
				AS_Config.ResizeRange = 1000;
				AS_Config.OptByRange = true; // TODO: enable by default, comment out any/all zonewide related things
				SpawnListResize(false);
			}
			if (ImGui::RadioButton("Range distance (recommended setting)", &optZonewide, 1)) {
				optZonewide = 1; // this is not a boolean, this indicates which radio button to enable
				AS_Config.ResizeRange = previousRangeDistance;
				AS_Config.OptByRange = true;
				SpawnListResize(false);
			}
			if (ImGui::Checkbox("Enable auto saving of configuration", &AS_Config.OptAutoSave)) {
				AS_Config.OptAutoSave = !AS_Config.OptAutoSave;
				DoCommandf("/autosize autosave");
			}
			ImGui::NewLine();
			if (ImGui::Button("Display status output")) {
				DoCommandf("/autosize status");
			}

			ImGui::SeparatorText("Toggles");
			if (ImGui::Checkbox("Resize: Yourself", &AS_Config.OptSelf)) {
				AS_Config.OptSelf = !AS_Config.OptSelf;
				DoCommandf("/autosize self");
			}
			if (ImGui::Checkbox("Resize: Other players (incluldes those mounted)", &AS_Config.OptPC)) {
				AS_Config.OptPC = !AS_Config.OptPC;
				DoCommandf("/autosize pc");
			}
			if (ImGui::Checkbox("Resize: Pets", &AS_Config.OptPet)) {
				AS_Config.OptPet = !AS_Config.OptPet;
				DoCommandf("/autosize pets");
			}
			if (ImGui::Checkbox("Resize: Mercs", &AS_Config.OptMerc)) {
				AS_Config.OptMerc = !AS_Config.OptMerc;
				DoCommandf("/autosize mercs");
			}
			if (ImGui::Checkbox("Resize: Mounts and the Player(s) on them", &AS_Config.OptMount)) {
				AS_Config.OptMount = !AS_Config.OptMount;
				DoCommandf("/autosize mounts");
			}
			if (ImGui::Checkbox("Resize: Corpse(s)", &AS_Config.OptCorpse)) {
				AS_Config.OptCorpse = !AS_Config.OptCorpse;
				DoCommandf("/autosize corpse");
			}

			if (ImGui::Checkbox("Resize: NPC(s)", &AS_Config.OptNPC)) {
				AS_Config.OptNPC = !AS_Config.OptNPC;
				DoCommandf("/autosize npc");
			}
			
			// display the button if any option is not enabled
			if (!AS_Config.OptCorpse || !AS_Config.OptMerc || !AS_Config.OptMount || !AS_Config.OptNPC || !AS_Config.OptPC || !AS_Config.OptPet || !AS_Config.OptSelf) {
				ImGui::NewLine();
				if (ImGui::Button("Resize Everything")) {
					DoCommandf("/autosize everything");
				}
		}
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Settings"))
		{
			// included in case code review asks for it to be
			// provided instead of being a hidden value where
			// no one is aware of how it works
			/*
			ImGui::BeginDisabled(true);
			ImGui::InputInt("Zonewide##input", &FAR_CLIP_PLANE, 10, 1000);
			ImGui::EndDisabled();
			*/
			ImGui::BeginDisabled(AS_Config.ResizeRange == 1000);
			ImGui::DragInt("Range distance##inputRD", &AS_Config.ResizeRange, 1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::EndDisabled();
			ImGui::DragInt("Self size##inputSS",		&AS_Config.SizeSelf,	1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Other player size##inputSS",&AS_Config.SizePC,		1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Pet size##inputPS",			&AS_Config.SizePet,		1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Merc size##inputMercSize",	&AS_Config.SizeMerc,	1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Mount size##inputMountSize",&AS_Config.SizeMount,	1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Corpse size##inputCS",		&AS_Config.SizeCorpse,	1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("Everything size##inputES",	&AS_Config.SizeDefault, 1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("NPC size##inputNS",			&AS_Config.SizeNPC,		1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp);
			// using SizeTarget will produce a bad experience
			// while having other options enabled
			/*
			if (ImGui::DragInt("Target size##inputTS", &AS_Config.SizeTarget, 1, 1, 250, "%d", ImGuiSliderFlags_AlwaysClamp)) {
				DoCommandf("/squelch /autosize target");
			}
			*/
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::TreePop();
}

void DoGroupCommand(std::string who, std::string_view command)
{
	if (comms == "None")
	{
		WriteChatf("MQ2AutoSize: Cannot execute group command, no group plugin configured.");
		return;
	}

	std::string groupCommand;
	groupCommand = "/squelch ";

	if (comms == "DanNet")
	{
		if (who == "zone")
			groupCommand += fmt::format("/dgza {}", command);
		else if (who == "raid")
			groupCommand += fmt::format("/dgra {}", command);
		else if (who == "group")
			groupCommand += fmt::format("/dgga {}", command);
		else if (who == "all")
			groupCommand += fmt::format("/dgae all {}", command);
	}
	else if (comms == "EQBC")
	{
		if (who == "group")
			groupCommand += fmt::format("/bcga /{}", command);
		else if (who == "all")
			groupCommand += fmt::format("/bcaa /{}", command);
	}

	if (!groupCommand.empty())
		DoCommandf(groupCommand.c_str());
}

// decide which plugin to use for communication
void ChooseInstructionPlugin() {
	// use DanNet if it's loaded
	if (GetPlugin("MQ2Dannet")) {
		comms = "DanNet";
		return;
	}
	// use EQBC if dannet isn't available
	if (GetPlugin("MQ2EQBC")) {
		comms = "EQBC";
	}
}