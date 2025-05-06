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
    // --- Gestione Percorsi VIRTUALI (Invariato) ---
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
                LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE tramite ricerca figli root per path: " << path;
                fileMap[path] = existingChild;
                return existingChild;
            }
        }
        auto virtual_it = fileMap.find(path);
        if (virtual_it != fileMap.end()) {
            LOG(LogDebug) << "findOrCreateFile: Trovato FileData virtuale ESISTENTE tramite mappa locale per path: " << path;
            return virtual_it->second;
        }
        LOG(LogInfo) << "findOrCreateFile: Creazione NUOVO FileData virtuale per path: " << path;
        if (type != GAME) {
             LOG(LogWarning) << "findOrCreateFile: Virtual path encountered for non-GAME type? Path: " << path;
             return NULL;
        }
        FileData* virtualItem = new FileData(GAME, path, system);
        fileMap[path] = virtualItem;
        if (rootCheck) { rootCheck->addChild(virtualItem); }
        else { LOG(LogError) << "findOrCreateFile: Could not get root folder for system " << system->getName() << " while adding virtual game."; }
        return virtualItem;
    }
    // --- FINE Gestione Percorsi VIRTUALI ---

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
    if (!contains) // Il percorso è ESTERNO alla cartella root del sistema
    {
        if (system->getName() == "epicgamestore") // Ed è il sistema Epic
        {
            LOG(LogDebug) << "findOrCreateFile: Handling external path for epicgamestore system: " << path;
            bool pathExists = Utils::FileSystem::exists(path);
            if (!pathExists) {
                LOG(LogWarning) << "gameList: External game path from gamelist does not exist on disk, ignoring: " << path;
                return NULL;
            }
            bool isDirectory = Utils::FileSystem::isDirectory(path);

            // Verifica consistenza tipo
            if (type == FOLDER && !isDirectory) {
                LOG(LogWarning) << "gameList: Path specified as folder in gamelist, but is not a directory on disk: " << path;
                return NULL;
            }
            if (type == GAME && !isDirectory) { // File Game
                if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) {
                    LOG(LogWarning) << "gameList: External game file extension is not known by systemlist, ignoring: " << path;
                    return NULL;
                }
            }
            // (Implicitamente, se type==GAME e isDirectory==true, è OK per Epic; se type==FOLDER e isDirectory==true, è OK)

            // Crea e aggiungi direttamente alla root
            FileData* newItem = (type == GAME) ? (FileData*)new FileData(GAME, path, system) : (FileData*)new FolderData(path, system);
            if (newItem != nullptr) {
                fileMap[path] = newItem; // Aggiungi alla mappa locale usando path assoluto
                root->addChild(newItem); // Aggiungi come figlio diretto della root
                LOG(LogDebug) << "findOrCreateFile: Successfully created/added external item: " << path;
                return newItem; // *** IMPORTANTE: Termina qui per percorsi esterni ***
            } else {
                LOG(LogError) << "findOrCreateFile: Failed to create FileData/FolderData for external path: " << path;
                return NULL;
            }
        }
        else // Percorso esterno, ma NON è epicgamestore -> Rifiuta
        {
             LOG(LogWarning) << "File path \"" << path << "\" is outside system path \"" << system->getStartPath() << "\"";
             return NULL;
        }
    }
    // --- FINE NUOVA GESTIONE SEPARATA ---

    // --- Logica Originale (modificata leggermente) per Percorsi INTERNI (contains == true) ---
    LOG(LogDebug) << "findOrCreateFile: Processing internal path: " << path << " (Relative: " << relative << ")";
    auto pathList = Utils::FileSystem::getPathList(relative);
    if (pathList.empty()) {
         LOG(LogWarning) << "findOrCreateFile: Path list is empty for internal path: " << path;
         if (path == root->getPath() && root->getType() == type) return root; // Potrebbe essere la root stessa
         return NULL;
    }

	auto path_it = pathList.begin();
	FolderData* treeNode = root; // Nodo corrente nell'albero in memoria

	while(path_it != pathList.end())
	{
        std::string currentSegment = *path_it;
        std::string key = Utils::FileSystem::combine(treeNode->getPath(), currentSegment); // Percorso completo del segmento corrente
		FileData* item = nullptr;

        // Cerca tra i figli del nodo corrente se esiste già un FileData per questo segmento
        for (auto child : treeNode->getChildren()) {
            if (child->getPath() == key) {
                item = child;
                break;
            }
        }

		if (item != nullptr) // Trovato elemento esistente (figlio diretto di treeNode)
		{
            LOG(LogDebug) << "findOrCreateFile: Found existing child node for segment: " << key;
			if (item->getType() == FOLDER) {
				treeNode = (FolderData*) item; // Scendi nell'albero
            } else { // È un file
                 // Se siamo all'ultimo segmento, abbiamo trovato il file, altrimenti errore
                 if (path_it == --pathList.end()) {
                     // Verifica che il tipo corrisponda a quello richiesto dalla gamelist
                     if (item->getType() == type) {
                          // Aggiungi alla mappa se non c'è già (improbabile ma sicuro)
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
        else // item == nullptr -> Elemento NON trovato tra i figli di treeNode per questo segmento
        {
            LOG(LogDebug) << "findOrCreateFile: No existing child node for segment: " << key;
            // Dobbiamo creare il FileData/FolderData in memoria, ma solo se esiste sul disco

            // Se siamo all'ultimo segmento del percorso
		    if(path_it == --pathList.end())
		    {
			    if(type == FOLDER) // La Gamelist dice che questo è una cartella
			    {
				    if (!Utils::FileSystem::isDirectory(path)) { // Verifica sul disco
					    LOG(LogWarning) << "gameList: folder from gamelist does not exist on disk, ignoring: " << path;
                        return NULL;
                    }
                    // Crea FolderData in memoria
                    item = new FolderData(path, system);
                    fileMap[path] = item;
                    treeNode->addChild(item);
                    LOG(LogDebug) << "findOrCreateFile: Created FOLDER object for final segment: " << path;
                    return item;
			    }
			    else // type == GAME -> La Gamelist dice che questo è un gioco
			    {
                    // Verifica esistenza e estensione
				    if (!Utils::FileSystem::exists(path)) {
                        LOG(LogWarning) << "gameList: game file from gamelist does not exist on disk, ignoring: " << path;
                        return NULL;
                    }
                    if (!system->getSystemEnvData()->isValidExtension(Utils::String::toLower(Utils::FileSystem::getExtension(path)))) {
				        LOG(LogWarning) << "gameList: game file extension is not known by systemlist, ignoring: " << path;
				        return NULL;
                    }

				    // Crea FileData in memoria
				    item = new FileData(GAME, path, system);
				    if (!item->isArcadeAsset())
				    {
					    fileMap[path] = item;
					    treeNode->addChild(item);
                        LOG(LogDebug) << "findOrCreateFile: Created GAME object for final segment: " << path;
				    } else { // Ignora asset arcade in questa fase
                        LOG(LogDebug) << "findOrCreateFile: Ignoring arcade asset: " << path;
                        delete item;
                        item = nullptr;
                    }
				    return item;
			    }
		    }
            else // Non siamo all'ultimo elemento, dobbiamo creare una cartella intermedia
            {
                 // Verifica che la cartella intermedia esista sul disco
                 if (Utils::FileSystem::isDirectory(key))
                 {
                      LOG(LogDebug) << "findOrCreateFile: Creating intermediate FOLDER object for: " << key;
                      FolderData* folder = new FolderData(key, system);
                      fileMap[key] = folder; // Aggiungi la cartella intermedia alla mappa
                      treeNode->addChild(folder);
                      treeNode = folder; // Scendi nella cartella appena creata
                 } else {
                      LOG(LogWarning) << "gameList: intermediate folder from gamelist does not exist on disk, cannot proceed: " << key;
                      return NULL; // Non possiamo continuare se manca un pezzo del percorso
                 }
            }
        } // fine else (item == nullptr)

		path_it++;
	} // fine while

    LOG(LogError) << "findOrCreateFile: Reached unexpected end after loop for internal path: " << path;
	return NULL; // Non dovrebbe arrivare qui
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
				bool isVirtualPath = Utils::String::startsWith(path, "epic:/"); // Specifico per Epic
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

	SystemData* system = file->getSourceFileData()->getSystem();
	if (!Settings::HiddenSystemsShowGames() && !system->isVisible())
		return false;

	std::string fp = file->getFullPath();
	fp = Utils::FileSystem::createRelativePath(file->getFullPath(), system->getRootFolder()->getFullPath(), true);
	fp = Utils::FileSystem::getParent(fp) + "/" + Utils::FileSystem::getStem(fp) + ".xml";

	std::string path = Utils::FileSystem::getAbsolutePath(fp, getGamelistRecoveryPath(system));
	path = Utils::FileSystem::getCanonicalPath(path);

	return saveToXml(file, path);
}

bool removeFromGamelistRecovery(FileData* file)
{
	SystemData* system = file->getSourceFileData()->getSystem();
	if (system == nullptr)
		return false;

	std::string fp = file->getFullPath();
	fp = Utils::FileSystem::createRelativePath(file->getFullPath(), system->getRootFolder()->getFullPath(), true);
	fp = Utils::FileSystem::getParent(fp) + "/" + Utils::FileSystem::getStem(fp) + ".xml";

	std::string path = Utils::FileSystem::getAbsolutePath(fp, getGamelistRecoveryPath(system));
	path = Utils::FileSystem::getCanonicalPath(path);

	if (Utils::FileSystem::exists(path))
		return Utils::FileSystem::removeFile(path);

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
