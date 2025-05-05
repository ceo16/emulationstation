#include "components/TextEditComponent.h"

#include "resources/Font.h"
#include "utils/StringUtil.h"
#include "LocaleES.h"
#include "Window.h"
#include "ThemeData.h" // Aggiunto per ThemeData::getMenuTheme()
#include "InputConfig.h" // Aggiunto per BUTTON_OK, ecc.
#include "Log.h"

// --- ASSICURATI CHE QUESTI SIANO QUI ---
#include <SDL_clipboard.h>
#include <SDL_keyboard.h>
#include <SDL_timer.h> // Per SDL_GetKeyboardState se non già incluso altrove

#define TEXT_PADDING_HORIZ 10
#define TEXT_PADDING_VERT 2

#define CURSOR_REPEAT_START_DELAY 500
#define CURSOR_REPEAT_SPEED 28 // lower is faster

#define BLINKTIME	1000

TextEditComponent::TextEditComponent(Window* window) : GuiComponent(window),
	mBox(window, ":/textinput_ninepatch.png"), mFocused(false), 
	mScrollOffset(0.0f, 0.0f), mCursor(0), mEditing(false), mFont(Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT)), 
	mCursorRepeatDir(0)
{
	mBlinkTime = 0;
	mDeferTextInputStart = false;

	auto theme = ThemeData::getMenuTheme();
	mBox.setImagePath(ThemeData::getMenuTheme()->Icons.textinput_ninepatch);

	addChild(&mBox);
	
	onFocusLost();

	setSize(4096, mFont->getHeight() + TEXT_PADDING_VERT);
}

void TextEditComponent::onFocusGained()
{
	mFocused = true;
	mBox.setImagePath(ThemeData::getMenuTheme()->Icons.textinput_ninepatch_active);	

	mWindow->postToUiThread([this]() { startEditing(); });
}

void TextEditComponent::onFocusLost()
{
	mFocused = false;
	mBox.setImagePath(ThemeData::getMenuTheme()->Icons.textinput_ninepatch);
	
	mWindow->postToUiThread([this]() { stopEditing(); });
}

void TextEditComponent::onSizeChanged()
{
	GuiComponent::onSizeChanged();
	mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-34, -32 - TEXT_PADDING_VERT));
	onTextChanged(); // wrap point probably changed
}

void TextEditComponent::setValue(const std::string& val)
{
	mText = val;
	onTextChanged();
}

std::string TextEditComponent::getValue() const
{
	return mText;
}

void TextEditComponent::textInput(const char* text)
{
	if (mEditing)
	{
		mCursorRepeatDir = 0;
		if (text[0] == '\b')
		{
			if (mCursor > 0)
			{
				size_t newCursor = Utils::String::prevCursor(mText, mCursor);
				mText.erase(mText.begin() + newCursor, mText.begin() + mCursor);
				mCursor = (unsigned int)newCursor;
			}
		}
		else if (mCursor > 2 && Utils::String::isKorean(text) && Utils::String::isKorean(mText.substr(mCursor - 3, 3).c_str()))
		{
			Utils::String::koreanTextInput(text, mText, mCursor);
		}
		else {
			mText.insert(mCursor, text);
			size_t newCursor = Utils::String::nextCursor(mText, mCursor);
			mCursor = (unsigned int)newCursor;
		}
	}

	onTextChanged();
	onCursorChanged();
}

bool TextEditComponent::hasAnyKeyPressed()
{
	bool anyKeyPressed = false;

	int numKeys;
	const Uint8* keys = SDL_GetKeyboardState(&numKeys);
	for (int i = 0; i < numKeys && !anyKeyPressed; i++)
		anyKeyPressed |= keys[i];

	return anyKeyPressed;
}

void TextEditComponent::startEditing()
{
	if (mEditing)
		return;
	
	if (hasAnyKeyPressed())
	{
		// Defer if a key is pressed to avoid repeat behaviour if a TextEditComponent is opened with a keypress
		mDeferTextInputStart = true;
	}
	else
	{
		mDeferTextInputStart = false;
		SDL_StartTextInput();
	}

	mEditing = true;
	updateHelpPrompts();
}

