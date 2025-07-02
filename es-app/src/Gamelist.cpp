#include "Gamelist.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <pugixml/src/pugixml.hpp>
#include "Genres.h"
#include "Paths.h"
#include <fstream> 

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

std::string getGamelistRecoveryPath(SystemData* system)
{
	return Utils::FileSystem::getGenericPath(Paths::getUserEmulationStationPath() + "/recovery/" + system->getName());
}

static bool isPathActuallyVirtual(const std::string& path) {
    return Utils::String::startsWith(path, "epic:/") ||    // Assicurati che i tuoi prefissi reali siano "epic://" vs "epic:/"
           Utils::String::startsWith(path, "steam:/") ||   // Assicurati che i tuoi prefissi reali siano "steam://" vs "steam:/"
		   Utils::String::startsWith(path, "steam_online_appid:/") || 
           Utils::String::startsWith(path, "xbox_online_prodid:/") ||
           Utils::String::startsWith(path, "xbox_online_pfn:/") ||
           Utils::String::startsWith(path, "xbox:/pfn/") || // Se usi anche questo formato generale
		   Utils::String::startsWith(path, "ea_virtual:/") ||			   // Aggiunto per i giochi virtuali EA
		   Utils::String::startsWith(path, "eaplay:/") ||
		   Utils::String::startsWith(path, "amazon_virtual:/") ||
           Utils::String::startsWith(path, "amazon_installed:/") ||
           Utils::String::startsWith(path, "gog_virtual:/") || // <-- AGGIUNGI
           Utils::String::startsWith(path, "gog_installed:/");  // <-- AGGIUNGI
}

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type, std::unordered_map<std::string, FileData*>& fileMap)
{
    // Priorità 1: Gestione dei percorsi VIRTUALI noti
    // Utilizza i prefissi ESATTI come appaiono nel tuo gamelist.xml e nei log.
    // Il log mostra "xbox_online_prodid:/" (singola slash)
    if (Utils::String::startsWith(path, "epic:/") ||         // Assumi "epic://"
        Utils::String::startsWith(path, "steam:/") ||        // Assumi "steam://"
		Utils::String::startsWith(path, "steam_online_appid:/") || 
        Utils::String::startsWith(path, "xbox_online_prodid:/") ||  // CORRETTO DAL LOG
        Utils::String::startsWith(path, "xbox_online_pfn:/") ||    // CORRETTO DAL LOG (presumendo stesso formato)
        Utils::String::startsWith(path, "xbox:/pfn/")  ||    // Altro formato virtuale Xbox, se usato
		Utils::String::startsWith(path, "ea_virtual:/") || 		// Aggiunto per i giochi virtuali EA
		Utils::String::startsWith(path, "eaplay:/") ||
        Utils::String::startsWith(path, "amazon_virtual:/") ||
		Utils::String::startsWith(path, "amazon_installed:/") ||
           Utils::String::startsWith(path, "gog_virtual:/") || // <-- AGGIUNGI
           Utils::String::startsWith(path, "gog_installed:/"))  // <-- AGGIUNGI
    {
        LOG(LogDebug) << "findOrCreateFile: Handling VIRTUAL path for system '" << system->getName() << "': " << path;
        FolderData* rootCheck = system->getRootFolder();
        FileData* existingItem = nullptr;

        auto map_check = fileMap.find(path);
        if (map_check != fileMap.end()) {
            LOG(LogDebug) << "findOrCreateFile: Found existing VIRTUAL item in fileMap: " << path;
            return map_check->second;
        }

        if (rootCheck != nullptr) {
            for (FileData* child : rootCheck->getChildren()) {
                if (child && child->getPath() == path) { // Aggiunto controllo child non nullo
                    existingItem = child;
                    break;
                }
            }
            if (existingItem) {
                LOG(LogDebug) << "findOrCreateFile: Found existing VIRTUAL item in root children: " << path;
                fileMap[path] = existingItem; // Cache it
                return existingItem;
            }
        }

        LOG(LogInfo) << "findOrCreateFile: Creating NEW VIRTUAL FileData for path: " << path << " with type from gamelist: " << type;
        FileType determinedType = type;
        if (type != GAME) {
             LOG(LogWarning) << "findOrCreateFile: Virtual path " << path << " (type " << type << ") is not GAME. Forcing to GAME type for store/online entries.";
             determinedType = GAME;
        }
        FileData* virtualItem = new FileData(determinedType, path, system);
        fileMap[path] = virtualItem;
        if (rootCheck) {
            rootCheck->addChild(virtualItem);
        } else {
            LOG(LogError) << "findOrCreateFile: Cannot add virtual item " << path << ", root folder is null for system " << system->getName();
        }
        return virtualItem;
    }

    // --- Gestione Percorsi NON VIRTUALI (incluse le eccezioni come AUMID per Xbox) ---
    // Questa parte della logica rimane come la tua versione originale che ora funziona per gli AUMID.

    auto pGame_cache = fileMap.find(path); // Rinominato per evitare conflitto con pGame sotto
    if (pGame_cache != fileMap.end()) {
        LOG(LogDebug) << "findOrCreateFile: Found item in fileMap cache for non-virtual path: " << path;
        return pGame_cache->second;
    }

    FolderData* root = system->getRootFolder();
    if (!root) {
        LOG(LogError) << "findOrCreateFile: Root folder is NULL for system " << system->getName() << " for non-virtual path processing: " << path;
        return nullptr;
    }

    bool contains = false; // Rilevante per la logica originale dei percorsi interni/esterni
    std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

    if (!contains) // Il path NON è contenuto nella cartella ROM principale del sistema
    {
        LOG(LogDebug) << "findOrCreateFile: Path '" << path << "' is external to system start path '" << root->getPath() << "' for system '" << system->getName() << "'.";
        FileData* newItem = nullptr;
        bool createItem = false;

        if (system->getName() == "epicgamestore") {
            LOG(LogDebug) << "findOrCreateFile: Handling external path for epicgamestore: " << path;
            bool pathActuallyExistsOnDisk = Utils::FileSystem::exists(path);
            // La tua logica per EpicAllowNotInstalled e controlli di esistenza/tipo
            if (!Settings::getInstance()->getBool("EpicAllowNotInstalled") && !pathActuallyExistsOnDisk) {
                 LOG(LogWarning) << "findOrCreateFile: External Epic path from gamelist does not exist: " << path << ". Ignoring.";
                 return nullptr;
            }
            LOG(LogInfo) << "findOrCreateFile: External Epic path " << path << " accepted. Exists on disk: " << pathActuallyExistsOnDisk;
            createItem = true;
            // Ulteriori tuoi controlli di validità per Epic qui...
            if (createItem) { // Supponendo che i controlli siano passati
                bool isDirectoryOnDisk = pathActuallyExistsOnDisk && Utils::FileSystem::isDirectory(path);
                if (type == FOLDER && !isDirectoryOnDisk && pathActuallyExistsOnDisk) {
                    LOG(LogWarning) << "findOrCreateFile: Epic path " << path << " type FOLDER in gamelist, but not a directory on disk. Ignoring."; return nullptr;
                }
                if (type == GAME && isDirectoryOnDisk) {
                     LOG(LogWarning) << "findOrCreateFile: Epic path " << path << " type GAME in gamelist, but is a directory on disk. Ignoring."; return nullptr;
                }
                if (type == GAME && !isDirectoryOnDisk && pathActuallyExistsOnDisk) {
                    if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) {
                        LOG(LogWarning) << "findOrCreateFile: External Epic game file extension unknown: " << path << ". Ignoring."; return nullptr;
                    }
                }
            }

        } else if (system->getName() == "xbox") {
            if (path.find('!') != std::string::npos) { // Euristica per AUMID
                LOG(LogInfo) << "findOrCreateFile: Handling Xbox AUMID path from gamelist as 'conceptually existent': " << path;
                if (type != GAME) {
                    LOG(LogWarning) << "findOrCreateFile: Xbox AUMID path " << path << " in gamelist has type " << type << ", forcing to GAME.";
                    type = GAME;
                }
                createItem = true;
            } else {
                // Se arriva qui, è un path esterno per Xbox, non un AUMID e non uno dei virtuali noti dall'inizio.
                LOG(LogWarning) << "findOrCreateFile: External path for Xbox system that is not an AUMID and not a known virtual prefix: " << path << ". This should have been caught by virtual path checks if it was a virtual URI. Ignoring.";
                return nullptr;
            }
			// --- INIZIO BLOCCO AGGIUNTO PER EA GAMES ---
} else if (system->getName() == "EAGamesStore") {
    // Se il percorso inizia con i nostri prefissi personalizzati, lo accettiamo come valido.
    if (Utils::String::startsWith(path, "ea_installed:/") || Utils::String::startsWith(path, "ea_virtual:/")) {
        LOG(LogInfo) << "findOrCreateFile: Handling EA Games path from gamelist: " << path;
        createItem = true; // Imposta a true per creare il FileData
    } else {
        LOG(LogWarning) << "File path \"" << path << "\" for EA Games system is not a valid EA path. Ignoring.";
        return nullptr;
    }
// --- FINE BLOCCO AGGIUNTO PER EA GAMES ---
			
        } else {
            LOG(LogWarning) << "File path \"" << path << "\" is outside system start path and not handled (not Epic, not Xbox AUMID). System: " << system->getName() << ". Ignoring.";
            return nullptr;
        }

        if (createItem) {
            LOG(LogInfo) << "findOrCreateFile: Creating FileData for accepted external/AUMID path: " << path << " of type " << type;
            newItem = (type == GAME) ? new FileData(GAME, path, system) : new FolderData(path, system);
            fileMap[path] = newItem;
            root->addChild(newItem);
            return newItem;
        }
        return nullptr; // Se createItem non è diventato true
    }
    else // Il percorso È contenuto in system->getStartPath() (ROM normali)
    {
        // Questa è la tua logica originale per i percorsi interni, che presumo sia corretta
        // e che hai ripristinato o verificato. La incollo dalla tua ultima versione fornita.
        LOG(LogDebug) << "findOrCreateFile: Processing internal path: " << path << " (Relative: " << relative << ")";
        auto pathList = Utils::FileSystem::getPathList(relative);
        if (pathList.empty()) {
            LOG(LogWarning) << "findOrCreateFile: Path list is empty for internal path: " << path;
            if (path == root->getPath() && root->getType() == type) {
                if(fileMap.find(path) == fileMap.end()) fileMap[path] = root;
                return root;
            }
            return NULL; // Modificato da return NULL;
        }

        auto path_it = pathList.begin();
        FolderData* treeNode = root;
        FileData* item = nullptr; // Riusato per il risultato finale del ciclo

        while(path_it != pathList.end())
        {
            std::string currentSegment = *path_it;
            std::string key = Utils::FileSystem::combine(treeNode->getPath(), currentSegment); // Path assoluto del segmento corrente
            FileData* childInMemory = nullptr; // Rinominato da 'item' per chiarezza

            for (auto child : treeNode->getChildren()) {
                if (child->getPath() == key) {
                    childInMemory = child;
                    break;
                }
            }

            bool isLastSegment = (std::next(path_it) == pathList.end());

            if (childInMemory != nullptr) // Segmento trovato in memoria
            {
                LOG(LogDebug) << "findOrCreateFile: Found existing child node in memory for segment: " << key;
                if (isLastSegment) { // Ultimo segmento, deve essere il nostro file/cartella
                    if (childInMemory->getType() == type && childInMemory->getPath() == path) { // Controllo aggiuntivo sul path completo
                        item = childInMemory; // Trovato!
                    } else {
                        LOG(LogWarning) << "findOrCreateFile: Final segment found in memory, but type/path mismatch. Gamelist Path: " << path << " (Type: " << type
                                      << "), Memory Item Path: " << childInMemory->getPath() << " (Type: " << childInMemory->getType() << "). Ignoring.";
                        return nullptr;
                    }
                } else { // Segmento intermedio
                    if (childInMemory->getType() == FOLDER) {
                        treeNode = static_cast<FolderData*>(childInMemory);
                    } else {
                        LOG(LogError) << "findOrCreateFile: Path conflict. In-memory intermediate segment " << key << " is a FILE, but gamelist path continues: " << path;
                        return nullptr;
                    }
                }
            }
            else // Segmento non trovato in memoria, controlla su disco e crealo se necessario
            {
                LOG(LogDebug) << "findOrCreateFile: No existing child node in memory for segment: " << key << ". Checking disk.";
                if (!Utils::FileSystem::exists(key)) { // 'key' è il path assoluto del segmento
                    LOG(LogWarning) << "findOrCreateFile: Path segment from gamelist (" << key << " for " << path << ") does not exist on disk. Ignoring entry.";
                    return nullptr;
                }

                FileType typeOnDisk = Utils::FileSystem::isDirectory(key) ? FOLDER : GAME;
                
                if (isLastSegment) {
                    if (typeOnDisk != type) {
                        LOG(LogWarning) << "findOrCreateFile: Type mismatch for final segment " << key << ". Gamelist Type: " << type << ", Disk Type: " << typeOnDisk << ". Ignoring entry.";
                        return nullptr;
                    }
                    if (type == GAME) {
                        if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) { // Usa 'path' (completo) per getExtension
                            LOG(LogWarning) << "findOrCreateFile: Game file extension for " << path << " is not known by system '" << system->getName() << "'. Ignoring entry.";
                            return nullptr;
                        }
                        item = new FileData(GAME, path, system); // Usa 'path' (completo)
                        if (item->isArcadeAsset()) {
                            LOG(LogDebug) << "findOrCreateFile: Arcade asset " << path << ". Skipping specific FileData creation logic here.";
                            delete item;
                            return nullptr;
                        }
                    } else { // type == FOLDER
                        item = new FolderData(path, system); // Usa 'path' (completo)
                    }
                    LOG(LogDebug) << "findOrCreateFile: Created NEW FileData for final segment on disk: " << path << " of type " << type;
                    treeNode->addChild(item);
                } else { // Segmento intermedio da creare
                    if (typeOnDisk != FOLDER) {
                        LOG(LogWarning) << "findOrCreateFile: Intermediate path segment " << key << " is not a folder on disk, but gamelist implies it should be. Ignoring entry.";
                        return nullptr;
                    }
                    FolderData* newIntermediateFolder = new FolderData(key, system); // Path assoluto del segmento
                    LOG(LogDebug) << "findOrCreateFile: Created NEW intermediate FolderData for segment on disk: " << key;
                    treeNode->addChild(newIntermediateFolder);
                    fileMap[key] = newIntermediateFolder; // Aggiungi le cartelle intermedie alla mappa
                    treeNode = newIntermediateFolder;
                }
            }
            if (isLastSegment) break; // Usciamo dal ciclo se abbiamo processato l'ultimo segmento
            path_it++;
        } // Fine ciclo while(path_it != pathList.end())

        if (item) { // Se 'item' è stato assegnato (cioè, il file/cartella finale è stato trovato o creato)
            if (fileMap.find(path) == fileMap.end()) fileMap[path] = item;
            return item;
        }
    }

    LOG(LogWarning) << "findOrCreateFile: Failed to process path: " << path << " (System: " << system->getName() << "). Reached end of function unexpectedly.";
    return nullptr;
}


