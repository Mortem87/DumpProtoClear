#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "../dump_proto/tea.h"
#include "../lzo/lzoconf.h"
#include "../lzo/lzo1x.h"

#pragma comment(lib, "lzo.lib")

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef signed long LONG;

#pragma pack(push, 1)
struct ItemLimit
{
	BYTE type;
	LONG value;
};

struct ItemApply
{
	BYTE type;
	LONG value;
};

struct ClientItemTable
{
	DWORD vnum;
	DWORD vnumRange;
	char name[25];
	char localeName[25];
	BYTE type;
	BYTE subType;
	BYTE weight;
	BYTE size;
	DWORD antiFlags;
	DWORD flags;
	DWORD wearFlags;
	DWORD immuneFlags;
	DWORD gold;
	DWORD shopBuyPrice;
	ItemLimit limits[2];
	ItemApply applies[3];
	LONG values[6];
	LONG sockets[3];
	DWORD refinedVnum;
	WORD refineSet;
	BYTE alterToMagicItemPct;
	BYTE specular;
	BYTE gainSocketPct;
};
#pragma pack(pop)

typedef char ItemTableMustBe156Bytes[(sizeof(ClientItemTable) == 156) ? 1 : -1];

struct ProtoHeader
{
	DWORD fourCC;
	DWORD encryptSize;
	DWORD compressedSize;
	DWORD realSize;
};

static const DWORD kItemProtoKey[4] =
{
	173217,
	72619434,
	408587239,
	27973291
};

static const DWORD kItemProtoFourCC = 0x5850494D; // MIPX
static const DWORD kCompressedFourCC = 0x5A4F434D; // MCOZ

static const char* kItemTypes[] =
{
	"ITEM_NONE", "ITEM_WEAPON", "ITEM_ARMOR", "ITEM_USE", "ITEM_AUTOUSE",
	"ITEM_MATERIAL", "ITEM_SPECIAL", "ITEM_TOOL", "ITEM_LOTTERY", "ITEM_ELK",
	"ITEM_METIN", "ITEM_CONTAINER", "ITEM_FISH", "ITEM_ROD", "ITEM_RESOURCE",
	"ITEM_CAMPFIRE", "ITEM_UNIQUE", "ITEM_SKILLBOOK", "ITEM_QUEST",
	"ITEM_POLYMORPH", "ITEM_TREASURE_BOX", "ITEM_TREASURE_KEY",
	"ITEM_SKILLFORGET", "ITEM_GIFTBOX", "ITEM_PICK", "ITEM_HAIR", "ITEM_TOTEM",
	"ITEM_BLEND", "ITEM_COSTUME", "ITEM_DS", "ITEM_SPECIAL_DS", "ITEM_EXTRACT",
	"ITEM_SECONDARY_COIN", "ITEM_RING", "ITEM_BELT"
};