void TextEditComponent::stopEditing()
{
	if (!mEditing)
		return;

	SDL_StopTextInput();
	mEditing = false;
	mDeferTextInputStart = false;
	updateHelpPrompts();
}

bool TextEditComponent::input(InputConfig* config, Input input)
{
    bool const cursor_left = (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("left", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_LEFT);
    bool const cursor_right = (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("right", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RIGHT);

    // --- Gestione Rilascio Tasti ---
    if (input.value == 0)
    {
        if (cursor_left || cursor_right)
            mCursorRepeatDir = 0;

        // Ritorna false per il rilascio, non gestiamo eventi "key up" speciali qui
        return false;
    }

    // --- Se l'evento è una pressione (input.value != 0) ---

    // Se siamo in modalità modifica
    if (mEditing)
    {
        // Gestione specifica per la TASTIERA
        if (config->getDeviceId() == DEVICE_KEYBOARD)
        {
			if (input.id == SDLK_v) {
     SDL_Keymod modState = SDL_GetModState();
     LOG(LogInfo) << "Pressed V key. Mod state: " << modState << " Ctrl pressed? " << (modState & KMOD_CTRL);
	 }
            // --- CONTROLLO PER INCOLLA (Ctrl+V) ---
            // Prima dello switch per gestirlo prioritariamente
            if (input.id == SDLK_v && (SDL_GetModState() & KMOD_CTRL))
            {
                LOG(LogInfo) << "Ctrl+V detected, calling handlePaste()"; // Aggiungi log qui
                handlePaste();
                return true; // Input gestito
            }
            // --- FINE CONTROLLO PER INCOLLA ---

            // Gestione altri tasti speciali
            switch (input.id)
            {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                    mBlinkTime = 0;
                    mCursorRepeatDir = (input.id == SDLK_LEFT) ? -1 : 1;
                    mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
                    moveCursor(mCursorRepeatDir);
                    return true; // Consuma l'input freccia

                case SDLK_HOME:
                    setCursor(0);
                    return true;

                case SDLK_END:
                    setCursor(std::string::npos);
                    return true;

                case SDLK_DELETE:
                    if (mCursor < mText.length())
                    {
                        size_t nextCursor = Utils::String::nextCursor(mText, mCursor);
                        mText.erase(mText.begin() + mCursor, mText.begin() + nextCursor);
                        onTextChanged();
                        onCursorChanged();
                    }
                    return true;

                case SDLK_RETURN: // Tasto Invio
                {
                    if (isMultiline())
                    {
                        textInput("\n");
                    } else {
                        stopEditing();
                        // Il popup gestirà il salvataggio/chiusura
                        return false; // Lascia che GuiTextEditPopup gestisca Invio
                    }
                    return true; // Consuma Invio se multiline
                }

                case SDLK_ESCAPE: // Tasto Esc
                    stopEditing();
                    // Il popup gestirà l'annullamento/chiusura
                    return false; // Lascia che GuiTextEditPopup gestisca Esc

                default:
                    // Consuma altri tasti premuti durante la modifica
                    // L'input di testo effettivo avviene tramite l'evento SDL_TEXTINPUT gestito da Window::textInput
                    return true;
            } // Fine switch(input.id)
        }
        // Gestione per ALTRI DISPOSITIVI (non tastiera) durante l'editing
        else
        {
            if (cursor_left || cursor_right)
            {
                mBlinkTime = 0;
                mCursorRepeatDir = cursor_left ? -1 : 1;
                mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
                moveCursor(mCursorRepeatDir);
                return true; // Consuma freccia
            }
             else if (config->isMappedTo(BUTTON_OK, input))
             {
                 stopEditing();
                 // Il popup gestirà il salvataggio/chiusura
                 return false; // Lascia che GuiTextEditPopup gestisca OK
             }
             else if (config->isMappedTo(BUTTON_BACK, input))
             {
                 stopEditing();
                 // Il popup gestirà l'annullamento/chiusura
                 return false; // Lascia che GuiTextEditPopup gestisca BACK
             }
        }
    }
    // Se NON siamo in modalità modifica
    else
    {
        // Gestione tastiera quando NON si modifica
        if (config->getDeviceId() == DEVICE_KEYBOARD)
        {
             // Se premi Invio sulla tastiera e il componente ha focus -> inizia modifica
             if (input.id == SDLK_RETURN && mFocused)
             {
                 startEditing();
                 return true; // Consuma Invio
             }
        }
        // Gestione altri dispositivi quando NON si modifica
        else
        {
            // Se premi OK e il componente ha focus -> inizia modifica
            if (config->isMappedTo(BUTTON_OK, input) && mFocused)
            {
                startEditing();
                return true; // Consuma OK
            }
        }

        // Non gestire altri input (come frecce, back, ecc.) quando non si modifica,
        // lasciali propagare alla GUI superiore (ComponentList, MenuComponent, ecc.)
        return false;

    } // Fine else (!mEditing)

    // Se l'input non è stato gestito sopra, lascialo passare
    return false;
}

void TextEditComponent::update(int deltaTime)
{
	if (mEditing && mDeferTextInputStart)
	{
		if (!hasAnyKeyPressed())
		{
			SDL_StartTextInput();
			mDeferTextInputStart = false;
		}
	}

	mBlinkTime += deltaTime;
	if (mBlinkTime >= BLINKTIME)
		mBlinkTime = 0;

	updateCursorRepeat(deltaTime);
	GuiComponent::update(deltaTime);
}

void TextEditComponent::updateCursorRepeat(int deltaTime)
{
	if(mCursorRepeatDir == 0)
		return;

	mCursorRepeatTimer += deltaTime;
	while(mCursorRepeatTimer >= CURSOR_REPEAT_SPEED)
	{
		moveCursor(mCursorRepeatDir);
		mCursorRepeatTimer -= CURSOR_REPEAT_SPEED;
	}
}

void TextEditComponent::moveCursor(int amt)
{
	mCursor = (unsigned int)Utils::String::moveCursor(mText, mCursor, amt);
	onCursorChanged();
}

void TextEditComponent::setCursor(size_t pos)
{
	if(pos == std::string::npos)
		mCursor = (unsigned int)mText.length();
	else
		mCursor = (int)pos;

	moveCursor(0);
}

void TextEditComponent::handlePaste() {
    // Controlla se il componente è in modalità modifica e se c'è testo negli appunti
    if (mEditing && SDL_HasClipboardText()) { // mEditing indica se il componente sta accettando input testuale
        char* clipboardText_cstr = SDL_GetClipboardText();
        if (clipboardText_cstr) {
            std::string clipboardText = clipboardText_cstr; // Copia in una std::string
            SDL_free(clipboardText_cstr); // *** IMPORTANTE: Libera la memoria allocata da SDL ***

            // Opzionale: Pulisci/filtra il testo (es. rimuovi ritorni a capo se non è multiriga)
            if (!isMultiline()) { // Controlla se il campo è multiriga
               clipboardText = Utils::String::replace(clipboardText, "\n", "");
               clipboardText = Utils::String::replace(clipboardText, "\r", "");
            }

            // Inserisci il testo alla posizione corrente del cursore
            mText.insert(mCursor, clipboardText); // mText è la stringa del componente, mCursor la posizione
            mCursor += (unsigned int)clipboardText.length(); // Aggiorna la posizione del cursore

            // Aggiorna la cache del testo e la posizione dello scroll/cursore
            onTextChanged();   // Funzione per aggiornare la visualizzazione del testo
            onCursorChanged(); // Funzione per aggiustare lo scroll in base al cursore
        }
    }
}

void TextEditComponent::onTextChanged()
{
	std::string wrappedText = (isMultiline() ? mFont->wrapText(mText, getTextAreaSize().x()) : mText);
	mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(wrappedText, 0, 0, (ThemeData::getMenuTheme()->Text.color & 0xFFFFFF00) | getOpacity()));
	
	if(mCursor > (int)mText.length())
		mCursor = (unsigned int)mText.length();
}

void TextEditComponent::onCursorChanged()
{
	if(isMultiline())
	{
		Vector2f textSize = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor); 

		if(mScrollOffset.y() + getTextAreaSize().y() < textSize.y() + mFont->getHeight()) //need to scroll down?
		{
			mScrollOffset[1] = textSize.y() - getTextAreaSize().y() + mFont->getHeight();
		}else if(mScrollOffset.y() > textSize.y()) //need to scroll up?
		{
			mScrollOffset[1] = textSize.y();
		}
	}else{
		Vector2f cursorPos = mFont->sizeText(mText.substr(0, mCursor));

		if(mScrollOffset.x() + getTextAreaSize().x() < cursorPos.x())
		{
			mScrollOffset[0] = cursorPos.x() - getTextAreaSize().x();
		}else if(mScrollOffset.x() > cursorPos.x())
		{
			mScrollOffset[0] = cursorPos.x();
		}
	}
}

void TextEditComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = getTransform() * parentTrans;

	auto rect = Renderer::getScreenRect(trans, mSize);
	if (!Renderer::isVisibleOnScreen(rect))
		return;

	renderChildren(trans);

	// text + cursor rendering
	// offset into our "text area" (padding)
	trans.translation() += Vector3f(getTextAreaPos().x(), getTextAreaPos().y(), 0);

	Vector2i clipPos((int)trans.translation().x(), (int)trans.translation().y());
	Vector3f dimScaled = trans * Vector3f(getTextAreaSize().x(), getTextAreaSize().y(), 0); // use "text area" size for clipping
	Vector2i clipDim((int)(dimScaled.x() - trans.translation().x()), (int)(dimScaled.y() - trans.translation().y()));
	Renderer::pushClipRect(clipPos, clipDim);

	trans.translate(Vector3f(-mScrollOffset.x(), -mScrollOffset.y(), 0));
	Renderer::setMatrix(trans);

	if(mTextCache)
	{
		mFont->renderTextCache(mTextCache.get());
	}

	// pop the clip early to allow the cursor to be drawn outside of the "text area"
	Renderer::popClipRect();

	// draw cursor
	//if(mEditing)
	{
		Vector2f cursorPos;
		if(isMultiline())
		{
			cursorPos = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor);
		}
		else
		{
			cursorPos = mFont->sizeText(mText.substr(0, mCursor));
			cursorPos[1] = 0;
		}

		if (!mEditing || mBlinkTime < BLINKTIME / 2 || mCursorRepeatDir != 0)
		{
			float cursorHeight = mFont->getHeight() * 0.8f;

			auto cursorColor = (ThemeData::getMenuTheme()->Text.color & 0xFFFFFF00) | getOpacity();
			if (!mEditing)
				cursorColor = (ThemeData::getMenuTheme()->Text.color & 0xFFFFFF00) | (unsigned char) (getOpacity() * 0.25f);

			Renderer::drawRect(cursorPos.x(), cursorPos.y() + (mFont->getHeight() - cursorHeight) / 2, 2.0f, cursorHeight, cursorColor, cursorColor); // 0x000000FF
		}
	}
}

bool TextEditComponent::isMultiline()
{
	return (getSize().y() > mFont->getHeight() * 1.25f);
}

Vector2f TextEditComponent::getTextAreaPos() const
{
	return Vector2f(TEXT_PADDING_HORIZ / 2.0f, TEXT_PADDING_VERT / 2.0f);
}

Vector2f TextEditComponent::getTextAreaSize() const
{
	return Vector2f(mSize.x() - TEXT_PADDING_HORIZ, mSize.y() - TEXT_PADDING_VERT);
}

std::vector<HelpPrompt> TextEditComponent::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	
	if(mEditing)
		prompts.push_back(HelpPrompt("up/down/left/right", _("MOVE CURSOR")));
	else
		prompts.push_back(HelpPrompt(BUTTON_OK, _("EDIT")));
	
	return prompts;
}
