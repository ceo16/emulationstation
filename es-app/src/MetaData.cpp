#include "MetaData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"
#include <pugixml/src/pugixml.hpp>
#include "SystemData.h"
#include "LocaleES.h"
#include "Settings.h"
#include "FileData.h"
#include "ImageIO.h"
#include "EmulationStation.h"
#define N_(String) (String)

std::vector<MetaDataDecl> MetaDataList::mMetaDataDecls = {
MetaDataDecl(LaunchCommand, "launch",      MD_STRING, "",       false, N_("Launch Cmd"),   N_("enter launch command"), false ), // Usa LaunchCommand se hai corretto l'enum
MetaDataDecl(Installed,     "installed",   MD_BOOL,   "false",  false, N_("Installed"),    N_("is game installed?"),   false ), // <<< AGGIUNTA (usa MD_BOOL)
MetaDataDecl(Virtual,       "virtual",     MD_BOOL,   "false",  false, N_("Virtual"),      N_("is game virtual?"),     false ), // <<< AGGIUNTA (usa MD_BOOL)
MetaDataDecl(EpicId,        "epicid",      MD_STRING, "",       false, N_("Epic ID"),      N_("epic app name/id"),     false ), // Già presente, verifica ordine
MetaDataDecl(EpicNamespace, "epicns",      MD_STRING, "",       false, N_("Epic Namespace"),N_("epic namespace"),      false ), // Già presente, verifica ordine
MetaDataDecl(EpicCatalogId, "epiccstid",   MD_STRING, "",       false, N_("Epic Catalog"), N_("epic catalog id"),    false ), // <<< AGGIUNTA
MetaDataDecl(InstallDir,    "installdir",  MD_PATH,   "",       false, N_("Install Dir"),  N_("game install path"),    false ), // <<< AGGIUNTA (se mancava)
MetaDataDecl(Executable,    "executable",  MD_STRING, "",       false, N_("Executable"),   N_("game executable"),      false ), // <<< AGGIUNTA (se mancaa)
MetaDataDecl(SteamAppId,    "steamappid",  MD_STRING, "",       false, N_("Steam App ID"), N_("steam app id"),         false ),
MetaDataDecl(XboxPfn,       "xboxpfn",     MD_STRING, "",       false, N_("Xbox PFN"), N_("Xbox Package Family Name"), false ),
MetaDataDecl(XboxTitleId,   "xboxtitleid", MD_STRING, "",       false, N_("Xbox Title ID"),N_("Xbox Live Title ID"),       false ),
MetaDataDecl(XboxMediaType,"xboxmediatype",MD_STRING, "",       false, N_("Xbox Media Type"),N_("Xbox Media Type (Game, App)"), false ),
MetaDataDecl(XboxDevices,  "xboxdevices",  MD_STRING, "",       false, N_("Xbox Devices"),N_("Supported Xbox Devices"),   false ),
MetaDataDecl(XboxProductId, "xboxproductid", MD_STRING,   "",      false, N_("Xbox Product Id"),      N_("Supported Xbox Product"),   false ),
MetaDataDecl(XboxAumid, "xboxaumid",       MD_STRING,   "",      false, N_("Xbox AUMID"),   N_("Xbox Application User Model ID"),     false ), 
};


static std::map<MetaDataId, int> mMetaDataIndexes;
static std::string* mDefaultGameMap = nullptr;
static MetaDataType* mGameTypeMap = nullptr;
static std::map<std::string, MetaDataId> mGameIdMap;

static std::map<std::string, int> KnowScrapersIds =
{
	{ "ScreenScraper", 0 },
	{ "TheGamesDB", 1 },
	{ "HfsDB", 2 },
	{ "ArcadeDB", 3 },
    { "EPIC GAME STORE", 4 },
	{ "STEAM", 5 }
};

