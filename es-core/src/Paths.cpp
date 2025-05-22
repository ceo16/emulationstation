#include "Paths.h"
#include "../../es-app/src/SystemData.h"           // <-- INCLUDI LA DEFINIZIONE COMPLETA DI SYSTEMDATA QU
#include "utils/FileSystemUtil.h" 
#include "utils/StringUtil.h"     
#include "Log.h"                  // Includi Log.h se non è già transitiv
#include <iostream>               // Già presente nel tuo file
#include <fstream>                // Già presente nel tuo file

// Inizializzazione del puntatore statico (già presente nel tuo file)
Paths* Paths::_instance = nullptr;

// Definizione per SETTINGS_FILENAME (già presente nel tuo file)
#ifdef WIN32
#define SETTINGS_FILENAME "emulatorLauncher.cfg"
#else
#define SETTINGS_FILENAME "emulationstation.ini"
#endif

// Costruttore Paths() (dal tuo file)
Paths::Paths()
{	
    // LA TUA LOGICA DI INIZIALIZZAZIONE ESISTENTE per mRootPath, mUserEmulationStationPath, ecc.
    // Assicurati che mUserEmulationStationPath sia inizializzato in modo affidabile qui,
    // perché getGamelistRecoveryPath_impl lo userà.
	mEmulationStationPath = getExePath(); 
	mUserEmulationStationPath = Utils::FileSystem::getCanonicalPath(getHomePath() + "/.emulationstation");
	mRootPath = Utils::FileSystem::getParent(getHomePath()); 

	mLogPath = mUserEmulationStationPath;
	mThemesPath = mUserEmulationStationPath + "/themes";
	mMusicPath = mUserEmulationStationPath + "/music";
	mKeyboardMappingsPath = mUserEmulationStationPath + "/padtokey";
	mUserManualPath = mUserEmulationStationPath + "/notice.pdf";

#if defined(WIN32) && defined(_DEBUG)
	mSystemConfFilePath = mUserEmulationStationPath + "/batocera.conf";
#endif

	loadCustomConfiguration(false); 

#if BATOCERA
    // LA TUA LOGICA BATOCERA ESISTENTE...
	mRootPath = "/userdata";
	mEmulationStationPath = "/usr/share/emulationstation";
	mUserEmulationStationPath = "/userdata/system/configs/emulationstation";
	mLogPath = "/userdata/system/logs";
	mScreenShotsPath = "/userdata/screenshots";
	mSaveStatesPath = "/userdata/saves";
	mMusicPath = "/usr/share/batocera/music";
	mUserMusicPath = "/userdata/music";
	mThemesPath = "/usr/share/emulationstation/themes";
	mUserThemesPath = "/userdata/themes";
	mKeyboardMappingsPath = "/usr/share/evmapy";
	mUserKeyboardMappingsPath = "/userdata/system/configs/evmapy";
	mDecorationsPath = "/usr/share/batocera/datainit/decorations";
	mUserDecorationsPath = "/userdata/decorations";
	mShadersPath = "/usr/share/batocera/shaders/configs";
	mUserShadersPath = "/userdata/shaders/configs";
	mTimeZonesPath = "/usr/share/zoneinfo/";
	mRetroachivementSounds = "/usr/share/libretro/assets/sounds";
	mUserRetroachivementSounds = "/userdata/sounds/retroachievements";	
	mSystemConfFilePath = "/userdata/system/batocera.conf";
	mUserManualPath = "/usr/share/batocera/doc/notice.pdf";
	mVersionInfoPath = "/usr/share/batocera/batocera.version";
	mKodiPath = "/usr/bin/kodi";
#endif

/* EmuElec sample locations (dal tuo file)
#ifdef _ENABLEEMUELEC
    // ...
#endif
*/
	loadCustomConfiguration(true); 
}

