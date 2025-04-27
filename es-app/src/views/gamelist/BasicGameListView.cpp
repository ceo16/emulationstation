#include "views/gamelist/BasicGameListView.h"

#include "utils/FileSystemUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "SystemConf.h"
#include "FileData.h"
#include "LocaleES.h"
#include "GameNameFormatter.h"
#include "TextToSpeech.h"
#include "ThemeData.h"
#include "Window.h" 
#include "components/TextListComponent.h"
#include "Log.h"

BasicGameListView::BasicGameListView(Window* window, FolderData* root)
	: ISimpleGameListView(window, root), mList(window)
{
	mList.longMouseClick += this;

	mList.setSize(mSize.x(), mSize.y() * 0.8f);
	mList.setPosition(0, mSize.y() * 0.2f);
	mList.setDefaultZIndex(20);

	mList.setCursorChangedCallback([&](const CursorState& /*state*/) 
		{
		updateThemeExtrasBindings();
		  FileData* file = (mList.size() == 0 || mList.isScrolling()) ? NULL : mList.getSelected();
		  if (file != nullptr)
		    file->setSelectedGame();
		  
			if (mRoot->getSystem()->isCollection())
				updateHelpPrompts();
		});

	addChild(&mList);

	populateList(root->getChildrenListToDisplay());
}

void BasicGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);
	using namespace ThemeFlags;
	mList.applyTheme(theme, getName(), "gamelist", ALL);

	sortChildren();
}

void BasicGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if(change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

void BasicGameListView::populateList(const std::vector<FileData*>& files)
{
	updateHeaderLogoAndText(); // Invariato

	mList.clear(); // Invariato

	if (files.size() > 0) // Se ci sono file da mostrare
	{
		// Gestione cartella parent ".." (invariato)
		bool showParentFolder = false;
        if (mRoot != nullptr && mRoot->getSystem() != nullptr)
		    showParentFolder = mRoot->getSystem()->getShowParentFolder();

		if (showParentFolder && mCursorStack.size())
			mList.add(". .", createParentFolderData(), 0xFFFFFFFF); // Colore default per ".."


		// --- INIZIO MODIFICHE ---

		GameNameFormatter formatter(mRoot->getSystem()); // Invariato

		// Definisci i colori (Valori predefiniti - Modificali se necessario per il tuo tema!)
		unsigned int installedColor = 0x303030FF; // Grigio scuro quasi nero (RGBA)
		unsigned int uninstalledColor = 0xAAAAAAFF; // Grigio chiaro (RGBA)

		// Cicla sui FileData ricevuti nel vettore 'files'
		for (auto file : files) // 'file' è un FileData*
		{
			std::string name = formatter.getDisplayName(file); // Usa il nome formattato

			// Determina il colore
			unsigned int color = file->isInstalled() ? installedColor : uninstalledColor;

			// Aggiungi la voce alla lista visiva (mList) passando il colore
			mList.add(name, file, color);

		} // Fine ciclo for

		// --- FINE MODIFICHE ---


		// Gestione cursore se c'è ".." (invariato)
		if (showParentFolder && mCursorStack.size() && mList.size() > 1 && mList.getCursorIndex() == 0)
			mList.setCursorIndex(1);
	}
	else // Se 'files' è vuoto
	{
		addPlaceholder(); // Invariato
	}

	updateFolderPath(); // Invariato

	if (mShowing) // Invariato
		onShow();
}
FileData* BasicGameListView::getCursor()
{
	if (mList.size() == 0)
		return nullptr;

	return mList.getSelected();
}

std::shared_ptr<std::vector<FileData*>> recurseFind(FileData* toFind, FolderData* folder, std::stack<FileData*>& stack)
{
	auto items = folder->getChildrenListToDisplay();

	for (auto item : items)
		if (toFind == item)
			return std::make_shared<std::vector<FileData*>>(items);

	for (auto item : items)
	{
		if (item->getType() == FOLDER)
		{
			stack.push(item);

			auto ret = recurseFind(toFind, (FolderData*)item, stack);
			if (ret != nullptr)
				return ret;

			stack.pop();
		}
	}

	if (stack.empty())
		return std::make_shared<std::vector<FileData*>>(items);

	return nullptr;
}

void BasicGameListView::resetLastCursor()
{
	mList.resetLastCursor();
}

void BasicGameListView::setCursor(FileData* cursor)
{
	if (cursor && !mList.setCursor(cursor) && !cursor->isPlaceHolder())
	{
		std::stack<FileData*> stack;
		auto childrenToDisplay = mRoot->findChildrenListToDisplayAtCursor(cursor, stack);
		if (childrenToDisplay != nullptr)
		{
			mCursorStack = stack;
			populateList(*childrenToDisplay.get());
			mList.setCursor(cursor);
			TextToSpeech::getInstance()->say(cursor->getName());
		}
	}
}

void BasicGameListView::addPlaceholder()
{
	// empty list - add a placeholder
	FileData* placeholder = createNoEntriesPlaceholder();
	mList.add(placeholder->getName(), placeholder, true);
}

std::string BasicGameListView::getQuickSystemSelectRightButton()
{
	return "right";
}

std::string BasicGameListView::getQuickSystemSelectLeftButton()
{
	return "left";
}

void BasicGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void BasicGameListView::remove(FileData *game)
{
	mList.remove(game);

	mRoot->removeFromVirtualFolders(game);
	delete game;                                 // remove before repopulating (removes from parent)

	if (mList.size() == 0)
		addPlaceholder();

	ViewController::get()->reloadGameListView(this);
}

void BasicGameListView::setCursorIndex(int cursor)
{
	mList.setCursorIndex(cursor);
}

int BasicGameListView::getCursorIndex()
{
	return mList.getCursorIndex();
}

std::vector<FileData*> BasicGameListView::getFileDataEntries()
{
	return mList.getObjects();	
}


void BasicGameListView::onLongMouseClick(GuiComponent* component)
{
	if (component != &mList)
		return;

	if (Settings::getInstance()->getBool("GameOptionsAtNorth"))
		showSelectedGameSaveSnapshots();
	else
		showSelectedGameOptions();
}

bool BasicGameListView::onMouseWheel(int delta)
{
	return mList.onMouseWheel(delta);
}

void BasicGameListView::onShow()
{
	ISimpleGameListView::onShow();

	if (typeid(*this) == typeid(BasicGameListView))
	{
		FileData* selectedGame = getCursor();
		if (selectedGame != nullptr)
			selectedGame->setSelectedGame();
	}
}