void MetaDataList::initMetadata()
{
	MetaDataDecl gameDecls[] = 
	{
		// key,             type,                   default,            statistic,  name in GuiMetaDataEd,  prompt in GuiMetaDataEd
		{ Name,             "name",        MD_STRING,              "",                 false,      _("Name"),                 _("this game's name"),			true },
	//	{ SortName,         "sortname",    MD_STRING,              "",                 false,      _("sortname"),             _("enter game sort name"),	true },
		{ Desc,             "desc",        MD_MULTILINE_STRING,    "",                 false,      _("Description"),          _("this game's description"),		true },

#if WIN32 && !_DEBUG
		{ Emulator,         "emulator",    MD_LIST,				 "",                 false,       _("Emulator"),			 _("emulator"),					false },
		{ Core,             "core",	      MD_LIST,				 "",                 false,       _("Core"),				 _("core"),						false },
#else
		// Windows & recalbox gamelist.xml compatiblity -> Set as statistic to hide it from metadata editor
		{ Emulator,         "emulator",    MD_LIST,				 "",                 true,        _("Emulator"),			 _("emulator"),					false },
		{ Core,             "core",	     MD_LIST,				 "",                 true,        _("Core"),				 _("core"),						false },
#endif

		{ Image,            "image",       MD_PATH,                "",                 false,      _("Image"),                _("enter path to image"),		 true },
		{ Video,            "video",       MD_PATH,                "",                 false,      _("Video"),                _("enter path to video"),		 false },
		{ Marquee,          "marquee",     MD_PATH,                "",                 false,      _("Logo"),                 _("enter path to logo"),	     true },
		{ Thumbnail,        "thumbnail",   MD_PATH,                "",                 false,      _("Box"),				  _("enter path to box"),		 false },

		{ FanArt,           "fanart",      MD_PATH,                "",                 false,      _("Fan art"),              _("enter path to fanart"),	 true },
		{ TitleShot,        "titleshot",   MD_PATH,                "",                 false,      _("Title shot"),           _("enter path to title shot"), true },
		{ Manual,			"manual",	   MD_PATH,                "",                 false,      _("Manual"),               _("enter path to manual"),     true },
		{ Magazine,			"magazine",	   MD_PATH,                "",                 false,      _("Magazine"),             _("enter path to magazine"),     true },
		{ Map,			    "map",	       MD_PATH,                "",                 false,      _("Map"),                  _("enter path to map"),		 true },
		{ Bezel,            "bezel",       MD_PATH,                "",                 false,      _("Bezel (16:9)"),         _("enter path to bezel (16:9)"),	 true },

		// Non scrappable /editable medias
		{ Cartridge,        "cartridge",   MD_PATH,                "",                 true,       _("Cartridge"),            _("enter path to cartridge"),  true },
		{ BoxArt,			"boxart",	   MD_PATH,                "",                 true,       _("Alt BoxArt"),		      _("enter path to alt boxart"), true },
		{ BoxBack,			"boxback",	   MD_PATH,                "",                 false,      _("Box backside"),		  _("enter path to box background"), true },
		{ Wheel,			"wheel",	   MD_PATH,                "",                 true,       _("Wheel"),		          _("enter path to wheel"),      true },
		{ Mix,			    "mix",	       MD_PATH,                "",                 true,       _("Mix"),                  _("enter path to mix"),		 true },
		
		{ Rating,           "rating",      MD_RATING,              "0.000000",         false,      _("Rating"),               _("enter rating"),			false },
		{ ReleaseDate,      "releasedate", MD_DATE,                "not-a-date-time",  false,      _("Release date"),         _("enter release date"),		false },
		{ Developer,        "developer",   MD_STRING,              "",                 false,      _("Developer"),            _("this game's developer"),	false },
		{ Publisher,        "publisher",   MD_STRING,              "",                 false,      _("Publisher"),            _("this game's publisher"),	false },


		{ Genre,            "genre",       MD_STRING,              "",                 false,      _("Genre"),                _("enter game genre"),		false }, 
		{ Family,           "family",      MD_STRING,              "",                 false,      _("Game family"),		  _("this game's game family"),		false },

		// GenreIds is not serialized
		{ GenreIds,         "genres",      MD_STRING,              "",                 false,      _("Genres"),				  _("enter game genres"),		false },

		{ ArcadeSystemName, "arcadesystemname",  MD_STRING,        "",                 false,      _("Arcade system"),        _("this game's arcade system"), false },

		{ Players,          "players",     MD_STRING,              "",                false,       _("Players"),              _("this game's number of players"),	false },
		{ Favorite,         "favorite",    MD_BOOL,                "false",            false,      _("Favorite"),             _("enter favorite"),			false },
		{ Hidden,           "hidden",      MD_BOOL,                "false",            false,      _("Hidden"),               _("enter hidden"),			true },
		{ KidGame,          "kidgame",     MD_BOOL,                "false",            false,      _("Kidgame"),              _("enter kidgame"),			false },
		{ PlayCount,        "playcount",   MD_INT,                 "0",                true,       _("Play count"),           _("enter number of times played"), false },
		{ LastPlayed,       "lastplayed",  MD_TIME,                "0",                true,       _("Last played"),          _("enter last played date"), false },

		{ Crc32,            "crc32",       MD_STRING,              "",                 true,       _("Crc32"),                _("Crc32 checksum"),			false },
		{ Md5,              "md5",		   MD_STRING,              "",                 true,       _("Md5"),                  _("Md5 checksum"),			false },

		{ GameTime,         "gametime",    MD_INT,                 "0",                true,       _("Game time"),            _("how long the game has been played in total (seconds)"), false },

		{ Language,         "lang",        MD_STRING,              "",                 false,      _("Languages"),            _("this game's languages"),				false },
		{ Region,           "region",      MD_STRING,              "",                 false,      _("Region"),               _("this game's region"),					false },

		{ CheevosHash,      "cheevosHash", MD_STRING,              "",                 true,       _("Cheevos Hash"),          _("Cheevos checksum"),	    false },
		{ CheevosId,        "cheevosId",   MD_INT,                 "",				   true,       _("Cheevos Game ID"),       _("Cheevos Game ID"),		false },

		{ ScraperId,        "id",		   MD_INT,                 "",				   true,       _("Screenscraper Game ID"), _("Screenscraper Game ID"),	false, true },
        { LaunchCommand, "launch",      MD_STRING,           "",                 false, N_("Launch Cmd"),   N_("enter launch command"),     false },
        { Installed,     "installed",   MD_BOOL,             "false",            false, N_("Installed"),    N_("is game installed?"),       false },
        { Virtual,       "virtual",     MD_BOOL,             "false",            false, N_("Virtual"),      N_("is game virtual?"),         false },
        { EpicId,        "epicid",      MD_STRING,           "",                 false, N_("Epic ID"),      N_("epic app name/id"),         false },
        { EpicNamespace, "epicns",      MD_STRING,           "",                 false, N_("Epic Namespace"),N_("epic namespace"),          false },
        { EpicCatalogId, "epiccstid",   MD_STRING,           "",                 false, N_("Epic Catalog"), N_("epic catalog id"),        false },
        { InstallDir,    "installdir",  MD_PATH,             "",                 false, N_("Install Dir"),  N_("game install path"),        false },
        { Executable,    "executable",  MD_STRING,           "",                 false, N_("Executable"),   N_("game executable"),          false },
		{ SteamAppId,    "steamappid",  MD_STRING,           "",                 false, N_("Steam App ID"), N_("steam app id"),         false },
       { Path,             "path",             MD_PATH,     "",      false, N_("Path"),              N_("Path to the game file or folder"),                             true  }, // ID 51, isPath = true

    // Nuovi per Xbox
    { XboxPfn,          "xboxpfn",          MD_STRING,   "",      false, N_("Xbox PFN"),          N_("Xbox Package Family Name"),                                    false }, // ID 52
    { XboxTitleId,      "xboxtitleid",      MD_STRING,   "",      false, N_("Xbox Title ID"),     N_("Xbox Live Title ID"),                                          false }, // ID 53
    { XboxMediaType,    "xboxmediatype",    MD_STRING,   "",      false, N_("Xbox Media Type"),   N_("Xbox Media Type (Game, App, Dlc)"),                            false }, // ID 54
    { XboxDevices,      "xboxdevices",      MD_STRING,   "",      false, N_("Xbox Devices"),      N_("Supported Xbox Devices (PC, XboxSeries, etc.)"),               false },  // ID 55
	{ XboxProductId,    "xboxproductid",    MD_STRING,   "",      false, N_("Xbox Product Id"),   N_("Supported Xbox Product"),               false },// <--- NUOVA RIGA AGGIUNTA QUI // <--- NUOVA RIGA AGGIUNTA QUI
	 { XboxAumid,       "xboxaumid",       MD_STRING,   "",      false, N_("Xbox AUMID"),         N_("Xbox Application User Model ID"),                            false }, 
     { StoreProvider,    "storeprovider",    MD_STRING,   "false", false,      N_("Store Provider"),    N_("game store provider (STEAM, EPIC, EAGAMES, etc)"), false}, // Default a "false" per MD_STRING è strano, forse ""?
     { IsOwned,          "isowned",          MD_BOOL,     "false", false,      N_("Owned"),             N_("is game owned in online library"), false}, // Tag XML "isowned"
	{ EaOfferId,        "eaofferid",        MD_STRING,   "",      false,      N_("EA Offer ID"),       N_("EA Games Offer ID"), false},
	{ EaMasterTitleId,  "eamastertitleid",  MD_STRING,   "",      false,      N_("EA Master Title ID"),   N_("EA Games Master Title ID"), false },
	{ EaMultiplayerId,  "eamultiplayerid",  MD_STRING,   "",      false,      N_("EA Multiplayer ID"), N_("EA Games Multiplayer ID from manifest"), false },

		
	};
	
// Questa riga è FONDAMENTALE: popola mMetaDataDecls con il contenuto di gameDecls.
	mMetaDataDecls = std::vector<MetaDataDecl>(gameDecls, gameDecls + sizeof(gameDecls) / sizeof(gameDecls[0]));

	// Il resto della funzione initMetadata() dovrebbe rimanere invariato,
	// poiché ricalcola mMetaDataIndexes, mDefaultGameMap, mGameTypeMap, e mGameIdMap
	// basandosi sul contenuto aggiornato di mMetaDataDecls.

	mMetaDataIndexes.clear();
	for (int i = 0 ; i < (int)mMetaDataDecls.size() ; i++) // Cast a (int) per sicurezza con size_t
		mMetaDataIndexes[mMetaDataDecls[i].id] = i;

	// int maxID = mMetaDataDecls.size() + 1; // Questo calcolo di maxID potrebbe essere problematico se MetaDataId non è contiguo o parte da 0.
	                                        // È più sicuro usare MetaDataId::MAX_METADATA_TYPES che abbiamo definito nell'enum.
	int maxID = MetaDataId::MAX_METADATA_TYPES; // Usa il valore dall'enum MAX_METADATA_TYPES

	if (mDefaultGameMap != nullptr)
		delete[] mDefaultGameMap;

	if (mGameTypeMap != nullptr)
		delete[] mGameTypeMap;

	mDefaultGameMap = new std::string[maxID]; // indicizzato da MetaDataId enum
	mGameTypeMap = new MetaDataType[maxID];  // indicizzato da MetaDataId enum

	for (int i = 0; i < maxID; i++)
		mGameTypeMap[i] = MD_STRING; // Inizializza tutto a MD_STRING come fallback (anche se dovrebbe essere sovrascritto)

	mGameIdMap.clear(); // Pulisci la mappa prima di ripopolarla
	for (auto iter = mMetaDataDecls.cbegin(); iter != mMetaDataDecls.cend(); iter++)
	{
		// Verifica che iter->id sia all'interno dei limiti validi per gli array
		if (iter->id < maxID) { // Controllo di sicurezza
			mDefaultGameMap[iter->id] = iter->defaultValue;
			mGameTypeMap[iter->id] = iter->type;
		} else {
			LOG(LogError) << "MetaDataId " << iter->id << " for key '" << iter->key << "' is out of bounds (maxID=" << maxID << "). Skipping.";
		}
		mGameIdMap[iter->key] = iter->id;
	}
}