std::vector<FileData*> loadGamelistFile(const std::string xmlpath, SystemData* system, std::unordered_map<std::string, FileData*>& fileMap, size_t checkSize, bool fromFile)
{
    std::vector<FileData*> ret;

    LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

    pugi::xml_document doc;
    pugi::xml_parse_result result = fromFile ? doc.load_file(WINSTRINGW(xmlpath).c_str()) : doc.load_string(xmlpath.c_str());

    if (!result)
    {
        LOG(LogError) << "Error parsing XML file \"" << xmlpath << "\"!\n	" << result.description();
        return ret;
    }

    pugi::xml_node rootNode = doc.child("gameList");
    if (!rootNode)
    {
        LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
        return ret;
    }

    if (checkSize != SIZE_MAX)
    {
        auto parentSize = rootNode.attribute("parentHash").as_uint();
        if (parentSize != checkSize)
        {
            LOG(LogWarning) << "gamelist size don't match !";
            return ret;
        }
    }

    std::string relativeTo = system->getStartPath();
    bool trustGamelist = Settings::ParseGamelistOnly();

    for (pugi::xml_node fileNode : rootNode.children())
    {
        FileType type = GAME;
        std::string tag = fileNode.name();

        if (tag == "folder")
            type = FOLDER;
        else if (tag != "game")
            continue;

        pugi::xml_node pathNode = fileNode.child("path");
        if (!pathNode) {
            LOG(LogWarning) << "Gamelist entry in \"" << xmlpath << "\" is missing <path> tag. Skipping node.";
            continue;
        }
        // NOTA: resolveRelativePath potrebbe non essere necessario o desiderabile per path assoluti/URI come gli AUMID o i virtuali.
        // Se 'pathNode.text().get()' è già il path corretto (es. AUMID o URI virtuale), usalo direttamente.
        // Se invece può contenere path relativi per ROM normali, allora resolveRelativePath è corretto.
        // Per semplicità, manteniamo resolveRelativePath, ma assicurati che non alteri gli AUMID/URI.
        // Se li altera, usa direttamente pathNode.text().get() per i casi speciali.
        std::string path = Utils::FileSystem::resolveRelativePath(pathNode.text().get(), relativeTo, false);
        if (path.empty() && !pathNode.text().empty()) { // Se resolveRelativePath ha fallito per un path non relativo (come un AUMID)
             path = pathNode.text().get(); // Usa il path grezzo dall'XML
             LOG(LogDebug) << "loadGamelistFile: Used raw path from XML for unresolvable path: " << path;
        }


        if (path.empty()) {
            LOG(LogWarning) << "Gamelist entry in \"" << xmlpath << "\" has empty or unresolvable <path>. Original XML path: " << pathNode.text().get() << ". Skipping node.";
            continue;
        }

        FileData* file = nullptr;

        LOG(LogDebug) << "loadGamelistFile: Processing XML entry for path: \"" << path << "\" (System: " << system->getName() << ", Type from XML: " << type << ", TrustGamelist: " << trustGamelist << ")";

        // La decisione di come creare/trovare il FileData è ora delegata a findOrCreateFile,
        // che è stato modificato per gestire AUMID e virtuali correttamente.
        file = findOrCreateFile(system, path, type, fileMap);

        if (file == nullptr)
        {
            LOG(LogWarning) << "loadGamelistFile: findOrCreateFile returned NULL for path \"" << path << "\" (System: " << system->getName() << "). Skipping this gamelist entry's metadata.";
            continue;
        }

        // Applica i metadati dall'XML al FileData ottenuto
        // Il controllo isArcadeAsset rimane se rilevante per la tua logica.
        // Se trustGamelist è true, potresti voler saltare il popolamento dei metadati per gli asset arcade se vengono gestiti diversamente.
        if (!file->isArcadeAsset() || !trustGamelist)
        {
            LOG(LogDebug) << "loadGamelistFile: Applying metadata from XML to FileData for path: " << path;
            MetaDataList& mdl = file->getMetadata();
            mdl.loadFromXML(type == FOLDER ? FOLDER_METADATA : GAME_METADATA, fileNode, system);
            mdl.migrate(file, fileNode);

            if (mdl.getName().empty()) {
                mdl.set(MetaDataId::Name, file->getDisplayName());
            }

            // Questo blocco per 'hidden' dovrebbe applicarsi solo a percorsi fisici reali.
            // La funzione helper isPathActuallyVirtual è usata qui.
            if (!trustGamelist && !isPathActuallyVirtual(path) && Utils::FileSystem::exists(path) && !file->getHidden() && Utils::FileSystem::isHidden(path)) {
                mdl.set(MetaDataId::Hidden, "true");
            }

            Genres::convertGenreToGenreIds(&mdl);

            if (checkSize != SIZE_MAX) { // Caricamento da file di recovery
                mdl.setDirty();
            } else { // Caricamento dal gamelist principale
                mdl.resetChangedFlag();
            }
            ret.push_back(file);
        } else if (trustGamelist && file->isArcadeAsset()) {
            LOG(LogDebug) << "loadGamelistFile: Skipping explicit metadata load for arcade asset (trustGamelist=true): " << path;
            // Considera se aggiungere `file` a `ret` anche in questo caso se deve essere indicizzato/conteggiato.
            // Se viene aggiunto, resetta il suo dirty flag: file->getMetadata().resetChangedFlag();
        }
    }

    LOG(LogInfo) << "Finished parsing XML file \"" << xmlpath << "\". Loaded " << ret.size() << " valid entries.";
    return ret;
}

