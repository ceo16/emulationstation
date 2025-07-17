#include "guis/GuiSpotifyBrowser.h"
#include "Window.h"
#include "Log.h"
#include "HttpReq.h"
#include "json.hpp"
#include "SpotifyManager.h"
#include "guis/GuiMsgBox.h"
#include "renderers/Renderer.h"
#include "InputConfig.h"
#include "ThemeData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Paths.h"

#include <thread>
#include <fstream>

// --- SpotifyItemComponent (Questo codice è corretto e rimane invariato) ---
namespace {
    std::string getSpotifyImagePath() {
        std::string path = Paths::getUserEmulationStationPath() + "/roms/spotify/images";
        return path;
    }
}
SpotifyItemComponent::SpotifyItemComponent(Window* window, const std::string& text, const std::string& imageUrl)
    : GuiComponent(window), mImage(nullptr), mText(nullptr),
      mTextStr(text), mImageUrl(imageUrl), mInitialized(false)
{
}
void SpotifyItemComponent::render(const Transform4x4f& parentTrans)
{
    if (!mInitialized) {
        mInitialized = true;
        mImage = new ImageComponent(mWindow);
        mText = new TextComponent(mWindow, mTextStr, ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
        mText->setVerticalAlignment(ALIGN_CENTER);
        addChild(mImage);
        addChild(mText);
        if (!mImageUrl.empty()) {
            std::string safeName = Utils::String::replace(mImageUrl, "https://", "");
            safeName = Utils::String::replace(safeName, "/", "_");
            safeName = Utils::String::replace(safeName, ":", "_");
            std::string ext = Utils::FileSystem::getExtension(safeName);
            if (ext.find('?') != std::string::npos) ext = ext.substr(0, ext.find('?'));
            std::string imagePath = getSpotifyImagePath() + "/" + safeName + ext;
            if (Utils::FileSystem::exists(imagePath)) {
                mImage->setImage(imagePath);
            } else {
                std::thread([this, imagePath]() {
                    HttpReq req(mImageUrl);
                    req.wait();
                    if (req.status() == HttpReq::Status::REQ_SUCCESS) {
                        std::ofstream file(imagePath, std::ios::binary);
                        if (file) {
                            file.write(req.getContent().c_str(), req.getContent().length());
                            file.close();
                            mWindow->postToUiThread([this, imagePath]() { if(mImage) mImage->setImage(imagePath); });
                        }
                    } else {
                        LOG(LogError) << "Download fallito per: " << mImageUrl;
                    }
                }).detach();
            }
        }
        setSize(mSize);
    }
    GuiComponent::render(parentTrans);
}
void SpotifyItemComponent::onSizeChanged()
{
    if (!mInitialized) return;
    const float imageSize = 64.0f;
    if (mImage) {
        mImage->setOrigin(0, 0.5f);
        mImage->setPosition(10, mSize.y() / 2);
        mImage->setMaxSize(Vector2f(imageSize, imageSize));
    }
    if (mText) {
        const float textX = imageSize + 20;
        mText->setOrigin(0, 0.5f);
        mText->setPosition(textX, mSize.y() / 2);
        mText->setSize(mSize.x() - textX, mText->getFont()->getLetterHeight());
    }
}


// --- GuiSpotifyBrowser ---

GuiSpotifyBrowser::GuiSpotifyBrowser(Window* window)
  : GuiComponent(window),
    mMenu(window, _("SPOTIFY")),
    mState(SpotifyViewState::Playlists)
{
    std::string path = getSpotifyImagePath();
    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    addChild(&mMenu);
    setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    loadPlaylists();
}

void GuiSpotifyBrowser::centerMenu()
{
    float sw = Renderer::getScreenWidth();
    float sh = Renderer::getScreenHeight();
    auto sz = mMenu.getSize();
    mMenu.setPosition((sw - sz.x()) * 0.5f, (sh - sz.y()) * 0.5f);
}

void GuiSpotifyBrowser::loadPlaylists()
{
    mState = SpotifyViewState::Playlists;
    mMenu.clear();
    mMenu.setTitle(_("LE TUE PLAYLIST"));
    // RIMOSSO -> mMenu.addButton(...);

    mMenu.addEntry(_("Caricamento..."), false, nullptr);
    centerMenu();

    SpotifyManager::getInstance(mWindow)->getUserPlaylists(
      [this](const std::vector<SpotifyPlaylist>& playlists)
    {
        mMenu.clear();
        mMenu.setTitle(_("LE TUE PLAYLIST"));
        // RIMOSSO -> mMenu.addButton(...);

        if (playlists.empty()) {
            mMenu.addEntry(_("Nessuna playlist trovata."), false, nullptr);
        } else {
            for (auto& p : playlists) {
                ComponentListRow row;
                auto item = std::make_shared<SpotifyItemComponent>(mWindow, p.name, p.image_url);
                row.addElement(item, true);
                item->setSize(mMenu.getSize().x(), 74.0f);
                row.makeAcceptInputHandler([this, id = p.id] { loadTracks(id); });
                mMenu.addRow(row);
            }
        }
        centerMenu();
    });
}

void GuiSpotifyBrowser::loadTracks(std::string id)
{
    mState = SpotifyViewState::Tracks;
    mMenu.clear();
    mMenu.setTitle(_("TRACCE DELLA PLAYLIST"));
    // RIMOSSO -> mMenu.addButton(...);

    mMenu.addEntry(_("Caricamento tracce..."), false, nullptr);
    centerMenu();

    SpotifyManager::getInstance(mWindow)->getPlaylistTracks(
        id,
        [this, id](const std::vector<SpotifyTrack>& tracks)
        {
            mMenu.clear();
            mMenu.setTitle(_("TRACCE DELLA PLAYLIST"));
            // RIMOSSO -> mMenu.addButton(...);

            if (tracks.empty()) {
                mMenu.addEntry(_("Nessuna traccia trovata."), false, nullptr);
            } else {
                for (const auto& t : tracks) {
                    ComponentListRow row;
                    const std::string label = t.name + " — " + t.artist;
                    auto item = std::make_shared<SpotifyItemComponent>(mWindow, label, t.image_url);
                    row.addElement(item, true);
                    item->setSize(mMenu.getSize().x(), 74.0f);
                    row.makeAcceptInputHandler([uri = t.uri] { SpotifyManager::getInstance()->startPlayback(uri); });
                    mMenu.addRow(row);
                }
            }
            centerMenu();
        }
    );
}

bool GuiSpotifyBrowser::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

    if ((config->isMappedTo(BUTTON_BACK, input) || config->isMappedTo("start", input)) && input.value != 0)
    {
        if (mState == SpotifyViewState::Tracks)
        {
            loadPlaylists();
        }
        else
        {
            delete this;
        }
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiSpotifyBrowser::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("up/down", _("CHOOSE")));
    prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));

    if (mState == SpotifyViewState::Tracks) {
        prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
    } else {
        prompts.push_back(HelpPrompt("start", _("CLOSE")));
    }
    return prompts;
}