MetaDataType MetaDataList::getType(MetaDataId id) const
{
	return mGameTypeMap[id];
}

MetaDataType MetaDataList::getType(const std::string name) const
{
	return getType(getId(name));
}

MetaDataId MetaDataList::getId(const std::string& key) const
{
	return mGameIdMap[key];
}

MetaDataList::MetaDataList(MetaDataListType type) : mType(type), mWasChanged(false), mRelativeTo(nullptr)
{

}

void MetaDataList::loadFromXML(MetaDataListType type, pugi::xml_node& node, SystemData* system)
{
	mType = type;
	mRelativeTo = system;	

	mUnKnownElements.clear();
	mScrapeDates.clear();

	std::string value;
	std::string relativeTo = mRelativeTo->getStartPath();

	bool preloadMedias = Settings::PreloadMedias();
	if (preloadMedias && Settings::ParseGamelistOnly())
		preloadMedias = false;

	for (pugi::xml_node xelement : node.children())
	{
		std::string name = xelement.name();

		if (name == "scrap")
		{
			if (xelement.attribute("name") && xelement.attribute("date"))
			{
				auto scraperId = KnowScrapersIds.find(xelement.attribute("name").value());
				if (scraperId == KnowScrapersIds.cend())
					continue;
				
				Utils::Time::DateTime dateTime(xelement.attribute("date").value());
				if (!dateTime.isValid())
					continue;
								
				mScrapeDates[scraperId->second] = dateTime;
			}		
								
			continue;
		}

		auto it = mGameIdMap.find(name);
		if (it == mGameIdMap.cend())
		{
			if (name == "hash" || name == "path")
				continue;

			value = xelement.text().get();
			LOG(LogDebug) << "loadFromXML (Child): Processing Key=[" << name << "], Value=[" << value.substr(0, 50) << (value.length() > 50 ? "..." : "") << "]"; // Logga chiave e valore letto
			if (!value.empty())
				mUnKnownElements.push_back(std::tuple<std::string, std::string, bool>(name, value, true));

			continue;
		}

		MetaDataDecl& mdd = mMetaDataDecls[mMetaDataIndexes[it->second]];
		if (mdd.isAttribute)
			continue;

		value = xelement.text().get();

		if (mdd.id == MetaDataId::Name)
		{
			mName = value;
			continue;
		}

		if (mdd.id == MetaDataId::GenreIds)
			continue;

		if (value == mdd.defaultValue)
			continue;

		if (mdd.type == MD_BOOL)
			value = Utils::String::toLower(value);
		
		if (preloadMedias && mdd.type == MD_PATH && (mdd.id == MetaDataId::Image || mdd.id == MetaDataId::Thumbnail || mdd.id == MetaDataId::Marquee || mdd.id == MetaDataId::Video) &&
			!Utils::FileSystem::exists(Utils::FileSystem::resolveRelativePath(value, relativeTo, true)))
			continue;
		
		// Players -> remove "1-"
		// if (type == GAME_METADATA && mdd.id == MetaDataId::Players && Utils::String::startsWith(value, "1-"))
		// 	value = Utils::String::replace(value, "1-", "");

		set(mdd.id, value);
	}

	for (pugi::xml_attribute xattr : node.attributes())
	{
		std::string name = xattr.name();
		auto it = mGameIdMap.find(name);
		if (it == mGameIdMap.cend())
		{
			value = xattr.value();
			LOG(LogDebug) << "loadFromXML (Attr): Processing Key=[" << name << "], Value=[" << value.substr(0, 50) << (value.length() > 50 ? "..." : "") << "]"; // Logga chiave e valore letto
			if (!value.empty())
				mUnKnownElements.push_back(std::tuple<std::string, std::string, bool>(name, value, false));

			continue;
		}

		MetaDataDecl& mdd = mMetaDataDecls[mMetaDataIndexes[it->second]];
		if (!mdd.isAttribute)
			continue;

		value = xattr.value();

		if (value == mdd.defaultValue)
			continue;

		if (mdd.type == MD_BOOL)
			value = Utils::String::toLower(value);

		// Players -> remove "1-"
		// if (type == GAME_METADATA && mdd.id == MetaDataId::Players && Utils::String::startsWith(value, "1-"))
		// 	value = Utils::String::replace(value, "1-", "");

		if (mdd.id == MetaDataId::Name)
			mName = value;
		else
			set(mdd.id, value);
	}
}