void clearTemporaryGamelistRecovery(SystemData* system)
{	
	auto path = getGamelistRecoveryPath(system);
	Utils::FileSystem::deleteDirectoryFiles(path, true);
}

void parseGamelist(SystemData* system, std::unordered_map<std::string, FileData*>& fileMap)
{
	std::string xmlpath = system->getGamelistPath(false);

	auto size = Utils::FileSystem::getFileSize(xmlpath);
	if (size != 0)
		loadGamelistFile(xmlpath, system, fileMap, SIZE_MAX, true);

	auto files = Utils::FileSystem::getDirContent(getGamelistRecoveryPath(system), true);
	for (auto file : files)
		loadGamelistFile(file, system, fileMap, size, true);

	if (size != SIZE_MAX)
		system->setGamelistHash(size);	
}

bool addFileDataNode(pugi::xml_node& parent, FileData* file, const char* tag, SystemData* system, bool fullPaths = false)
{
	//create game and add to parent node
	pugi::xml_node newNode = parent.append_child(tag);

	//write metadata
	file->getMetadata().appendToXML(newNode, true, system->getStartPath(), fullPaths);

	if(newNode.children().begin() == newNode.child("name") //first element is name
		&& ++newNode.children().begin() == newNode.children().end() //theres only one element
		&& newNode.child("name").text().get() == file->getDisplayName()) //the name is the default
	{
		//if the only info is the default name, don't bother with this node
		//delete it and ultimately do nothing
		parent.remove_child(newNode);
		return false;
	}

	if (fullPaths)
		newNode.prepend_child("path").text().set(file->getPath().c_str());
	else
	{
		// there's something useful in there so we'll keep the node, add the path
		// try and make the path relative if we can so things still work if we change the rom folder location in the future
		std::string path = Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(), false).c_str();
		if (path.empty() && file->getType() == FOLDER)
			path = ".";

		newNode.prepend_child("path").text().set(path.c_str());
	}
	return true;	
}

