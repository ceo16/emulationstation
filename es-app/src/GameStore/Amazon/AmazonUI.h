#pragma once
#ifndef ES_APP_GAMESTORE_AMAZON_UI_H
#define ES_APP_GAMESTORE_AMAZON_UI_H

class Window;
class AmazonGamesStore;

class AmazonUI
{
public:
    AmazonUI(Window* window);
    void openAmazonStoreMenu();

private:
    Window* mWindow;
    AmazonGamesStore* mStore;
};

#endif // ES_APP_GAMESTORE_AMAZON_UI_H