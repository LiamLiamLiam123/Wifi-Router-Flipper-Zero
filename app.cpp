#include "app.hpp"

WifiRouterApp::WifiRouterApp()
{
    gui = static_cast<Gui *>(furi_record_open(RECORD_GUI));

    // Allocate ViewDispatcher
    if (!easy_flipper_set_view_dispatcher(&viewDispatcher, gui, this))
    {
        FURI_LOG_E(TAG, "Failed to allocate view dispatcher");
        return;
    }

    // Submenu
    if (!easy_flipper_set_submenu(&submenu, WifiRouterViewSubmenu,
                                  VERSION_TAG, callbackExitApp, &viewDispatcher))
    {
        FURI_LOG_E(TAG, "Failed to allocate submenu");
        return;
    }

    submenu_add_item(submenu, "Start/Stop AP", WifiRouterSubmenuRouter, submenuChoicesCallback, this);
    submenu_add_item(submenu, "Settings", WifiRouterSubmenuSettings, submenuChoicesCallback, this);
    submenu_add_item(submenu, "About", WifiRouterSubmenuAbout, submenuChoicesCallback, this);

    flipperHttp = flipper_http_alloc();
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "Failed to allocate FlipperHTTP");
        return;
    }

    createAppDataPath();

    // Switch to the submenu view
    view_dispatcher_switch_to_view(viewDispatcher, WifiRouterViewSubmenu);
}

WifiRouterApp::~WifiRouterApp()
{
    // Clean up router
    if (router)
    {
        router.reset();
    }

    // Clean up settings
    if (settings)
    {
        settings.reset();
    }

    // Clean up about
    if (about)
    {
        about.reset();
    }

    // Free submenu
    if (submenu)
    {
        view_dispatcher_remove_view(viewDispatcher, WifiRouterViewSubmenu);
        submenu_free(submenu);
    }

    // Free view dispatcher
    if (viewDispatcher)
    {
        view_dispatcher_free(viewDispatcher);
    }

    // Close GUI
    if (gui)
    {
        furi_record_close(RECORD_GUI);
    }

    // Free FlipperHTTP
    if (flipperHttp)
    {
        flipper_http_free(flipperHttp);
    }
}

uint32_t WifiRouterApp::callbackExitApp(void *context)
{
    UNUSED(context);
    return VIEW_NONE;
}

void WifiRouterApp::callbackSubmenuChoices(uint32_t index)
{
    switch (index)
    {
    case WifiRouterSubmenuRouter:
        if (!router)
        {
            router = std::make_unique<WifiRouterScreen>(&viewDispatcher, this);
        }
        else
        {
            view_dispatcher_switch_to_view(viewDispatcher, WifiRouterViewRouter);
        }
        break;
    case WifiRouterSubmenuAbout:
        if (!about)
        {
            about = std::make_unique<WifiRouterAbout>(&viewDispatcher);
        }
        view_dispatcher_switch_to_view(viewDispatcher, WifiRouterViewAbout);
        break;
    case WifiRouterSubmenuSettings:
        if (!settings)
        {
            settings = std::make_unique<WifiRouterSettings>(&viewDispatcher, this);
        }
        view_dispatcher_switch_to_view(viewDispatcher, WifiRouterViewSettings);
        break;
    default:
        break;
    }
}

void WifiRouterApp::createAppDataPath(const char *appId)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    char directory_path[256];
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s", appId);
    storage_common_mkdir(storage, directory_path);
    snprintf(directory_path, sizeof(directory_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data", appId);
    storage_common_mkdir(storage, directory_path);
    furi_record_close(RECORD_STORAGE);
}

bool WifiRouterApp::loadChar(const char *path_name, char *value, size_t value_size, const char *appId)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data/%s.txt", appId, path_name);
    if (!storage_file_open(file, file_path, FSAM_READ, FSOM_OPEN_EXISTING))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t read_count = storage_file_read(file, value, value_size);
    // ensure we don't go out of bounds
    if (read_count > 0 && read_count < value_size)
    {
        value[read_count - 1] = '\0';
    }
    else if (read_count >= value_size && value_size > 0)
    {
        value[value_size - 1] = '\0';
    }
    else
    {
        value[0] = '\0';
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return strlen(value) > 0;
}

bool WifiRouterApp::saveChar(const char *path_name, const char *value, const char *appId, bool overwrite)
{
    Storage *storage = static_cast<Storage *>(furi_record_open(RECORD_STORAGE));
    File *file = storage_file_alloc(storage);
    char file_path[256];
    snprintf(file_path, sizeof(file_path), STORAGE_EXT_PATH_PREFIX "/apps_data/%s/data/%s.txt", appId, path_name);
    if (!storage_file_open(file, file_path, FSAM_WRITE, overwrite ? FSOM_CREATE_ALWAYS : FSOM_OPEN_APPEND))
    {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    size_t append = overwrite ? 1 : 0; // add null terminator if overwriting
    size_t data_size = strlen(value) + append;
    if (storage_file_write(file, value, data_size) != data_size)
    {
        FURI_LOG_E(TAG, "Failed to write complete data to file: %s", file_path);
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return true;
}

void WifiRouterApp::runDispatcher()
{
    view_dispatcher_run(viewDispatcher);
}

bool WifiRouterApp::isBoardConnected()
{
    if (!flipperHttp)
    {
        FURI_LOG_E(TAG, "FlipperHTTP is not initialized");
        return false;
    }

    if (!flipper_http_send_command(flipperHttp, HTTP_CMD_PING))
    {
        FURI_LOG_E(TAG, "Failed to ping the device");
        return false;
    }

    furi_delay_ms(100);

    // Try to wait for pong response.
    uint32_t counter = 100;
    while (flipperHttp->state == INACTIVE && --counter > 0)
    {
        furi_delay_ms(100);
    }

    // last response should be PONG
    return flipperHttp->last_response && strcmp(flipperHttp->last_response, "[PONG]") == 0;
}

void WifiRouterApp::settingsItemSelected(uint32_t index)
{
    if (settings)
    {
        settings->settingsItemSelected(index);
    }
}

void WifiRouterApp::submenuChoicesCallback(void *context, uint32_t index)
{
    WifiRouterApp *app = (WifiRouterApp *)context;
    app->callbackSubmenuChoices(index);
}

extern "C"
{
    int32_t wifirouter_main(void *p)
    {
        // Suppress unused parameter warning
        UNUSED(p);

        // Create the app
        WifiRouterApp app;

        // Run the app
        app.runDispatcher();

        // return success
        return 0;
    }
}