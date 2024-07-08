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
void ChooseInstructionPlugin();
void emulate(std::string type);
void DrawAutoSize_MQSettingsPanel();
void SendGroupCommand(std::string who);
int RoundToNearestTen(int value);
static bool isInGroup();
static bool isInRaid();
int optZonewide = 2; // defaults to selecting Range
int selectedComms = 0; // defaults to none, OnPulse will query for updates
int previousRangeDistance = 0;
bool loaded_dannet = false;
bool loaded_eqbc = false;
unsigned long long commsCheck;

enum class CommunicationMode {
	None = 0,
	DanNet = 1,
	EQBC = 2
};

enum class ResizeMode {
	None = 0,
	Zonewide = 1,
	Range = 2
};

// our configuration
class COurSizes {
public:
	COurSizes() {
		OptPC = true;
		OptNPC = OptPet = OptMerc = OptMount = OptCorpse = OptSelf = OptAutoSave = false;
		ResizeRange = 50;
		SizePC = SizeNPC = SizePet = SizeMerc = SizeMount = SizeCorpse = SizeSelf = 1;
	};

	bool OptAutoSave;
	bool OptPC;
	bool OptNPC;
	bool OptPet;
	bool OptMerc;
	bool OptMount;
	bool OptCorpse;
	bool OptSelf;

	int ResizeRange;
	int SizePC;
	int SizeNPC;
	int SizePet;
	int SizeMerc;
	int SizeMount;
	int SizeCorpse;
	int SizeSelf;
};
COurSizes AS_Config;

// exposed TLO variables
class MQ2AutoSizeType* pAutoSizeType = 0;
class MQ2AutoSizeType : public MQ2Type {
public:
	enum AutoSizeMembers {
		Active,
		AutoSave,
		ResizePC,
		ResizeNPC,
		ResizePets,
		ResizeMercs,
		ResizeMounts,
		ResizeCorpse,
		ResizeSelf,
		Range,
		SizePC,
		SizeNPC,
		SizePets,
		SizeMercs,
		SizeMounts,
		SizeCorpse,
		SizeSelf
	};

	MQ2AutoSizeType() :MQ2Type("AutoSize") {
		TypeMember(Active);
		TypeMember(AutoSave);
		TypeMember(ResizePC);
		TypeMember(ResizeNPC);
		TypeMember(ResizePets);
		TypeMember(ResizeMercs);
		TypeMember(ResizeMounts);
		TypeMember(ResizeCorpse);
		TypeMember(ResizeSelf);
		TypeMember(Range);
		TypeMember(SizePC);
		TypeMember(SizeNPC);
		TypeMember(SizePets);
		TypeMember(SizeMercs);
		TypeMember(SizeMounts);
		TypeMember(SizeCorpse);
		TypeMember(SizeSelf);
	}

	~MQ2AutoSizeType() {}

	bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) {

		MQTypeMember* pMember = MQ2AutoSizeType::FindMember(Member);
		if (!pMember)
			return false;

		switch ((AutoSizeMembers)pMember->ID) {
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
			case Range:
				Dest.Int = AS_Config.ResizeRange;
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

	bool ToString(MQVarPtr VarPtr, char* Destination) {
		return true;
	}
};

bool dataAutoSize(const char* szIndex, MQTypeVar& ret) {
	ret.DWord = 1;
	ret.Type = pAutoSizeType;
	return true;
}

// class to access the ChangeHeight function
class PlayerZoneClient_Hook {
public:
	DETOUR_TRAMPOLINE_DEF(void, ChangeHeight_Trampoline, (float, float, float, bool))
	void ChangeHeight_Detour(float newHeight, float cameraPos, float speedScale, bool unused) {
		ChangeHeight_Trampoline(newHeight, cameraPos, speedScale, unused);
	}

	// this assures valid function call
	void ChangeHeight_Wrapper(float fNewSize) {
		float fView = OTHER_SIZE;
		PlayerClient* pSpawn = reinterpret_cast<PlayerClient*>(this);

		if (pSpawn->SpawnID == pLocalPlayer->SpawnID) {
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
static int getSaneSize(const std::string& section, const std::string& key, int defaultValue) {
	int size = GetPrivateProfileInt(section.c_str(), key.c_str(), defaultValue, INIFileName);
	return std::clamp(size, MIN_SIZE, MAX_SIZE);
}

void LoadINI() {
	// defaulted options to off
	AS_Config.OptAutoSave = getOptionValue("Config", "AutoSave", "off");
	AS_Config.OptPC = getOptionValue("Config", "ResizePC", "off");
	AS_Config.OptNPC = getOptionValue("Config", "ResizeNPC", "off");
	AS_Config.OptPet = getOptionValue("Config", "ResizePets", "off");
	AS_Config.OptMerc = getOptionValue("Config", "ResizeMercs", "off");
	AS_Config.OptMount = getOptionValue("Config", "ResizeMounts", "off");
	AS_Config.OptCorpse = getOptionValue("Config", "ResizeCorpse", "off");
	AS_Config.OptSelf = getOptionValue("Config", "ResizeSelf", "off");
	AS_Config.ResizeRange = getSaneSize("Config", "Range", AS_Config.ResizeRange);
	AS_Config.SizePC = getSaneSize("Config", "SizePC", MIN_SIZE);
	AS_Config.SizeNPC = getSaneSize("Config", "SizeNPC", MIN_SIZE);
	AS_Config.SizePet = getSaneSize("Config", "SizePets", MIN_SIZE);
	AS_Config.SizeMerc = getSaneSize("Config", "SizeMercs",	MIN_SIZE);
	AS_Config.SizeMount = getSaneSize("Config", "SizeMounts", MIN_SIZE);
	AS_Config.SizeCorpse = getSaneSize("Config", "SizeCorpse", MIN_SIZE);
	AS_Config.SizeSelf = getSaneSize("Config", "SizeSelf", MIN_SIZE);
	WriteChatf("\ay%s\aw:: Configuration file loaded.", MODULE_NAME);
	
	// apply new settings from INI read
	if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer) {
		SpawnListResize(false);
	}
}

// you can pass a param for an INI key to only save that single key or
// you can leave the param null and save everything in the map
// you can enable squelch which just not print to the client
void SaveINI(const std::string& param = "", const bool squelch = 0) {
	// Map to store configuration key-value pairs
	std::map<std::string, std::string> configMap = {
		{"AutoSave", AS_Config.OptAutoSave ? "on" : "off"},
		{"ResizePC", AS_Config.OptPC ? "on" : "off"},
		{"ResizeNPC", AS_Config.OptNPC ? "on" : "off"},
		{"ResizePets", AS_Config.OptPet ? "on" : "off"},
		{"ResizeMercs", AS_Config.OptMerc ? "on" : "off"},
		{"ResizeMounts", AS_Config.OptMount ? "on" : "off"},
		{"ResizeCorpse", AS_Config.OptCorpse ? "on" : "off"},
		{"ResizeSelf", AS_Config.OptSelf ? "on" : "off"},
		{"SizePC", std::to_string(AS_Config.SizePC)},
		{"SizeNPC", std::to_string(AS_Config.SizeNPC)},
		{"SizePets", std::to_string(AS_Config.SizePet)},
		{"SizeMercs", std::to_string(AS_Config.SizeMerc)},
		{"SizeMounts", std::to_string(AS_Config.SizeMount)},
		{"SizeCorpse", std::to_string(AS_Config.SizeCorpse)},
		{"SizeSelf", std::to_string(AS_Config.SizeSelf)}
	};

	// this writes a specific key to disk with its value
	if (!param.empty()) {
		auto it = configMap.find(param);
		if (it != configMap.end()) {
			WritePrivateProfileString("Config", it->first, it->second, INIFileName);
		}
		// special handling for Range key, we don't want to write the FAR_CLIP_PLANE to disk
		else if (param == "ResizeRange" && AS_Config.ResizeRange != FAR_CLIP_PLANE) {
			WritePrivateProfileString("Config", "Range", std::to_string(AS_Config.ResizeRange), INIFileName);
		}
	}
	else {
		// this writes all keys and their values to disk as normal
		for (const auto& [key, value] : configMap) {
			WritePrivateProfileString("Config", key, value, INIFileName);
		}
		// special handling for Range since it's not part of the map (intentionally)
		if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
			WritePrivateProfileString("Config", "Range", std::to_string(AS_Config.ResizeRange), INIFileName);
		}
	}

	// only display info if squelch is false
	if (!squelch) {
		WriteChatf("\ay%s\aw:: Configuration file saved.", MODULE_NAME);
	}
	
}

void ChangeSize(PlayerClient* pChangeSpawn, float fNewSize) {
	if (pChangeSpawn) {
		reinterpret_cast<PlayerZoneClient_Hook*>(pChangeSpawn)->ChangeHeight_Wrapper(fNewSize);
	}
}

void SizePasser(PSPAWNINFO pSpawn, bool bReset) {
	if (GetGameState() != GAMESTATE_INGAME) {
		return;
	}

	PSPAWNINFO pLPlayer = (PSPAWNINFO)pLocalPlayer;
	if (!pLPlayer || !pLPlayer->SpawnID || !pSpawn || !pSpawn->SpawnID) return;

	if (pSpawn->SpawnID == pLPlayer->SpawnID) {
		if (AS_Config.OptSelf) ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeSelf);
		return;
	}

	switch (GetSpawnType(pSpawn)) {
		case PC:
			if (AS_Config.OptPC) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizePC);
				return;
			}
			break;
		case NPC:
			if (AS_Config.OptNPC) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeNPC);
				return;
			}
			break;
		case PET:
			if (AS_Config.OptPet) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizePet);
				return;
			}
			break;
		case MERCENARY:
			if (AS_Config.OptMerc) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeMerc);
				return;
			}
			break;
		case MOUNT:
			if (AS_Config.OptMount && pSpawn->SpawnID != pLPlayer->SpawnID) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeMount);
				return;
			}
			break;
		case CORPSE:
			if (AS_Config.OptCorpse) {
				ChangeSize(pSpawn, bReset ? ZERO_SIZE : AS_Config.SizeCorpse);
				return;
			}
			break;
		default:
			break;
	}
}

