#include "guis/GuiScraperStart.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "SystemData.h"
#include "scrapers/ThreadedScraper.h"
#include "LocaleES.h"
#include "GuiLoading.h"
#include "GuiScraperSettings.h"
#include "views/gamelist/IGameListView.h"


GuiScraperStart::GuiScraperStart(Window* window)
    : GuiSettings(window, _("SCRAPER").c_str()), mOverwriteMedias(true) // Inizializza qui
{
    LOG(LogDebug) << "GuiScraperStart constructor - START"; // Log inizio

    std::string scraperName = Settings::getInstance()->getString("Scraper");
    LOG(LogDebug) << "Initial global scraper setting: " << scraperName;

    // --- SOURCE GROUP ---
    addGroup(_("SOURCE"));

    // Scraper selection: Assegna al membro mScraperList
    mScraperList = std::make_shared< OptionListComponent< std::string > >(mWindow, _("SCRAPING DATABASE"), false);
    if (!mScraperList) { LOG(LogError) << "Failed to create mScraperList"; return; } // Controllo aggiuntivo

    for (auto engine : Scraper::getScraperList())
        mScraperList->add(engine, engine, engine == scraperName);

    if (!mScraperList->hasSelection() && !Scraper::getScraperList().empty())
    {
        LOG(LogWarning) << "Scraper setting '" << scraperName << "' not found in list, selecting first.";
        mScraperList->selectFirstItem();
        // Aggiorna scraperName con quello effettivamente selezionato
        scraperName = mScraperList->getSelected();
        Settings::getInstance()->setString("Scraper", scraperName); // Aggiorna anche il setting globale se non era valido
    } else if (Scraper::getScraperList().empty()) {
         LOG(LogError) << "No scrapers available!";
         // Gestire questo caso, magari mostrando un errore
    }

    addWithLabel(_("SCRAPE FROM"), mScraperList); // Usa mScraperList

    // Bottone per le impostazioni dello scraper
    addEntry(_("SCRAPER SETTINGS"), true, std::bind(&GuiScraperStart::onShowScraperSettings, this));

    // --- FILTERS GROUP ---
    addGroup(_("FILTERS"));

    // Filtro Media: Assegna a mFilters
    // Le lambda qui sono solo per la UI, la logica vera è in getSearches
    mFilters = std::make_shared< OptionListComponent<FilterFunc> >(mWindow, _("GAMES TO SCRAPE FOR"), false);
    if (!mFilters) { LOG(LogError) << "Failed to create mFilters"; return; }
    mFilters->add(_("ALL"), [](FileData*) -> bool { return true; }, false); // Placeholder lambda
    mFilters->add(_("GAMES MISSING ANY MEDIA"), [](FileData*) -> bool { return false; }, true);  // Placeholder lambda
    mFilters->add(_("GAMES MISSING ALL MEDIA"), [](FileData*) -> bool { return false; }, false); // Placeholder lambda
    addWithLabel(_("GAMES TO SCRAPE FOR"), mFilters); // Usa mFilters

    // Filtro Data: Assegna a mDateFilters
    // Le lambda qui sono solo per la UI
    mDateFilters = std::make_shared< OptionListComponent<FilterFunc> >(mWindow, _("IGNORE RECENTLY SCRAPED GAMES"), false);
    if (!mDateFilters) { LOG(LogError) << "Failed to create mDateFilters"; return; }
    int idx = 3; // Leggi magari da Settings se vuoi salvare questa preferenza
    mDateFilters->add(_("NO"), [](FileData*) -> bool { return true; }, idx == 0); // Placeholder
    mDateFilters->add(_("LAST DAY"), [](FileData*) -> bool { return false; }, idx == 1); // Placeholder
    mDateFilters->add(_("LAST WEEK"), [](FileData*) -> bool { return false; }, idx == 2); // Placeholder
    mDateFilters->add(_("LAST 15 DAYS"), [](FileData*) -> bool { return false; }, idx == 3); // Placeholder
    mDateFilters->add(_("LAST MONTH"), [](FileData*) -> bool { return false; }, idx == 4); // Placeholder
    mDateFilters->add(_("LAST 3 MONTHS"), [](FileData*) -> bool { return false; }, idx == 5); // Placeholder
    mDateFilters->add(_("LAST YEAR"), [](FileData*) -> bool { return false; }, idx == 6); // Placeholder
    addWithLabel(_("IGNORE RECENTLY SCRAPED GAMES"), mDateFilters); // Usa mDateFilters

    // Filtro Sistemi: Assegna a mSystems e RIMUOVI il filtro basato sullo scraper iniziale
    mSystems = std::make_shared<OptionListComponent<SystemData*>>(mWindow, _("SYSTEMS INCLUDED"), true);
    if (!mSystems) { LOG(LogError) << "Failed to create mSystems"; return; }

    std::string currentSystem;
    // ... (la tua logica per trovare currentSystem rimane uguale) ...
    if (ViewController::get()->getState().viewing == ViewController::GAME_LIST) { /* ... come prima ... */ }

    LOG(LogDebug) << "Populating system list...";
    int systemCount = 0;
    for (auto system : SystemData::sSystemVector) {
        // Mostra TUTTI i sistemi di gioco validi, senza filtrarli per scraper qui!
        if (system && system->isGameSystem() && !system->isCollection() && !system->hasPlatformId(PlatformIds::PLATFORM_IGNORE)) {
            bool select = false;
            if (!currentSystem.empty()) {
                select = (system->getName() == currentSystem);
            } else {
                // Seleziona di default solo se il sistema ha un PlatformID (euristica originale)
                // O potresti selezionarli tutti di default se currentSystem è vuoto
                 select = !system->getPlatformIds().empty();
            }
            mSystems->add(system->getFullName(), system, select);
            systemCount++;
        }
    }
    LOG(LogDebug) << "Added " << systemCount << " systems to the list.";
    addWithLabel(_("SYSTEMS INCLUDED"), mSystems); // Usa mSystems

    // --- CALLBACKS ---
    // Aggiorna l'impostazione globale quando l'utente cambia scraper
    mScraperList->setSelectedChangedCallback([this](const std::string& name) {
        LOG(LogDebug) << "Scraper selected in UI: " << name;
        Settings::getInstance()->setString("Scraper", name);
        // Qui potresti voler aggiornare dinamicamente le opzioni specifiche dello scraper
        // se implementi GuiScraperSettings per i nuovi scraper.
    });

    // Aggiorna mOverwriteMedias in base alla selezione del filtro media
    mFilters->setSelectedChangedCallback([this](const FilterFunc& /*filter*/) {
        int selectedIdx = mFilters->getSelectedIndex(); // Usa l'indice per determinare
        if (selectedIdx == 1) { // Indice di "GAMES MISSING ANY MEDIA"
            mOverwriteMedias = false;
        } else { // Indice di "ALL" (0) o "GAMES MISSING ALL MEDIA" (2)
            mOverwriteMedias = true;
        }
        LOG(LogDebug) << "mOverwriteMedias set to: " << (mOverwriteMedias ? "true" : "false") << " (Media Filter Index: " << selectedIdx << ")";
    });

    // --- Pulsanti ---
    mMenu.clearButtons();
    mMenu.addButton(_("SCRAPE NOW"), _("START"), std::bind(&GuiScraperStart::pressedStart, this));
    mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });

    // --- Posizione Menu ---
    if (Renderer::ScreenSettings::fullScreenMenus())
        mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
    else
        mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);

     LOG(LogDebug) << "GuiScraperStart constructor - END"; // Log fine
}

