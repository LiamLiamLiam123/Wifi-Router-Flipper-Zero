#include "about/about.hpp"

WifiRouterAbout::WifiRouterAbout(ViewDispatcher **viewDispatcher) : widget(nullptr), viewDispatcherRef(viewDispatcher)
{
    easy_flipper_set_widget(&widget, WifiRouterViewAbout,
                            "WifiRouter\n\nBroadcasts a SoftAP\nwith configurable SSID,\npassword, and channel.\n\nRequires FlipperHTTP\nfirmware on ESP32\nWiFi devboard.\n\ngithub.com/jblanked/\nFlipperHTTP",
                            callbackToSubmenu, viewDispatcherRef);
}

WifiRouterAbout::~WifiRouterAbout()
{
    if (widget && viewDispatcherRef && *viewDispatcherRef)
    {
        view_dispatcher_remove_view(*viewDispatcherRef, WifiRouterViewAbout);
        widget_free(widget);
        widget = nullptr;
    }
}

uint32_t WifiRouterAbout::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return WifiRouterViewSubmenu;
}