void ResetAllByType(eSpawnType OurType) {
	PSPAWNINFO pSpawn = (PSPAWNINFO)pSpawnList;
	PSPAWNINFO pLPlayer = (PSPAWNINFO)pLocalPlayer;
	if (GetGameState() != GAMESTATE_INGAME || !pLPlayer || !pLPlayer->SpawnID || !pSpawn || !pSpawn->SpawnID) {
		return;
	}

	while (pSpawn) {
		if (pSpawn->SpawnID == pLPlayer->SpawnID) {
			pSpawn = pSpawn->pNext;
			continue;
		}

		eSpawnType ListType = GetSpawnType(pSpawn);
		if (ListType == OurType) ChangeSize(pSpawn, ZERO_SIZE);
		// Handle Everything resize all by using NONE type
		// it's not a great option but we aren't using NONE 
		// for anything else so its available for use
		if (OurType == 0) {
			ChangeSize(pSpawn, ZERO_SIZE);
		}
		pSpawn = pSpawn->pNext;
	}
}

void SpawnListResize(bool bReset) {
	if (GetGameState() != GAMESTATE_INGAME) return;
	PSPAWNINFO pSpawn = (PSPAWNINFO)pSpawnList;
	while (pSpawn) {
		SizePasser(pSpawn, bReset);
		pSpawn = pSpawn->pNext;
	}
}

PLUGIN_API void OnAddSpawn(PSPAWNINFO pNewSpawn) {
	// nothing additional to perform over existing functionality
}

PLUGIN_API void OnEndZone() {
	SpawnListResize(false);
}

PLUGIN_API void OnPulse() {
	if (GetGameState() != GAMESTATE_INGAME) return;
	if (uiSkipPulse < SKIP_PULSES) {
		uiSkipPulse++;
		return;
	}

	// imgui panel related
	// check if communication plugins are still running and adjust UI as needed
	if (GetTickCount64() > commsCheck) {
		commsCheck = std::int64_t(commsCheck) + 1000; // only check again 1 second from now
		ChooseInstructionPlugin();
	}
	

	PSPAWNINFO pAllSpawns = (PSPAWNINFO)pSpawnList;
	float fDist = 0.0f;
	uiSkipPulse = 0;

	while (pAllSpawns) {
		fDist = GetDistance((PSPAWNINFO)pLocalPlayer, pAllSpawns);
		if (fDist < AS_Config.ResizeRange) {
			SizePasser(pAllSpawns, false);
		}
		else if (fDist < AS_Config.ResizeRange + 50) {
			SizePasser(pAllSpawns, true);
		}
		pAllSpawns = pAllSpawns->pNext;
	}
}

void OutputHelp() {
	WriteChatf("\ay%s\aw:: Command Usage Help", MODULE_NAME);
	WriteChatf("  \ag/autosize\ax - Toggles zone-wide AutoSize on/off");
	WriteChatf("  \ag/autosize\ax \aydist\ax - Toggles distance-based AutoSize on/off");
	WriteChatf("  \ag/autosize\ax \ayrange #\ax - Sets range for distance checking");
	WriteChatf("--- Valid Resize Toggles ---");
	WriteChatf("  \ag/autosize\ax [ \aypc\ax | \aynpc\ax | \aypets\ax | \aymercs\ax | \aymounts\ax | \aycorpse\ax | \ayeverything\ax | \ayself\ax ]");
	WriteChatf("--- Valid Size Syntax (1 to 250) ---");
	WriteChatf("  \ag/autosize\ax [ \aysizepc\ax | \aysizenpc\ax | \aysizepets\ax | \aysizemercs\ax | \aysizemounts\ax | \aysizecorpse\ax | \aysizeself\ax ] [ \ay#\ax ]");
	WriteChatf("--- Other Valid Commands ---");
	WriteChatf("  \ag/autosize\ax [ \ayhelp\ax | \aystatus\ax | \ayautosave\ax | \aysave\ax | \ayload\ax ]");
}

void OutputStatus() {
	char szMethod[100] = { 0 };
	char szOn[10] = "\agon\ax";
	char szOff[10] = "\aroff\ax";

	// replaced above with a more direct feedback
	if (!AS_Config.OptPC && !AS_Config.OptNPC && !AS_Config.OptPet && !AS_Config.OptMerc && !AS_Config.OptCorpse && !AS_Config.OptSelf) {
		sprintf_s(szMethod, "\arInactive\ax");
	}
	else if (AS_Config.ResizeRange < FAR_CLIP_PLANE) {
		sprintf_s(szMethod, "\ayRange\ax) RangeSize(\ag%d\ax", AS_Config.ResizeRange);
	}
	else if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
		// covers the idea of Zonewide by using FAR_CLIP_PLANE value (which is about 100% Far Cliping Plane)
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
		szOff // // no longer available but left to ensure that no random script that reads this, breaks.
	);
	WriteChatf("Sizes: PC(\ag%d\ax) NPC(\ag%d\ax) Pets(\ag%d\ax) Mercs(\ag%d\ax) Mounts(\ag%d\ax) Corpses(\ag%d\ax) Target(\ag%d\ax) Self(\ag%d\ax) Everything(\ag%d\ax)",
		AS_Config.SizePC,
		AS_Config.SizeNPC,
		AS_Config.SizePet,
		AS_Config.SizeMerc,
		AS_Config.SizeMount,
		AS_Config.SizeCorpse,
		0, // no longer available but left to ensure that no random script that reads this, breaks.
		AS_Config.SizeSelf,
		0 // no longer available but left to ensure that no random script that reads this, breaks.
	);
}