void GuiScraperStart::onShowScraperSettings()
{
	mWindow->pushGui(new GuiScraperSettings(mWindow));
}

void GuiScraperStart::pressedStart()
{
	std::vector<SystemData*> systems = mSystems->getSelectedObjects();
	for(auto system : systems)
	{
		if (system->getPlatformIds().empty())
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, 
				_("WARNING: SOME OF YOUR SELECTED SYSTEMS DO NOT HAVE A PLATFORM SET. RESULTS MAY BE FOR DIFFERENT SYSTEMS!\nCONTINUE ANYWAY?"),  
				_("YES"), std::bind(&GuiScraperStart::start, this),  
				_("NO"), nullptr)); 

			return;
		}
	}

	start();
}

void GuiScraperStart::start()
{
	if (ThreadedScraper::isRunning())
	{
		Window* window = mWindow;

		mWindow->pushGui(new GuiMsgBox(mWindow,
			_("SCRAPING IS RUNNING. DO YOU WANT TO STOP IT?"),
			_("YES"), [this, window] { ThreadedScraper::stop(); },
			_("NO"), nullptr));

		return;
	}

	mWindow->pushGui(new GuiLoading<std::queue<ScraperSearchParams>>(mWindow, _("PLEASE WAIT"),
		[this](IGuiLoadingHandler* gui)
		{
			return getSearches(mSystems->getSelectedObjects(), mFilters->getSelected(), mDateFilters->getSelected(), gui);
		},
		[this](std::queue<ScraperSearchParams> searches)
		{
			if (searches.empty())
				mWindow->pushGui(new GuiMsgBox(mWindow, _("NO GAMES FIT THAT CRITERIA.")));
			else
			{
				ThreadedScraper::start(mWindow, searches);
				close();
			}
		}));
}



