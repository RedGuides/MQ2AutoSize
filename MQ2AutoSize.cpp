// MQ2AutoSize.cpp : Resize spawns by distance (client only)

#include <mq/Plugin.h>
#include <chrono>

// Plugin Setup
PreSetup("MQ2AutoSize");
PLUGIN_VERSION(1.3);

// Constants
constexpr std::chrono::milliseconds UPDATE_INTERVAL{ 200 }; // Controls the update frequency to perform a radius-based resize
constexpr int MIN_SIZE = 1;                         // Minimum size value
constexpr int MAX_SIZE = 250;                       // Maximum size value
constexpr int FAR_CLIP_PLANE = 1000;                // Placeholder for EQ Far Clip Plane value
constexpr float OTHER_SIZE = 1.0f;                  // Default size for other entities
constexpr float ZERO_SIZE = 0.0f;                   // Size representing zero

// Variables
std::chrono::steady_clock::time_point lastUpdate;   // Skip pulse counter
int previousRangeDistance = 0;                      // Previous range distance
bool loadedDannet = false;                          // State of DanNet plugin
bool loadedEQBC = false;                            // State of EQBC plugin

// Function Declarations
static void SpawnListResize(bool bReset);           // Function to resize the spawn list
static void ChooseInstructionPlugin();              // Function to choose the instruction plugin
static void Emulate(std::string_view type);         // Function to emulate certain behavior (zonewide vs range)
static void DrawAutoSize_MQSettingsPanel();         // Function to draw the MQ settings panel
static void SendGroupCommand(std::string_view who); // Function to send a command to a group
static int RoundToNearestTen(int value);            // Function to round a value to the nearest ten
static bool IsInGroup();                            // Function to check if in a group
static bool IsInRaid();                             // Function to check if in a raid
void SaveINI(const std::string& param, const bool squelch); // Function to save INI values to disk

enum class eCommunicationMode
{
	None = 0,
	DanNet = 1,
	EQBC = 2,

	Default = None,
};
eCommunicationMode selectedComms = eCommunicationMode::Default;

enum class eResizeMode
{
	None = 0,
	Zonewide = 1,
	Range = 2,

	Default = Range,
};
eResizeMode ResizeMode = eResizeMode::Default;

// our configuration
struct COurSizes
{
	bool Enabled = true;
	bool OptAutoSave = false;
	bool OptPC = true;
	bool OptNPC = false;
	bool OptPet = false;
	bool OptMerc = false;
	bool OptMount = false;
	bool OptCorpse = false;
	bool OptSelf = false;
	int ResizeRange = 50;
	int SizePC = 1;
	int SizeNPC = 1;
	int SizePet = 1;
	int SizeMerc = 1;
	int SizeMount = 1;
	int SizeCorpse = 1;
	int SizeSelf = 1;
};
COurSizes AS_Config;

// exposed TLO variables
class MQ2AutoSizeType : public MQ2Type
{
public:
	enum class AutoSizeMembers
	{
		Enabled,
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

	MQ2AutoSizeType() : MQ2Type("AutoSize")
	{
		ScopedTypeMember(AutoSizeMembers, Enabled);
		ScopedTypeMember(AutoSizeMembers, Active);
		ScopedTypeMember(AutoSizeMembers, AutoSave);
		ScopedTypeMember(AutoSizeMembers, ResizePC);
		ScopedTypeMember(AutoSizeMembers, ResizeNPC);
		ScopedTypeMember(AutoSizeMembers, ResizePets);
		ScopedTypeMember(AutoSizeMembers, ResizeMercs);
		ScopedTypeMember(AutoSizeMembers, ResizeMounts);
		ScopedTypeMember(AutoSizeMembers, ResizeCorpse);
		ScopedTypeMember(AutoSizeMembers, ResizeSelf);
		ScopedTypeMember(AutoSizeMembers, Range);
		ScopedTypeMember(AutoSizeMembers, SizePC);
		ScopedTypeMember(AutoSizeMembers, SizeNPC);
		ScopedTypeMember(AutoSizeMembers, SizePets);
		ScopedTypeMember(AutoSizeMembers, SizeMercs);
		ScopedTypeMember(AutoSizeMembers, SizeMounts);
		ScopedTypeMember(AutoSizeMembers, SizeCorpse);
		ScopedTypeMember(AutoSizeMembers, SizeSelf);
	}