bool ToggleOption(const char* pszToggleOutput, bool* pbOption) {
	*pbOption = !*pbOption;
	WriteChatf("\ay%s\aw:: Option (\ay%s\ax) now %s\ax", MODULE_NAME, pszToggleOutput, *pbOption ? "\agenabled" : "\ardisabled");
	if (AS_Config.OptAutoSave) SaveINI();
	return *pbOption;
}

void SetSizeConfig(const char* pszOption, int iNewSize, int* iOldSize) {
	// special handling for Range being set to Zonewide
	if (ci_equals(pszOption, "range") && iNewSize == FAR_CLIP_PLANE) {
		// set the pointer to the new value
		*iOldSize = iNewSize;
		WriteChatf("\ay%s\aw:: Range size is \agZonewide\ax - %d units", MODULE_NAME, FAR_CLIP_PLANE);
		// return early to avoid saving to INI
		return;
	}

	// make sure that we are setting values for a valid reason
	if ((iNewSize != *iOldSize) && (iNewSize >= MIN_SIZE && iNewSize <= MAX_SIZE)) {
		int iPrevSize = *iOldSize;
		// set the pointer to the new value
		*iOldSize = iNewSize;
		WriteChatf("\ay%s\aw:: %s size changed from \ay%d\ax to \ag%d", MODULE_NAME, pszOption, iPrevSize, *iOldSize);
	}
	else {
		WriteChatf("\ay%s\aw:: %s size is \ag%d\ax (was not modified)", MODULE_NAME, pszOption, *iOldSize);
	}
	if (AS_Config.OptAutoSave) SaveINI();
}