std::queue<ScraperSearchParams> GuiScraperStart::getSearches(std::vector<SystemData*> systems, FilterFunc /*unusedMediaLambda*/, FilterFunc /*unusedDateLambda*/, IGuiLoadingHandler* handler)
{
    LOG(LogDebug) << "GuiScraperStart::getSearches - Start (Defensive Version)";
    std::queue<ScraperSearchParams> queue;

    try // Blocco try generale
    {
        // --- Controllo iniziale dei membri UI (ASSICURATI SIANO MEMBRI IN .h) ---
        if (!mScraperList || !mFilters || !mDateFilters || !mSystems) {
             LOG(LogError) << "GuiScraperStart::getSearches - Errore: Uno o più componenti UI membri (mScraperList, mFilters, mDateFilters, mSystems) è nullo!";
             mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE INTERNO DELLO SCRAPER (UI COMPONENT NULL).")));
             return queue;
        }
        LOG(LogDebug) << "GuiScraperStart::getSearches - UI components checked.";

        // --- Ottieni Scraper Selezionato ---
        std::string selectedScraperName = mScraperList->getSelected();
        Scraper* selectedScraper = Scraper::getScraper(selectedScraperName);

        if (!selectedScraper) {
             LOG(LogError) << "GuiScraperStart::getSearches - Scraper selezionato non valido: " << selectedScraperName;
             // Forse mostrare GuiMsgBox qui? O gestire dopo il ritorno di GuiLoading.
             return queue;
        }
        LOG(LogInfo) << "GuiScraperStart::getSearches - Scraper selezionato per questo lavoro: " << selectedScraperName;

        // --- Ottieni Indici Filtri ---
        int mediaFilterIndex = mFilters->getSelectedIndex();
        int dateFilterIndex = mDateFilters->getSelectedIndex();
        bool currentOverwriteSetting = mOverwriteMedias; // Questo dovrebbe essere aggiornato dal callback di mFilters

        LOG(LogDebug) << "GuiScraperStart::getSearches - MediaFilter Index: " << mediaFilterIndex << ", DateFilter Index: " << dateFilterIndex << ", Overwrite: " << (currentOverwriteSetting ? "true" : "false");

        int pos = 1;
        int totalGamesProcessed = 0;
        int totalGamesAddedToQueue = 0;
        int systemsSkipped = 0;
        int gamesSkippedByFilter = 0;
        int gamesErroredDuringFilter = 0;

        // --- Ciclo Sistemi ---
        for (auto system : systems)
        {
            if (!system) {
                LOG(LogWarning) << "GuiScraperStart::getSearches - Trovato puntatore a sistema nullo!";
                systemsSkipped++;
                continue;
            }
             LOG(LogDebug) << "GuiScraperStart::getSearches - Processando sistema: " << system->getName();

            // --- Controlla Supporto Sistema (con scraper selezionato) ---
            if (!selectedScraper->isSupportedPlatform(system)) {
                LOG(LogInfo) << "Sistema saltato '" << system->getName() << "' perché non supportato dallo scraper: " << selectedScraperName;
                systemsSkipped++;
                pos++; // Incrementa per il calcolo percentuale
                continue;
            }

            if (handler != nullptr) {
                 int percent = (pos * 100) / (systems.empty() ? 1 : systems.size());
                 handler->setText(_("PREPARAZIONE") + " (" + std::to_string(percent) + "%) - " + system->getName());
            }

            // --- Ottieni Lista Giochi (con try-catch) ---
            std::vector<FileData*> games;
            try {
                 if (!system->getRootFolder()) {
                      LOG(LogError) << "Errore: Root folder nulla per sistema '" << system->getName() << "'";
                      gamesErroredDuringFilter++; // Conta come errore
                      continue;
                 }
                 games = system->getRootFolder()->getFilesRecursive(GAME);
            } catch (const std::exception& e) {
                 LOG(LogError) << "Eccezione durante getFilesRecursive per sistema '" << system->getName() << "': " << e.what();
                 gamesErroredDuringFilter += games.size(); // Considera tutti i giochi persi
                 continue;
            } catch (...) {
                  LOG(LogError) << "Eccezione sconosciuta durante getFilesRecursive per sistema '" << system->getName() << "'.";
                  gamesErroredDuringFilter += games.size();
                  continue;
            }

            totalGamesProcessed += games.size();
            LOG(LogDebug) << "  Sistema '" << system->getName() << "' ha " << games.size() << " giochi da filtrare.";

            // --- Ciclo Giochi ---
            for(auto game : games)
            {
                 if (!game) {
                      LOG(LogWarning) << "  Trovato puntatore a gioco nullo nel sistema '" << system->getName() << "'!";
                      gamesErroredDuringFilter++;
                      continue;
                 }

                 // --- RIVALUTA FILTRI QUI USANDO selectedScraper e indici (con try-catch) ---
                bool mediaFilterPassed = true;
                bool dateFilterPassed = true;
                try
                {
                     // Filtro Media
                     if (mediaFilterIndex == 1) { // GAMES MISSING ANY MEDIA
                         mediaFilterPassed = selectedScraper->hasMissingMedia(game);
                     } else if (mediaFilterIndex == 2) { // GAMES MISSING ALL MEDIA
                         mediaFilterPassed = !selectedScraper->hasAnyMedia(game);
                     } // Index 0 ("ALL") -> mediaFilterPassed resta true

                     // Filtro Data
                     if (dateFilterIndex > 0) { // Se non è "NO" (Index 0)
                         auto now = Utils::Time::now();
                         int days = 0;
                         if (dateFilterIndex == 1) days = 1;
                         else if (dateFilterIndex == 2) days = 7;
                         else if (dateFilterIndex == 3) days = 15;
                         else if (dateFilterIndex == 4) days = 31;
                         else if (dateFilterIndex == 5) days = 90;
                         else if (dateFilterIndex == 6) days = 365;

                         if (days > 0) {
                             auto date = game->getMetadata().getScrapeDate(selectedScraperName);
                             dateFilterPassed = (date == nullptr || !date->isValid() || date->getTime() <= (now - (days * 86400)));
                         }
                     }
                } catch (const std::exception& filterEx) {
                     LOG(LogError) << "Eccezione durante valutazione filtri per gioco '" << game->getName() << "': " << filterEx.what();
                     gamesErroredDuringFilter++;
                     continue; // Salta questo gioco se il filtro causa eccezione
                } catch (...) {
                      LOG(LogError) << "Eccezione sconosciuta durante valutazione filtri per gioco '" << game->getName() << "'.";
                      gamesErroredDuringFilter++;
                      continue;
                }
                // --- Fine rivalutazione filtri ---

                if (dateFilterPassed && mediaFilterPassed)
                {
                    ScraperSearchParams search;
                    search.game = game;
                    search.system = system;
                    search.overWriteMedias = currentOverwriteSetting;
                    queue.push(search);
                    totalGamesAddedToQueue++;
                } else {
                     gamesSkippedByFilter++;
                     // LOG(LogDebug) << "  Gioco saltato dai filtri: " << game->getName() << " (DateOK: " << dateFilterPassed << ", MediaOK: " << mediaFilterPassed << ")";
                }
            } // Fine ciclo giochi
            pos++;
        } // Fine ciclo sistemi

        LOG(LogInfo) << "GuiScraperStart::getSearches - Fine. Sistemi Saltati: " << systemsSkipped << ", Giochi Processati: " << totalGamesProcessed << ", Giochi Aggiunti alla Coda: " << totalGamesAddedToQueue << ", Giochi Saltati dai Filtri: " << gamesSkippedByFilter << ", Giochi con Errori Filtro/Lista: " << gamesErroredDuringFilter;

    } catch (const std::exception& e) {
         LOG(LogError) << "Eccezione generale FATALE in GuiScraperStart::getSearches: " << e.what();
         mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE CRITICO DURANTE LA PREPARAZIONE DELLO SCRAPING.")));
         std::queue<ScraperSearchParams> emptyQueue; // Svuota la coda per evitare problemi dopo
         std::swap(queue, emptyQueue);
    } catch (...) {
         LOG(LogError) << "Eccezione sconosciuta generale FATALE in GuiScraperStart::getSearches.";
          mWindow->pushGui(new GuiMsgBox(mWindow, _("ERRORE CRITICO SCONOSCIUTO DURANTE LA PREPARAZIONE DELLO SCRAPING.")));
         std::queue<ScraperSearchParams> emptyQueue;
         std::swap(queue, emptyQueue);
    }

    return queue;
}

bool GuiScraperStart::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if (consumed)
		return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		close();
		return true;
	}

	if (config->isMappedTo("start", input) && input.value != 0)
	{
		// close everything
		Window* window = mWindow;
		while (window->peekGui() && window->peekGui() != ViewController::get())
			delete window->peekGui();
	}

	return false;
}

std::vector<HelpPrompt> GuiScraperStart::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	prompts.push_back(HelpPrompt("start", _("CLOSE")));
	return prompts;
}
