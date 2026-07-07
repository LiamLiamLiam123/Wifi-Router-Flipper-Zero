#pragma once
#include "easy_flipper/easy_flipper.h"

class WifiRouterApp;

class WifiRouterScreen
{
private:
    void *appContext;          // reference to the app context (WifiRouterApp *)
    View *view = nullptr;      // custom status view
    ViewDispatcher **viewDispatcherRef;
    FuriTimer *timer = nullptr; // periodic status refresh timer
    bool apRunning = false;     // cached AP state (last known)
    char lastSsid[64];          // last SSID used
    uint8_t lastChannel = 1;    // last channel used
    char lastMessage[128];      // last status / error message

    static uint32_t callbackToSubmenu(void *context);
    static void viewDraw(Canvas *canvas, void *context);
    static bool viewInput(InputEvent *event, void *context);
    static void timerCallback(void *context);

    void refreshStatus();
    void toggleAp();

public:
    WifiRouterScreen(ViewDispatcher **viewDispatcher, void *appContext);
    ~WifiRouterScreen();

    bool isActive() const { return view != nullptr; }
};