// Add migration for alternative formats & old tags
void MetaDataList::migrate(FileData* file, pugi::xml_node& node)
{
	if (get(MetaDataId::Crc32).empty())
	{
		pugi::xml_node xelement = node.child("hash");
		if (xelement)
			set(MetaDataId::Crc32, xelement.text().get());
	}
}

void MetaDataList::appendToXML(pugi::xml_node& parent, bool ignoreDefaults, const std::string& relativeTo, bool fullPaths) const
{
	const std::vector<MetaDataDecl>& mdd = getMDD();

	for(auto mddIter = mdd.cbegin(); mddIter != mdd.cend(); mddIter++)
	{
		if (mddIter->id == 0)
		{
			parent.append_child("name").text().set(mName.c_str());
			continue;
		}

		// Don't save GenreIds
		if (mddIter->id == MetaDataId::GenreIds)
			continue;

		auto mapIter = mMap.find(mddIter->id);
		if(mapIter != mMap.cend())
		{
			// we have this value!
			// if it's just the default (and we ignore defaults), don't write it
			if (ignoreDefaults && mapIter->second == mddIter->defaultValue)
				continue;

			// try and make paths relative if we can
			std::string value = mapIter->second;
			if (mddIter->type == MD_PATH)
			{
				if (fullPaths && mRelativeTo != nullptr)
					value = Utils::FileSystem::resolveRelativePath(value, mRelativeTo->getStartPath(), true);
				else
					value = Utils::FileSystem::createRelativePath(value, relativeTo, true);
			}
			LOG(LogDebug) << "appendToXML: ABOUT TO WRITE Key=[" << mddIter->key << "], Value=[" << value << "]";	
			if (mddIter->isAttribute)
				parent.append_attribute(mddIter->key.c_str()).set_value(value.c_str());
			else
				parent.append_child(mddIter->key.c_str()).text().set(value.c_str());
		}
	}

	for (std::tuple<std::string, std::string, bool> element : mUnKnownElements)
	{	
		bool isElement = std::get<2>(element);
		if (isElement)
			parent.append_child(std::get<0>(element).c_str()).text().set(std::get<1>(element).c_str());
		else 
			parent.append_attribute(std::get<0>(element).c_str()).set_value(std::get<1>(element).c_str());
	}

	if (mScrapeDates.size() > 0)
	{
		for (auto scrapeDate : mScrapeDates)
		{
			std::string name;

			for (auto sids : KnowScrapersIds)
			{
				if (sids.second == scrapeDate.first)
				{
					name = sids.first;
					break;
				}
			}

			if (!name.empty())
			{
				auto scraper = parent.append_child("scrap");
				scraper.append_attribute("name").set_value(name.c_str());
				scraper.append_attribute("date").set_value(scrapeDate.second.getIsoString().c_str());
			}
		}
	}
}

