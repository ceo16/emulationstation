#include "scrapers/XboxScraper.h" 
#include "Log.h"
#include "SystemData.h"         
#include "FileData.h"           
#include "MetaData.h"           // Necessario per MetaDataId
#include "utils/TimeUtil.h"
#include "utils/StringUtil.h" 
#include "utils/RandomString.h"   
#include "HttpReq.h"            
#include "json.hpp"             
#include "GameStore/Xbox/XboxModels.h" 
#include "GameStore/Xbox/XboxStore.h" 
#include "GameStore/GameStoreManager.h" 
#include "Settings.h"           
#include "SystemConf.h"         
#include "PlatformId.h"         

namespace Scrapers
{

namespace nj = nlohmann; 

// --- Implementazione di XboxScraperHttpRequest ---

XboxScraperHttpRequest::XboxScraperHttpRequest(
    std::vector<ScraperSearchResult>& resultsWrite,
    const std::string& url,
    const ScraperSearchParams& searchParams,
    ::XboxStoreAPI* api, 
    RequestType type,
    std::string pfn,
    std::string productIdForMedia,
    HttpReqOptions* options 
    )
    : ScraperHttpRequest(resultsWrite, url, options), 
      mSearchParams(searchParams),
      mXboxApi(api), 
      mRequestType(type),
      mPfn(pfn),
      mProductIdForMedia(productIdForMedia)
{
}

bool XboxScraperHttpRequest::process(HttpReq* request, std::vector<ScraperSearchResult>& results)
{
    if (!request) {
        LOG(LogError) << "XboxScraperHttpRequest::process: HttpReq object è nullo.";
        return false;
    }

    if (request->status() != HttpReq::REQ_SUCCESS) {
        LOG(LogError) << "XboxScraperHttpRequest: Richiesta HTTP fallita (Stato: " << request->status() << ", Errore: " 
                      << request->getErrorMsg() 
                      << ") per URL: " << request->getUrl();
        return false; 
    }

    switch (mRequestType)
    {
        case RequestType::MEDIA_INFO:
            return processDisplayCatalogResponse(request, results);
        default:
            LOG(LogError) << "XboxScraperHttpRequest: Tipo di richiesta sconosciuto: " << static_cast<int>(mRequestType);
            return false;
    }
}

bool XboxScraperHttpRequest::processDisplayCatalogResponse(HttpReq* request, std::vector<ScraperSearchResult>& results)
{
    std::string pfn_ref = mPfn;
    LOG(LogDebug) << "XboxScraper: Processing DisplayCatalogResponse per ProductID: " << mProductIdForMedia
                  << " (Gioco PFN di riferimento: " << pfn_ref << ")";

    std::string content = request->getContent();
    if (Settings::getInstance()->getBool("Debug") || Settings::getInstance()->getBool("DebugScraper")) {
        LOG(LogDebug) << "XboxScraper: Contenuto grezzo da DisplayCatalog per ProductID " << mProductIdForMedia << " (PFN: " << pfn_ref << "):\n" << content;
    }

    if (content.empty()) {
        LOG(LogWarning) << "XboxScraper: Risposta vuota da DisplayCatalog per ProductID: " << mProductIdForMedia << " (PFN: " << pfn_ref << ")";
        return false;
    }

    try { // Inizio Blocco try
        nj::json responseJson = nj::json::parse(content);
        if (!responseJson.contains("Products") || !responseJson["Products"].is_array() || responseJson["Products"].empty()) {
            LOG(LogWarning) << "XboxScraper: Risposta DisplayCatalog non contiene 'Products' validi per ProductID: " << mProductIdForMedia << " (PFN: " << pfn_ref << "). Risposta: " << content.substr(0, 500);
            return false;
        }

        nj::json product = responseJson["Products"][0];
        if (!product.contains("LocalizedProperties") || !product["LocalizedProperties"].is_array() || product["LocalizedProperties"].empty()) {
            LOG(LogWarning) << "XboxScraper: Risposta DisplayCatalog non contiene 'LocalizedProperties' validi per ProductID: " << mProductIdForMedia << " (PFN: " << pfn_ref << ")";
            return false;
        }
        nj::json localizedProps = product["LocalizedProperties"][0];

        ScraperSearchResult* targetResult = nullptr;
        for (auto& res : results) {
            if (res.pfn == pfn_ref) {
                targetResult = &res;
                break;
            }
        }

        if (!targetResult) {
            LOG(LogWarning) << "XboxScraper: PUNTO CRITICO - Nessun ScraperSearchResult esistente trovato per PFN: " << pfn_ref << " durante processDisplayCatalogResponse.";
            return true; 
        }

        LOG(LogDebug) << "XboxScraper: PUNTO CRITICO - Trovato ScraperSearchResult per PFN: " << pfn_ref << " per il gioco: " << targetResult->mdl.getName();
        bool mediaFoundThisCall = false;

        // Estrarre metadati testuali aggiuntivi
        if (targetResult->mdl.get(MetaDataId::Desc).empty() && localizedProps.contains("ProductDescription") && localizedProps["ProductDescription"].is_string()) {
            targetResult->mdl.set(MetaDataId::Desc, localizedProps["ProductDescription"].get<std::string>());
            LOG(LogDebug) << "XboxScraper: Descrizione aggiornata da displaycatalog per " << pfn_ref;
        }
        if (targetResult->mdl.get(MetaDataId::Developer).empty() && localizedProps.contains("DeveloperName") && localizedProps["DeveloperName"].is_string()) {
            targetResult->mdl.set(MetaDataId::Developer, localizedProps["DeveloperName"].get<std::string>());
            LOG(LogDebug) << "XboxScraper: Developer aggiornato da displaycatalog per " << pfn_ref;
        }
        if (targetResult->mdl.get(MetaDataId::Publisher).empty() && localizedProps.contains("PublisherName") && localizedProps["PublisherName"].is_string()) {
            targetResult->mdl.set(MetaDataId::Publisher, localizedProps["PublisherName"].get<std::string>());
            LOG(LogDebug) << "XboxScraper: Publisher aggiornato da displaycatalog per " << pfn_ref;
        }
        if (localizedProps.contains("ProductTitle") && localizedProps["ProductTitle"].is_string() && !localizedProps["ProductTitle"].get<std::string>().empty()) {
            std::string currentName = targetResult->mdl.get(MetaDataId::Name);
            std::string newName = localizedProps["ProductTitle"].get<std::string>();
            if (currentName.empty() || currentName == pfn_ref) {
                targetResult->mdl.set(MetaDataId::Name, newName);
                LOG(LogDebug) << "XboxScraper: Nome aggiornato da displaycatalog per " << pfn_ref << " a: " << newName;
            }
        }

        // Dichiarazione delle variabili per immagini
        std::string bestBoxartUrl, bestFanartUrl, bestLogoUrl, bestTitleshotUrl;
        int boxartW = 0, fanartW = 0, logoW = 0, titleshotW = 0;

        struct ImageCandidate {
            std::string url;
            std::string type;
            std::string purpose;
            int width;
            int height;
        };
        std::vector<ImageCandidate> allFoundImages;

        // Gestione Immagini
        if (localizedProps.contains("Images") && localizedProps["Images"].is_array()) {
            for (const auto& imgNode : localizedProps["Images"]) {
                if (!imgNode.is_object() || !imgNode.contains("Uri") || !imgNode["Uri"].is_string()) continue;
                std::string imgUrl = imgNode["Uri"].get<std::string>();
                if (Utils::String::startsWith(imgUrl, "//")) imgUrl = "https:" + imgUrl;

                std::string apiImageType = imgNode.value("ImageType", "");
                std::string apiImagePurpose = imgNode.value("Purpose", "");
                int currentWidth = imgNode.value("Width", 0);
                int currentHeight = imgNode.value("Height", 0);

                allFoundImages.push_back({imgUrl, apiImageType, apiImagePurpose, currentWidth, currentHeight});
                LOG(LogDebug) << "[XboxScraper Process] Trovato URL immagine potenziale: Tipo=" << apiImageType
                              << ", Scopo=" << apiImagePurpose << ", Larghezza=" << currentWidth
                              << ", Altezza=" << currentHeight
                              << ", URL=" << imgUrl;

                if ((apiImageType == "Poster" || apiImageType == "BoxArt" || (apiImageType == "Tile" && apiImagePurpose == "Poster"))) {
                    if (currentWidth > boxartW) { bestBoxartUrl = imgUrl; boxartW = currentWidth; LOG(LogDebug) << "[XboxScraper Process] >> Candidato BoxArt (Criteri Primari): " << imgUrl; }
                }
                else if (apiImageType == "SuperHeroArt" || apiImageType == "HeroArt" || apiImageType == "BrandedKeyArt") {
                    if (currentWidth > fanartW) { bestFanartUrl = imgUrl; fanartW = currentWidth; LOG(LogDebug) << "[XboxScraper Process] >> Candidato Fanart (Criteri Primari): " << imgUrl; }
                }
                else if (apiImageType == "Logo" || apiImageType == "FeaturePromotionalSquareArt" || apiImageType == "SquareLogo") {
                    if (currentWidth > logoW) { bestLogoUrl = imgUrl; logoW = currentWidth; LOG(LogDebug) << "[XboxScraper Process] >> Candidato Logo (Criteri Primari): " << imgUrl; }
                }
                else if (apiImageType == "Screenshot" && (apiImagePurpose == "Screenshot" || apiImagePurpose == "Title" || apiImagePurpose.empty())) {
                    if (currentWidth > titleshotW || (bestTitleshotUrl.empty() && currentWidth > 0)) {
                        bestTitleshotUrl = imgUrl; titleshotW = currentWidth; LOG(LogDebug) << "[XboxScraper Process] >> Candidato Titleshot (Criteri Primari): " << imgUrl;
                    }
                }
            } // Fine ciclo for

            // FALLBACK AVANZATO PER BOXART
            if (bestBoxartUrl.empty() && !allFoundImages.empty()) {
                LOG(LogInfo) << "[XboxScraper Process] BoxArt non trovata con criteri specifici. Avvio Fallback Avanzato da " << allFoundImages.size() << " immagini.";
                ImageCandidate chosenFallback = {"", "", "", 0, 0};
                for (const auto& candidate : allFoundImages) {
                    if (candidate.width < 200 || (candidate.height != 0 && candidate.height < 200)) {
                        LOG(LogDebug) << "[XboxScraper BoxArt Fallback] Scartato (troppo piccolo): " << candidate.url << " L" << candidate.width << " A" << candidate.height;
                        continue;
                    }
                    if (candidate.height > 0) {
                        float aspectRatio_W_div_H = static_cast<float>(candidate.width) / candidate.height;
                        if (aspectRatio_W_div_H < 0.5f || aspectRatio_W_div_H > 1.5f) {
                            LOG(LogDebug) << "[XboxScraper BoxArt Fallback] Scartato (proporzioni non da copertina: " << aspectRatio_W_div_H << "): " << candidate.url;
                            continue;
                        }
                    } else {
                        if (candidate.width > 1200) {
                            LOG(LogDebug) << "[XboxScraper BoxArt Fallback] Scartato (potenziale banner, L > 1200, A non nota): " << candidate.url;
                            continue;
                        }
                    }
                    if (candidate.width > chosenFallback.width) {
                        chosenFallback = candidate;
                        LOG(LogDebug) << "[XboxScraper BoxArt Fallback] >> Nuovo candidato fallback migliore: " << chosenFallback.url << " L" << chosenFallback.width << " A" << chosenFallback.height;
                    }
                }
                if (!chosenFallback.url.empty()) {
                    bestBoxartUrl = chosenFallback.url;
                    boxartW = chosenFallback.width;
                    LOG(LogInfo) << "[XboxScraper Process] Fallback Avanzato: Selezionata immagine come BoxArt: " << bestBoxartUrl
                                 << " (Tipo Orig: '" << chosenFallback.type << "', Scopo Orig: '" << chosenFallback.purpose
                                 << "', L: " << chosenFallback.width << ", A: " << chosenFallback.height << ")";
                } else {
                    LOG(LogWarning) << "[XboxScraper Process] Fallback Avanzato: Non è stato possibile selezionare nessuna immagine adatta per BoxArt.";
                }
            }

            // FALLBACK AVANZATO PER FANART
            if (bestFanartUrl.empty() && !allFoundImages.empty()) {
                LOG(LogInfo) << "[XboxScraper Process] Fanart non trovata con criteri specifici. Avvio Fallback Avanzato per Fanart da " << allFoundImages.size() << " immagini.";
                ImageCandidate chosenFanartFallback = {"", "", "", 0, 0};
                int bestFanartScore = 0;
                for (const auto& candidate : allFoundImages) {
                    if (!bestBoxartUrl.empty() && candidate.url == bestBoxartUrl) {
                        LOG(LogDebug) << "[XboxScraper Fanart Fallback] Scartato (già usato come BoxArt): " << candidate.url;
                        continue;
                    }
                    if (candidate.width < 1000 || (candidate.height != 0 && candidate.height < 560)) {
                        LOG(LogDebug) << "[XboxScraper Fanart Fallback] Scartato (troppo piccolo per fanart): " << candidate.url << " L" << candidate.width << " A" << candidate.height;
                        continue;
                    }
                    if (candidate.height > 0) {
                        float aspectRatio_W_div_H = static_cast<float>(candidate.width) / candidate.height;
                        if (aspectRatio_W_div_H < 1.55f || aspectRatio_W_div_H > 2.1f) {
                            LOG(LogDebug) << "[XboxScraper Fanart Fallback] Scartato (proporzioni non da fanart: " << aspectRatio_W_div_H << "): " << candidate.url;
                            continue;
                        }
                    } else {
                        if (candidate.width < 1280) {
                            LOG(LogDebug) << "[XboxScraper Fanart Fallback] Scartato (L < 1280 e Altezza non nota, rischioso per fanart): " << candidate.url;
                            continue;
                        }
                        LOG(LogDebug) << "[XboxScraper Fanart Fallback] Altezza non disponibile per " << candidate.url << ", controllo proporzioni non stringente (basato solo su larghezza).";
                    }
                    int currentScore = (candidate.height > 0) ? (candidate.width * candidate.height) : candidate.width;
                    if (currentScore > bestFanartScore) {
                        chosenFanartFallback = candidate;
                        bestFanartScore = currentScore;
                        LOG(LogDebug) << "[XboxScraper Fanart Fallback] >> Nuovo candidato Fanart migliore: " << chosenFanartFallback.url << " L" << chosenFanartFallback.width << " A" << chosenFanartFallback.height << " (Score: " << bestFanartScore << ")";
                    }
                }
                if (!chosenFanartFallback.url.empty()) {
                    bestFanartUrl = chosenFanartFallback.url;
                    fanartW = chosenFanartFallback.width;
                    LOG(LogInfo) << "[XboxScraper Process] Fallback Avanzato: Selezionata immagine come Fanart: " << bestFanartUrl
                                 << " (Tipo Orig: '" << chosenFanartFallback.type << "', Scopo Orig: '" << chosenFanartFallback.purpose
                                 << "', L: " << chosenFanartFallback.width << ", A: " << chosenFanartFallback.height << ")";
                } else {
                    LOG(LogWarning) << "[XboxScraper Process] Fallback Avanzato: Non è stato possibile selezionare nessuna immagine adatta per Fanart.";
                }
            }

            // <<<<< INIZIO FALLBACK AVANZATO PER LOGO/MARQUEE >>>>>
            if (bestLogoUrl.empty() && !allFoundImages.empty()) {
                LOG(LogInfo) << "[XboxScraper Process] Logo/Marquee non trovato con criteri specifici. Avvio Fallback Avanzato per Logo da " << allFoundImages.size() << " immagini.";
                ImageCandidate chosenLogoFallback = {"", "", "", 0, 0};
                int bestLogoScore = -1; // Inizializza a -1 per assicurare che il primo candidato valido venga preso

                for (const auto& candidate : allFoundImages) {
                    // Non usare immagini già scelte per boxart o fanart
                    if ((!bestBoxartUrl.empty() && candidate.url == bestBoxartUrl) ||
                        (!bestFanartUrl.empty() && candidate.url == bestFanartUrl)) {
                        LOG(LogDebug) << "[XboxScraper Logo Fallback] Scartato (già usato come BoxArt o Fanart): " << candidate.url;
                        continue;
                    }

                    // Priorità 1: Tipo o Scopo espliciti (anche se spesso vuoti, vale la pena controllare)
                    if (!candidate.type.empty() || !candidate.purpose.empty()) {
                        if (candidate.type == "Logo" || candidate.purpose == "Logo" ||
                            candidate.type == "ClearLogo" || candidate.purpose == "ClearLogo" ||
                            candidate.type == "Icon" || candidate.purpose == "Icon") {
                            
                            int currentExplicitScore = candidate.width; // Per loghi espliciti, la larghezza può essere un buon criterio
                            if (currentExplicitScore > bestLogoScore) { // Scegli il più largo tra quelli con tipo/scopo esplicito
                                chosenLogoFallback = candidate;
                                bestLogoScore = currentExplicitScore;
                                LOG(LogDebug) << "[XboxScraper Logo Fallback] >> Candidato Logo (Tipo/Scopo esplicito): " << candidate.url << " L" << candidate.width << " A" << candidate.height;
                            }
                            // Continua il ciclo per vedere se ci sono altri loghi espliciti più grandi, non fermarti al primo.
                        }
                    }
                } // Fine primo ciclo per loghi espliciti

                // Se non abbiamo trovato un logo esplicito, proviamo con criteri basati su dimensioni/proporzioni
                if (chosenLogoFallback.url.empty()) {
                    LOG(LogDebug) << "[XboxScraper Logo Fallback] Nessun logo con Tipo/Scopo esplicito. Tento fallback su dimensioni/proporzioni.";
                    bestLogoScore = -1; // Resetta lo score per il secondo tentativo

                    for (const auto& candidate : allFoundImages) {
                         // Riesegui il controllo per non usare boxart/fanart
                        if ((!bestBoxartUrl.empty() && candidate.url == bestBoxartUrl) ||
                            (!bestFanartUrl.empty() && candidate.url == bestFanartUrl)) {
                            continue; // Già scartato
                        }

                        float aspectRatio_W_div_H = 0.0f;
                        if (candidate.height > 0) {
                            aspectRatio_W_div_H = static_cast<float>(candidate.width) / candidate.height;
                        }

                        bool potentiallyLogo = false;
                        int currentScore = 0;

                        // Criterio A: Immagine Larga (tipico marquee/logo banner)
                        // Larghezza > 200, Altezza relativamente piccola, Proporzioni > 2.0:1
                        if (candidate.width > 200 && candidate.height > 0 && candidate.height < (candidate.width / 1.8f) && candidate.width < 1000 && candidate.height < 300) {
                             // Esempio: un logo 400x100 (AR 4.0), 600x200 (AR 3.0)
                            if (aspectRatio_W_div_H >= 2.0f) {
                                potentiallyLogo = true;
                                currentScore = candidate.width; // Preferisci i banner più larghi
                                LOG(LogDebug) << "[XboxScraper Logo Fallback] Considerato come logo banner (L: " << candidate.width << ", A: " << candidate.height << ", AR: " << aspectRatio_W_div_H << "): " << candidate.url;
                            }
                        }
                        // Criterio B: Immagine Piccola/Icona (quadrata o quasi, non troppo grande)
                        // Esempio: Larghezza e Altezza tra 40 e 300, proporzioni vicine a 1:1
                        else if (candidate.width > 40 && candidate.width < 400 &&
                                 candidate.height > 40 && candidate.height < 400 &&
                                 candidate.height > 0 && // Assicura altezza > 0 per calcolo AR
                                 aspectRatio_W_div_H >= 0.75f && aspectRatio_W_div_H <= 1.35f) {
                            potentiallyLogo = true;
                            currentScore = candidate.width * candidate.height; // Per icone, l'area
                            LOG(LogDebug) << "[XboxScraper Logo Fallback] Considerato come logo icona/quadrato (L: " << candidate.width << ", A: " << candidate.height << ", AR: " << aspectRatio_W_div_H << "): " << candidate.url;
                        }

                        if (potentiallyLogo && currentScore > bestLogoScore) {
                            chosenLogoFallback = candidate;
                            bestLogoScore = currentScore;
                            LOG(LogDebug) << "[XboxScraper Logo Fallback] >> Nuovo candidato Logo (dimensioni/proporzioni): " << chosenLogoFallback.url << " (Score: " << bestLogoScore << ")";
                        }
                    }
                } // Fine del secondo tentativo per loghi basati su dimensioni/proporzioni

                if (!chosenLogoFallback.url.empty()) {
                    bestLogoUrl = chosenLogoFallback.url;
                    logoW = chosenLogoFallback.width;
                    LOG(LogInfo) << "[XboxScraper Process] Fallback Avanzato: Selezionata immagine come Logo/Marquee: " << bestLogoUrl
                                 << " (Tipo Orig: '" << chosenLogoFallback.type << "', Scopo Orig: '" << chosenLogoFallback.purpose
                                 << "', L: " << chosenLogoFallback.width << ", A: " << chosenLogoFallback.height << ")";
                } else {
                    LOG(LogWarning) << "[XboxScraper Process] Fallback Avanzato: Non è stato possibile selezionare nessuna immagine adatta per Logo/Marquee.";
                }
            } // Chiusura if (bestLogoUrl.empty() && !allFoundImages.empty())
            // <<<<< FINE FALLBACK AVANZATO PER LOGO/MARQUEE >>>>>

        } else { LOG(LogDebug) << "XboxScraper: Nodo 'Images' non trovato o vuoto in DisplayCatalog (PFN: " << pfn_ref << ")"; }

        // 4. ASSEGNAZIONE FINALE AI METADATI
        if (!bestBoxartUrl.empty()) {
            targetResult->urls[MetaDataId::Image] = ScraperSearchItem(bestBoxartUrl);
            targetResult->urls[MetaDataId::Thumbnail] = ScraperSearchItem(bestBoxartUrl);
            targetResult->boxartUrl = bestBoxartUrl;
            mediaFoundThisCall = true;
            LOG(LogDebug) << "XboxScraper: Boxart assegnata per " << pfn_ref << ": " << bestBoxartUrl;
        } else {
             LOG(LogWarning) << "[XboxScraper Process] !!! BoxArt ancora NON TROVATA per PFN: " << pfn_ref << " dopo tutti i tentativi.";
        }
        if (!bestFanartUrl.empty()) {
            targetResult->urls[MetaDataId::FanArt] = ScraperSearchItem(bestFanartUrl);
            mediaFoundThisCall = true;
            LOG(LogDebug) << "XboxScraper: Fanart assegnato per " << pfn_ref << ": " << bestFanartUrl;
        } else {
            LOG(LogWarning) << "[XboxScraper Process] !!! Fanart ancora NON TROVATO per PFN: " << pfn_ref << " dopo tutti i tentativi.";
        }
        if (!bestLogoUrl.empty()) { // <<< ASSEGNAZIONE LOGO/MARQUEE
            targetResult->urls[MetaDataId::Marquee] = ScraperSearchItem(bestLogoUrl);
            mediaFoundThisCall = true;
            LOG(LogDebug) << "XboxScraper: Marquee/Logo assegnato per " << pfn_ref << ": " << bestLogoUrl;
        } else {
            LOG(LogWarning) << "[XboxScraper Process] !!! Marquee/Logo ancora NON TROVATO per PFN: " << pfn_ref << " dopo tutti i tentativi.";
        }
        if (!bestTitleshotUrl.empty()) {
            targetResult->urls[MetaDataId::TitleShot] = ScraperSearchItem(bestTitleshotUrl);
            targetResult->titleshotUrl = bestTitleshotUrl;
            mediaFoundThisCall = true;
            LOG(LogDebug) << "XboxScraper: TitleShot (Screenshot) assegnato per " << pfn_ref << ": " << bestTitleshotUrl;
        } // Log warning per titleshot mancante è opzionale


        // Gestione Video (la tua logica corretta per CMSVideos, HLS/DASH, VideoPurpose)
        if (localizedProps.contains("CMSVideos") && localizedProps["CMSVideos"].is_array() && !localizedProps["CMSVideos"].empty()) {
            std::string foundVideoUrl;
            const auto& cmsVideosArray = localizedProps["CMSVideos"]; 

            for (const auto& videoNode : cmsVideosArray) {
                if (!videoNode.is_object()) continue;
                std::string hlsUrl = videoNode.value("HLS", "");
                std::string dashUrl = videoNode.value("DASH", "");
                std::string videoUrlToUse;

                if (!hlsUrl.empty()) videoUrlToUse = hlsUrl;
                else if (!dashUrl.empty()) videoUrlToUse = dashUrl;
                else { LOG(LogDebug) << "[XboxScraper Video Process] Video entry non contiene URL HLS o DASH validi."; continue; }
                
                if (Utils::String::startsWith(videoUrlToUse, "//")) videoUrlToUse = "https:" + videoUrlToUse;

                std::string videoPurpose = videoNode.value("VideoPurpose", "");
                LOG(LogDebug) << "[XboxScraper Video Process] Video Trovato: URL=" << videoUrlToUse << ", VideoPurpose=" << videoPurpose;

                if (videoPurpose == "HeroTrailer" || videoPurpose == "Trailer" || videoPurpose == "Preview" || videoPurpose == "Video" || videoPurpose.empty()) {
                    foundVideoUrl = videoUrlToUse;
                    LOG(LogDebug) << "[XboxScraper Video Process] >> Selezionato video per Scopo primario ('" << videoPurpose << "'): " << foundVideoUrl;
                    break; 
                }
            }
            if (foundVideoUrl.empty() && !cmsVideosArray.empty()) {
                LOG(LogDebug) << "[XboxScraper Video Process] Nessun video con Scopo primario. Tento fallback sul primo URI HLS/DASH valido.";
                for (const auto& videoNode : cmsVideosArray) { 
                    if (!videoNode.is_object()) continue;
                    std::string hlsUrl = videoNode.value("HLS", "");
                    std::string dashUrl = videoNode.value("DASH", "");
                    std::string videoUrlToUse;

                    if (!hlsUrl.empty()) videoUrlToUse = hlsUrl;
                    else if (!dashUrl.empty()) videoUrlToUse = dashUrl;
                    else continue;

                    if (!videoUrlToUse.empty()){
                        if (Utils::String::startsWith(videoUrlToUse, "//")) videoUrlToUse = "https:" + videoUrlToUse;
                        foundVideoUrl = videoUrlToUse;
                        LOG(LogDebug) << "[XboxScraper Video Process] >> Selezionato video (Fallback - primo valido): " << foundVideoUrl;
                        break;
                    }
                }
            }
            if (!foundVideoUrl.empty()) {
                targetResult->urls[MetaDataId::Video] = ScraperSearchItem(foundVideoUrl);
                mediaFoundThisCall = true;
                LOG(LogInfo) << "XboxScraper: Video URL TROVATO e assegnato: " << foundVideoUrl << " per PFN: " << pfn_ref;
            } else {
                LOG(LogWarning) << "XboxScraper: Nessun URL video valido trovato (neanche con fallback) per PFN: " << pfn_ref << " nel nodo CMSVideos.";
            }
        } else {
            LOG(LogDebug) << "XboxScraper: Nodo 'CMSVideos' (o 'Videos') non trovato o vuoto (PFN: " << pfn_ref << ")";
        }

        if (mediaFoundThisCall) {
            LOG(LogInfo) << "XboxScraper: Media (URL) processati e assegnati a ScraperSearchResult per " << targetResult->mdl.get(MetaDataId::Name);
        }
        return true;

    } catch (const nj::json::exception& e) {
        LOG(LogError) << "XboxScraper: Errore JSON in processDisplayCatalogResponse per ProductID " << mProductIdForMedia << " (PFN: " << pfn_ref << "): " << e.what();
        return false;
    }
} // FINE DELLA FUNZIONE processDisplayCatalogResponse 


// --- Implementazione di XboxScraper ---
XboxScraper::XboxScraper() {
    // Definisci quali tipi di media questo scraper può fornire URL per.
    // Il download effettivo e la gestione dei path locali sono gestiti da Scraper.cpp -> resolveMetaDataAssets
    mSupportedMedia = { 
        ScraperMediaSource::Box2d,      // Immagine principale / copertina
        ScraperMediaSource::FanArt,     // Immagine di sfondo
        ScraperMediaSource::Video,      // Video trailer/gameplay
        ScraperMediaSource::Marquee,    // Logo / marquee
        ScraperMediaSource::Screenshot  // Screenshot / titleshot
    };
}

::XboxStoreAPI* XboxScraper::getXboxApi() { 
    GameStoreManager* gsm = GameStoreManager::getInstance(nullptr);  
    if (gsm) {
        GameStore* store = gsm->getStore("XboxStore"); 
        if (store) {
            XboxStore* xboxStoreInstance = dynamic_cast<XboxStore*>(store);
            if (xboxStoreInstance) {
                return xboxStoreInstance->getApi(); 
            } else {
                LOG(LogError) << "XboxScraper::getXboxApi: Impossibile eseguire il cast di GameStore a XboxStore.";
            }
        } else {
            LOG(LogWarning) << "XboxScraper::getXboxApi: XboxStore non trovato in GameStoreManager.";
        }
    } else {
        LOG(LogError) << "XboxScraper::getXboxApi: GameStoreManager non disponibile (nullptr).";
    }
    return nullptr;
}

void XboxScraper::generateRequests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requestsQueue,
    std::vector<ScraperSearchResult>& results) 
{
    if (!params.game || !params.system) {
        LOG(LogError) << "XboxScraper::generateRequests: params.game o params.system è nullo.";
        return;
    }

    ::XboxStoreAPI* xboxApi = this->getXboxApi(); 
    if (!xboxApi) {
        LOG(LogError) << "XboxScraper::generateRequests: Impossibile ottenere XboxStoreAPI.";
        return; 
    }
    
    std::string gamePfn = params.game->getMetadata().get(MetaDataId::XboxPfn);
    std::string gameNameForLog = params.game->getName();

    if (gamePfn.empty()) {
        std::string path = params.game->getPath();
        if (Utils::String::startsWith(path, "xbox:/pfn/")) {
            gamePfn = path.substr(10);
        }
        if (gamePfn.empty()){
            // Se il PFN è vuoto e non possiamo derivarlo, non possiamo procedere per questo gioco.
            LOG(LogWarning) << "XboxScraper::generateRequests: PFN mancante per gioco: " << gameNameForLog << ", Path: " << path << ". Impossibile fare scraping per questo gioco.";
            return; 
        }
    }

    LOG(LogDebug) << "XboxScraper::generateRequests: Inizio per PFN: " << gamePfn << " (Gioco: " << gameNameForLog << ")";
    
    ScraperSearchResult currentSearch("Xbox"); // Nome dello scraper
    currentSearch.pfn = gamePfn; // Salva il PFN nel risultato per riferimento futuro (usato in processDisplayCatalogResponse)

    MetaDataList& searchMetadata = currentSearch.mdl; // Riferimento ai metadati del risultato dello scraper
    const MetaDataList& existingGameMetadata = params.game->getMetadata(); // Metadati del gioco esistente

    // Preserva 'Installed'
    std::string installedVal = existingGameMetadata.get(MetaDataId::Installed);
    if (!installedVal.empty()) { // Sarà "true" o "false"
        searchMetadata.set(MetaDataId::Installed, installedVal);
        LOG(LogDebug) << "XboxScraper: Preserving 'Installed' (" << installedVal << ") for " << gameNameForLog;
    } else if (params.game->getPath().find("!") != std::string::npos && !Utils::String::startsWith(params.game->getPath(), "xbox_online_")) {
        // Se il path sembra un AUMID (gioco installato) e 'Installed' non è settato, impostalo a true.
        searchMetadata.set(MetaDataId::Installed, "true");
        LOG(LogDebug) << "XboxScraper: Setting 'Installed' to true (heuristic from AUMID path) for " << gameNameForLog;
    }


    // Preserva 'Virtual'
    std::string virtualVal = existingGameMetadata.get(MetaDataId::Virtual);
    if (!virtualVal.empty()) { // Sarà "true" o "false"
        searchMetadata.set(MetaDataId::Virtual, virtualVal);
        LOG(LogDebug) << "XboxScraper: Preserving 'Virtual' (" << virtualVal << ") for " << gameNameForLog;
    } else if (params.game->getPath().find("!") != std::string::npos && !Utils::String::startsWith(params.game->getPath(), "xbox_online_")) {
         searchMetadata.set(MetaDataId::Virtual, "false");
         LOG(LogDebug) << "XboxScraper: Setting 'Virtual' to false (heuristic from AUMID path) for " << gameNameForLog;
    }


    // Preserva 'XboxAumid' - cruciale per i giochi installati
    std::string aumidVal = existingGameMetadata.get(MetaDataId::XboxAumid);
    if (!aumidVal.empty()) {
        searchMetadata.set(MetaDataId::XboxAumid, aumidVal);
        LOG(LogDebug) << "XboxScraper: Preserving 'XboxAumid' (" << aumidVal << ") for " << gameNameForLog;
    } else if (params.game->getPath().find("!") != std::string::npos && !Utils::String::startsWith(params.game->getPath(), "xbox_online_")) {
        // Se il path è l'AUMID stesso, usalo per XboxAumid se non c'è già
        searchMetadata.set(MetaDataId::XboxAumid, params.game->getPath());
        LOG(LogDebug) << "XboxScraper: Setting 'XboxAumid' from game path (" << params.game->getPath() << ") for " << gameNameForLog;
    }


    // Preserva 'LaunchCommand' - cruciale per i giochi installati
    std::string launchVal = existingGameMetadata.get(MetaDataId::LaunchCommand);
    if (!launchVal.empty()) {
        searchMetadata.set(MetaDataId::LaunchCommand, launchVal);
        LOG(LogDebug) << "XboxScraper: Preserving 'LaunchCommand' for " << gameNameForLog;
    } else if (params.game->getPath().find("!") != std::string::npos && !Utils::String::startsWith(params.game->getPath(), "xbox_online_")) {
         // Per i giochi installati, il launch command è spesso l'AUMID stesso (il path del FileData)
        searchMetadata.set(MetaDataId::LaunchCommand, params.game->getPath());
        LOG(LogDebug) << "XboxScraper: Setting 'LaunchCommand' from game path (" << params.game->getPath() << ") for " << gameNameForLog;
    }
    // ---- FINE LOGICA DI PRESERVAZIONE ----


    bool gotBasicMetadata = false;
    Xbox::OnlineTitleInfo titleInfo;

    try {
        titleInfo = xboxApi->GetTitleInfo(gamePfn); 
        if (!(titleInfo.pfn.empty() && titleInfo.name.empty())) { // Controllo base se titleInfo è popolata
            gotBasicMetadata = true;
            currentSearch.mdl.set(MetaDataId::Name, titleInfo.name.empty() ? gamePfn : titleInfo.name);
            
			if (!gamePfn.empty()) {
               currentSearch.mdl.set(MetaDataId::XboxPfn, gamePfn);
               LOG(LogDebug) << "XboxScraper: Salvato XboxPfn [" << gamePfn << "] nei metadati per il nome base del file.";
           }
			
            // Questi campi da titleInfo.detail potrebbero essere vuoti se titlehub non li fornisce.
            // Verranno (ri)popolati da processDisplayCatalogResponse se displaycatalog li ha.
            if (!titleInfo.detail.description.empty()) currentSearch.mdl.set(MetaDataId::Desc, titleInfo.detail.description);
            if (!titleInfo.detail.developerName.empty()) currentSearch.mdl.set(MetaDataId::Developer, titleInfo.detail.developerName);
            if (!titleInfo.detail.publisherName.empty()) currentSearch.mdl.set(MetaDataId::Publisher, titleInfo.detail.publisherName);
            
            if (!titleInfo.detail.releaseDate.empty()) {
                time_t timestamp = Utils::Time::iso8601ToTime(titleInfo.detail.releaseDate);
                if (timestamp != Utils::Time::NOT_A_DATE_TIME) {
                    currentSearch.mdl.set(MetaDataId::ReleaseDate, Utils::Time::timeToMetaDataString(timestamp));
                } else {
                     LOG(LogDebug) << "XboxScraper: Formato releaseDate da titleInfo.detail.releaseDate non riconosciuto: " << titleInfo.detail.releaseDate;
                }
            }

            // --- SALVATAGGIO DELLO STORE ID ALFANUMERICO (XboxProductId) ---
            if (!titleInfo.detail.productId.empty()) { // productId è il nostro Store ID alfanumerico
                currentSearch.mdl.set(MetaDataId::XboxProductId, titleInfo.detail.productId);
                LOG(LogInfo) << "XboxScraper: Salvataggio di XboxProductId (Store ID) [" << titleInfo.detail.productId << "] nei metadati per " << titleInfo.name;
            }
            // --- FINE SALVATAGGIO ---

            LOG(LogDebug) << "XboxScraper::generateRequests: Metadati testuali base (o parziali) da titleInfo per " << currentSearch.mdl.get(MetaDataId::Name);
        } else {
             LOG(LogInfo) << "XboxScraper::generateRequests: Nessuna info significativa da GetTitleInfo per PFN: " << gamePfn << ". Uso PFN come nome.";
             currentSearch.mdl.set(MetaDataId::Name, gamePfn); // Imposta almeno il nome al PFN
        }
    } catch (const std::exception& e) {
        LOG(LogError) << "XboxScraper::generateRequests: Eccezione da GetTitleInfo per PFN " << gamePfn << ": " << e.what();
        currentSearch.mdl.set(MetaDataId::Name, gamePfn); // Fallback: usa PFN come nome
    }

    results.push_back(currentSearch); // Aggiungi il risultato alla lista dei risultati. Sarà usato da processDisplayCatalogResponse.

    // --- INIZIO LOGICA SELEZIONE ProductID PER MEDIA (MODIFICATA) ---
    std::string productIdForMedia = ""; 

    if (gotBasicMetadata) { 
        LOG(LogDebug) << "XboxScraper: Valori ID da titleInfo per PFN " << gamePfn << ":";
        LOG(LogDebug) << "  detail.productId (Store ID alfanumerico?): [" << titleInfo.detail.productId << "]";
        LOG(LogDebug) << "  modernTitleId (ID numerico): [" << titleInfo.modernTitleId << "]";
        LOG(LogDebug) << "  windowsPhoneProductId: [" << titleInfo.windowsPhoneProductId << "]";
        LOG(LogDebug) << "  titleId (hex): [" << titleInfo.titleId << "]";

        if (!titleInfo.detail.productId.empty()) { // Questo è lo Store ID alfanumerico che speriamo sia stato estratto
            productIdForMedia = titleInfo.detail.productId;
            LOG(LogInfo) << "XboxScraper: Usato titleInfo.detail.productId (da availabilities o simile) per la query media: [" << productIdForMedia << "]";
        }
        else if (!titleInfo.modernTitleId.empty()) { // Fallback se detail.productId è vuoto
            productIdForMedia = titleInfo.modernTitleId;
            LOG(LogInfo) << "XboxScraper: titleInfo.detail.productId vuoto. Usato titleInfo.modernTitleId per la query media: [" << productIdForMedia << "]";
        }
        else if (!titleInfo.windowsPhoneProductId.empty()) { // Altro fallback
            productIdForMedia = titleInfo.windowsPhoneProductId;
            LOG(LogInfo) << "XboxScraper: modernTitleId vuoto. Usato titleInfo.windowsPhoneProductId per la query media: [" << productIdForMedia << "]";
        }
    } else {
        LOG(LogWarning) << "XboxScraper: gotBasicMetadata era false, titleInfo potrebbe non essere popolato (nessun ID da titleInfo verrà usato inizialmente).";
    }

    // Fallback finale: prova a prenderlo dai metadati esistenti del gioco se ancora vuoto
    // Questo è utile se il gioco è stato già scansionato e MetaDataId::XboxProductId è stato salvato.
    if (productIdForMedia.empty()) {
        std::string mdXboxProductId = params.game->getMetadata().get(MetaDataId::XboxProductId);
        if (!mdXboxProductId.empty()) {
            productIdForMedia = mdXboxProductId;
            LOG(LogInfo) << "XboxScraper: Tutti gli ID da titleInfo erano vuoti o non validi. Usato MetaDataId::XboxProductId dai metadati locali per la query media: [" << productIdForMedia << "]";
        }
    }
    
    LOG(LogInfo) << "XboxScraper: ProductID finale selezionato per PFN " << gamePfn << " per la query media a displaycatalog: [" << productIdForMedia << "]";
    // --- FINE LOGICA SELEZIONE ProductID PER MEDIA ---

    if (productIdForMedia.empty()) {
        LOG(LogWarning) << "XboxScraper::generateRequests: ProductID per media NON TROVATO per PFN: " << gamePfn << ". Impossibile recuperare immagini/video da displaycatalog.";
        return; // Non possiamo procedere senza un ProductID per displaycatalog
    }
    
  std::string market = "";             // Es. "IT"
    std::string languages = "";          // Es. "it-IT" (rinominata da languagesApiFormat per coerenza con il tuo snippet)
    
    // 1. PROVA A USARE LA LINGUA PRE-IMPOSTATA SUL FILEDATA (impostata da SystemData.cpp)
    std::string langTagFromGameData = params.game->getMetadata().get(MetaDataId::Language); //

    if (!langTagFromGameData.empty()) {
        LOG(LogInfo) << "XboxScraper: Trovata lingua prioritaria nel FileData: '" << langTagFromGameData 
                     << "' per il gioco '" << params.game->getName() << "'. Tento di usarla.";

        // Logica di conversione da langTagFromGameData (es. "it_IT") a market ("IT") e languages ("it-IT")
        size_t separatorPos = langTagFromGameData.find_first_of("-_");
        std::string langPart = "";
        std::string regionPart = "";

        if (separatorPos != std::string::npos && separatorPos > 0 && separatorPos + 1 < langTagFromGameData.length()) {
            langPart = Utils::String::toLower(langTagFromGameData.substr(0, separatorPos));
            regionPart = Utils::String::toUpper(langTagFromGameData.substr(separatorPos + 1));
            languages = langPart + "-" + regionPart; // Es. "it-IT"
            market = regionPart; // Usa la parte regione per il market (es. "IT" da "it_IT")
        } else if (langTagFromGameData.length() == 2) { // Solo codice lingua, es. "it"
            langPart = Utils::String::toLower(langTagFromGameData);
            market = Utils::String::toUpper(langTagFromGameData); // "IT" da "it"
            languages = langPart + "-" + market; // Tenta "it-IT"
        } else {
            LOG(LogWarning) << "XboxScraper: Formato lingua da FileData '" << langTagFromGameData 
                            << "' non standard per conversione automatica. Procedo con fallback.";
            // Lascia market e languages vuoti per far scattare i fallback sotto.
            market = ""; 
            languages = "";
        }
        
        // Controlli di validità sui valori derivati dal FileData
        bool marketFromGameDataValid = (!market.empty() && market.length() == 2);
        bool languagesFromGameDataValid = (!languages.empty() && languages.find('-') != std::string::npos && languages.length() >= 5);

        if (marketFromGameDataValid && languagesFromGameDataValid) {
             LOG(LogInfo) << "XboxScraper: OK - Usando lingua da FileData: Market='" << market << "', Languages='" << languages << "'";
        } else {
             LOG(LogWarning) << "XboxScraper: FALLITO - Derivazione market/languages da FileData Language Tag ('" << langTagFromGameData 
                             << "') fallita o risultato non valido (Market: " << market << ", Languages: " << languages 
                             << "). Procedo con altre strategie.";
             market = ""; // Resetta per sicurezza
             languages = ""; // Resetta per sicurezza
        }
    } else {
        LOG(LogDebug) << "XboxScraper: Nessuna lingua pre-impostata trovata nel FileData per '" << params.game->getName() << "'. Uso altre strategie.";
    }

    // 2. Se non impostata/derivata dal FileData, usa la TUA LOGICA ESISTENTE per market e languages
    // (che inizia con la lettura da Settings::getInstance()->getString("ScraperXboxMarket"))
    if (market.empty()) { // Solo se non già impostato validamente da FileData
        market = Settings::getInstance()->getString("ScraperXboxMarket"); 
        if (!market.empty()) {
            LOG(LogDebug) << "XboxScraper: Market preso da es_settings (ScraperXboxMarket): '" << market << "'";
        } else {
            // Il tuo blocco 'if (market.empty()) { ... }' originale per derivare da system.language
            std::string sysLangFull = SystemConf::getInstance()->get("system.language"); 
            if (sysLangFull.length() >= 2) {
                market = Utils::String::toLower(sysLangFull.substr(0,2)); 
                if (market == "en") market = "us"; 
                // Dovresti convertire in maiuscolo per Xbox Market, es. "IT", "US"
                market = Utils::String::toUpper(market);
            } else {
                market = "us"; 
            }
            LOG(LogDebug) << "XboxScraper: Market derivato da system.language ('" << sysLangFull << "'): '" << market << "'";
        }
    }
    // Assicura validità finale per market o usa default
    if (market.empty() || market.length() != 2) {
        LOG(LogWarning) << "XboxScraper: Market '" << market << "' non valido dopo tentativi, fallback a 'US'.";
        market = "us";
    }


    if (languages.empty()) { // Solo se non già impostato validamente da FileData
        languages = Settings::getInstance()->getString("ScraperXboxLanguages"); 
        if (!languages.empty()) {
            LOG(LogDebug) << "XboxScraper: Languages preso da es_settings (ScraperXboxLanguages): '" << languages << "'";
        } else {
            // Il tuo blocco 'if (languages.empty()) { ... }' originale per derivare da system.language
            languages = SystemConf::getInstance()->get("system.language"); 
            if (!languages.empty()) {
                size_t underscorePos = languages.find('_');
                if (underscorePos != std::string::npos) {
                    // Converti it_IT -> it-it (poi l'API Xbox potrebbe volere it-IT, da verificare)
                    std::string langPart = Utils::String::toLower(languages.substr(0, underscorePos));
                    std::string regionPart = Utils::String::toUpper(languages.substr(underscorePos + 1)); // Mantieni regione maiuscola es. IT
                    languages = langPart + "-" + regionPart; // -> it-IT
                } else if (languages.length() == 2) { 
                    languages = Utils::String::toLower(languages) + "-" + Utils::String::toUpper(languages); 
                    if (languages == "en-EN") languages = "en-US"; // Il tuo caso speciale era en-us
                }
            }
            // Il tuo fallback se ancora vuoto o non ha '-'
            if (languages.empty() || languages.find('-') == std::string::npos) {
                languages = "en-US"; // Default più standard per API
            }
            LOG(LogDebug) << "XboxScraper: Languages derivato da system.language o default: '" << languages << "'";
        }
    }
    // Assicura validità finale per languages o usa default (questa era la tua riga originale)
    if (languages.find('-') == std::string::npos) {
        LOG(LogWarning) << "XboxScraper: Languages '" << languages << "' non valido dopo tentativi (manca '-'), fallback a 'en-US'.";
        languages = "en-US"; 
    }

    // A questo punto, market e languages dovrebbero avere i valori corretti e finali.
    LOG(LogInfo) << "[XboxScraper] Parametri di LINGUA FINALI per DisplayCatalog: Market=[" << market << "], Languages=[" << languages << "]";
    if (languages.find('-') == std::string::npos) languages = "en-us"; 

    std::string msCv = Utils::String::generateRandom(16); 
    std::string mediaUrl = "https://displaycatalog.mp.microsoft.com/v7.0/products?bigIds=" + productIdForMedia +
                               "&market=" + market + "&languages=" + languages + "&MS-CV=" + msCv;
	LOG(LogInfo) << "[XboxScraper] URL costruito per DisplayCatalog (Product ID: " << productIdForMedia << "): " << mediaUrl;
    
    HttpReqOptions* mediaHttpOptions = new HttpReqOptions(); 

    LOG(LogDebug) << "XboxScraper::generateRequests: Creazione XboxScraperHttpRequest (MEDIA_INFO) per URL: " << mediaUrl;
    
    requestsQueue.push(std::make_unique<XboxScraperHttpRequest>(
        results,      
        mediaUrl,
        params,       
        xboxApi,
        XboxScraperHttpRequest::RequestType::MEDIA_INFO,
        gamePfn,      
        productIdForMedia,
        mediaHttpOptions 
    ));
}

bool XboxScraper::isSupportedPlatform(SystemData* system) {
    if (!system) return false;
    return system->getThemeFolder() == "xbox" || system->getThemeFolder() == "xbox360" || system->getThemeFolder() == "xboxone" || system->getThemeFolder() == "pc" || system->hasPlatformId(PlatformIds::PC); 
}

const std::set<Scraper::ScraperMediaSource>& XboxScraper::getSupportedMedias() {
    return mSupportedMedia; 
}

} // namespace Scrapers