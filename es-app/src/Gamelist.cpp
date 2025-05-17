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

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type, std::unordered_map<std::string, FileData*>& fileMap)
{
    // --- Gestione Percorsi VIRTUALI --
    if (Utils::String::startsWith(path, "epic:/"))
    {
        FolderData* rootCheck = system->getRootFolder();
        if (rootCheck != nullptr)
        {
            FileData* existingChild = nullptr;
            for(FileData* child : rootCheck->getChildren()) {
                 if(child->getPath() == path) {
                      existingChild = child;
                      break;
                 }
            }
            if (existingChild != nullptr) {
                LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Epic) tramite ricerca figli root per path: " << path;
                fileMap[path] = existingChild; // Assicurati che sia aggiunto alla mappa locale se trovato così
                return existingChild;
            }
        }
        auto virtual_it = fileMap.find(path);
        if (virtual_it != fileMap.end()) {
            LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Epic) tramite mappa locale per path: " << path;
            return virtual_it->second;
        }
        LOG(LogInfo) << "findOrCreateFile: Creazione NUOVO FileData virtuale (Epic) per path: " << path;
        if (type != GAME) {
             LOG(LogWarning) << "findOrCreateFile: Virtual path (Epic) encountered for non-GAME type? Path: " << path;
             return NULL;
        }
        FileData* virtualItem = new FileData(GAME, path, system);
        fileMap[path] = virtualItem;
        if (rootCheck) { rootCheck->addChild(virtualItem); }
        else { LOG(LogError) << "findOrCreateFile: Could not get root folder for system " << system->getName() << " while adding virtual game (Epic)."; }
        return virtualItem;
    }
    else if (Utils::String::startsWith(path, "steam:/")) // <<< NUOVA SEZIONE PER STEAM
    {
        FolderData* rootCheck = system->getRootFolder(); // Il 'system' dovrebbe essere "steam"
        if (rootCheck != nullptr)
        {
            FileData* existingChild = nullptr;
            for(FileData* child : rootCheck->getChildren()) {
                 if(child->getPath() == path) { // Confronta il percorso esatto
                      existingChild = child;
                      break;
                 }
            }
            if (existingChild != nullptr) {
                LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Steam) tramite ricerca figli root per path: " << path;
                fileMap[path] = existingChild; // Assicurati che sia aggiunto alla mappa locale se trovato così
                return existingChild;
            }
        }

        auto virtual_it = fileMap.find(path);
        if (virtual_it != fileMap.end()) {
            LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Steam) tramite mappa locale per path: " << path;
            return virtual_it->second;
        }

        LOG(LogInfo) << "findOrCreateFile: Creazione NUOVO FileData virtuale (Steam) per path: " << path;
        if (type != GAME) { // I giochi degli store sono sempre di tipo GAME
             LOG(LogWarning) << "findOrCreateFile: Percorso virtuale Steam per tipo non GAME? Path: " << path;
             return NULL;
        }
        FileData* virtualItem = new FileData(GAME, path, system); // 'system' sarà l'oggetto SystemData per "steam"
        fileMap[path] = virtualItem; // Aggiungi alla mappa locale
        if (rootCheck) {
            rootCheck->addChild(virtualItem); // Aggiungi come figlio della root del sistema Steam
        } else {
            LOG(LogError) << "findOrCreateFile: Impossibile ottenere root folder per sistema " << system->getName() << " aggiungendo gioco virtuale Steam.";
        }
        return virtualItem;
    }
	
	 else if (Utils::String::startsWith(path, "xbox:/")) // <<< NUOVA CONDIZIONE PER XBOX
    {
        FolderData* rootCheck = system->getRootFolder(); 
        if (rootCheck != nullptr)
        {
            FileData* existingChild = nullptr;
            for(FileData* child : rootCheck->getChildren()) {
                 if(child->getPath() == path) {
                      existingChild = child;
                      break;
                 }
            }
            if (existingChild != nullptr) {
                LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Xbox) tramite ricerca figli root per path: " << path;
                fileMap[path] = existingChild;
                return existingChild;
            }
        }

        auto virtual_it = fileMap.find(path);
        if (virtual_it != fileMap.end()) {
            LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE (Xbox) tramite mappa locale per path: " << path;
            return virtual_it->second;
        }

        LOG(LogInfo) << "findOrCreateFile: Creazione NUOVO FileData virtuale (Xbox) per path: " << path;
        if (type != GAME) { 
             LOG(LogWarning) << "findOrCreateFile: Percorso virtuale Xbox per tipo non GAME? Path: " << path;
             return NULL; 
        }
        FileData* virtualItem = new FileData(GAME, path, system); 
        fileMap[path] = virtualItem; 
        if (rootCheck) { 
            rootCheck->addChild(virtualItem); 
        } else {
            LOG(LogError) << "findOrCreateFile: Impossibile ottenere root folder per sistema " << system->getName() << " aggiungendo gioco virtuale Xbox.";
        }
        return virtualItem;
    }