void MetaDataList::set(MetaDataId id, const std::string& value)
{
    if (id == MetaDataId::Name)
    {
        if (mName == value)
            return;

        mName = value;
        mWasChanged = true;
        return;
    }

    auto prev = mMap.find(id);
    if (prev != mMap.cend() && prev->second == value)
        return;

    if (mGameTypeMap[id] == MD_PATH && mRelativeTo != nullptr)
    {
        // Se il valore è un URL (inizia con http), salvalo così com'è. NON TOCCARLO.
        if (Utils::String::startsWith(value, "http"))
        {
            mMap[id] = value;
        }
        // ALTRIMENTI, è un file locale e possiamo creare il percorso relativo.
        else
        {
            mMap[id] = Utils::FileSystem::createRelativePath(value, mRelativeTo->getStartPath(), true);
        }
    }
    else
    {
        mMap[id] = Utils::String::trim(value);
    }

    mWasChanged = true;
}

const std::string MetaDataList::get(MetaDataId id, bool resolveRelativePaths) const
{
    if (id == MetaDataId::Name)
        return mName;

    auto it = mMap.find(id);
    if (it != mMap.end())
    {
        const std::string& path = it->second;

        // PRIMO CONTROLLO: Se è un URL, non toccarlo e restituiscilo subito.
        if (Utils::String::startsWith(path, "http"))
            return path; // Se è un URL, restituiscilo subito, pulito.

        // Se non è un URL, allora è un file locale e possiamo procedere
        // con la vecchia logica per risolvere i percorsi relativi.
        if (resolveRelativePaths && mGameTypeMap[id] == MD_PATH && mRelativeTo != nullptr)
            return Utils::FileSystem::resolveRelativePath(path, mRelativeTo->getStartPath(), true);

        // Se nessuna delle condizioni sopra è vera, restituisci il percorso così com'è.
        return path;
    }

    // Se non trova nulla, restituisce il valore di default.
    return mDefaultGameMap[id];
}