// loadCustomConfiguration (la tua implementazione esistente)
void Paths::loadCustomConfiguration(bool overridesOnly)
{
    // ... (LA TUA IMPLEMENTAZIONE ESISTENTE, SENZA MODIFICHE QUI) ...
	std::map<std::string, std::string*> files =
	{
		{ "config", &mSystemConfFilePath },
		{ "manual", &mUserManualPath },
		{ "versioninfo", &mVersionInfoPath },
		{ "kodi", &mKodiPath }
	};

	std::map<std::string, std::string*> folders = 
	{
		// Folders
		{ "root", &mRootPath },
		{ "log", &mLogPath },
		{ "screenshots", &mScreenShotsPath },
		{ "saves", &mSaveStatesPath },
		{ "system.music", &mMusicPath },
		{ "music", &mUserMusicPath },
		{ "system.themes", &mThemesPath },
		{ "themes", &mUserThemesPath },
		{ "system.padtokey", &mKeyboardMappingsPath },
		{ "padtokey", &mUserKeyboardMappingsPath },
		{ "system.decorations", &mDecorationsPath },
		{ "decorations", &mUserDecorationsPath },
		{ "system.shaders", &mShadersPath },
		{ "shaders", &mUserShadersPath },
		{ "system.videofilters", &mVideoFiltersPath },
		{ "videofilters", &mUserVideoFiltersPath },
		{ "system.retroachievementsounds", &mRetroachivementSounds },
		{ "retroachievementsounds", &mUserRetroachivementSounds },
		{ "timezones", &mTimeZonesPath },
#if WIN32
		{ "kodi", &mKodiPath }
#endif
	};

	std::map<std::string, std::string> ret;

	std::string path = mEmulationStationPath + std::string("/") + SETTINGS_FILENAME;
	if (!Utils::FileSystem::exists(path))
		path = mUserEmulationStationPath + std::string("/") + SETTINGS_FILENAME;
	if (!Utils::FileSystem::exists(path))
		path = Utils::FileSystem::getParent(mUserEmulationStationPath) + std::string("/") + SETTINGS_FILENAME;

	if (!Utils::FileSystem::exists(path))
		return;
		
	std::string relativeTo = Utils::FileSystem::getParent(path);

	if (!overridesOnly)
	{
		for (auto var : folders)
		{
			std::string variable = var.first;

			if (variable == "root")
			{
				ret[variable] = Utils::FileSystem::getParent(relativeTo);
				continue;
			}

			if (variable == "log")
			{
				ret[variable] = mUserEmulationStationPath;
				continue;
			}

#if WIN32
			if (variable == "kodi")
			{
				if (Utils::FileSystem::exists("C:\\Program Files\\Kodi\\kodi.exe"))
					ret[variable] = "C:\\Program Files\\Kodi\\kodi.exe";
				else if (Utils::FileSystem::exists("C:\\Program Files (x86)\\Kodi\\kodi.exe"))
					ret[variable] = "C:\\Program Files (x86)\\Kodi\\kodi.exe";
				else if (Utils::FileSystem::exists(Utils::FileSystem::combine(Utils::FileSystem::getParent(relativeTo), "kodi\\kodi.exe")))
					ret[variable] = Utils::FileSystem::combine(Utils::FileSystem::getParent(relativeTo), "kodi\\kodi.exe");

				continue;
			}
#endif

			if (Utils::String::startsWith(variable, "system."))
			{
				auto name = Utils::FileSystem::getGenericPath(variable.substr(7));

				auto dir = Utils::FileSystem::getCanonicalPath(mUserEmulationStationPath + "/" + name);
				if (Utils::FileSystem::isDirectory(dir))
					ret[variable] = dir;
				else
				{
					auto dir2 = Utils::FileSystem::getCanonicalPath(relativeTo + "/../system/" + name); // rinominato dir
					if (Utils::FileSystem::isDirectory(dir2))
						ret[variable] = dir2;
				}
			}
			else
			{
				auto dir = Utils::FileSystem::getCanonicalPath(relativeTo + "/../" + variable);
				if (Utils::FileSystem::isDirectory(dir))
					ret[variable] = dir;
				else
				{
					auto dir2 = Utils::FileSystem::getCanonicalPath(relativeTo + "/../system/" + variable); // rinominato dir
					if (Utils::FileSystem::isDirectory(dir2))
						ret[variable] = dir2;
				}
			}
		}

		for (auto var : files)
		{
			auto file = Utils::FileSystem::resolveRelativePath(var.first, relativeTo, true);
			if (Utils::FileSystem::exists(file) && !Utils::FileSystem::isDirectory(file))
				ret[var.first] = file;
		}
	}
	else
	{
		std::string line;
		std::ifstream systemConf(path);
		if (systemConf && systemConf.is_open())
		{
			while (std::getline(systemConf, line))
			{
				int idx = line.find("=");
				if (idx == (int)std::string::npos || line.find("#") == 0 || line.find(";") == 0) // cast a int per confronto
					continue;

				std::string key = line.substr(0, idx);
				std::string value = Utils::String::replace(line.substr(idx + 1), "\\", "/");
				if (!key.empty() && !value.empty())
				{
					auto dir = Utils::FileSystem::resolveRelativePath(value, relativeTo, true);
					if (Utils::FileSystem::isDirectory(dir)) // Dovrebbe essere isDirectory o exists a seconda del tipo
						ret[key] = dir;
				}
			}
			systemConf.close();
		}
	}

	for (auto vv : folders)
	{
		auto it = ret.find(vv.first);
		if (it == ret.cend())
			continue;
		(*vv.second) = it->second;
	}

	for (auto vv : files)
	{
		auto it = ret.find(vv.first);
		if (it == ret.cend())
			continue;
		(*vv.second) = it->second;
	}
}