	bool GetMember(MQVarPtr VarPtr, const char* Member, char* Index, MQTypeVar& Dest) override
	{

		MQTypeMember* pMember = MQ2AutoSizeType::FindMember(Member);
		if (!pMember)
			return false;

		switch (static_cast<AutoSizeMembers>(pMember->ID))
		{
		case AutoSizeMembers::Enabled:
			Dest.Int = AS_Config.Enabled;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::Active:
			Dest.Int = AS_Config.OptPC || AS_Config.OptNPC || AS_Config.OptPet || AS_Config.OptMerc || AS_Config.OptMount || AS_Config.OptCorpse || AS_Config.OptSelf;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::AutoSave:
			Dest.Int = AS_Config.OptAutoSave;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizePC:
			Dest.Int = AS_Config.OptPC;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizeNPC:
			Dest.Int = AS_Config.OptNPC;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizePets:
			Dest.Int = AS_Config.OptPet;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizeMercs:
			Dest.Int = AS_Config.OptMerc;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizeMounts:
			Dest.Int = AS_Config.OptMount;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizeCorpse:
			Dest.Int = AS_Config.OptCorpse;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::ResizeSelf:
			Dest.Int = AS_Config.OptSelf;
			Dest.Type = datatypes::pBoolType;
			return true;
		case AutoSizeMembers::Range:
			Dest.Int = AS_Config.ResizeRange;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizePC:
			Dest.Int = AS_Config.SizePC;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizeNPC:
			Dest.Int = AS_Config.SizeNPC;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizePets:
			Dest.Int = AS_Config.SizePet;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizeMercs:
			Dest.Int = AS_Config.SizeMerc;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizeMounts:
			Dest.Int = AS_Config.SizeMount;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizeCorpse:
			Dest.Int = AS_Config.SizeCorpse;
			Dest.Type = datatypes::pIntType;
			return true;
		case AutoSizeMembers::SizeSelf:
			Dest.Int = AS_Config.SizeSelf;
			Dest.Type = datatypes::pIntType;
			return true;
		}
		return false;
	}
};
MQ2AutoSizeType* pAutoSizeType = nullptr;

bool dataAutoSize(const char*, MQTypeVar& ret)
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
	}
};

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
static int getSaneSize(const char* section, const char* key, int defaultValue)
{
	int size = GetPrivateProfileInt(section, key, defaultValue, INIFileName);

	// special handling for Zonewide
	if (size == FAR_CLIP_PLANE)
	{
		return size;
	}

	return std::clamp(size, MIN_SIZE, MAX_SIZE);
}

void LoadINI()
{
	AS_Config.OptAutoSave = GetPrivateProfileBool("Config", "AutoSave", true, INIFileName);
	AS_Config.OptPC = GetPrivateProfileBool("Config", "ResizePC", false, INIFileName);
	AS_Config.OptNPC = GetPrivateProfileBool("Config", "ResizeNPC", false, INIFileName);
	AS_Config.OptPet = GetPrivateProfileBool("Config", "ResizePets", false, INIFileName);
	AS_Config.OptMerc = GetPrivateProfileBool("Config", "ResizeMercs", false, INIFileName);
	AS_Config.OptMount = GetPrivateProfileBool("Config", "ResizeMounts", false, INIFileName);
	AS_Config.OptCorpse = GetPrivateProfileBool("Config", "ResizeCorpse", false, INIFileName);
	AS_Config.OptSelf = GetPrivateProfileBool("Config", "ResizeSelf", false, INIFileName);
	AS_Config.ResizeRange = getSaneSize("Config", "Range", AS_Config.ResizeRange);
	AS_Config.SizePC = getSaneSize("Config", "SizePC", MIN_SIZE);
	AS_Config.SizeNPC = getSaneSize("Config", "SizeNPC", MIN_SIZE);
	AS_Config.SizePet = getSaneSize("Config", "SizePets", MIN_SIZE);
	AS_Config.SizeMerc = getSaneSize("Config", "SizeMercs", MIN_SIZE);
	AS_Config.SizeMount = getSaneSize("Config", "SizeMounts", MIN_SIZE);
	AS_Config.SizeCorpse = getSaneSize("Config", "SizeCorpse", MIN_SIZE);
	AS_Config.SizeSelf = getSaneSize("Config", "SizeSelf", MIN_SIZE);
	WriteChatf("\ay%s\aw:: Configuration file loaded.", mqplugin::PluginName);

	// special handling for Zonewide vs Range
	if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
	{
		ResizeMode = eResizeMode::Zonewide;
	}
	else
	{
		ResizeMode = eResizeMode::Range;
	}

	// apply new settings from INI read
	if (GetGameState() == GAMESTATE_INGAME && pLocalPlayer)
	{
		SpawnListResize(false);
	}
}

// provide an INI key parameter to save only that key, or leave it null to save all keys
// enable squelch to suppress output to the client
void SaveINI(const std::string& param = "", const bool squelch = false)
{
	// Map to store configuration key-value pairs
	std::vector<std::pair<std::string, std::string>> configMap =
	{
		{"AutoSave", AS_Config.OptAutoSave ? "on" : "off"},
		{"ResizePC", AS_Config.OptPC ? "on" : "off"},
		{"ResizeNPC", AS_Config.OptNPC ? "on" : "off"},
		{"ResizePets", AS_Config.OptPet ? "on" : "off"},
		{"ResizeMercs", AS_Config.OptMerc ? "on" : "off"},
		{"ResizeMounts", AS_Config.OptMount ? "on" : "off"},
		{"ResizeCorpse", AS_Config.OptCorpse ? "on" : "off"},
		{"ResizeSelf", AS_Config.OptSelf ? "on" : "off"},
		{"Range", std::to_string(AS_Config.ResizeRange)},
		{"SizePC", std::to_string(AS_Config.SizePC)},
		{"SizeNPC", std::to_string(AS_Config.SizeNPC)},
		{"SizePets", std::to_string(AS_Config.SizePet)},
		{"SizeMercs", std::to_string(AS_Config.SizeMerc)},
		{"SizeMounts", std::to_string(AS_Config.SizeMount)},
		{"SizeCorpse", std::to_string(AS_Config.SizeCorpse)},
		{"SizeSelf", std::to_string(AS_Config.SizeSelf)}
	};

	// this writes a specific key to disk with its value
	if (!param.empty())
	{
		auto it = std::find_if(configMap.begin(), configMap.end(), [&](const auto& p) { return p.first == param; });
		if (it != configMap.end())
		{
			WritePrivateProfileString("Config", it->first, it->second, INIFileName);
		}
	}
	else
	{
		// this writes all keys and their values to disk as normal
		for (const auto& [key, value] : configMap)
		{
			WritePrivateProfileString("Config", key, value, INIFileName);
		}
	}

	// only display info if squelch is false
	if (!squelch)
	{
		WriteChatf("\ay%s\aw:: Configuration file saved.", mqplugin::PluginName);
	}
}