// --- FINE Gestione Percorsi VIRTUALI --


    // --- Se NON è VIRTUAL ---

    // Controlla cache prima di tutto (usando il path assoluto come chiave)
    auto pGame = fileMap.find(path);
    if (pGame != fileMap.end()) {
        LOG(LogDebug) << "findOrCreateFile: Found item in fileMap cache for path: " << path;
        return pGame->second;
    }

    FolderData* root = system->getRootFolder();
    bool contains = false;
    std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

    // --- INIZIO NUOVA GESTIONE SEPARATA PER ESTERNI/INTERNI ---
    // Questa logica gestisce i percorsi fisici esterni alla rom folder del sistema,
    // come nel caso dei giochi Epic installati in C:\Program Files\Epic Games.
    // Per Steam, se i giochi sono "virtuali" (steam://), questa sezione non dovrebbe essere raggiunta.
    // Se invece consideri un gioco Steam installato come "esterno" perché il suo path di installazione
    // è fuori dalla rom folder di EmulationStation, allora dovrai adattare questa logica
    // o assicurarti che i giochi Steam siano sempre gestiti come virtuali.
    // Data la gestione attuale di Epic, sembra che l'approccio preferito sia trattare
    // gli store games come entità virtuali basate su URI.
    if (!contains)
    {
        // La logica per epicgamestore qui permette percorsi fisici esterni.
        // Per Steam, se tutti i giochi sono `steam://...`, questa parte non è rilevante.
        // Se hai giochi Steam con percorsi *fisici* esterni che vuoi gestire,
        // dovresti aggiungere una condizione simile a `if (system->getName() == "steam")`.
        // Tuttavia, è più probabile che tu voglia che i giochi Steam siano sempre virtuali.
        if (system->getName() == "epicgamestore")
        {
            LOG(LogDebug) << "findOrCreateFile: Handling external path for epicgamestore system: " << path;
            bool pathExists = Utils::FileSystem::exists(path);
            if (!pathExists) {
                LOG(LogWarning) << "gameList: External game path from gamelist does not exist on disk, ignoring: " << path;
                return NULL;
            }
            bool isDirectory = Utils::FileSystem::isDirectory(path);

            if (type == FOLDER && !isDirectory) {
                LOG(LogWarning) << "gameList: Path specified as folder in gamelist, but is not a directory on disk: " << path;
                return NULL;
            }
            if (type == GAME && !isDirectory) {
                if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) {
                    LOG(LogWarning) << "gameList: External game file extension is not known by systemlist, ignoring: " << path;
                    return NULL;
                }
            }
            FileData* newItem = (type == GAME) ? (FileData*)new FileData(GAME, path, system) : (FileData*)new FolderData(path, system);
            if (newItem != nullptr) {
                fileMap[path] = newItem;
                root->addChild(newItem);
                LOG(LogDebug) << "findOrCreateFile: Successfully created/added external item: " << path;
                return newItem;
            } else {
                LOG(LogError) << "findOrCreateFile: Failed to create FileData/FolderData for external path: " << path;
                return NULL;
            }
        }
        else // Percorso esterno, ma NON è epicgamestore (o steam, se aggiunto sopra) -> Rifiuta
        {
             LOG(LogWarning) << "File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
             return NULL;
        }
    }
    // --- FINE NUOVA GESTIONE SEPARATA ---

    // --- Logica Originale (modificata leggermente) per Percorsi INTERNI (contains == true) ---
    // Questa parte gestisce i file e le cartelle che si trovano all'interno della rom folder configurata per il sistema.
    // Non dovrebbe essere impattata direttamente dalla gestione dei giochi virtuali Steam,
    // a meno che tu non stia cercando di mappare percorsi `steam://` a file fisici in modo complesso.
    LOG(LogDebug) << "findOrCreateFile: Processing internal path: " << path << " (Relative: " << relative << ")";
    auto pathList = Utils::FileSystem::getPathList(relative);
    if (pathList.empty()) {
         LOG(LogWarning) << "findOrCreateFile: Path list is empty for internal path: " << path;
         if (path == root->getPath() && root->getType() == type) return root;
         return NULL;
    }

	auto path_it = pathList.begin();
	FolderData* treeNode = root;

	while(path_it != pathList.end())
	{
        std::string currentSegment = *path_it;
        std::string key = Utils::FileSystem::combine(treeNode->getPath(), currentSegment);
		FileData* item = nullptr;

        for (auto child : treeNode->getChildren()) {
            if (child->getPath() == key) {
                item = child;
                break;
            }
        }

		if (item != nullptr)
		{
            LOG(LogDebug) << "findOrCreateFile: Found existing child node for segment: " << key;
			if (item->getType() == FOLDER) {
				treeNode = (FolderData*) item;
            } else {
                 if (path_it == --pathList.end()) {
                     if (item->getType() == type) {
                          if (fileMap.find(path) == fileMap.end()) fileMap[path] = item;
                          return item;
                     } else {
                          LOG(LogWarning) << "findOrCreateFile: Type mismatch for existing file " << path << " (Expected " << type << ", Got " << item->getType() << ")";
                          return NULL;
                     }
                 } else {
                     LOG(LogError) << "findOrCreateFile: Path continues after finding a file: " << path;
                     return NULL;
                 }
            }
		}
        else
        {
            LOG(LogDebug) << "findOrCreateFile: No existing child node for segment: " << key;
		    if(path_it == --pathList.end())
		    {
			    if(type == FOLDER)
			    {
				    if (!Utils::FileSystem::isDirectory(path)) {
					    LOG(LogWarning) << "gameList: folder from gamelist does not exist on disk, ignoring: " << path;
                        return NULL;
                    }
                    item = new FolderData(path, system);
                    fileMap[path] = item;
                    treeNode->addChild(item);
                    LOG(LogDebug) << "findOrCreateFile: Created FOLDER object for final segment: " << path;
                    return item;
			    }
			    else // type == GAME
			    {
				    if (!Utils::FileSystem::exists(path)) {
                        LOG(LogWarning) << "gameList: game file from gamelist does not exist on disk, ignoring: " << path;
                        return NULL;
                    }
                    if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) {
				        LOG(LogWarning) << "gameList: game file extension is not known by systemlist, ignoring: " << path;
				        return NULL;
                    }
				    item = new FileData(GAME, path, system);
				    if (!item->isArcadeAsset()) // isArcadeAsset è una logica specifica, mantienila se rilevante
				    {
					    fileMap[path] = item;
					    treeNode->addChild(item);
                        LOG(LogDebug) << "findOrCreateFile: Created GAME object for final segment: " << path;
				    } else {
                        LOG(LogDebug) << "findOrCreateFile: Ignoring arcade asset: " << path;
                        delete item;
                        item = nullptr;
                    }
				    return item;
			    }
		    }
            else
            {
                 if (Utils::FileSystem::isDirectory(key))
                 {
                      LOG(LogDebug) << "findOrCreateFile: Creating intermediate FOLDER object for: " << key;
                      FolderData* folder = new FolderData(key, system);
                      fileMap[key] = folder;
                      treeNode->addChild(folder);
                      treeNode = folder;
                 } else {
                      LOG(LogWarning) << "gameList: intermediate folder from gamelist does not exist on disk, cannot proceed: " << key;
                      return NULL;
                 }
            }
        }
		path_it++;
	}

    LOG(LogError) << "findOrCreateFile: Reached unexpected end after loop for internal path: " << path;
	return NULL;
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

	pugi::xml_node rootNode = doc.child("gameList"); // Rinominato per chiarezza
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

		const std::string path = Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(), relativeTo, false);

		FileData* file = nullptr;

		if (trustGamelist)
		{
			// Se ci fidiamo ciecamente della gamelist, cerchiamo o creiamo l'elemento
			file = findOrCreateFile(system, path, type, fileMap);
		}
		else // Se NON ci fidiamo (caso standard e quello problematico per Epic)
		{
			// --- NUOVO BLOCCO CORRETTO ---
			LOG(LogDebug) << "loadGamelistFile: Processing path (trustGamelist=false): " << path;
			auto mapEntry = fileMap.find(path);

			if (mapEntry != fileMap.end())
			{
				// Trovato nella mappa locale (già processato dal filesystem scan?)
				LOG(LogDebug) << "  Found path in fileMap.";
				file = mapEntry->second;
			}
			else
			{
				// Non trovato nella mappa locale, controlliamo esistenza e validità
				LOG(LogDebug) << "  Path NOT found in fileMap. Checking virtual/existence/type...";
				bool isVirtualPath = Utils::String::startsWith(path, "epic:/") || Utils::String::startsWith(path, "steam:") || Utils::String::startsWith(path, "xbox:/");
				bool pathExists = !isVirtualPath && Utils::FileSystem::exists(path);
				bool isDirectory = pathExists && Utils::FileSystem::isDirectory(path);
				bool isFile = pathExists && !isDirectory; // Assumiamo sia un file se esiste e non è directory

				LOG(LogDebug) << "  isVirtualPath: " << (isVirtualPath ? "true" : "false")
				              << ", pathExists: " << (pathExists ? "true" : "false")
				              << ", isDirectory: " << (isDirectory ? "true" : "false")
				              << ", isFile: " << (isFile ? "true" : "false");

				// Determina se il percorso è valido per essere processato ulteriormente
				bool isValidPath = false;
				if (isVirtualPath) {
					// I percorsi virtuali sono sempre considerati validi a questo punto
					isValidPath = true;
					LOG(LogDebug) << "  Path is VIRTUAL. Considered valid.";
				} else if (pathExists) {
					// Se il percorso esiste fisicamente
					if (isDirectory) {
						// Le directory sono valide (per FolderData o potenzialmente per tipi speciali di GameData)
						isValidPath = true;
						LOG(LogDebug) << "  Path EXISTS and is a DIRECTORY. Considered valid.";
					} else if (isFile) {
						// I file devono avere un'estensione valida definita per il sistema
						std::string ext = Utils::String::toLower(Utils::FileSystem::getExtension(path));
						if (system->getSystemEnvData()->isValidExtension(ext)) {
							isValidPath = true;
							LOG(LogDebug) << "  Path EXISTS, is a FILE, and has a VALID EXTENSION ('" << ext << "'). Considered valid.";
						} else {
							LOG(LogWarning) << "  Path EXISTS and is a FILE, but has INVALID extension ('" << ext << "'). Ignoring node.";
						}
					}
				} else {
					// Se non è virtuale e non esiste
					LOG(LogWarning) << "  Path is NOT virtual and DOES NOT EXIST. Ignoring node.";
				}

				// Se il percorso è valido (virtuale, directory esistente, o file esistente con estensione valida)
				if (isValidPath)
				{
					LOG(LogDebug) << "  Condition MET: Calling findOrCreateFile.";
					file = findOrCreateFile(system, path, type, fileMap); // Cerca o crea il FileData in memoria
					if (file != nullptr)
						LOG(LogDebug) << "   findOrCreateFile successful.";
					else // findOrCreateFile potrebbe fallire per altre ragioni (es. fuori dal path di sistema)
						LOG(LogWarning) << "   findOrCreateFile returned NULL for valid path: " << path;
				} else {
					// Se il percorso non è valido (inesistente, o file con estensione errata), salta questo nodo XML
					continue;
				}
			}
			// --- FINE NUOVO BLOCCO CORRETTO ---
		} // Fine blocco if(!trustGamelist)


		// Se non siamo riusciti a trovare o creare il FileData corrispondente, saltiamo questo nodo XML
		if (file == nullptr)
		{
			// findOrCreateFile dovrebbe aver già loggato un Warning/Error se necessario
			// LOG(LogError) << "Error finding/creating FileData for \"" << path << "\", skipping XML node.";
			continue;
		}

        // Se arriviamo qui, abbiamo un 'file' (FileData*) valido a cui applicare i metadati
		// (Il controllo isArcadeAsset è probabilmente superfluo se findOrCreateFile li gestisce)
		if (!trustGamelist || !file->isArcadeAsset())
		{
			MetaDataList& mdl = file->getMetadata();
			mdl.loadFromXML(type == FOLDER ? FOLDER_METADATA : GAME_METADATA, fileNode, system);
			mdl.migrate(file, fileNode); // Gestisce eventuali conversioni da formati vecchi

			// Assicura che il nome sia impostato se mancava
			if (mdl.getName().empty())
				mdl.set(MetaDataId::Name, file->getDisplayName());

			// Aggiorna lo stato 'hidden' se il file system dice che è nascosto ma la gamelist no
			if (!trustGamelist && !file->getHidden() && Utils::FileSystem::isHidden(path))
				mdl.set(MetaDataId::Hidden, "true");

			// Converte i generi stringa in ID (se applicabile)
			Genres::convertGenreToGenreIds(&mdl);

			// Gestisce lo stato 'dirty' (modificato)
			if (checkSize != SIZE_MAX) // Se stiamo caricando da un file di recovery
				mdl.setDirty();
			else // Se stiamo caricando il gamelist principale
				mdl.resetChangedFlag(); // Considera i dati come non modificati inizialmente

			ret.push_back(file); // Aggiungi alla lista dei file processati con successo
		}
		else if (trustGamelist && file->isArcadeAsset()) {
			// Se trustGamelist=true, gli asset arcade vengono creati da findOrCreateFile,
			// ma potremmo voler evitare di processare i loro metadati qui se già gestiti altrove.
			LOG(LogDebug) << "Skipping metadata load for arcade asset (trustGamelist=true): " << path;
		}

	} // Fine ciclo for sui nodi XML

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

	if (!system->isGameSystem() || system->isCollection() || (!Settings::HiddenSystemsShowGames() && system->isHidden()))
		return;

	FolderData* rootFolder = system->getRootFolder();
	if (rootFolder == nullptr)
	{
		LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
		return;
	}

	std::vector<FileData*> dirtyFiles;
	
	auto files = rootFolder->getFilesRecursive(GAME | FOLDER, false, nullptr, false);
	for (auto file : files)
		if (file->getSystem() == system && file->getMetadata().wasChanged())
			dirtyFiles.push_back(file);

	if (dirtyFiles.size() == 0)
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
			LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" << result.description();

		root = doc.child("gameList");
		if(!root)
		{
			LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlReadPath << "\"!";
			root = doc.append_child("gameList");
		}
	}
	else //set up an empty gamelist to append to		
		root = doc.append_child("gameList");

	std::map<std::string, pugi::xml_node> xmlMap;

	for (pugi::xml_node fileNode : root.children())
	{
		pugi::xml_node path = fileNode.child("path");
		if (path)
	    {
            // USA getCanonicalPath SOLO per percorsi NON virtuali
            std::string nodePathText = path.text().get();
            std::string lookupPath;
            if (Utils::String::startsWith(nodePathText, "epic://")) {
                lookupPath = nodePathText; // Usa il percorso grezzo per i virtuali
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
	  // Determina quale chiave usare per la ricerca
    std::string currentFilePath = file->getPath();
    std::string lookupKey;
    if (Utils::String::startsWith(currentFilePath, "epic://")) {
        lookupKey = currentFilePath; // Usa il percorso grezzo per i virtuali
    } else {
        lookupKey = Utils::FileSystem::getCanonicalPath(currentFilePath);
    }

    auto xmf = xmlMap.find(lookupKey); // Usa lookupKey invece di getCanonicalPath diretto
    if (xmf != xmlMap.cend())
    {
        removed = true;
        root.remove_child(xmf->second);
        // Log aggiuntivo (Opzionale ma utile)
        LOG(LogDebug) << "updateGamelist: Removed existing XML node for path: " << lookupKey;
    } else {
        // Log aggiuntivo (Opzionale ma utile)
         LOG(LogDebug) << "updateGamelist: No existing XML node found for path: " << lookupKey;
    }
		
		const char* tag = (file->getType() == GAME) ? "game" : "folder";

		// it was either removed or never existed to begin with; either way, we can add it now
		if (addFileDataNode(root, file, tag, system))
			++numUpdated; // Only if really added
		else if (removed)
			++numUpdated; // Only if really removed
	}

	// Now write the file
	if (numUpdated > 0) 
	{
		//make sure the folders leading up to this path exist (or the write will fail)
		std::string xmlWritePath(system->getGamelistPath(true));
		Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

		LOG(LogInfo) << "Added/Updated " << numUpdated << " entities in '" << xmlReadPath << "'";

		if (!doc.save_file(WINSTRINGW(xmlWritePath).c_str()))
			LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath << "\" (for system " << system->getName() << ")!";
		else
			clearTemporaryGamelistRecovery(system);
	}
	else
		clearTemporaryGamelistRecovery(system);
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