static const char* kWeaponSubTypes[] =
{
	"WEAPON_SWORD", "WEAPON_DAGGER", "WEAPON_BOW", "WEAPON_TWO_HANDED",
	"WEAPON_BELL", "WEAPON_FAN", "WEAPON_ARROW", "WEAPON_MOUNT_SPEAR",
	"WEAPON_CLAW"
};
static const char* kArmorSubTypes[] =
{
	"ARMOR_BODY", "ARMOR_HEAD", "ARMOR_SHIELD", "ARMOR_WRIST",
	"ARMOR_FOOTS", "ARMOR_NECK", "ARMOR_EAR", "ARMOR_NUM_TYPES"
};
static const char* kUseSubTypes[] =
{
	"USE_POTION", "USE_TALISMAN", "USE_TUNING", "USE_MOVE",
	"USE_TREASURE_BOX", "USE_MONEYBAG", "USE_BAIT", "USE_ABILITY_UP",
	"USE_AFFECT", "USE_CREATE_STONE", "USE_SPECIAL", "USE_POTION_NODELAY",
	"USE_CLEAR", "USE_INVISIBILITY", "USE_DETACHMENT", "USE_BUCKET",
	"USE_POTION_CONTINUE", "USE_CLEAN_SOCKET", "USE_CHANGE_ATTRIBUTE",
	"USE_ADD_ATTRIBUTE", "USE_ADD_ACCESSORY_SOCKET",
	"USE_PUT_INTO_ACCESSORY_SOCKET", "USE_ADD_ATTRIBUTE2", "USE_RECIPE",
	"USE_CHANGE_ATTRIBUTE2", "USE_BIND", "USE_UNBIND", "USE_TIME_CHARGE_PER",
	"USE_TIME_CHARGE_FIX", "USE_PUT_INTO_BELT_SOCKET",
	"USE_PUT_INTO_RING_SOCKET"
};
static const char* kAutoUseSubTypes[] =
{
	"AUTOUSE_POTION", "AUTOUSE_ABILITY_UP", "AUTOUSE_BOMB", "AUTOUSE_GOLD",
	"AUTOUSE_MONEYBAG", "AUTOUSE_TREASURE_BOX"
};
static const char* kMaterialSubTypes[] =
{
	"MATERIAL_LEATHER", "MATERIAL_BLOOD", "MATERIAL_ROOT", "MATERIAL_NEEDLE",
	"MATERIAL_JEWEL", "MATERIAL_DS_REFINE_NORMAL",
	"MATERIAL_DS_REFINE_BLESSED", "MATERIAL_DS_REFINE_HOLLY"
};
static const char* kSpecialSubTypes[] =
{
	"SPECIAL_MAP", "SPECIAL_KEY", "SPECIAL_DOC", "SPECIAL_SPIRIT"
};
static const char* kToolSubTypes[] = { "TOOL_FISHING_ROD" };
static const char* kLotterySubTypes[] = { "LOTTERY_TICKET", "LOTTERY_INSTANT" };
static const char* kMetinSubTypes[] = { "METIN_NORMAL", "METIN_GOLD" };
static const char* kFishSubTypes[] = { "FISH_ALIVE", "FISH_DEAD" };
static const char* kResourceSubTypes[] =
{
	"RESOURCE_FISHBONE", "RESOURCE_WATERSTONEPIECE", "RESOURCE_WATERSTONE",
	"RESOURCE_BLOOD_PEARL", "RESOURCE_BLUE_PEARL", "RESOURCE_WHITE_PEARL",
	"RESOURCE_BUCKET", "RESOURCE_CRYSTAL", "RESOURCE_GEM", "RESOURCE_STONE",
	"RESOURCE_METIN", "RESOURCE_ORE"
};
static const char* kUniqueSubTypes[] =
{
	"UNIQUE_NONE", "UNIQUE_BOOK", "UNIQUE_SPECIAL_RIDE", "UNIQUE_3",
	"UNIQUE_4", "UNIQUE_5", "UNIQUE_6", "UNIQUE_7", "UNIQUE_8",
	"UNIQUE_9", "USE_SPECIAL"
};
static const char* kCostumeSubTypes[] = { "COSTUME_BODY", "COSTUME_HAIR" };
static const char* kDragonSoulSubTypes[] =
{
	"DS_SLOT1", "DS_SLOT2", "DS_SLOT3", "DS_SLOT4", "DS_SLOT5", "DS_SLOT6"
};
static const char* kExtractSubTypes[] =
{
	"EXTRACT_DRAGON_SOUL", "EXTRACT_DRAGON_HEART"
};

struct EnumNames
{
	const char* const* names;
	size_t count;
};

static EnumNames GetSubTypes(BYTE type)
{
#define ENUM_NAMES(array) { array, sizeof(array) / sizeof((array)[0]) }
	switch (type)
	{
	case 1: { EnumNames result = ENUM_NAMES(kWeaponSubTypes); return result; }
	case 2: { EnumNames result = ENUM_NAMES(kArmorSubTypes); return result; }
	case 3: { EnumNames result = ENUM_NAMES(kUseSubTypes); return result; }
	case 4: { EnumNames result = ENUM_NAMES(kAutoUseSubTypes); return result; }
	case 5: { EnumNames result = ENUM_NAMES(kMaterialSubTypes); return result; }
	case 6: { EnumNames result = ENUM_NAMES(kSpecialSubTypes); return result; }
	case 7: { EnumNames result = ENUM_NAMES(kToolSubTypes); return result; }
	case 8: { EnumNames result = ENUM_NAMES(kLotterySubTypes); return result; }
	case 10: { EnumNames result = ENUM_NAMES(kMetinSubTypes); return result; }
	case 12: { EnumNames result = ENUM_NAMES(kFishSubTypes); return result; }
	case 14: { EnumNames result = ENUM_NAMES(kResourceSubTypes); return result; }
	case 16: { EnumNames result = ENUM_NAMES(kUniqueSubTypes); return result; }
	case 28: { EnumNames result = ENUM_NAMES(kCostumeSubTypes); return result; }
	case 29:
	case 30: { EnumNames result = ENUM_NAMES(kDragonSoulSubTypes); return result; }
	case 31: { EnumNames result = ENUM_NAMES(kExtractSubTypes); return result; }
	default:
		{
			EnumNames empty = { NULL, 0 };
			return empty;
		}
	}
#undef ENUM_NAMES
}