void ChangeSize(PlayerClient* pChangeSpawn, float fNewSize)
{
	if (pChangeSpawn)
	{
		reinterpret_cast<PlayerZoneClient_Hook*>(pChangeSpawn)->ChangeHeight_Wrapper(fNewSize);
	}
}


void HandleResize(PlayerClient* pSpawn, bool bReset, int size) {
	// only resize if functionality is enabled
	if (AS_Config.Enabled) {
		ChangeSize(pSpawn, bReset ? ZERO_SIZE : size);
	}
	else {
		ChangeSize(pSpawn, ZERO_SIZE);
	}
}

void SizePasser(PlayerClient* pSpawn, bool bReset)
{
	if (GetGameState() != GAMESTATE_INGAME)
	{
		return;
	}

	if (!pLocalPlayer || !pSpawn) return;

	if (pSpawn->SpawnID == pLocalPlayer->SpawnID)
	{
		if (AS_Config.OptSelf)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizeSelf);
		}
			
		return;
	}

	switch (GetSpawnType(pSpawn))
	{
	case PC:
		if (AS_Config.OptPC)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizePC);
		}
		break;
	case NPC:
		if (AS_Config.OptNPC)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizeNPC);	
		}
		break;
	case PET:
		if (AS_Config.OptPet)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizePet);
		}
		break;
	case MERCENARY:
		if (AS_Config.OptMerc)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizeMerc);
		}
		break;
	case MOUNT:
		if (AS_Config.OptMount && pSpawn->SpawnID != pLocalPlayer->SpawnID)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizeMount);
		}
		break;
	case CORPSE:
		if (AS_Config.OptCorpse)
		{
			HandleResize(pSpawn, bReset, AS_Config.SizeCorpse);
		}
		break;
	default:
		break;
	}
}

void ResetAllByType(eSpawnType OurType)
{
	PlayerClient* pSpawn = pSpawnList;
	if (GetGameState() != GAMESTATE_INGAME || !pLocalPlayer || !pSpawn)
	{
		return;
	}

	while (pSpawn)
	{
		if (pSpawn->SpawnID != pLocalPlayer->SpawnID)
		{
			eSpawnType ListType = GetSpawnType(pSpawn);
			if (ListType == OurType)
				ChangeSize(pSpawn, ZERO_SIZE);
		}
		pSpawn = pSpawn->GetNext();
	}
}

void SpawnListResize(bool bReset)
{
	if (GetGameState() != GAMESTATE_INGAME)
		return;

	PlayerClient* pAllSpawns = pSpawnList;
	while (pAllSpawns)
	{
		SizePasser(pAllSpawns, bReset);
		pAllSpawns = pAllSpawns->GetNext();
	}
}

PLUGIN_API void OnEndZone()
{
	SpawnListResize(false);
}

PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME)
	{
		return;
	}

	if (std::chrono::steady_clock::now() < lastUpdate + UPDATE_INTERVAL)
	{
		return;
	}
	lastUpdate = std::chrono::steady_clock::now();

	PlayerClient* pAllSpawns = pSpawnList;
	while (pAllSpawns)
	{
		float fDist = GetDistance(pLocalPlayer, pAllSpawns);
		if (fDist < AS_Config.ResizeRange)
		{
			SizePasser(pAllSpawns, false);
		}
		else if (fDist < AS_Config.ResizeRange + 50)
		{
			SizePasser(pAllSpawns, true);
		}

		pAllSpawns = pAllSpawns->GetNext();
	}
}

void OutputHelp()
{
	WriteChatf("\ay%s\aw:: Command Usage Help", mqplugin::PluginName);
	WriteChatf("  \ag/autosize\ax - Toggles AutoSize functionality on/off");
	WriteChatf("  \ag/autosize\ax \aydist\ax - Toggles distance-based vs Zonewide");
	WriteChatf("  \ag/autosize\ax \ayrange #\ax - Sets range for distance checking");
	WriteChatf("--- Valid Resize Toggles ---");
	WriteChatf("  \ag/autosize\ax [ \aypc\ax | \aynpc\ax | \aypets\ax | \aymercs\ax | \aymounts\ax | \aycorpse\ax | \ayeverything\ax | \ayself\ax ]");
	WriteChatf("--- Valid Size Syntax (%d to %d) ---", MIN_SIZE, MAX_SIZE);
	WriteChatf("  \ag/autosize\ax [ \aysizepc\ax | \aysizenpc\ax | \aysizepets\ax | \aysizemercs\ax | \aysizemounts\ax | \aysizecorpse\ax | \aysizeself\ax ] [ \ay#\ax ]");
	WriteChatf("--- Other Valid Commands ---");
	WriteChatf("  \ag/autosize\ax [ \ayhelp\ax | \aygui\ax | \ayui\ax | \ayshow\ax | \aystatus\ax | \ayautosave\ax | \aysave\ax | \ayload\ax ]");
	WriteChatf("--- Ability to set options ---");
	WriteChatf("  \ag/autosize\ax [ \ayautosize\ax | \aypc\ax | \aynpc\ax | \aypets\ax | \aymercs\ax | \aymounts\ax | \aycorpse\ax | \ayeverything\ax | \ayself\ax ] [\agon\ax | \aroff\ax]");
}