// --- IMPLEMENTAZIONE DELLE NUOVE FUNZIONI ---

// Metodi statici pubblici (chiamano le versioni _impl)
std::string Paths::getGamelistRecoveryPath(const SystemData* system) {
    if (getInstance() == nullptr) {
        LOG(LogError) << "Paths::getInstance() è nullo in getGamelistRecoveryPath(system)!";
        return ""; 
    }
    return getInstance()->getGamelistRecoveryPath_impl(system);
}

std::string Paths::getGamelistRecoveryPath() { 
    if (getInstance() == nullptr) {
        LOG(LogError) << "Paths::getInstance() è nullo in getGamelistRecoveryPath()!";
        return "";
    }
    return getInstance()->getGamelistRecoveryPath_impl();
}

// Metodi _impl privati (non statici)
std::string Paths::getGamelistRecoveryPath_impl(const SystemData* system) const {
    // Usa mUserEmulationStationPath, che dovrebbe essere inizializzato nel costruttore.
    // Se mUserEmulationStationPath è vuoto, significa che l'inizializzazione di Paths non è completa o è fallita.
    if (mUserEmulationStationPath.empty()) {
        LOG(LogError) << "Paths::getGamelistRecoveryPath_impl(system): mUserEmulationStationPath è vuoto! Impossibile costruire il percorso di recovery.";
        return ""; // Ritorna una stringa vuota per indicare errore
    }

    std::string recovery_path = mUserEmulationStationPath + "/recovery"; // Cartella base di recovery

    if (system != nullptr && !system->getName().empty()) {
        // Sanitizza il nome del sistema per creare una sottocartella valida
        // Utils::FileSystem::createValidFileName dovrebbe essere disponibile se l'hai aggiunto.
        std::string systemFolderName = Utils::FileSystem::createValidFileName(system->getName());
        recovery_path += "/" + systemFolderName;
    } else {
        // Se system è nullo o non ha nome, usiamo solo la cartella base /recovery.
        LOG(LogDebug) << "Paths::getGamelistRecoveryPath_impl(system): SystemData è nullo o nome vuoto, uso path di recovery generale: " << recovery_path;
    }

    Utils::FileSystem::createDirectory(recovery_path); // Assicura che la directory esista
    LOG(LogDebug) << "Paths::getGamelistRecoveryPath_impl(system) -> Ritorno path: [" << recovery_path << "]";
    return recovery_path;
}

std::string Paths::getGamelistRecoveryPath_impl() const {
    if (mUserEmulationStationPath.empty()) {
        LOG(LogError) << "Paths::getGamelistRecoveryPath_impl(): mUserEmulationStationPath è vuoto! Impossibile costruire il percorso di recovery.";
        return "";
    }
    std::string recovery_path = mUserEmulationStationPath + "/recovery";
    Utils::FileSystem::createDirectory(recovery_path);
    LOG(LogDebug) << "Paths::getGamelistRecoveryPath_impl() -> Ritorno path: [" << recovery_path << "]";
    return recovery_path;
}
// --- FINE NUOVE IMPLEMENTAZIONI ---