void AutoSizeCmd(PSPAWNINFO pLPlayer, char* szLine) {
	char szCurArg[MAX_STRING] = { 0 };
	char szNumber[MAX_STRING] = { 0 };
	GetArg(szCurArg, szLine, 1);
	GetArg(szNumber, szLine, 2);
	int iNewSize = std::atoi(szNumber);

	if (!*szCurArg)	{
		// fake Zonewide with large value which is effectively Far Cliping Plane at 100%
		if (AS_Config.ResizeRange >= MIN_SIZE && AS_Config.ResizeRange < FAR_CLIP_PLANE) {
			// this means we are currently using Range distance normally
			// now we want look like we are toggling to Zonewide
			// we will do this by increasing the ResizeRange to FAR_CLIP_PLANE
			// SetSizeConfig() will mention Zonewide based on FAR_CLIP_PLANE value being used
			//SetSizeConfig("range", FAR_CLIP_PLANE, &AS_Config.ResizeRange);
			emulate("zonewide");
		}
		else if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
			// this means we are pretending to be Zonewide and need to revert to Range based
			// we will do this by reseting the ResizeRange to what is in INI
			emulate("range");

			// TODO: DELETE
			//AS_Config.ResizeRange = FAR_CLIP_PLANE;
			//if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
				//SetSizeConfig("range", previousRangeDistance, &AS_Config.ResizeRange);
			//}
		}
		return;
	}
	else if (ci_equals(szCurArg, "dist")) {
		if (ci_equals(szNumber, "on")) {
			if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
				emulate("range");
			}
		}
		else if (ci_equals(szNumber, "off")) {
			if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
				emulate("zonewide");
			}
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
				emulate("range");
			}
			else if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
				emulate("zonewide");
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "save")) {
		SaveINI();
		return;
	}
	else if (ci_equals(szCurArg, "load")) {
		LoadINI();
		return;
	}
	else if (ci_equals(szCurArg, "autosave")) {
		if (ci_equals(szNumber, "on")) {
			if (!AS_Config.OptAutoSave) {
				ToggleOption("Autosave", &AS_Config.OptAutoSave);
			}
		}
		else if (ci_equals(szNumber, "off")) {
			if (AS_Config.OptAutoSave) {
				ToggleOption("Autosave", &AS_Config.OptAutoSave);
			}
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			ToggleOption("Autosave", &AS_Config.OptAutoSave);
		}
		return;
	}
	else if (ci_equals(szCurArg, "range")) {
		SetSizeConfig("range", iNewSize, &AS_Config.ResizeRange);
		return;
	}
	else if (ci_equals(szCurArg, "size")) {
		WriteChatf("\ay%s\aw:: This feature (\ay%s\ax) has been deprecated. Check /mqsetting -> plugins -> AutoSize.", MODULE_NAME, szCurArg);
	}
	else if (ci_equals(szCurArg, "sizepc")) {
		SetSizeConfig("PC", iNewSize, &AS_Config.SizePC);
	}
	else if (ci_equals(szCurArg, "sizenpc")) {
		SetSizeConfig("NPC", iNewSize, &AS_Config.SizeNPC);
	}
	else if (ci_equals(szCurArg, "sizepets")) {
		SetSizeConfig("Pet", iNewSize, &AS_Config.SizePet);
	}
	else if (ci_equals(szCurArg, "sizemercs")) {
		SetSizeConfig("Mercs", iNewSize, &AS_Config.SizeMerc);
	}
	else if (ci_equals(szCurArg, "sizemounts")) {
		SetSizeConfig("Mounts", iNewSize, &AS_Config.SizeMount);
	}
	else if (ci_equals(szCurArg, "sizecorpse")) {
		SetSizeConfig("Corpses", iNewSize, &AS_Config.SizeCorpse);
	}
	else if (ci_equals(szCurArg, "sizeself")) {
		SetSizeConfig("Self", iNewSize, &AS_Config.SizeSelf);
	}
	else if (ci_equals(szCurArg, "pc")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptPC) {
			ToggleOption("PC", &AS_Config.OptPC);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptPC) {
			ToggleOption("PC", &AS_Config.OptPC);
			ResetAllByType(PC);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("PC", &AS_Config.OptPC)) {
				ResetAllByType(PC);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "npc")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptNPC) {
			ToggleOption("NPC", &AS_Config.OptNPC);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptNPC) {
			ToggleOption("NPC", &AS_Config.OptNPC);
			ResetAllByType(NPC);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("NPC", &AS_Config.OptNPC)) {
				ResetAllByType(NPC);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "everything")) {
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
	else if (ci_equals(szCurArg, "pets")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptPet) {
			ToggleOption("Pets", &AS_Config.OptPet);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptPet) {
			ToggleOption("Pets", &AS_Config.OptPet);
			ResetAllByType(PET);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("Pets", &AS_Config.OptPet)) {
				ResetAllByType(PET);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "mercs")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptMerc) {
			ToggleOption("Mercs", &AS_Config.OptMerc);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptMerc) {
			ToggleOption("Mercs", &AS_Config.OptMerc);
			ResetAllByType(MERCENARY);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("Mercs", &AS_Config.OptMerc)) {
				ResetAllByType(MERCENARY);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "mounts")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptMount) {
			ToggleOption("Mounts", &AS_Config.OptMount);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptMount) {
			ToggleOption("Mounts", &AS_Config.OptMount);
			ResetAllByType(MOUNT);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("Mounts", &AS_Config.OptMount)) {
				ResetAllByType(MOUNT);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "corpse")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptCorpse) {
			ToggleOption("Corpses", &AS_Config.OptCorpse);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptCorpse) {
			ToggleOption("Corpses", &AS_Config.OptCorpse);
			ResetAllByType(CORPSE);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("Corpses", &AS_Config.OptCorpse)) {
				ResetAllByType(CORPSE);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "target")) {
		// deprecated because when you use this while having features enabled this feature didn't actually work.
		// if you don't have anything resizing, why would you want to resize just the target?
		WriteChatf("\ay%s\aw:: This feature (\ay%s\ax) has been deprecated. Check /mqsetting -> plugins -> AutoSize.", MODULE_NAME, szCurArg);
	}
	else if (ci_equals(szCurArg, "self")) {
		if (ci_equals(szNumber, "on") && !AS_Config.OptSelf) {
			ToggleOption("Self", &AS_Config.OptSelf);
		}
		else if (ci_equals(szNumber, "off") && AS_Config.OptSelf) {
			ToggleOption("Self", &AS_Config.OptSelf);
			ChangeSize((PSPAWNINFO)pLocalPlayer, ZERO_SIZE);
		}
		else if (!ci_equals(szNumber, "on") && !ci_equals(szNumber, "off")) {
			if (!ToggleOption("Self", &AS_Config.OptSelf)) {
				ChangeSize((PSPAWNINFO)pLocalPlayer, ZERO_SIZE);
			}
		}
		return;
	}
	else if (ci_equals(szCurArg, "help")) {
		OutputHelp();
		return;
	}
	else if (ci_equals(szCurArg, "status")) {
		OutputStatus();
		return;
	}
	else if (ci_equals(szCurArg, "on")) {
		emulate("zonewide");
	}
	else if (ci_equals(szCurArg, "off")) {
		emulate("range");
	}
	else {
		WriteChatf("\ay%s\aw:: \arInvalid command parameter.", MODULE_NAME);
		return;
	}
}

PLUGIN_API void InitializePlugin() {
	EzDetour(PlayerZoneClient__ChangeHeight, &PlayerZoneClient_Hook::ChangeHeight_Detour, &PlayerZoneClient_Hook::ChangeHeight_Trampoline);
	pAutoSizeType = new MQ2AutoSizeType;
	AddMQ2Data("AutoSize", dataAutoSize);
	AddCommand("/autosize", AutoSizeCmd);
	AddSettingsPanel("plugins/AutoSize", DrawAutoSize_MQSettingsPanel);
	LoadINI();
}

PLUGIN_API void ShutdownPlugin() {
	RemoveDetour(PlayerZoneClient__ChangeHeight);
	RemoveSettingsPanel("plugins/AutoSize");
	RemoveCommand("/autosize");
	SpawnListResize(true);
	if (AS_Config.OptAutoSave) {
		SaveINI();
	}
	RemoveMQ2Data("AutoSize");
	delete pAutoSizeType;
}