bool saveToXml(FileData* file, const std::string& fileName, bool fullPaths)
{
	SystemData* system = file->getSourceFileData()->getSystem();
	if (system == nullptr)
		return false;	

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("gameList");

	const char* tag = file->getType() == GAME ? "game" : "folder";

	root.append_attribute("parentHash").set_value(system->getGamelistHash());

	if (addFileDataNode(root, file, tag, system, fullPaths))
	{
		std::string folder = Utils::FileSystem::getParent(fileName);
		if (!Utils::FileSystem::exists(folder))
			Utils::FileSystem::createDirectory(folder);

		Utils::FileSystem::removeFile(fileName);
		if (!doc.save_file(WINSTRINGW(fileName).c_str()))
		{
			LOG(LogError) << "Error saving metadata to \"" << fileName << "\" (for system " << system->getName() << ")!";
			return false;
		}

		return true;
	}

	return false;
}

bool saveToGamelistRecovery(FileData* file)
{
    if (!Settings::getInstance()->getBool("SaveGamelistsOnExit"))
        return false;

    if (!file || !file->getSourceFileData() || !file->getSourceFileData()->getSystem()) {
        LOG(LogWarning) << "[Gamelist saveToGamelistRecovery] File, SourceFileData o SystemData nullo. Impossibile salvare il recovery.";
        return false;
    }

    SystemData* system = file->getSourceFileData()->getSystem();
    if (!Settings::HiddenSystemsShowGames() && !system->isVisible())
        return false;

    std::string originalGamePath = file->getPath();
    
    // Ottieni la directory base per i file di recovery del sistema
    // La chiamata a Paths::getGamelistRecoveryPath(system) dovrebbe funzionare se Paths.h è incluso
    // e se non ci sono conflitti di namespace (es. se Gamelist.cpp ha 'using namespace Paths;')
    std::string recoveryDir = Paths::getGamelistRecoveryPath(system); // Usa Paths:: se necessario
    if (recoveryDir.empty()) {
        LOG(LogError) << "[Gamelist saveToGamelistRecovery] Impossibile ottenere la directory di recovery per il sistema: " << system->getName();
        return false;
    }
    if (!Utils::FileSystem::exists(recoveryDir)) { // Assicurati che la directory esista
         Utils::FileSystem::createDirectory(recoveryDir); // createDirectory dovrebbe creare anche i parenti
         if (!Utils::FileSystem::exists(recoveryDir)) {
              LOG(LogError) << "[Gamelist saveToGamelistRecovery] Impossibile creare la directory di recovery: " << recoveryDir;
              return false;
         }
    }

    // Sanitizza il path originale del gioco per creare un nome file VALIDO
    std::string safeBaseFileName = Utils::FileSystem::createValidFileName(originalGamePath); 
    
    std::string finalRecoveryPath = recoveryDir + "/" + safeBaseFileName + ".xml";

    LOG(LogDebug) << "[Gamelist saveToGamelistRecovery] Tentativo salvataggio. Original Path: [" << originalGamePath 
                  << "], Final Recovery Path: [" << finalRecoveryPath << "]";

  pugi::xml_document doc;
    pugi::xml_node gameNode = doc.append_child("game"); 
    if (!gameNode) {
        LOG(LogError) << "[Gamelist saveToGamelistRecovery] Impossibile creare il nodo <game> PugiXML per: " << finalRecoveryPath;
        return false; // o return; se void
    }

    SystemEnvironmentData* envData = system->getSystemEnvData();
    std::string startPath = envData ? envData->mStartPath : "";
    // Assicurati che il quarto parametro 'detailed' sia quello che vuoi (true di solito per i gamelist)
    file->getMetadata().appendToXML(gameNode, true, startPath, true); // Passa gameNode

  std::ofstream fileStream(finalRecoveryPath, std::ios_base::out | std::ios_base::trunc);
    if (!fileStream) { // Controlla se l'APERTURA del file è riuscita
        LOG(LogError) << "[Gamelist saveToGamelistRecovery] ERRORE nell'apertura del file XML per scrittura: \"" << finalRecoveryPath << "\"!";
        return false; // O la gestione dell'errore appropriata
    }

    // La funzione doc.save(std::ostream&...) è void. Non restituisce un valore booleano.
    doc.save(fileStream, "  ", pugi::format_default | pugi::format_write_bom, pugi::encoding_utf8);

    // Controlla lo stato dello stream DOPO la scrittura per verificare errori.
    if (!fileStream.good()) { // Se lo stream è andato in uno stato di errore durante la scrittura
        LOG(LogError) << "[Gamelist saveToGamelistRecovery] ERRORE durante la scrittura PugiXML su file stream per: \"" << finalRecoveryPath << "\"!";
        fileStream.close(); // Chiudi comunque lo stream
        // Considera di cancellare il file parzialmente scritto se è un errore critico
        // Utils::FileSystem::removeFile(finalRecoveryPath); 
        return false; 
    }
    fileStream.close();

    if (!fileStream.good()) { 
        LOG(LogWarning) << "[Gamelist saveToGamelistRecovery] Errore durante le operazioni di stream per: \"" << finalRecoveryPath << "\".";
        return false; 
    }

    LOG(LogDebug) << "[Gamelist saveToGamelistRecovery] File di recovery salvato: " << finalRecoveryPath;
    return true;
}