static const char* kAntiFlagNames[] =
{
	"ANTI_FEMALE", "ANTI_MALE", "ANTI_MUSA", "ANTI_ASSASSIN", "ANTI_SURA",
	"ANTI_MUDANG", "ANTI_GET", "ANTI_DROP", "ANTI_SELL", "ANTI_EMPIRE_A",
	"ANTI_EMPIRE_B", "ANTI_EMPIRE_C", "ANTI_SAVE", "ANTI_GIVE", "ANTI_PKDROP",
	"ANTI_STACK", "ANTI_MYSHOP", "ANTI_SAFEBOX", "ANTI_WOLFMAN"
};
static const char* kFlagNames[] =
{
	"ITEM_TUNABLE", "ITEM_SAVE", "ITEM_STACKABLE", "COUNT_PER_1GOLD",
	"ITEM_SLOW_QUERY", "ITEM_UNIQUE", "ITEM_MAKECOUNT", "ITEM_IRREMOVABLE",
	"CONFIRM_WHEN_USE", "QUEST_USE", "QUEST_USE_MULTIPLE", "QUEST_GIVE",
	"ITEM_QUEST", "LOG", "STACKABLE", "SLOW_QUERY", "REFINEABLE",
	"IRREMOVABLE", "ITEM_APPLICABLE"
};
static const char* kWearFlagNames[] =
{
	"WEAR_BODY", "WEAR_HEAD", "WEAR_FOOTS", "WEAR_WRIST", "WEAR_WEAPON",
	"WEAR_NECK", "WEAR_EAR", "WEAR_SHIELD", "WEAR_UNIQUE", "WEAR_ARROW",
	"WEAR_HAIR", "WEAR_ABILITY", "WEAR_COSTUME_BODY", "WEAR_COSTUME_HAIR",
	"WEAR_COSTUME_ACCE", "WEAR_COSTUME_MOUNT"
};
static const char* kImmuneFlagNames[] =
{
	"PARA", "CURSE", "STUN", "SLEEP", "SLOW", "POISON", "TERROR"
};
static const char* kLimitNames[] =
{
	"LIMIT_NONE", "LEVEL", "STR", "DEX", "INT", "CON", "PC_BANG",
	"REAL_TIME", "REAL_TIME_FIRST_USE", "TIMER_BASED_ON_WEAR"
};
static const char* kApplyNames[] =
{
	"APPLY_NONE", "APPLY_MAX_HP", "APPLY_MAX_SP", "APPLY_CON", "APPLY_INT",
	"APPLY_STR", "APPLY_DEX", "APPLY_ATT_SPEED", "APPLY_MOV_SPEED",
	"APPLY_CAST_SPEED", "APPLY_HP_REGEN", "APPLY_SP_REGEN", "APPLY_POISON_PCT",
	"APPLY_STUN_PCT", "APPLY_SLOW_PCT", "APPLY_CRITICAL_PCT",
	"APPLY_PENETRATE_PCT", "APPLY_ATTBONUS_HUMAN", "APPLY_ATTBONUS_ANIMAL",
	"APPLY_ATTBONUS_ORC", "APPLY_ATTBONUS_MILGYO", "APPLY_ATTBONUS_UNDEAD",
	"APPLY_ATTBONUS_DEVIL", "APPLY_STEAL_HP", "APPLY_STEAL_SP",
	"APPLY_MANA_BURN_PCT", "APPLY_DAMAGE_SP_RECOVER", "APPLY_BLOCK",
	"APPLY_DODGE", "APPLY_RESIST_SWORD", "APPLY_RESIST_TWOHAND",
	"APPLY_RESIST_DAGGER", "APPLY_RESIST_BELL", "APPLY_RESIST_FAN",
	"APPLY_RESIST_BOW", "APPLY_RESIST_FIRE", "APPLY_RESIST_ELEC",
	"APPLY_RESIST_MAGIC", "APPLY_RESIST_WIND", "APPLY_REFLECT_MELEE",
	"APPLY_REFLECT_CURSE", "APPLY_POISON_REDUCE", "APPLY_KILL_SP_RECOVER",
	"APPLY_EXP_DOUBLE_BONUS", "APPLY_GOLD_DOUBLE_BONUS",
	"APPLY_ITEM_DROP_BONUS", "APPLY_POTION_BONUS", "APPLY_KILL_HP_RECOVER",
	"APPLY_IMMUNE_STUN", "APPLY_IMMUNE_SLOW", "APPLY_IMMUNE_FALL",
	"APPLY_SKILL", "APPLY_BOW_DISTANCE", "APPLY_ATT_GRADE_BONUS",
	"APPLY_DEF_GRADE_BONUS", "APPLY_MAGIC_ATT_GRADE", "APPLY_MAGIC_DEF_GRADE",
	"APPLY_CURSE_PCT", "APPLY_MAX_STAMINA", "APPLY_ATTBONUS_WARRIOR",
	"APPLY_ATTBONUS_ASSASSIN", "APPLY_ATTBONUS_SURA", "APPLY_ATTBONUS_SHAMAN",
	"APPLY_ATTBONUS_MONSTER", "APPLY_MALL_ATTBONUS", "APPLY_MALL_DEFBONUS",
	"APPLY_MALL_EXPBONUS", "APPLY_MALL_ITEMBONUS", "APPLY_MALL_GOLDBONUS",
	"APPLY_MAX_HP_PCT", "APPLY_MAX_SP_PCT", "APPLY_SKILL_DAMAGE_BONUS",
	"APPLY_NORMAL_HIT_DAMAGE_BONUS", "APPLY_SKILL_DEFEND_BONUS",
	"APPLY_NORMAL_HIT_DEFEND_BONUS", "APPLY_PC_BANG_EXP_BONUS",
	"APPLY_PC_BANG_DROP_BONUS", "APPLY_EXTRACT_HP_PCT", "APPLY_RESIST_WARRIOR",
	"APPLY_RESIST_ASSASSIN", "APPLY_RESIST_SURA", "APPLY_RESIST_SHAMAN",
	"APPLY_ENERGY", "APPLY_DEF_GRADE", "APPLY_COSTUME_ATTR_BONUS",
	"APPLY_MAGIC_ATTBONUS_PER", "APPLY_MELEE_MAGIC_ATTBONUS_PER",
	"APPLY_RESIST_ICE", "APPLY_RESIST_EARTH", "APPLY_RESIST_DARK",
	"APPLY_ANTI_CRITICAL_PCT", "APPLY_ANTI_PENETRATE_PCT",
	"APPLY_ATTBONUS_WOLFMAN", "APPLY_RESIST_WOLFMAN", "APPLY_RESIST_CLAW"
};

