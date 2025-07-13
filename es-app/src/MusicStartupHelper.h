#pragma once

// Forward declaration: dice al compilatore che una classe di nome "Window" esiste,
// senza dover includere l'intero file Window.h.
class Window;

// Questa è la DICHIARAZIONE della funzione.
// Specifica il tipo di ritorno (void), il nome e i tipi dei parametri (Window*).
void startBackgroundMusicBasedOnSetting(Window* window);