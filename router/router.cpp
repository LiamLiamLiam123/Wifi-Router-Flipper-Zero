#include "router/router.hpp"
#include "app.hpp"
#include <gui/view.h>

WifiRouterScreen::WifiRouterScreen(ViewDispatcher **viewDispatcher, void *appContext)
    : appContext(appContext), viewDispatcherRef(viewDispatcher)
{
    lastSsid[0] = '\0';
    lastMessage[0] = '\0';

    if (!easy_flipper_set_view(&view, WifiRouterViewRouter,
                              viewDraw, viewInput, callbackToSubmenu, viewDispatcher, this))
    {
        FURI_LOG_E(TAG, "Failed to allocate router view");
        return;
    }

    // Load configured SSID/channel for display
    WifiRouterApp *app = static_cast<WifiRouterApp *>(appContext);
    char ssid[64];
    char channel[8];
    if (app->loadChar("ap_ssid", ssid, sizeof(ssid)))
    {
        snprintf(lastSsid, sizeof(lastSsid), "%s", ssid);
    }
    else
    {
        snprintf(lastSsid, sizeof(lastSsid), "WifiRouter");
    }
    if (app->loadChar("ap_channel", channel, sizeof(channel)))
    {
        int ch = atoi(channel);
        if (ch >= 1 && ch <= 13)
            lastChannel = (uint8_t)ch;
    }

    snprintf(lastMessage, sizeof(lastMessage), "Press OK to start AP");

    // Start periodic refresh timer (every 1s)
    timer = furi_timer_alloc(timerCallback, FuriTimerTypePeriodic, this);
    if (timer)
    {
        furi_timer_start(timer, 1000);
    }

    if (viewDispatcherRef && *viewDispatcherRef)
    {
        view_dispatcher_switch_to_view(*viewDispatcherRef, WifiRouterViewRouter);
    }
}

WifiRouterScreen::~WifiRouterScreen()
{
    if (timer)
    {
        furi_timer_stop(timer);
        furi_timer_free(timer);
        timer = nullptr;
    }

    if (view && viewDispatcherRef && *viewDispatcherRef)
    {
        view_dispatcher_remove_view(*viewDispatcherRef, WifiRouterViewRouter);
        view_free(view);
        view = nullptr;
    }
}

uint32_t WifiRouterScreen::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return WifiRouterViewSubmenu;
}

void WifiRouterScreen::viewDraw(Canvas *canvas, void *context)
{
    WifiRouterScreen *self = static_cast<WifiRouterScreen *>(context);
    furi_check(self);

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 11, "WifiRouter");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, "SSID:");
    canvas_draw_str(canvas, 36, 24, self->lastSsid);

    char chBuf[32];
    snprintf(chBuf, sizeof(chBuf), "Channel: %u", (unsigned)self->lastChannel);
    canvas_draw_str(canvas, 2, 36, chBuf);

    canvas_draw_str(canvas, 2, 48, "Status:");
    canvas_draw_str(canvas, 36, 48, self->apRunning ? "BROADCASTING" : "Stopped");

    canvas_draw_str(canvas, 2, 60, self->lastMessage);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 126, 64, AlignRight, AlignBottom, "OK=Toggle  Back=Menu");
}

bool WifiRouterScreen::viewInput(InputEvent *event, void *context)
{
    if (event->type != InputTypeShort)
    {
        return false;
    }

    WifiRouterScreen *self = static_cast<WifiRouterScreen *>(context);
    furi_check(self);

    if (event->key == InputKeyOk)
    {
        self->toggleAp();
        return true;
    }

    return false;
}

void WifiRouterScreen::timerCallback(void *context)
{
    WifiRouterScreen *self = static_cast<WifiRouterScreen *>(context);
    furi_check(self);
    self->refreshStatus();
}

void WifiRouterScreen::refreshStatus()
{
    WifiRouterApp *app = static_cast<WifiRouterApp *>(appContext);
    FlipperHTTP *fhttp = app->getFlipperHttp();
    if (!fhttp)
    {
        snprintf(lastMessage, sizeof(lastMessage), "No board");
        apRunning = false;
        return;
    }

    // Query AP status from the board
    if (flipper_http_ap_status(fhttp))
    {
        furi_delay_ms(150);
        if (fhttp->last_response)
        {
            if (strstr(fhttp->last_response, "[AP/ACTIVE]") != NULL ||
                strstr(fhttp->last_response, "[SUCCESS]") != NULL)
            {
                apRunning = true;
                snprintf(lastMessage, sizeof(lastMessage), "Beacons broadcasting");
            }
            else if (strstr(fhttp->last_response, "[AP/IDLE]") != NULL ||
                     strstr(fhttp->last_response, "[AP/STOPPED]") != NULL)
            {
                apRunning = false;
                snprintf(lastMessage, sizeof(lastMessage), "Press OK to start AP");
            }
            else if (strstr(fhttp->last_response, "[ERROR]") != NULL)
            {
                snprintf(lastMessage, sizeof(lastMessage), "Board error");
            }
        }
    }

    if (view)
    {
        // view will be redrawn on next GUI cycle
    }
}

void WifiRouterScreen::toggleAp()
{
    WifiRouterApp *app = static_cast<WifiRouterApp *>(appContext);
    FlipperHTTP *fhttp = app->getFlipperHttp();
    if (!fhttp)
    {
        easy_flipper_dialog("Error", "WiFi devboard not found.\nConnect board & retry.");
        return;
    }

    if (!app->isBoardConnected())
    {
        easy_flipper_dialog("Error", "Board not responding.\nCheck FlipperHTTP firmware.");
        return;
    }

    if (apRunning)
    {
        if (flipper_http_ap_stop(fhttp))
        {
            apRunning = false;
            snprintf(lastMessage, sizeof(lastMessage), "AP stopped");
        }
        else
        {
            snprintf(lastMessage, sizeof(lastMessage), "Failed to stop AP");
        }
        return;
    }

    // Start: load config
    char ssid[64];
    char pass[64];
    char channel[8];

    if (!app->loadChar("ap_ssid", ssid, sizeof(ssid)))
    {
        snprintf(ssid, sizeof(ssid), "WifiRouter");
        app->saveChar("ap_ssid", ssid);
    }
    if (!app->loadChar("ap_pass", pass, sizeof(pass)))
    {
        pass[0] = '\0'; // open network by default
    }
    uint8_t ch = 1;
    if (app->loadChar("ap_channel", channel, sizeof(channel)))
    {
        int v = atoi(channel);
        if (v >= 1 && v <= 13)
            ch = (uint8_t)v;
    }

    snprintf(lastSsid, sizeof(lastSsid), "%s", ssid);
    lastChannel = ch;

    if (flipper_http_ap_start(fhttp, ssid, pass, ch))
    {
        apRunning = true;
        snprintf(lastMessage, sizeof(lastMessage), "AP started - check WiFi");
    }
    else
    {
        snprintf(lastMessage, sizeof(lastMessage), "Failed to start AP");
    }
}