static std::string NumberText(DWORD value)
{
	std::ostringstream out;
	out << value;
	return out.str();
}

static std::string SignedNumberText(LONG value)
{
	std::ostringstream out;
	out << value;
	return out.str();
}

static std::string EnumText(BYTE value, const char* const* names, size_t count)
{
	if (value < count)
		return names[value];

	std::ostringstream out;
	out << "UNKNOWN_" << static_cast<unsigned int>(value);
	return out.str();
}

static std::string SubTypeText(BYTE type, BYTE value)
{
	EnumNames names = GetSubTypes(type);
	if (names.names == NULL)
		return value == 0 ? "0" : EnumText(value, NULL, 0);
	return EnumText(value, names.names, names.count);
}

static std::string FlagsText(DWORD value, const char* const* names, size_t count)
{
	if (value == 0)
		return "0";

	std::ostringstream out;
	DWORD knownBits = 0;
	bool first = true;
	for (size_t i = 0; i < count; ++i)
	{
		const DWORD bit = static_cast<DWORD>(1) << i;
		if ((value & bit) != 0)
		{
			if (!first)
				out << '|';
			out << names[i];
			first = false;
			knownBits |= bit;
		}
	}

	const DWORD unknownBits = value & ~knownBits;
	if (unknownBits != 0)
	{
		if (!first)
			out << '|';
		out << "UNKNOWN_0x" << std::hex << unknownBits;
	}
	return out.str();
}