// La tua funzione removeFromGamelistRecovery MODIFICATA:
bool removeFromGamelistRecovery(FileData* file)
{
    if (!file || !file->getSourceFileData() || !file->getSourceFileData()->getSystem()) {
        LOG(LogWarning) << "[Gamelist removeFromGamelistRecovery] File, SourceFileData o SystemData nullo.";
        return false;
    }
    SystemData* system = file->getSourceFileData()->getSystem();

    std::string originalGamePath = file->getPath();
    std::string safeBaseFileName = Utils::FileSystem::createValidFileName(originalGamePath);
    
    std::string recoveryDir = Paths::getGamelistRecoveryPath(system); // Usa Paths:: se necessario
    if (recoveryDir.empty()) {
         LOG(LogWarning) << "[Gamelist removeFromGamelistRecovery] Impossibile ottenere la directory di recovery per il sistema: " << system->getName();
         return false;
    }

    std::string finalRecoveryPath = recoveryDir + "/" + safeBaseFileName + ".xml";

    LOG(LogDebug) << "[Gamelist removeFromGamelistRecovery] Tentativo rimozione file: [" << finalRecoveryPath << "]";

    if (Utils::FileSystem::exists(finalRecoveryPath))
        return Utils::FileSystem::removeFile(finalRecoveryPath);

    return false; 
}