void OutputStatus()
{
	char szMethod[100] = { 0 };
	char szOn[10] = "\agon\ax";
	char szOff[10] = "\aroff\ax";

	if (!AS_Config.Enabled || !AS_Config.OptPC && !AS_Config.OptNPC && !AS_Config.OptPet && !AS_Config.OptMerc && !AS_Config.OptCorpse && !AS_Config.OptSelf)
	{
		strcpy_s(szMethod, "\arInactive\ax");
	}
	else if (AS_Config.ResizeRange < FAR_CLIP_PLANE)
	{
		sprintf_s(szMethod, "\ayRange\ax) RangeSize(\ag%d\ax", AS_Config.ResizeRange);
	}
	else if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
	{
		// covers the idea of Zonewide by using FAR_CLIP_PLANE value (which is about 100% Far Cliping Plane)
		strcpy_s(szMethod, "\ayZonewide\ax");
	}

	WriteChatf("\ay%s\aw:: Current Status -- Method: (%s)%s", mqplugin::PluginName, szMethod, AS_Config.OptAutoSave ? " \agAUTOSAVING" : "");
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
		szOff // no longer available but left to ensure that no random script that reads this, breaks.
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

bool ToggleOption(const char* pszToggleOutput, bool* pbOption)
{
	*pbOption = !*pbOption;
	WriteChatf("\ay%s\aw:: Option (\ay%s\ax) now %s\ax", mqplugin::PluginName, pszToggleOutput, *pbOption ? "\agenabled" : "\ardisabled");

	if (AS_Config.OptAutoSave)
		SaveINI();

	return *pbOption;
}

void SetOption(const char* optString, bool* pbOption, bool bValue)
{
	*pbOption = bValue;
	WriteChatf("\ay%s\aw:: Option (\ay%s\ax) now %s\ax", mqplugin::PluginName, optString, *pbOption ? "\agenabled" : "\ardisabled");

	if (AS_Config.OptAutoSave)
		SaveINI();
}

void SetSizeConfig(const char* pszOption, int iNewSize, int* iOldSize)
{
	// make sure that we are setting values for a valid reason
	if ((iNewSize != *iOldSize) && ((iNewSize >= MIN_SIZE && iNewSize <= MAX_SIZE) || (iNewSize == FAR_CLIP_PLANE)))
	{
		int iPrevSize = *iOldSize;
		// set the pointer to the new value
		*iOldSize = iNewSize;

		if (ci_equals(pszOption, "range"))
		{
			WriteChatf("\ay%s\aw:: Range size is \agZonewide\ax (%d units)", mqplugin::PluginName, FAR_CLIP_PLANE);
		}
		else {
			WriteChatf("\ay%s\aw:: %s size changed from \ay%d\ax to \ag%d", mqplugin::PluginName, pszOption, iPrevSize, *iOldSize);
		}
		
	}
	else
	{
		WriteChatf("\ay%s\aw:: %s size is \ag%d\ax (was not modified)", mqplugin::PluginName, pszOption, *iOldSize);
	}

	if (AS_Config.OptAutoSave)
		SaveINI();
}

void AutoSizeCmd(PlayerClient*, const char* szLine)
{
	char szCurArg[MAX_STRING] = { 0 };
	char szNumber[MAX_STRING] = { 0 };
	GetArg(szCurArg, szLine, 1);
	GetArg(szNumber, szLine, 2);
	int iNewSize = GetIntFromString(szNumber, MIN_SIZE);

	if (!szCurArg[0])
	{
		// toggle AutoSize functionality via configuration
		ToggleOption("AutoSize functionality", &AS_Config.Enabled);
	}
	else if (ci_equals(szCurArg, "gui") || ci_equals(szCurArg, "ui"))
	{
		DoCommand("/mqsettings plugins/autosize");
	}
	else if (ci_equals(szCurArg, "on"))
	{
		SetOption("AutoSize functionality", &AS_Config.Enabled, true);
	}
	else if (ci_equals(szCurArg, "off"))
	{
		SetOption("AutoSize functionality", &AS_Config.Enabled, false);
	}
	else if (ci_equals(szCurArg, "dist"))
	{
		if (ci_equals(szNumber, "on"))
		{
			if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
			{
				// this means we are currently set to Zonewide and need to set to Range instead
				// we want look like we are toggling to Range from Zonewide setting
				// we will do this by reseting the ResizeRange to what it was prior
				Emulate("range");
			}
		}
		else if (ci_equals(szNumber, "off"))
		{
			if (AS_Config.ResizeRange != FAR_CLIP_PLANE)
			{
				// this means we are currently set to Range distance and want to be Zonewide
				// we want look like we are toggling to Zonewide from Range setting
				// we will do this by increasing the ResizeRange to FAR_CLIP_PLANE
				Emulate("zonewide");
			}
		}
		else
		{
			if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
			{
				Emulate("range");
			}
			else if (AS_Config.ResizeRange != FAR_CLIP_PLANE)
			{
				Emulate("zonewide");
			}
		}
	}
	else if (ci_equals(szCurArg, "save"))
	{
		SaveINI();
	}
	else if (ci_equals(szCurArg, "load"))
	{
		LoadINI();
	}
	else if (ci_equals(szCurArg, "autosave"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Autosave", &AS_Config.OptAutoSave, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Autosave", &AS_Config.OptAutoSave, false);
		}
		else
		{
			ToggleOption("Autosave", &AS_Config.OptAutoSave);
		}
	}
	else if (ci_equals(szCurArg, "range"))
	{
		SetSizeConfig("range", iNewSize, &AS_Config.ResizeRange);
	}
	else if (ci_equals(szCurArg, "size"))
	{
		WriteChatf("\ay%s\aw:: This feature (\ay%s\ax) has been deprecated. Check /mqsetting -> plugins -> AutoSize.", mqplugin::PluginName, szCurArg);
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
		if (ci_equals(szNumber, "on"))
		{
			SetOption("PC", &AS_Config.OptPC, true);

		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("PC", &AS_Config.OptPC, false);
			ResetAllByType(PC);
		}
		else
		{
			if (!ToggleOption("PC", &AS_Config.OptPC))
			{
				ResetAllByType(PC);
			}
		}
	}
	else if (ci_equals(szCurArg, "npc"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("NPC", &AS_Config.OptNPC, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("NPC", &AS_Config.OptNPC, false);
			ResetAllByType(NPC);
		}
		else
		{
			if (!ToggleOption("NPC", &AS_Config.OptNPC))
			{
				ResetAllByType(NPC);
			}
		}
	}
	else if (ci_equals(szCurArg, "everything"))
	{
		if (!AS_Config.OptSelf)
		{
			DoCommand("/squelch /autosize self on");
		}
		if (!AS_Config.OptCorpse)
		{
			DoCommand("/squelch /autosize corpse on");
		}
		if (!AS_Config.OptMerc)
		{
			DoCommand("/squelch /autosize mercs on");
		}
		if (!AS_Config.OptMount)
		{
			DoCommand("/squelch /autosize mounts on");
		}
		if (!AS_Config.OptNPC)
		{
			DoCommand("/squelch /autosize npc on");
		}
		if (!AS_Config.OptPC)
		{
			DoCommand("/squelch /autosize pc on");
		}
		if (!AS_Config.OptPet)
		{
			DoCommand("/squelch /autosize pets on");
		}
	}
	else if (ci_equals(szCurArg, "pets"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Pets", &AS_Config.OptPet, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Pets", &AS_Config.OptPet, false);
			ResetAllByType(PET);
		}
		else
		{
			if (!ToggleOption("Pets", &AS_Config.OptPet))
			{
				ResetAllByType(PET);
			}
		}
	}
	else if (ci_equals(szCurArg, "mercs"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Mercs", &AS_Config.OptMerc, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Mercs", &AS_Config.OptMerc, false);
			ResetAllByType(MERCENARY);
		}
		else
		{
			if (!ToggleOption("Mercs", &AS_Config.OptMerc))
			{
				ResetAllByType(MERCENARY);
			}
		}
	}
	else if (ci_equals(szCurArg, "mounts"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Mounts", &AS_Config.OptMount, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Mounts", &AS_Config.OptMount, false);
			ResetAllByType(MOUNT);
		}
		else
		{
			if (!ToggleOption("Mounts", &AS_Config.OptMount))
			{
				ResetAllByType(MOUNT);
			}
		}
	}
	else if (ci_equals(szCurArg, "corpse"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Corpses", &AS_Config.OptCorpse, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Corpses", &AS_Config.OptCorpse, false);
			ResetAllByType(CORPSE);
		}
		else
		{
			if (!ToggleOption("Corpses", &AS_Config.OptCorpse))
			{
				ResetAllByType(CORPSE);
			}
		}
	}
	else if (ci_equals(szCurArg, "target"))
	{
		// deprecated because when you use this while having features enabled this feature didn't actually work.
		// if you don't have anything resizing, why would you want to resize just the target?
		WriteChatf("\ay%s\aw:: This feature (\ay%s\ax) has been deprecated. Check /mqsetting -> plugins -> AutoSize.", mqplugin::PluginName, szCurArg);
	}
	else if (ci_equals(szCurArg, "self"))
	{
		if (ci_equals(szNumber, "on"))
		{
			SetOption("Self", &AS_Config.OptSelf, true);
		}
		else if (ci_equals(szNumber, "off"))
		{
			SetOption("Self", &AS_Config.OptSelf, false);
			ChangeSize(pLocalPlayer, ZERO_SIZE);
		}
		else
		{
			if (!ToggleOption("Self", &AS_Config.OptSelf))
			{
				ChangeSize(pLocalPlayer, ZERO_SIZE);
			}
		}
	}
	else if (ci_equals(szCurArg, "help"))
	{
		OutputHelp();
	}
	else if (ci_equals(szCurArg, "status"))
	{
		OutputStatus();
	}
	else
	{
		WriteChatf("\ay%s\aw:: \arInvalid command parameter.", mqplugin::PluginName);
	}
}