static std::string CleanName(const char* name, size_t capacity)
{
	size_t length = 0;
	while (length < capacity && name[length] != '\0')
		++length;

	std::string result(name, length);
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '\t' || result[i] == '\r' || result[i] == '\n')
			result[i] = ' ';
	}
	return result;
}

static bool ReadFile(const char* path, std::vector<BYTE>& bytes)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		std::cerr << "Cannot open input file: " << path << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	const std::streamoff length = file.tellg();
	if (length < 0 || static_cast<unsigned long long>(length) > 512ULL * 1024 * 1024)
	{
		std::cerr << "Input file is too large or could not be sized." << std::endl;
		return false;
	}
	file.seekg(0, std::ios::beg);
	bytes.resize(static_cast<size_t>(length));
	if (length != 0)
		file.read(reinterpret_cast<char*>(&bytes[0]), length);
	if (!file)
	{
		std::cerr << "Failed while reading input file." << std::endl;
		return false;
	}
	return true;
}

static DWORD ReadDword(const std::vector<BYTE>& bytes, size_t offset)
{
	DWORD value = 0;
	std::memcpy(&value, &bytes[offset], sizeof(value));
	return value;
}

static bool DecodeItemProto(const std::vector<BYTE>& fileBytes,
	std::vector<BYTE>& rawTable, DWORD& itemCount)
{
	const size_t outerHeaderSize = 5 * sizeof(DWORD);
	if (fileBytes.size() < outerHeaderSize + sizeof(ProtoHeader))
	{
		std::cerr << "File is too short to contain an item_proto header." << std::endl;
		return false;
	}

	const DWORD fileFourCC = ReadDword(fileBytes, 0);
	const DWORD version = ReadDword(fileBytes, 4);
	const DWORD stride = ReadDword(fileBytes, 8);
	itemCount = ReadDword(fileBytes, 12);
	const DWORD dataSize = ReadDword(fileBytes, 16);

	if (fileFourCC != kItemProtoFourCC)
	{
		std::cerr << "Invalid item_proto signature (expected MIPX)." << std::endl;
		return false;
	}
	if (version != 1)
	{
		std::cerr << "Unsupported item_proto version: " << version << std::endl;
		return false;
	}
	if (stride != sizeof(ClientItemTable))
	{
		std::cerr << "Unsupported item record size: " << stride
			<< " (this exporter expects " << sizeof(ClientItemTable)
			<< "). The binary may use a different Metin2 version/layout." << std::endl;
		return false;
	}
	if (itemCount == 0 || itemCount > std::numeric_limits<DWORD>::max() / stride)
	{
		std::cerr << "Invalid item count in binary header." << std::endl;
		return false;
	}
	if (dataSize != fileBytes.size() - outerHeaderSize)
	{
		std::cerr << "The data size in the file header does not match the file length." << std::endl;
		return false;
	}
	if (dataSize < sizeof(ProtoHeader) + sizeof(DWORD))
	{
		std::cerr << "Compressed block is too short." << std::endl;
		return false;
	}

	const size_t blobOffset = outerHeaderSize;
	const DWORD blobFourCC = ReadDword(fileBytes, blobOffset);
	const DWORD encryptSize = ReadDword(fileBytes, blobOffset + 4);
	const DWORD compressedSize = ReadDword(fileBytes, blobOffset + 8);
	const DWORD realSize = ReadDword(fileBytes, blobOffset + 12);

	if (blobFourCC != kCompressedFourCC)
	{
		std::cerr << "Invalid compressed-data signature (expected MCOZ)." << std::endl;
		return false;
	}
	if (encryptSize == 0 || encryptSize % 8 != 0 ||
		encryptSize > dataSize - sizeof(ProtoHeader) - sizeof(DWORD))
	{
		std::cerr << "Invalid encrypted data length." << std::endl;
		return false;
	}
	if (dataSize != sizeof(ProtoHeader) + sizeof(DWORD) + encryptSize)
	{
		std::cerr << "Compressed block length does not match the file header." << std::endl;
		return false;
	}
	if (compressedSize < sizeof(DWORD) ||
		compressedSize > encryptSize - sizeof(DWORD))
	{
		std::cerr << "Invalid compressed data length." << std::endl;
		return false;
	}
	if (realSize != itemCount * stride)
	{
		std::cerr << "Uncompressed size does not match item count and record size." << std::endl;
		return false;
	}

	std::vector<BYTE> decrypted(encryptSize);
	const BYTE* encrypted = &fileBytes[blobOffset + sizeof(ProtoHeader)];
	if (tea_decrypt(reinterpret_cast<unsigned long*>(&decrypted[0]),
		reinterpret_cast<const unsigned long*>(encrypted),
		kItemProtoKey, static_cast<int>(encryptSize)) != static_cast<int>(encryptSize))
	{
		std::cerr << "TEA decryption failed." << std::endl;
		return false;
	}

	DWORD innerFourCC = 0;
	std::memcpy(&innerFourCC, &decrypted[0], sizeof(innerFourCC));
	if (innerFourCC != kCompressedFourCC)
	{
		std::cerr << "TEA decryption did not produce the expected MCOZ marker; "
			<< "the key or binary format may not match." << std::endl;
		return false;
	}

	if (lzo_init() != LZO_E_OK)
	{
		std::cerr << "Could not initialize the bundled LZO library." << std::endl;
		return false;
	}

	rawTable.resize(realSize);
	lzo_uint outputSize = realSize;
	const int result = lzo1x_decompress_safe(
		&decrypted[sizeof(DWORD)],
		compressedSize,
		&rawTable[0],
		&outputSize,
		NULL);
	if (result != LZO_E_OK || outputSize != realSize)
	{
		std::cerr << "LZO decompression failed (code " << result << ")." << std::endl;
		return false;
	}
	return true;
}