bool hasDirtyFile(SystemData* system)
{
	if (system == nullptr || !system->isGameSystem() || (!Settings::HiddenSystemsShowGames() && !system->isVisible())) // || system->hasPlatformId(PlatformIds::IMAGEVIEWER))
		return false;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
		return false;

	for (auto file : rootFolder->getFilesRecursive(GAME | FOLDER))
		if (file->getMetadata().wasChanged())
			return true;

	return false;
}

void updateGamelist(SystemData* system)
{
	// We do this by reading the XML again, adding changes and then writing it back,
	// because there might be information missing in our systemdata which would then miss in the new XML.
	// We have the complete information for every game though, so we can simply remove a game
	// we already have in the system from the XML, and then add it back from its GameData information...

	if (system == nullptr || Settings::IgnoreGamelist())
		return;

	// System is not a game system, a collection, or is hidden (and settings hide games from hidden systems)
	if (!system->isGameSystem() || system->isCollection() || (!Settings::HiddenSystemsShowGames() && system->isHidden()))
		return;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
	{
		LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
		return;
	}

	std::vector<FileData*> dirtyFiles;
	
	// Get all files (games and folders) recursively, not just displayed ones, not from other systems, and not from virtual storage (unless explicitly needed elsewhere)
	auto files = rootFolder->getFilesRecursive(GAME | FOLDER, false, system, false);
	for (auto file : files)
	{
		// Ensure the FileData is for the current system and its metadata was changed
		if (file->getSystem() == system && file->getMetadata().wasChanged()) 
		{
			dirtyFiles.push_back(file);
		}
	}

	if (dirtyFiles.empty()) // Check if vector is empty, not size comparison to 0
	{
		clearTemporaryGamelistRecovery(system);
		return;
	}

	int numUpdated = 0;

	pugi::xml_document doc;
	pugi::xml_node root;
	std::string xmlReadPath = system->getGamelistPath(false);

	if(Utils::FileSystem::exists(xmlReadPath))
	{
		//parse an existing file first
		pugi::xml_parse_result result = doc.load_file(WINSTRINGW(xmlReadPath).c_str());
		if(!result)
		{
			LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();
			// Even if parsing fails, we'll try to create a new gamelist structure
			root = doc.append_child("gameList");
		}
		else
		{
			root = doc.child("gameList");
		}
		
		if(!root) // If gamelist node is still missing (e.g. empty file or incorrect root)
		{
			LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"! Creating new one.";
			// Remove potentially malformed content and start fresh
			if(doc.first_child()) doc.remove_child(doc.first_child());
			root = doc.append_child("gameList");
		}
	}
	else //set up an empty gamelist to append to		
		root = doc.append_child("gameList");

	std::map<std::string, pugi::xml_node> xmlMap;

	for (pugi::xml_node fileNode = root.first_child(); fileNode; fileNode = fileNode.next_sibling()) // Iterate safely
	{
		pugi::xml_node pathNode = fileNode.child("path"); // Renamed for clarity
		if (pathNode)
	    {
            std::string nodePathText = pathNode.text().get();
            std::string lookupPath;

            // MODIFIED SECTION for lookupPath
            if (Utils::String::startsWith(nodePathText, "epic:/") ||
                Utils::String::startsWith(nodePathText, "steam:/") ||
				Utils::String::startsWith(nodePathText, "steam_online_appid:/") || 
                Utils::String::startsWith(nodePathText, "xbox_online_prodid:/") ||
                Utils::String::startsWith(nodePathText, "xbox_online_pfn:/") ||
                Utils::String::startsWith(nodePathText, "xbox:/pfn/") ||
                Utils::String::startsWith(nodePathText, "ea_virtual:/") ||
                Utils::String::startsWith(nodePathText, "ea_installed:/") ||
                Utils::String::startsWith(nodePathText, "eaplay:/") ||
                Utils::String::startsWith(nodePathText, "amazon_virtual:/") || // Il tuo, corretto!
                Utils::String::startsWith(nodePathText, "amazon_installed:/") || // Aggiunto per completezza
		        Utils::String::startsWith(nodePathText, "gog_virtual:/") || // <-- AGGIUNGI
                Utils::String::startsWith(nodePathText, "gog_installed:/"))  // <-- AGGIUNGI

            {
                lookupPath = nodePathText; // Use the raw path for all known virtual prefixes
            } else {
                lookupPath = Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(nodePathText, system->getStartPath(), true));
            }
            xmlMap[lookupPath] = fileNode;
        }
    }
	
	// iterate through all files, checking if they're already in the XML
	for(auto file : dirtyFiles)
	{
		bool removed = false;

		// check if the file already exists in the XML
		// if it does, remove it before adding
	    std::string currentFilePath = file->getPath();
	    std::string lookupKey;

        // MODIFIED SECTION for lookupKey
        if (Utils::String::startsWith(currentFilePath, "epic:/") ||
            Utils::String::startsWith(currentFilePath, "steam:/") ||
            Utils::String::startsWith(currentFilePath, "xbox_online_prodid:/") ||
            Utils::String::startsWith(currentFilePath, "xbox_online_pfn:/") ||
           Utils::String::startsWith(currentFilePath, "xbox:/pfn/") ||
           Utils::String::startsWith(currentFilePath, "ea_virtual:/") ||
           Utils::String::startsWith(currentFilePath, "ea_installed:/") ||
           Utils::String::startsWith(currentFilePath, "eaplay:/") ||
           Utils::String::startsWith(currentFilePath, "amazon_virtual:/") || // Il tuo, corretto!
           Utils::String::startsWith(currentFilePath, "amazon_installed:/") || // Aggiunto per completezza
		   Utils::String::startsWith(currentFilePath, "gog_virtual:/") || // <-- AGGIUNGI
           Utils::String::startsWith(currentFilePath, "gog_installed:/"))  // <-- AGGIUNGI

        {
            lookupKey = currentFilePath; // Use the raw path for all known virtual prefixes
        } else {
            lookupKey = Utils::FileSystem::getCanonicalPath(currentFilePath);
        }

	    auto xmf = xmlMap.find(lookupKey); 
	    if (xmf != xmlMap.cend())
	    {
	        removed = true;
	        root.remove_child(xmf->second);
	        LOG(LogDebug) << "updateGamelist: Removed existing XML node for path: " << lookupKey;
	    } else {
	        LOG(LogDebug) << "updateGamelist: No existing XML node found for path: " << lookupKey << ". Will add as new.";
	    }
		
		const char* tag = (file->getType() == GAME) ? "game" : "folder";

		// it was either removed or never existed to begin with; either way, we can add it now
		if (addFileDataNode(root, file, tag, system))
		{
			// file->getMetadata().resetChangedFlag(); // Reset dirty flag if successfully added/updated in XML
			++numUpdated; 
		}
		else if (removed) // Node was removed, but addFileDataNode decided not to re-add (e.g. only default info)
		{
			// file->getMetadata().resetChangedFlag(); // Still, the change (removal) was processed
			++numUpdated; 
		}
	}

	// Now write the file
	if (numUpdated > 0) 
	{
		//make sure the folders leading up to this path exist (or the write will fail)
		std::string xmlWritePath(system->getGamelistPath(true));
		Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

		LOG(LogInfo) << "Added/Updated " << numUpdated << " entities in gamelist for system '" << system->getName() << "' (path: " << xmlReadPath << ")";

		if (!doc.save_file(WINSTRINGW(xmlWritePath).c_str(), "  ", pugi::format_default | pugi::format_write_bom, pugi::encoding_utf8)) // Added indentation and UTF-8 BOM
			LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
		else
		{
			// If save was successful, reset dirty flags for the processed files
			for(auto file : dirtyFiles) {
				file->getMetadata().resetChangedFlag();
			}
			clearTemporaryGamelistRecovery(system);
			system->setGamelistHash(Utils::FileSystem::getFileSize(xmlWritePath)); // Update hash
		}
	}
	else
	{
		LOG(LogInfo) << "No effective changes made to gamelist for system '" << system->getName() << "'.";
		clearTemporaryGamelistRecovery(system);
	}
}