// Implementazioni di getHomePath, setHomePath, getExePath, setExePath (dal tuo file)
// Assicurati che queste variabili statiche siano definite solo una volta, solitamente in Paths.cpp
static std::string static_homePath_variable; // Rinominata per evitare conflitti
static std::string static_exePath_variable;  // Rinominata per evitare conflitti

std::string& Paths::getHomePath()
{
	if (static_homePath_variable.length()) // Usa la variabile rinominata
		return static_homePath_variable;

#ifdef WIN32
	if (static_homePath_variable.empty()) // Usa la variabile rinominata
	{
		std::string portableCfg = getExePath() + "/.emulationstation/es_systems.cfg";
		if (Utils::FileSystem::exists(portableCfg))
			static_homePath_variable = getExePath(); // Usa la variabile rinominata
	}
#endif

	char* envHome = getenv("HOME");
	if (envHome)
		static_homePath_variable = Utils::FileSystem::getGenericPath(envHome); // Usa la variabile rinominata

#ifdef WIN32
	if (static_homePath_variable.empty()) // Usa la variabile rinominata
	{
		char* envHomeDrive = getenv("HOMEDRIVE");
		char* envHomePath = getenv("HOMEPATH");
		if (envHomeDrive && envHomePath)
			static_homePath_variable = Utils::FileSystem::getGenericPath(std::string(envHomeDrive) + "/" + envHomePath); // Usa la variabile rinominata
	}
#endif 

	if (static_homePath_variable.empty()) // Usa la variabile rinominata
		static_homePath_variable = Utils::FileSystem::getCWDPath(); // Usa la variabile rinominata

	static_homePath_variable = Utils::FileSystem::getGenericPath(static_homePath_variable); // Usa la variabile rinominata
	return static_homePath_variable; // Usa la variabile rinominata
}


void Paths::setHomePath(const std::string& _path)
{
	static_homePath_variable = Utils::FileSystem::getGenericPath(_path); // Usa la variabile rinominata
}


std::string& Paths::getExePath()
{
    // L'implementazione di getExePath nel tuo file caricato è vuota.
    // Solitamente, setExePath imposta una variabile statica e getExePath la ritorna.
    // Se static_exePath_variable non è impostata da setExePath prima di questa chiamata,
    // ritornerà una stringa vuota o un comportamento indefinito.
    // Assicurati che setExePath venga chiamata all'avvio dell'applicazione (es. in main.cpp).
	return static_exePath_variable; // Usa la variabile rinominata
}

void Paths::setExePath(const std::string& _path)
{
	std::string path_val = Utils::FileSystem::getCanonicalPath(_path); // Rinomina per evitare shadowing
	if (Utils::FileSystem::isRegularFile(path_val))
		path_val = Utils::FileSystem::getParent(path_val);
	static_exePath_variable = Utils::FileSystem::getGenericPath(path_val); // Usa la variabile rinominata
}

std::string Paths::findEmulationStationFile(const std::string& fileName)
{
    // Questa funzione usa altri metodi statici di Paths.
    // Assicurati che getEmulationStationPath() e getUserEmulationStationPath()
    // siano implementati correttamente (probabilmente nel costruttore o tramite i loro _impl).
	std::string localVersionFile = Paths::getEmulationStationPath() + "/" + fileName;
	if (Utils::FileSystem::exists(localVersionFile))
		return localVersionFile;

	localVersionFile = Paths::getUserEmulationStationPath() + "/" + fileName;
	if (Utils::FileSystem::exists(localVersionFile))
		return localVersionFile;

    //getParent potrebbe restituire una stringa vuota se il path è già radice o malformato
	std::string parentUserESPath = Utils::FileSystem::getParent(Paths::getUserEmulationStationPath());
    if (!parentUserESPath.empty()) {
        localVersionFile = parentUserESPath + "/" + fileName;
        if (Utils::FileSystem::exists(localVersionFile))
            return localVersionFile;
    }

	return std::string();
}