static void WriteField(std::ostream& out, const std::string& value, bool& first)
{
	if (!first)
		out.put('\t');
	out.write(value.data(), static_cast<std::streamsize>(value.size()));
	first = false;
}

static void EndRow(std::ostream& out)
{
	out.write("\r\n", 2);
}

static std::string VnumText(const ClientItemTable& item)
{
	std::ostringstream out;
	out << item.vnum;
	if (item.vnumRange != 0)
		out << '~' << (static_cast<unsigned long long>(item.vnum) + item.vnumRange);
	return out.str();
}

static bool WriteOutputs(const std::string& protoPath, const std::string& namesPath,
	const std::vector<BYTE>& rawTable, DWORD itemCount)
{
	std::ofstream proto(protoPath.c_str(), std::ios::binary);
	if (!proto)
	{
		std::cerr << "Cannot create output file: " << protoPath << std::endl;
		return false;
	}
	std::ofstream names(namesPath.c_str(), std::ios::binary);
	if (!names)
	{
		std::cerr << "Cannot create localized-name file: " << namesPath << std::endl;
		return false;
	}

	const char* headers[] =
	{
		"Vnum", "Name", "Type", "Subtype", "Size", "AntiFlags", "Flags",
		"WearFlags", "ImmuneFlags", "Gold", "ShopBuyPrice", "RefinedVnum",
		"RefineSet", "AlterToMagicItemPct", "LimitType0", "LimitValue0",
		"LimitType1", "LimitValue1", "ApplyType0", "ApplyValue0",
		"ApplyType1", "ApplyValue1", "ApplyType2", "ApplyValue2", "Value0",
		"Value1", "Value2", "Value3", "Value4", "Value5", "Specular",
		"GainSocketPct", "AddonType", "Weight", "Socket0", "Socket1", "Socket2"
	};
	bool first = true;
	for (size_t i = 0; i < sizeof(headers) / sizeof(headers[0]); ++i)
		WriteField(proto, headers[i], first);
	EndRow(proto);
	const char* nameHeader = "Vnum\tLocaleName\r\n";
	names.write(nameHeader, static_cast<std::streamsize>(std::strlen(nameHeader)));

	for (DWORD i = 0; i < itemCount; ++i)
	{
		ClientItemTable item;
		std::memcpy(&item, &rawTable[static_cast<size_t>(i) * sizeof(item)], sizeof(item));

		first = true;
		WriteField(proto, VnumText(item), first);
		WriteField(proto, CleanName(item.name, sizeof(item.name)), first);
		WriteField(proto, EnumText(item.type, kItemTypes,
			sizeof(kItemTypes) / sizeof(kItemTypes[0])), first);
		WriteField(proto, SubTypeText(item.type, item.subType), first);
		WriteField(proto, NumberText(item.size), first);
		WriteField(proto, FlagsText(item.antiFlags, kAntiFlagNames,
			sizeof(kAntiFlagNames) / sizeof(kAntiFlagNames[0])), first);
		WriteField(proto, FlagsText(item.flags, kFlagNames,
			sizeof(kFlagNames) / sizeof(kFlagNames[0])), first);
		WriteField(proto, FlagsText(item.wearFlags, kWearFlagNames,
			sizeof(kWearFlagNames) / sizeof(kWearFlagNames[0])), first);
		WriteField(proto, FlagsText(item.immuneFlags, kImmuneFlagNames,
			sizeof(kImmuneFlagNames) / sizeof(kImmuneFlagNames[0])), first);
		WriteField(proto, NumberText(item.gold), first);
		WriteField(proto, NumberText(item.shopBuyPrice), first);
		WriteField(proto, NumberText(item.refinedVnum), first);
		WriteField(proto, NumberText(item.refineSet), first);
		WriteField(proto, NumberText(item.alterToMagicItemPct), first);

		for (size_t j = 0; j < 2; ++j)
		{
			WriteField(proto, EnumText(item.limits[j].type, kLimitNames,
				sizeof(kLimitNames) / sizeof(kLimitNames[0])), first);
			WriteField(proto, SignedNumberText(item.limits[j].value), first);
		}
		for (size_t j = 0; j < 3; ++j)
		{
			WriteField(proto, EnumText(item.applies[j].type, kApplyNames,
				sizeof(kApplyNames) / sizeof(kApplyNames[0])), first);
			std::ostringstream value;
			value << item.applies[j].value;
			WriteField(proto, value.str(), first);
		}
		for (size_t j = 0; j < 6; ++j)
		{
			std::ostringstream value;
			value << item.values[j];
			WriteField(proto, value.str(), first);
		}
		WriteField(proto, NumberText(item.specular), first);
		WriteField(proto, NumberText(item.gainSocketPct), first);
		WriteField(proto, "0", first);
		WriteField(proto, NumberText(item.weight), first);
		for (size_t j = 0; j < 3; ++j)
			WriteField(proto, SignedNumberText(item.sockets[j]), first);
		EndRow(proto);

		bool firstName = true;
		WriteField(names, NumberText(item.vnum), firstName);
		WriteField(names, CleanName(item.localeName, sizeof(item.localeName)), firstName);
		EndRow(names);
	}

	proto.close();
	names.close();
	if (!proto || !names)
	{
		std::cerr << "A write error occurred while creating the output files." << std::endl;
		return false;
	}
	return true;
}

