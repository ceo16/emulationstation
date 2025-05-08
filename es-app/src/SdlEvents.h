#pragma once
#ifndef ES_APP_SDL_EVENTS_H
#define ES_APP_SDL_EVENTS_H

#include <SDL.h>
#include <SDL_events.h>

#define SDL_STEAM_REFRESH_COMPLETE              (SDL_USEREVENT + 4)
#define SDL_STEAM_METADATA_UPDATE_COMPLETE      (SDL_USEREVENT + 5) // Era SDL_STEAM_METADATA_UPDATED, standardizziamo a _COMPLETE
#define SDL_STEAM_LOGIN_SUCCESS                 (SDL_USEREVENT + 6) // Nuovo
#define SDL_STEAM_LOGIN_FAILURE                 (SDL_USEREVENT + 7) // Nuovo
#define SDL_STEAM_LOGOUT_COMPLETE               (SDL_USEREVENT + 8) // Nuovo

// Dichiarazione 'extern' dell'ID evento globale
// Questo dice al compilatore "questa variabile esiste, ma è definita altrove"
extern Uint32 SDL_EPIC_REFRESH_COMPLETE; // //


#endif // ES_APP_SDL_EVENTS_H