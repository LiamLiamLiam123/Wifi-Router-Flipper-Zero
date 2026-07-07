#include "font/font.h"
#include "easy_flipper/easy_flipper.h"
#include "flipper_http/flipper_http.h"
#include "router/router.hpp"
#include "settings/settings.hpp"
#include "about/about.hpp"

#define TAG "WifiRouter"
#define VERSION "1.0"
#define VERSION_TAG TAG " " VERSION
#define APP_ID "wifirouter"

typedef enum
{
    WifiRouterSubmenuRouter = 0,
    WifiRouterSubmenuAbout = 1,
    WifiRouterSubmenuSettings = 2,
} WifiRouterSubmenuIndex;

typedef enum
{
    WifiRouterViewMain = 0,
    WifiRouterViewSubmenu = 1,
    WifiRouterViewAbout = 2,
    WifiRouterViewSettings = 3,
    WifiRouterViewTextInput = 4,
    WifiRouterViewRouter = 5,
} WifiRouterView;

class WifiRouterApp
{
private:
    std::unique_ptr<WifiRouterAbout> about;          // About class instance
    FlipperHTTP *flipperHttp = nullptr;              // FlipperHTTP instance for UART comms
    std::unique_ptr<WifiRouterScreen> router;        // Router (AP control) view instance
    std::unique_ptr<WifiRouterSettings> settings;    // Settings class instance
    Submenu *submenu = nullptr;                      // Submenu for the app
    //
    static uint32_t callbackExitApp(void *context);                    // Callback to exit the app
    void callbackSubmenuChoices(uint32_t index);                       // Callback for submenu choices
    void createAppDataPath(const char *appId = APP_ID);                // Create the app data path in storage
    void settingsItemSelected(uint32_t index);                         // Handle settings item selection
    static void submenuChoicesCallback(void *context, uint32_t index); // Callback for submenu choices

public:
    WifiRouterApp();
    ~WifiRouterApp();
    //
    Gui *gui = nullptr;                       // GUI instance for the app
    ViewDispatcher *viewDispatcher = nullptr; // ViewDispatcher for managing views
    //
    FlipperHTTP *getFlipperHttp() const noexcept { return flipperHttp; }          // get the FlipperHTTP handle
    bool loadChar(const char *path_name, char *value, size_t value_size, const char *appId = APP_ID);           // load a string from storage
    bool saveChar(const char *path_name, const char *value, const char *appId = APP_ID, bool overwrite = true); // save a string to storage
    void runDispatcher();                                                                                       // run the app's view dispatcher to handle views and events
    bool isBoardConnected();                                                                                    // check if the board is connected
};