PLUGIN_API void InitializePlugin()
{
	EzDetour(PlayerZoneClient__ChangeHeight, &PlayerZoneClient_Hook::ChangeHeight_Detour, &PlayerZoneClient_Hook::ChangeHeight_Trampoline);
	pAutoSizeType = new MQ2AutoSizeType;
	AddMQ2Data("AutoSize", dataAutoSize);
	AddCommand("/autosize", AutoSizeCmd);
	AddSettingsPanel("plugins/AutoSize", DrawAutoSize_MQSettingsPanel);
	LoadINI();

	loadedEQBC = IsPluginLoaded("MQ2EQBC");
	loadedDannet = IsPluginLoaded("MQ2DanNet");
	ChooseInstructionPlugin();
}

PLUGIN_API void ShutdownPlugin()
{
	RemoveDetour(PlayerZoneClient__ChangeHeight);
	RemoveSettingsPanel("plugins/AutoSize");
	RemoveCommand("/autosize");
	SpawnListResize(true);
	if (AS_Config.OptAutoSave)
	{
		SaveINI();
	}
	RemoveMQ2Data("AutoSize");
	delete pAutoSizeType;
}

PLUGIN_API void OnLoadPlugin(const char* pluginName)
{
	DebugSpewAlways("OnLoadPluign: MQ2AutoSize -> %s", pluginName);

	// dannet plugin is loading
	if (ci_equals(pluginName, "MQ2DanNet"))
	{
		loadedDannet = true;
		ChooseInstructionPlugin();
	}

	// eqbc plugin is loading
	if (ci_equals(pluginName, "MQ2EQBC"))
	{
		loadedEQBC = true;
		ChooseInstructionPlugin();
	}
}