void resetGamelistUsageData(SystemData* system)
{
	if (!system->isGameSystem() || system->isCollection() || (!Settings::HiddenSystemsShowGames() && !system->isVisible())) //  || system->hasPlatformId(PlatformIds::IMAGEVIEWER)
		return;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
	{
		LOG(LogError) << "resetGamelistUsageData : Found no root folder for system \"" << system->getName() << "\"!";
		return;
	}

	std::stack<FolderData*> stack;
	stack.push(rootFolder);

	while (stack.size())
	{
		FolderData* current = stack.top();
		stack.pop();

		for (auto it : current->getChildren())
		{
			if (it->getType() == FOLDER)
			{
				stack.push((FolderData*)it);
				continue;
			}
						
			it->setMetadata(MetaDataId::GameTime, "");
			it->setMetadata(MetaDataId::PlayCount, "");
			it->setMetadata(MetaDataId::LastPlayed, "");
		}
	}
}

void cleanupGamelist(SystemData* system)
{
	if (!system->isGameSystem() || system->isCollection() || (!Settings::HiddenSystemsShowGames() && !system->isVisible())) //  || system->hasPlatformId(PlatformIds::IMAGEVIEWER)
		return;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
	{
		LOG(LogError) << "CleanupGamelist : Found no root folder for system \"" << system->getName() << "\"!";
		return;
	}

	std::string xmlReadPath = system->getGamelistPath(false);
	if (!Utils::FileSystem::exists(xmlReadPath))
		return;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(WINSTRINGW(xmlReadPath).c_str());
	if (!result)
	{
		LOG(LogError) << "CleanupGamelist : Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();
		return;
	}

	pugi::xml_node root = doc.child("gameList");
	if (!root)
	{
		LOG(LogError) << "CleanupGamelist : Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
		return;
	}

	std::map<std::string, FileData*> fileMap;
	for (auto file : rootFolder->getFilesRecursive(GAME | FOLDER, false, nullptr, false))
		fileMap[Utils::FileSystem::getCanonicalPath(file->getPath())] = file;

	bool dirty = false;

	std::set<std::string> knownXmlPaths;
	std::set<std::string> knownMedias;

	for (pugi::xml_node fileNode = root.first_child(); fileNode; )
	{
		pugi::xml_node next = fileNode.next_sibling();

		pugi::xml_node path = fileNode.child("path");
		if (!path)
		{
			dirty = true;
			root.remove_child(fileNode);
			fileNode = next;
			continue;
		}
		
		std::string gamePath = Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(path.text().get(), system->getStartPath(), true));

		auto file = fileMap.find(gamePath);
		if (file == fileMap.cend())
		{
			dirty = true;
			root.remove_child(fileNode);
			fileNode = next;
			continue;
		}

		knownXmlPaths.insert(gamePath);

		for (auto mdd : MetaDataList::getMDD())
		{
			if (mdd.type != MetaDataType::MD_PATH)
				continue;

			pugi::xml_node mddPath = fileNode.child(mdd.key.c_str());

			std::string mddFullPath = (mddPath ? Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(mddPath.text().get(), system->getStartPath(), true)) : "");
			if (!Utils::FileSystem::exists(mddFullPath))
			{
				std::string ext = ".jpg";
				std::string folder = "/images/";
				std::string suffix;

				switch (mdd.id)
				{
				case MetaDataId::Image: suffix = "image"; break;
				case MetaDataId::Thumbnail: suffix = "thumb"; break;
				case MetaDataId::Marquee: suffix = "marquee"; break;
				case MetaDataId::Video: suffix = "video"; folder = "/videos/"; ext = ".mp4"; break;
				case MetaDataId::FanArt: suffix = "fanart"; break;
				case MetaDataId::BoxBack: suffix = "boxback"; break;
				case MetaDataId::BoxArt: suffix = "box"; break;
				case MetaDataId::Wheel: suffix = "wheel"; break;
				case MetaDataId::TitleShot: suffix = "titleshot"; break;
				case MetaDataId::Manual: suffix = "manual"; folder = "/manuals/"; ext = ".pdf"; break;
				case MetaDataId::Magazine: suffix = "magazine"; folder = "/magazines/"; ext = ".pdf"; break;
				case MetaDataId::Map: suffix = "map"; break;
				case MetaDataId::Cartridge: suffix = "cartridge"; break;
				}

				if (!suffix.empty())
				{					
					std::string mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + "-"+ suffix + ext;

					if (ext == ".pdf" && !Utils::FileSystem::exists(mediaPath))
					{
						mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + ".pdf";
						if (!Utils::FileSystem::exists(mediaPath))
							mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + ".cbz";
					}
					else if (ext != ".jpg" && !Utils::FileSystem::exists(mediaPath))
						mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + ext;
					else if (ext == ".jpg" && !Utils::FileSystem::exists(mediaPath))
						mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + "-" + suffix + ".png";

					if (mdd.id == MetaDataId::Image && !Utils::FileSystem::exists(mediaPath))
						mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + ".jpg";
					if (mdd.id == MetaDataId::Image && !Utils::FileSystem::exists(mediaPath))
						mediaPath = system->getStartPath() + folder + file->second->getDisplayName() + ".png";

					if (Utils::FileSystem::exists(mediaPath))
					{
						auto relativePath = Utils::FileSystem::createRelativePath(mediaPath, system->getStartPath(), true);

						fileNode.append_child(mdd.key.c_str()).text().set(relativePath.c_str());

						LOG(LogInfo) << "CleanupGamelist : Add resolved " << mdd.key << " path " << mediaPath << " to game " << gamePath << " in system " << system->getName();
						dirty = true;

						knownMedias.insert(mediaPath);
						continue;
					}
				}

				if (mddPath)
				{
					LOG(LogInfo) << "CleanupGamelist : Remove " << mdd.key << " path to game " << gamePath << " in system " << system->getName();

					dirty = true;
					fileNode.remove_child(mdd.key.c_str());
				}

				continue;
			}

			knownMedias.insert(mddFullPath);
		}

		fileNode = next;
	}
	
	// iterate through all files, checking if they're already in the XML
	for (auto file : fileMap)
	{
		auto fileName = file.first;
		auto fileData = file.second;

		if (knownXmlPaths.find(fileName) != knownXmlPaths.cend())
			continue;

		const char* tag = (fileData->getType() == GAME) ? "game" : "folder";

		if (addFileDataNode(root, fileData, tag, system))
		{
			LOG(LogInfo) << "CleanupGamelist : Add " << fileName << " to system " << system->getName();
			dirty = true;
		}
	}

	// Cleanup unknown files in system rom path
	auto allFiles = Utils::FileSystem::getDirContent(system->getStartPath(), true);
	for (auto dirFile : allFiles)
	{
		if (knownXmlPaths.find(dirFile) != knownXmlPaths.cend())
			continue;

		if (knownMedias.find(dirFile) != knownMedias.cend())
			continue;

		if (dirFile.empty())
			continue;

		if (Utils::FileSystem::isDirectory(dirFile))
			continue;

		std::string parent = Utils::String::toLower(Utils::FileSystem::getFileName(Utils::FileSystem::getParent(dirFile)));
		if (parent != "images" && parent != "videos" && parent != "manuals" && parent != "downloaded_images" && parent != "downloaded_videos")
			continue;

		if (Utils::FileSystem::getParent(Utils::FileSystem::getParent(dirFile)) != system->getStartPath())
			if (Utils::FileSystem::getFileName(Utils::FileSystem::getParent(Utils::FileSystem::getParent(dirFile))) != "media")
				continue;

		std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(dirFile));
		if (ext == ".txt" || ext == ".xml" || ext == ".old")
			continue;
		
		LOG(LogInfo) << "CleanupGamelist : Remove unknown file " << dirFile << " to system " << system->getName();

		Utils::FileSystem::removeFile(dirFile);
	}

	// Now write the file
	if (dirty)
	{
		// Make sure the folders leading up to this path exist (or the write will fail)
		std::string xmlWritePath(system->getGamelistPath(true));
		Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));		

		std::string oldXml = xmlWritePath + ".old";
		Utils::FileSystem::removeFile(oldXml);
		Utils::FileSystem::copyFile(xmlWritePath, oldXml);

		if (!doc.save_file(WINSTRINGW(xmlWritePath).c_str()))
			LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
		else
			clearTemporaryGamelistRecovery(system);
	}
	else
		clearTemporaryGamelistRecovery(system);
}