void DrawAutoSize_MQSettingsPanel() {
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MQ2AutoSize");
	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("AutoSizeTabBar", tab_bar_flags)) {
				
		if (ImGui::BeginTabItem("Options")) {
			ImGui::SeparatorText("General");
			// General: auto save
			if (ImGui::Checkbox("Enable auto saving of configuration", &AS_Config.OptAutoSave)) {
				AS_Config.OptAutoSave = !AS_Config.OptAutoSave;
				DoCommandf("/autosize autosave");
			}
			// General: Zodewide
			if (ImGui::RadioButton("Zonewide (max clipping plane)", &optZonewide, static_cast<int>(ResizeMode::Zonewide))) {
				optZonewide = static_cast<int>(ResizeMode::Zonewide);
				emulate("zonewide");
			}
			// General: Range
			if (ImGui::BeginTable("OptionsResizeRangeTable", 2, ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableNextColumn();
				if (ImGui::RadioButton("", &optZonewide, static_cast<int>(ResizeMode::Range))) {
					optZonewide = static_cast<int>(ResizeMode::Range);
					emulate("range");
				}
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(AS_Config.ResizeRange == FAR_CLIP_PLANE);
				ImGui::PushItemWidth(50.0f);
				if (ImGui::SliderInt("Range distance (recommended setting)##inputRD", &AS_Config.ResizeRange, 10, 250, "%d", ImGuiSliderFlags_NoInput|ImGuiSliderFlags_AlwaysClamp)) {
					AS_Config.ResizeRange = RoundToNearestTen(AS_Config.ResizeRange);
					if (AS_Config.OptAutoSave) {
						SaveINI("ResizeRange", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::EndTable();
			}
			// General: Status output
			if (ImGui::Button("Display status output")) {
				DoCommandf("/autosize status");
			}
			ImGui::SeparatorText("Toggles and Values");
			if (ImGui::BeginTable("OptionsResizeSelfTable", 2, ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				// Option: Self
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptSelf", &AS_Config.OptSelf)) {
					AS_Config.OptSelf = !AS_Config.OptSelf;
					DoCommandf("/autosize self");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptSelf);
				if (ImGui::SliderInt("Resize: Self##inputSS", &AS_Config.SizeSelf, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizeSelf", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Other PCs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptPC", &AS_Config.OptPC)) {
					AS_Config.OptPC = !AS_Config.OptPC;
					DoCommandf("/autosize pc");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptPC);
				if (ImGui::SliderInt("Resize: Other player(s) (incluldes those mounted)##inputOP", &AS_Config.SizePC, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizePC", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Pets
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptPet", &AS_Config.OptPet)) {
					AS_Config.OptPet = !AS_Config.OptPet;
					DoCommandf("/autosize pets");
				}
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptPet);
				if (ImGui::SliderInt("Resize: Pets##inputPS", &AS_Config.SizePet, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizePet", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Mercs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptMerc", &AS_Config.OptMerc)) {
					AS_Config.OptMerc = !AS_Config.OptMerc;
					DoCommandf("/autosize mercs");
				}
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptMerc);
				if (ImGui::SliderInt("Resize: Mercs##inputMercSize", &AS_Config.SizeMerc, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizeMerc", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Mounts
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptMount", &AS_Config.OptMount)) {
					AS_Config.OptMount = !AS_Config.OptMount;
					DoCommandf("/autosize mounts");
				}
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptMount);
				if (ImGui::SliderInt("Resize: Mounts and the Player(s) on them##inputMountSize", &AS_Config.SizeMount, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizeMount", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Corpses
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptCorpse", &AS_Config.OptCorpse)) {
					AS_Config.OptCorpse = !AS_Config.OptCorpse;
					DoCommandf("/autosize corpse");
				}
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptCorpse);
				if (ImGui::SliderInt("Resize: Corpse(s)##inputCS", &AS_Config.SizeCorpse, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizeCorpse", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: NPCs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptNPC", &AS_Config.OptNPC)) {
					AS_Config.OptNPC = !AS_Config.OptNPC;
					DoCommandf("/autosize npc");
				}
				ImGui::TableNextColumn();
				ImGui::PushItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptNPC);
				if (ImGui::SliderInt("Resize: NPC(s)##inputNS", &AS_Config.SizeNPC, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp)) {
					if (AS_Config.OptAutoSave) {
						SaveINI("SizeNPC", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::EndTable();
			}
			
			// display the button if any option is not enabled
			if (!AS_Config.OptCorpse || !AS_Config.OptMerc || !AS_Config.OptMount || !AS_Config.OptNPC || !AS_Config.OptPC || !AS_Config.OptPet || !AS_Config.OptSelf) {
				if (ImGui::Button("Resize Everything (select all)")) {
					DoCommandf("/autosize everything");
				}
			}

			ImGui::SeparatorText("Synchronize clients");
			ImGui::BeginDisabled(loaded_dannet || loaded_eqbc);
			if (ImGui::RadioButton("None", &selectedComms, static_cast<int>(CommunicationMode::None))) {
				selectedComms = static_cast<int>(CommunicationMode::None);
				return;
			}
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!loaded_dannet);
			if (ImGui::RadioButton("MQ2DanNet", &selectedComms, static_cast<int>(CommunicationMode::DanNet))) {
				selectedComms = static_cast<int>(CommunicationMode::DanNet);
			}
			if (!loaded_dannet) {
				ImGui::SameLine();
				ImGui::Text("(plugin not loaded)");
			}
			ImGui::EndDisabled();
			
			ImGui::BeginDisabled(!loaded_eqbc);
			if (ImGui::RadioButton("MQ2EQBC", &selectedComms, static_cast<int>(CommunicationMode::EQBC))) {
				selectedComms = static_cast<int>(CommunicationMode::EQBC);
			}
			if (!loaded_eqbc) {
				ImGui::SameLine();
				ImGui::Text("(plugin not loaded)");
			}
			ImGui::EndDisabled();

			// dannet
			if (selectedComms == static_cast<int>(CommunicationMode::DanNet) && loaded_dannet) {
				if (ImGui::Button("All")) {
					SendGroupCommand("all");
				}
				ImGui::SameLine();
				if (ImGui::Button("Zone")) {
					SendGroupCommand("zone");
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(!isInRaid());
				if (ImGui::Button("Raid")) {
					SendGroupCommand("raid");
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::BeginDisabled(!isInGroup());
				if (ImGui::Button("Group")) {
					SendGroupCommand("group");
				}
				ImGui::EndDisabled();
			} else if (selectedComms == static_cast<int>(CommunicationMode::EQBC) && loaded_eqbc) {
				// eqbc
				if (ImGui::Button("All")) {
					SendGroupCommand("all");
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(!isInGroup());
				if (ImGui::Button("Group")) {
					SendGroupCommand("group");
				}
				ImGui::EndDisabled();
			}

			ImGui::EndTabItem();
		}
				
		if (ImGui::BeginTabItem("Commands")) {
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
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizepc #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for PC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizenpc #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for NPC spawn types");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize sizepets #");
				ImGui::TableNextColumn(); ImGui::Text("Sets size for pet spawn types");
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
				ImGui::TableNextColumn(); ImGui::Text("/autosize autosave");
				ImGui::TableNextColumn(); ImGui::Text("Automatically save settings to INI file when an option is toggled or size is set");
				ImGui::EndTable();
			}
			ImGui::Unindent();

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

// send instruction to select few
// -> dannet: all, zone, raid, group
// -> eqbc: all, group
void SendGroupCommand(std::string who) {
	if (selectedComms == static_cast<int>(CommunicationMode::None)) {
		WriteChatf("MQ2AutoSize: Cannot execute group command, no group plugin configured.");
		return;
	}

	// the groupCommand is used as a single string which is made up of several
	// concatenated instructions that relate to different options and size
	// values. When enhancing or modifying be sure to remember to add a space
	// to the end of the concatenated string if there are further instructions
	// that will be added
	std::string groupCommand = "/squelch ";
	std::string instruction;

	// if auto save is enabled
	if (AS_Config.OptAutoSave) {
		instruction = "/autosize load";
	}
	else {
		// if auto save is not enabled, we have to go through every setting
		// and create a command which covers both options and size values 
		// in a single multiline command based on the current settings of
		// the player instructing the synchronization to happen
		instruction = "/multiline ; "; // must start with this as there are several instructions
		
		// autosave
		instruction += fmt::format("/autosize autosave {}; ", AS_Config.OptAutoSave ? "on" : "off");
		
		// zonewide and range
		if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
			// this covers the use case of the instructor having "zonewide" enabled
			instruction += "/autosize on; ";
		}
		else if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
			// this covers the use case of the instructor having "range" enabled
			instruction += fmt::format("/autosize off; /autosize range {}; ", AS_Config.ResizeRange);
		}
		
		// OptPC + SizePC
		instruction += fmt::format("/autosize pc {}; /autosize sizepc {}; ", AS_Config.OptPC ? "on" : "off", AS_Config.SizePC);
		
		// OptNPC + SizeNPC
		instruction += fmt::format("/autosize npc {}; /autosize sizenpc {}; ", AS_Config.OptNPC ? "on" : "off", AS_Config.SizeNPC);
				
		// OptPet + SizePets
		instruction += fmt::format("/autosize pets {}; /autosize sizepets {}; ", AS_Config.OptPet ? "on" : "off", AS_Config.SizePet);

		// OptMerc + SizeMercs
		instruction += fmt::format("/autosize mercs {}; /autosize sizemercs {}; ", AS_Config.OptMerc ? "on" : "off", AS_Config.SizeMerc);
		
		// OptMount + SizeMounts
		instruction += fmt::format("/autosize mounts {}; /autosize sizemounts {}; ", AS_Config.OptMount ? "on" : "off", AS_Config.SizeMount);
		
		// OptCorpse + SizeCorpses
		instruction += fmt::format("/autosize corpse {}; /autosize sizecorpse {}; ", AS_Config.OptCorpse ? "on" : "off", AS_Config.SizeCorpse);
		
		// OptSelf + SizeSelf
		instruction += fmt::format("/autosize self {}; /autosize sizeself {}; ", AS_Config.OptSelf ? "on" : "off", AS_Config.SizeSelf);
	}

	// instructions are sent to others, since we have the configuration already
	if (selectedComms == static_cast<int>(CommunicationMode::DanNet)) {
		if (who == "zone")
			groupCommand += fmt::format("/dgze {}", instruction);
		else if (who == "raid")
			groupCommand += fmt::format("/dgre {}", instruction);
		else if (who == "group")
			groupCommand += fmt::format("/dgge {}", instruction);
		else if (who == "all")
			groupCommand += fmt::format("/dge {}", instruction);
	}
	else if (selectedComms == static_cast<int>(CommunicationMode::EQBC)) {
		if (who == "group")
			groupCommand += fmt::format("/bcg /{}", instruction);
		else if (who == "all")
			groupCommand += fmt::format("/bca /{}", instruction);
	}

	if (!groupCommand.empty())
		DoCommandf(groupCommand.c_str());
}

/**
 * This function checks the loaded state of the MQ2EQBC and MQ2Dannet plugins
 * and sets the communication mode accordingly.
 * 
 * - If DanNet is loaded, it sets the communication mode to DanNet.
 * - If EQBC is loaded and DanNet is not loaded, it sets the communication mode to EQBC.
 * - If neither plugin is loaded, it sets the communication mode to None.
 *
 * It also handles transitions between different plugin states to ensure that
 * the communication mode is updated correctly when the state changes.
 * 
 * If both communication plugins are loaded, the default is to use DanNet.
 */
// choose the plugin to use for communication
void ChooseInstructionPlugin() {
	bool prev_loaded_eqbc = loaded_eqbc;
	bool prev_loaded_dannet = loaded_dannet;

	loaded_eqbc = GetPlugin("MQ2EQBC");
	loaded_dannet = GetPlugin("MQ2Dannet");

	if (loaded_dannet) {
		selectedComms = static_cast<int>(CommunicationMode::DanNet);
	}
	else if (loaded_eqbc) {
		selectedComms = static_cast<int>(CommunicationMode::EQBC);
	}
	else {
		selectedComms = static_cast<int>(CommunicationMode::None);
	}

	// check for changes in loaded plugins
	if (prev_loaded_dannet && !loaded_dannet && loaded_eqbc) {
		selectedComms = static_cast<int>(CommunicationMode::EQBC);
	}
	else if (prev_loaded_eqbc && !loaded_eqbc && loaded_dannet) {
		selectedComms = static_cast<int>(CommunicationMode::DanNet);
	}
	else if (!prev_loaded_eqbc && !prev_loaded_dannet) {
		if (loaded_eqbc && !loaded_dannet) {
			selectedComms = static_cast<int>(CommunicationMode::EQBC);
		}
		else if (!loaded_eqbc && loaded_dannet) {
			selectedComms = static_cast<int>(CommunicationMode::DanNet);
		}
		else {
			selectedComms = static_cast<int>(CommunicationMode::None);
		}
	}
}

/**
 * This function adjusts the configuration settings based on the provided type.
 * 
 * It can emulate a "zonewide" configuration by setting a large range. 
 * It can also revert to a previous range value for "range" type.
 *
 * @param type The type of configuration to emulate. Valid values are "zonewide" and "range".
 *    - "zonewide": Sets the range to FAR_CLIP_PLANE value.
 *    - "range": Sets the range to the previously configured value.
 */
// this function is used as a toggle between Zone and Range
// params: zonewide or range
void emulate(std::string type) {
	if (type == "zonewide") {
		if (AS_Config.ResizeRange != FAR_CLIP_PLANE) {
			previousRangeDistance = AS_Config.ResizeRange;
			optZonewide = static_cast<int>(ResizeMode::Zonewide);
			AS_Config.ResizeRange = FAR_CLIP_PLANE;
			WriteChatf("\ay%s\aw:: AutoSize (\ayRange\ax) now \ardisabled\ax!", MODULE_NAME);
			WriteChatf("\ay%s\aw:: AutoSize (\ayZonewide\ax) now \agenabled\ax!", MODULE_NAME);
			SpawnListResize(false);
		}
		return;
	}
	else if (type == "range") {
		if (AS_Config.ResizeRange == FAR_CLIP_PLANE) {
			AS_Config.ResizeRange = previousRangeDistance;
			optZonewide = static_cast<int>(ResizeMode::Range);
			WriteChatf("\ay%s\aw:: AutoSize (\ayZonewide\ax) now \ardisabled\ax!", MODULE_NAME);
			WriteChatf("\ay%s\aw:: AutoSize (\ayRange\ax) now \agenabled\ax!", MODULE_NAME);
			SpawnListResize(false);
		}
	}
}

int RoundToNearestTen(int value) {
	// clamp lower value
	if (value < 10) {
		return 10;
	}
	else if (value > MAX_SIZE) {
		return 250;
	}

	int roundedValue = (value + 9) / 10 * 10;

	// clamp upper value post rounding
	if (roundedValue > MAX_SIZE) {
		return 250;
	}
	else {
		return roundedValue;
	}
}

static bool isInGroup() {
	if (pLocalPC) {
		if (pLocalPC->Group) {
			return true;
		}
	}
	return false;
}

static bool isInRaid() {
	if (pRaid) {
		if (pRaid->RaidMemberCount) {
			return true;
		}
	}
	return false;
}