PLUGIN_API void OnUnloadPlugin(const char* pluginName)
{
	DebugSpewAlways("OnUnloadPlugin: MQ2AutoSize -> %s", pluginName);

	// dannet plugin is about to unload
	if (ci_equals(pluginName, "MQ2DanNet"))
	{
		loadedDannet = false;
		ChooseInstructionPlugin();
	}

	// eqbc plugin is about to unload
	if (ci_equals(pluginName, "MQ2EQBC"))
	{
		loadedEQBC = false;
		ChooseInstructionPlugin();
	}
}

template <typename T>
bool RadioButton(const char* label, T* value, T defaultValue)
{
	using UnderlyingType = std::underlying_type_t<T>;
	return ImGui::RadioButton(label, reinterpret_cast<UnderlyingType*>(value), static_cast<UnderlyingType>(defaultValue));
}

void DrawAutoSize_MQSettingsPanel()
{
	ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MQ2AutoSize");
	if (ImGui::BeginTabBar("AutoSizeTabBar"))
	{
		if (ImGui::BeginTabItem("Options"))
		{
			ImGui::SeparatorText("General");
			// General: Enable / Disable AutoSize functionality
			ImGui::Checkbox("Enable AutoSize functionality", &AS_Config.Enabled);
			
			// General: auto save
			if (ImGui::Checkbox("Enable auto saving of configuration", &AS_Config.OptAutoSave))
			{
				DoCommandf("/autosize autosave %s", AS_Config.OptAutoSave ? "on" : "off");
			}
			// General: Zonewide
			if (RadioButton("Zonewide (max clipping plane)", &ResizeMode, eResizeMode::Zonewide))
			{
				ResizeMode = eResizeMode::Zonewide;
				Emulate("zonewide");
			}
			// General: Range
			if (ImGui::BeginTable("OptionsResizeRangeTable", 2, ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableNextColumn();
				if (RadioButton("##rangeselector", &ResizeMode, eResizeMode::Range))
				{
					ResizeMode = eResizeMode::Range;
					Emulate("range");
				}
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(ResizeMode == eResizeMode::Zonewide);
				ImGui::SetNextItemWidth(50.0f);
				if (ImGui::SliderInt("Range distance (recommended setting)", &AS_Config.ResizeRange, 10, MAX_SIZE, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					AS_Config.ResizeRange = RoundToNearestTen(AS_Config.ResizeRange);
					if (AS_Config.OptAutoSave)
					{
						SaveINI("Range", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::EndTable();
			}
			// General: Status output
			if (ImGui::Button("Display status output"))
			{
				DoCommand("/autosize status");
			}
			ImGui::SeparatorText("Toggles and Values");
			if (ImGui::BeginTable("OptionsResizeSelfTable", 2, ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 20.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
				// Option: Self
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptSelf", &AS_Config.OptSelf))
				{
					DoCommandf("/autosize self %s", AS_Config.OptSelf ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptSelf);
				if (ImGui::SliderInt("Resize: Self", &AS_Config.SizeSelf, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizeSelf", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Other PCs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptPC", &AS_Config.OptPC))
				{
					DoCommandf("/autosize pc %s", AS_Config.OptPC ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptPC);
				if (ImGui::SliderInt("Resize: Other player(s) (incluldes those mounted)", &AS_Config.SizePC, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizePC", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Pets
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptPet", &AS_Config.OptPet))
				{
					DoCommandf("/autosize pets %s", AS_Config.OptPet ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptPet);
				if (ImGui::SliderInt("Resize: Pets", &AS_Config.SizePet, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizePets", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Mercs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptMerc", &AS_Config.OptMerc))
				{
					DoCommandf("/autosize mercs %s", AS_Config.OptMerc ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptMerc);
				if (ImGui::SliderInt("Resize: Mercs", &AS_Config.SizeMerc, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizeMercs", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Mounts
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptMount", &AS_Config.OptMount))
				{
					DoCommandf("/autosize mounts %s", AS_Config.OptMount ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptMount);
				if (ImGui::SliderInt("Resize: Mounts and the Player(s) on them", &AS_Config.SizeMount, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizeMounts", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: Corpses
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptCorpse", &AS_Config.OptCorpse))
				{
					DoCommandf("/autosize corpse %s", AS_Config.OptCorpse ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptCorpse);
				if (ImGui::SliderInt("Resize: Corpse(s)", &AS_Config.SizeCorpse, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizeCorpse", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::TableNextRow();
				// Option: NPCs
				ImGui::TableNextColumn();
				if (ImGui::Checkbox("##OptNPC", &AS_Config.OptNPC))
				{
					DoCommandf("/autosize npc %s", AS_Config.OptNPC ? "on" : "off");
				}
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(50.0f);
				ImGui::BeginDisabled(!AS_Config.OptNPC);
				if (ImGui::SliderInt("Resize: NPC(s)", &AS_Config.SizeNPC, 1, 30, "%d", ImGuiSliderFlags_NoInput | ImGuiSliderFlags_AlwaysClamp))
				{
					if (AS_Config.OptAutoSave)
					{
						SaveINI("SizeNPC", true);
					}
				}
				ImGui::EndDisabled();
				ImGui::EndTable();
			}

			// display the button if autosave option is disabled
			if (!AS_Config.OptAutoSave)
			{
				if (ImGui::Button("Reload INI"))
				{
					LoadINI();
				}
			}

			// display the button if any option is not enabled
			if (!AS_Config.OptCorpse || !AS_Config.OptMerc || !AS_Config.OptMount || !AS_Config.OptNPC || !AS_Config.OptPC || !AS_Config.OptPet || !AS_Config.OptSelf)
			{
				// use the same line if Reload INI button is visible
				if (!AS_Config.OptAutoSave)
				{
					ImGui::SameLine();
				}

				// this button will enable the options which are disabled
				if (ImGui::Button("Resize Everything (select all)"))
				{
					DoCommand("/autosize everything");
				}
			}

			ImGui::SeparatorText("Synchronize clients");
			ImGui::TextWrapped("This section provides the ability to broadcast your settings to connected peers based on which communication path exists.");
			ImGui::BeginDisabled(loadedDannet || loadedEQBC);
			RadioButton("None", &selectedComms, eCommunicationMode::None);
			if (loadedDannet || loadedEQBC)
			{
				ImGui::SameLine();
				ImGui::Text("(disabled since plugin(s) are available)");
			}
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!loadedDannet);
			RadioButton("MQ2DanNet", &selectedComms, eCommunicationMode::DanNet);
			if (!loadedDannet)
			{
				ImGui::SameLine();
				ImGui::Text("(disabled as plugin is not loaded)");
			}
			ImGui::EndDisabled();

			ImGui::BeginDisabled(!loadedEQBC);
			RadioButton("MQ2EQBC", &selectedComms, eCommunicationMode::EQBC);
			if (!loadedEQBC)
			{
				ImGui::SameLine();
				ImGui::Text("(disabled as plugin is not loaded)");
			}
			ImGui::EndDisabled();

			// dannet
			if (selectedComms == eCommunicationMode::DanNet && loadedDannet)
			{
				if (ImGui::Button("All"))
				{
					SendGroupCommand("all");
				}
				ImGui::SameLine();
				if (ImGui::Button("Zone"))
				{
					SendGroupCommand("zone");
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(!IsInRaid());
				if (ImGui::Button("Raid"))
				{
					SendGroupCommand("raid");
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				ImGui::BeginDisabled(!IsInGroup());
				if (ImGui::Button("Group"))
				{
					SendGroupCommand("group");
				}
				ImGui::EndDisabled();
			}
			else if (selectedComms == eCommunicationMode::EQBC && loadedEQBC)
			{
				// eqbc
				if (ImGui::Button("All"))
				{
					SendGroupCommand("all");
				}
				ImGui::SameLine();
				ImGui::BeginDisabled(!IsInGroup());
				if (ImGui::Button("Group"))
				{
					SendGroupCommand("group");
				}
				ImGui::EndDisabled();
			}
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "NOTE: The sync selection is session-based (temporary) and is not saved as config.");
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Commands"))
		{
			ImGui::SeparatorText("Toggles");
			ImGui::Indent();
			if (ImGui::BeginTable("AutoSizeTogglesTable", 2, ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("");
				ImGui::TableNextColumn(); ImGui::Text("/autosize autosave");
				ImGui::TableNextColumn(); ImGui::Text("Automatically save settings to INI file when an option is toggled or size is set");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize functionality (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize dist");
				ImGui::TableNextColumn(); ImGui::Text("Toggles distance-based vs Zonewide (range 1000)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize pc");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize PC spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize npc");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize NPC spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize pets");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize pet spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize mercs");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize mercenary spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize mounts");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize mounted player spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize corpse");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize corpse spawn types (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize self");
				ImGui::TableNextColumn(); ImGui::Text("Toggles AutoSize for your character (on/off)");
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("/autosize everything");
				ImGui::TableNextColumn(); ImGui::Text("Enables AutoSize for all spawn types");
				ImGui::EndTable();
				ImGui::Unindent();
			}

			ImGui::SeparatorText("Size configuration (valid: 1 to 250)");
			ImGui::Indent();
			if (ImGui::BeginTable("AutoSizeSizeTable", 2, ImGuiTableFlags_RowBg))
			{
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
			if (ImGui::BeginTable("AutoSizeOtherCmdsTable", 2, ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("");
				ImGui::TableNextColumn(); ImGui::Text("/autosize [gui | ui]");
				ImGui::TableNextColumn(); ImGui::Text("Display the ImGui panel under /mqsettings");
				ImGui::TableNextRow();
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
				ImGui::EndTable();
			}
			ImGui::Unindent();

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

// send instruction to selected groups:
// -> DanNet: all, zone, raid, group
// -> EQBC: all, group
void SendGroupCommand(std::string_view who)
{
	if (selectedComms == eCommunicationMode::None)
	{
		WriteChatf("MQ2AutoSize: Cannot execute group command, no group plugin configured.");
		return;
	}

	// The groupCommand is used as a single string which is made up of several
	// concatenated instructions that relate to different options and size
	// values. When enhancing or modifying be sure to remember to add a space
	// to the end of the concatenated string if there are further instructions
	// that will be added.
	std::string groupCommand = "/squelch ";
	std::string instruction;

	// if auto save is enabled
	if (AS_Config.OptAutoSave)
	{
		// check if zonewide is enabled and send instruction
		if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
		{
			instruction += "/multiline ; /autosize load; /autosize dist off";
		}
		else
		{
			// send instruction to load configuration and set Enabled functionality
			instruction += fmt::format("/multiline ; /autosize load; /timed 5 /autosize {}", AS_Config.Enabled ? "on" : "off");
		}
	}
	else
	{
		// if auto save is not enabled, we have to go through every setting
		// and create a command which covers both options and size values 
		// in a single multiline command based on the current settings of
		// the player instructing the synchronization to happen
		instruction = "/multiline ; "; // must start with this as there are several instructions

		// autosave
		instruction += fmt::format("/autosize autosave {}; ", AS_Config.OptAutoSave ? "on" : "off");

		// zonewide and range
		if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
		{
			// this covers the use case of the instructor having "zonewide" enabled
			instruction += "/autosize dist off; ";
		}
		else if (AS_Config.ResizeRange != FAR_CLIP_PLANE)
		{
			// this covers the use case of the instructor having "range" enabled
			instruction += fmt::format("/autosize dist on; /autosize range {}; ", AS_Config.ResizeRange);
		}

		// Enabled value
		instruction += fmt::format("/autosize {}; ", AS_Config.Enabled ? "on" : "off");

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
	if (selectedComms == eCommunicationMode::DanNet)
	{
		if (who == "zone")
			groupCommand += fmt::format("/dgze {}", instruction);
		else if (who == "raid")
			groupCommand += fmt::format("/dgre {}", instruction);
		else if (who == "group")
			groupCommand += fmt::format("/dgge {}", instruction);
		else if (who == "all")
			groupCommand += fmt::format("/dge {}", instruction);
	}
	else if (selectedComms == eCommunicationMode::EQBC)
	{
		if (who == "group")
			groupCommand += fmt::format("/bcg /{}", instruction);
		else if (who == "all")
			groupCommand += fmt::format("/bca /{}", instruction);
	}

	if (!groupCommand.empty())
		DoCommand(groupCommand.c_str());
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
void ChooseInstructionPlugin()
{
	// prefer DanNet unless EQBC is currently selected
	if (loadedEQBC && loadedDannet)
	{
		if (selectedComms != eCommunicationMode::EQBC)
		{
			selectedComms = eCommunicationMode::DanNet;
		}
	}
	else if (loadedDannet)
	{
		selectedComms = eCommunicationMode::DanNet;
	}
	else if (loadedEQBC)
	{
		selectedComms = eCommunicationMode::EQBC;
	}
	else
	{
		selectedComms = eCommunicationMode::None;
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
void Emulate(std::string_view type)
{
	if (type == "zonewide")
	{
		if (AS_Config.ResizeRange != FAR_CLIP_PLANE)
		{
			previousRangeDistance = AS_Config.ResizeRange;
			ResizeMode = eResizeMode::Zonewide;
			AS_Config.ResizeRange = FAR_CLIP_PLANE;
			WriteChatf("\ay%s\aw:: AutoSize (\ayRange\ax) now \ardisabled\ax!", mqplugin::PluginName);
			WriteChatf("\ay%s\aw:: AutoSize (\ayZonewide\ax) now \agenabled\ax!", mqplugin::PluginName);
			SpawnListResize(false);
		}
	}
	else if (type == "range")
	{
		if (AS_Config.ResizeRange == FAR_CLIP_PLANE)
		{
			// special handling if Zonewide was pre-selected on plugin load
			if (previousRangeDistance == 0)
			{
				AS_Config.ResizeRange = MAX_SIZE;
			}
			else {
				AS_Config.ResizeRange = previousRangeDistance;
			}
			
			ResizeMode = eResizeMode::Range;
			WriteChatf("\ay%s\aw:: AutoSize (\ayZonewide\ax) now \ardisabled\ax!", mqplugin::PluginName);
			WriteChatf("\ay%s\aw:: AutoSize (\ayRange\ax) now \agenabled\ax!", mqplugin::PluginName);
			SpawnListResize(false);
		}
	}

	if (AS_Config.OptAutoSave)
	{
		SaveINI("Range", true);
	}
}

// This is used in the imgui panel to jump to the next 10th place to align with
// the experience of adjusting the EQ Clipping Plane. Each % of clipping plane
// is about 10 units within game.
int RoundToNearestTen(int value)
{
	// clamp lower value
	if (value < 10)
	{
		return 10;
	}

	if (value > MAX_SIZE)
	{
		return MAX_SIZE;
	}

	int roundedValue = (value + 9) / 10 * 10;

	// clamp upper value post rounding
	if (roundedValue > MAX_SIZE)
	{
		return MAX_SIZE;
	}
	
	return roundedValue;
}

// check if we are in a group
static bool IsInGroup()
{
	return pLocalPC && pLocalPC->Group;
}

// check if we are in a raid
static bool IsInRaid()
{
	return pRaid && pRaid->RaidMemberCount;
}
