#include "guis/GuiBusyInfoPopup.h" // Include l'header corrispondente
#include "components/BusyComponent.h"
#include "components/TextComponent.h"
#include "components/NinePatchComponent.h" // Include per lo sfondo
#include "renderers/Renderer.h"
#include "resources/Font.h"
#include "LocaleES.h" // Per _()
#include "Log.h"

GuiBusyInfoPopup::GuiBusyInfoPopup(Window* window, const std::string& text)
    : GuiComponent(window)
{
    // Dimensioni e posizione (esempio centrato)
    const float width = Renderer::getScreenWidth() * 0.5f; // Più largo
    const float height = Renderer::getScreenHeight() * 0.25f; // Un po' più alto
    setSize(width, height);
    setPosition((Renderer::getScreenWidth() - width) / 2, (Renderer::getScreenHeight() - height) / 2);

    // Sfondo (opzionale ma consigliato)
    mBackground = new NinePatchComponent(window);
    mBackground->setImagePath(":/frame.png"); // Usa un'immagine esistente per la cornice
    mBackground->setEdgeColor(0xFFFFFFFF);   // Colore bordo
    mBackground->setCenterColor(0x000000D0); // Colore centro (nero semi-trasparente)
    mBackground->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32)); // Adatta con padding
    addChild(mBackground);

    // Componente Busy
    mBusyAnim = std::make_shared<BusyComponent>(window);
    // Errore C2039: 'setBusyText': non è un membro di 'BusyComponent'
    // ---> Il testo va impostato sul TextComponent interno, non direttamente su BusyComponent
    // mBusyAnim->setBusyText(text); // <-- RIMUOVI QUESTA LINEA

    // Componente Testo
    mText = std::make_shared<TextComponent>(window, text, Font::get(FONT_SIZE_MEDIUM), 0xFFFFFFFF, ALIGN_CENTER);
    mText->setSize(width * 0.9f, 0); // Larghezza testo

    // Posiziona Animazione e Testo (esempio: animazione sopra, testo sotto)
    float animSize = std::min(width, height) * 0.4f; // Dimensione animazione
    mBusyAnim->setSize(animSize, animSize);
    mBusyAnim->setPosition((width - animSize) / 2, height * 0.15f);

    mText->setPosition((width - mText->getSize().x()) / 2, mBusyAnim->getPosition().y() + mBusyAnim->getSize().y() + height * 0.05f);

    // Errore C2664: 'void GuiComponent::addChild(GuiComponent *)': impossibile convertire l'argomento 1 da 'std::shared_ptr<BusyComponent>' a 'GuiComponent *'
    // ---> Dobbiamo passare il puntatore grezzo usando .get()
    addChild(mBusyAnim.get()); // <-- CORREZIONE: Usa .get()
    addChild(mText.get());     // <-- CORREZIONE: Usa .get()

    // Avvia animazione
    mBusyAnim->reset();
}

bool GuiBusyInfoPopup::input(InputConfig* config, Input input)
{
    // Consuma tutti gli input mentre è visibile
    return true;
}

void GuiBusyInfoPopup::update(int deltaTime)
{
    GuiComponent::update(deltaTime); // Aggiorna i figli (BusyComponent si aggiorna qui)
}

void GuiBusyInfoPopup::render(const Transform4x4f& parentTrans)
{
	    LOG(LogInfo) << "[DEBUG_POPUP] GuiBusyInfoPopup::render() called. Visible: " << mVisible; // <-- AGGIUNGI QUESTA
    Transform4x4f trans = parentTrans * getTransform();
    // Non disegniamo uno sfondo qui perché lo fa il NinePatchComponen
    // Renderer::setMatrix(trans);
    // Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0x000000A0);

    // Renderizza figli (sfondo, animazione, testo)
    renderChildren(trans);
}