void MetaDataList::set(const std::string& key, const std::string& value)
{
	if (mGameIdMap.find(key) == mGameIdMap.cend())
		return;

	set(getId(key), value);
}

const bool MetaDataList::exists(const std::string& key) const
{
	return mGameIdMap.find(key) != mGameIdMap.cend();
}

const std::string MetaDataList::get(const std::string& key, bool resolveRelativePaths) const
{
	if (mGameIdMap.find(key) == mGameIdMap.cend())
		return "";

	return get(getId(key), resolveRelativePaths);
}

int MetaDataList::getInt(MetaDataId id) const
{
	return atoi(get(id).c_str());
}

float MetaDataList::getFloat(MetaDataId id) const
{
	return Utils::String::toFloat(get(id));
}

bool MetaDataList::wasChanged() const
{
	return mWasChanged;
}

void MetaDataList::resetChangedFlag()
{
	mWasChanged = false;
}

// In MetaData.cpp
// Assicurati che Utils::String::startsWith sia disponibile (da StringUtil.h)
// e che ImageIO::loadImageSize, ImageIO::removeImageCache siano correttamente inclusi.

void MetaDataList::importScrappedMetadata(const MetaDataList& source)
{
	int type = MetaDataImportType::Types::ALL; 

	if (Settings::getInstance()->getString("Scraper") == "ScreenScraper") 
	{
		if (Settings::getInstance()->getString("ScrapperImageSrc").empty()) type &= ~MetaDataImportType::Types::IMAGE;
		if (Settings::getInstance()->getString("ScrapperThumbSrc").empty()) type &= ~MetaDataImportType::Types::THUMB;
		if (Settings::getInstance()->getString("ScrapperLogoSrc").empty()) type &= ~MetaDataImportType::Types::MARQUEE;
		if (!Settings::getInstance()->getBool("ScrapeVideos")) type &= ~MetaDataImportType::Types::VIDEO;
		if (!Settings::getInstance()->getBool("ScrapeFanart")) type &= ~MetaDataImportType::Types::FANART;
		if (!Settings::getInstance()->getBool("ScrapeBoxBack")) type &= ~MetaDataImportType::Types::BOXBACK;
		if (!Settings::getInstance()->getBool("ScrapeTitleShot")) type &= ~MetaDataImportType::Types::TITLESHOT;
		if (!Settings::getInstance()->getBool("ScrapeMap")) type &= ~MetaDataImportType::Types::MAP;
		if (!Settings::getInstance()->getBool("ScrapeManual")) type &= ~MetaDataImportType::Types::MANUAL;
		if (!Settings::getInstance()->getBool("ScrapeCartridge")) type &= ~MetaDataImportType::Types::CARTRIDGE;
	}

	bool scapeNames = Settings::getInstance()->getBool("ScrapeNames");
	bool scrapeDescription = Settings::getInstance()->getBool("ScrapeDescription");

	for (auto mdd : getMDD())
	{
		// Se il metadato è uno di quelli che vogliamo preservare, saltalo.
		if (mdd.isStatistic && mdd.id != MetaDataId::ScraperId) continue;
		if (mdd.id == MetaDataId::KidGame) continue;
		if (mdd.id == MetaDataId::Region || mdd.id == MetaDataId::Language) continue;

        // *** INIZIO DELLA CORREZIONE FONDAMENTALE ***
        // Aggiungiamo tutti i nostri tag custom e di stato a questa lista di eccezioni.
        // Lo scraper non deve MAI sovrascrivere questi valori.
		if (mdd.id == MetaDataId::Favorite || 
            mdd.id == MetaDataId::Hidden || 
            mdd.id == MetaDataId::Emulator || 
            mdd.id == MetaDataId::Core ||
            mdd.id == MetaDataId::Installed || // Preserva il flag 'installato'
            mdd.id == MetaDataId::Virtual ||   // Preserva il flag 'virtuale'
			mdd.id == MetaDataId::LaunchCommand ||   // Preserva il flag 'virtuale'
            mdd.id == MetaDataId::IsOwned ||   // Preserva il flag 'posseduto'
            mdd.key == "storeprovider" ||      // Preserva il nome dello store
            mdd.key == "ea_offerid" ||         // Preserva tutti i tag custom
            mdd.key == "ea_mastertitleid" ||
            mdd.key == "ea_multiplayerid" ||
            mdd.key == "steam_appid" ||
            mdd.key == "epic_id" ||
            mdd.key == "epic_ns" ||
            mdd.key == "epic_catalog_id" ||
            mdd.key == "xbox_pfn" ||
            mdd.key == "xbox_titleid" ||
            mdd.key == "xbox_productid" ||
            mdd.key == "xbox_aumid")
        {
			continue;
        }
        // *** FINE DELLA CORREZIONE FONDAMENTALE ***

		if (mdd.id == MetaDataId::Name && !scapeNames) {
			if (!get(mdd.id).empty()) continue;
		}
		if (mdd.id == MetaDataId::Desc && !scrapeDescription) {
			if (!get(mdd.id).empty()) continue;
		}
		
		if (mdd.id == MetaDataId::Image && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::IMAGE) != MetaDataImportType::Types::IMAGE)) continue;
		if (mdd.id == MetaDataId::Thumbnail && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::THUMB) != MetaDataImportType::Types::THUMB)) continue;
		if (mdd.id == MetaDataId::Marquee && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::MARQUEE) != MetaDataImportType::Types::MARQUEE)) continue;
		if (mdd.id == MetaDataId::Video) {
			const std::string& videoValue = source.get(mdd.id);
			bool isHttpUrl = Utils::String::startsWith(videoValue, "http://") || Utils::String::startsWith(videoValue, "https://");
			if (((type & MetaDataImportType::Types::VIDEO) != MetaDataImportType::Types::VIDEO) && !isHttpUrl) {
                if (videoValue.empty()) continue;
            } else if (videoValue.empty() && !isHttpUrl) {
                continue;
            }
		}
		if (mdd.id == MetaDataId::TitleShot && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::TITLESHOT) != MetaDataImportType::Types::TITLESHOT)) continue;
		if (mdd.id == MetaDataId::FanArt && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::FANART) != MetaDataImportType::Types::FANART)) continue;
		if (mdd.id == MetaDataId::BoxBack && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::BOXBACK) != MetaDataImportType::Types::BOXBACK)) continue;
		if (mdd.id == MetaDataId::Map && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::MAP) != MetaDataImportType::Types::MAP)) continue;
		if (mdd.id == MetaDataId::Manual && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::MANUAL) != MetaDataImportType::Types::MANUAL)) continue;
		if (mdd.id == MetaDataId::Cartridge && (source.get(mdd.id).empty() || (type & MetaDataImportType::Types::CARTRIDGE) != MetaDataImportType::Types::CARTRIDGE)) continue;
		if (mdd.id == MetaDataId::Rating && source.getFloat(mdd.id) < 0) continue;

		// Se siamo arrivati qui, il metadato può essere importato.
		set(mdd.id, source.get(mdd.id));

		if (mdd.type == MetaDataType::MD_PATH)
		{
			const std::string& pathOrUrlValue = get(mdd.id);
			if (mdd.id == MetaDataId::Video && (Utils::String::startsWith(pathOrUrlValue, "http://") || Utils::String::startsWith(pathOrUrlValue, "https://")))
			{
				LOG(LogDebug) << "MetaDataList::importScrappedMetadata: Video URL detected (" << pathOrUrlValue << "). Skipping ImageIO.";
			}
			else if (!pathOrUrlValue.empty()) 
			{
                ImageIO::removeImageCache(pathOrUrlValue);
                unsigned int x = 0, y = 0;
                if (mdd.id != MetaDataId::Video && mdd.id != MetaDataId::Manual && mdd.id != MetaDataId::Magazine) {
                    if (!ImageIO::loadImageSize(pathOrUrlValue.c_str(), &x, &y))
                    {
                        LOG(LogWarning) << "MetaDataList::importScrappedMetadata: Could not load image size for: " << pathOrUrlValue;
                    }
                }
			}
		}
	}

	if (Utils::String::startsWith(source.getName(), "ZZZ(notgame)"))
		set(MetaDataId::Hidden, "true");
}


std::string MetaDataList::getRelativeRootPath()
{
	if (mRelativeTo)
		return mRelativeTo->getStartPath();

	return "";
}

void MetaDataList::setScrapeDate(const std::string& scraper)
{
	auto it = KnowScrapersIds.find(scraper);
	if (it == KnowScrapersIds.cend())
		return;

	mScrapeDates[it->second] = Utils::Time::DateTime::now();
	mWasChanged = true;
}

Utils::Time::DateTime* MetaDataList::getScrapeDate(const std::string& scraper)
{
	auto it = KnowScrapersIds.find(scraper);
	if (it != KnowScrapersIds.cend())
	{
		auto itd = mScrapeDates.find(it->second);
		if (itd != mScrapeDates.cend())
			return &itd->second;
	}

	return nullptr;
}