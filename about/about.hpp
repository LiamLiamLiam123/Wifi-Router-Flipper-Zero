#pragma once
#include "easy_flipper/easy_flipper.h"

class WifiRouterAbout
{
private:
    Widget *widget;
    ViewDispatcher **viewDispatcherRef;

    static constexpr const uint32_t WifiRouterViewIdSubmenu = 1; // View ID for submenu
    static constexpr const uint32_t WifiRouterViewIdAbout = 2;   // View ID for about

    // Backward compatibility aliases
    static constexpr const uint32_t WifiRouterViewSubmenu = WifiRouterViewIdSubmenu;
    static constexpr const uint32_t WifiRouterViewAbout = WifiRouterViewIdAbout;

    static uint32_t callbackToSubmenu(void *context);

public:
    WifiRouterAbout(ViewDispatcher **viewDispatcher);
    ~WifiRouterAbout();
};