static std::string SiblingPath(const std::string& path, const char* filename)
{
	const std::string::size_type slash = path.find_last_of("\\/");
	if (slash == std::string::npos)
		return filename;
	return path.substr(0, slash + 1) + filename;
}

int main(int argc, char** argv)
{
	if (sizeof(unsigned long) != sizeof(DWORD))
	{
		std::cerr << "This project must be built for Win32/x86 for the legacy TEA format."
			<< std::endl;
		return 1;
	}
	if (argc > 3)
	{
		std::cerr << "Usage: item_proto_exporter.exe [item_proto] [item_proto.txt]"
			<< std::endl;
		return 1;
	}

	const std::string inputPath = argc >= 2 ? argv[1] : "item_proto";
	const std::string outputPath = argc >= 3 ? argv[2] : "item_proto.txt";
	const std::string namesPath = SiblingPath(outputPath, "item_names.txt");

	std::vector<BYTE> fileBytes;
	std::vector<BYTE> rawTable;
	DWORD itemCount = 0;
	if (!ReadFile(inputPath.c_str(), fileBytes) ||
		!DecodeItemProto(fileBytes, rawTable, itemCount))
		return 1;

	if (!WriteOutputs(outputPath, namesPath, rawTable, itemCount))
		return 1;

	std::cout << "Exported " << itemCount << " items to " << outputPath
		<< " and localized names to " << namesPath << std::endl;
	return 0;
}
