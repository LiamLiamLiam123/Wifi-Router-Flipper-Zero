#include "settings.hpp"
#include "app.hpp"

WifiRouterSettings::WifiRouterSettings(ViewDispatcher **view_dispatcher, WifiRouterApp *appContext) : appContext(appContext), view_dispatcher_ref(view_dispatcher)
{
    if (!easy_flipper_set_variable_item_list(&variable_item_list, WifiRouterViewSettings,
                                             settingsItemSelectedCallback, callbackToSubmenu, view_dispatcher, this))
    {
        return;
    }

    variable_item_ap_ssid = variable_item_list_add(variable_item_list, "AP SSID", 1, nullptr, nullptr);
    variable_item_ap_pass = variable_item_list_add(variable_item_list, "AP Password", 1, nullptr, nullptr);
    variable_item_ap_channel = variable_item_list_add(variable_item_list, "AP Channel", 13, nullptr, nullptr);

    char loaded[64];
    if (appContext->loadChar("ap_ssid", loaded, sizeof(loaded)))
    {
        variable_item_set_current_value_text(variable_item_ap_ssid, loaded);
    }
    else
    {
        variable_item_set_current_value_text(variable_item_ap_ssid, "WifiRouter");
    }
    if (appContext->loadChar("ap_pass", loaded, sizeof(loaded)))
    {
        variable_item_set_current_value_text(variable_item_ap_pass, "*****");
    }
    else
    {
        variable_item_set_current_value_text(variable_item_ap_pass, "open");
    }
    if (appContext->loadChar("ap_channel", loaded, sizeof(loaded)))
    {
        variable_item_set_current_value_text(variable_item_ap_channel, loaded);
    }
    else
    {
        variable_item_set_current_value_text(variable_item_ap_channel, "1");
    }
}

WifiRouterSettings::~WifiRouterSettings()
{
    // Free text input first
    freeTextInput();

    if (variable_item_list && view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_remove_view(*view_dispatcher_ref, WifiRouterViewSettings);
        variable_item_list_free(variable_item_list);
        variable_item_list = nullptr;
        variable_item_ap_ssid = nullptr;
        variable_item_ap_pass = nullptr;
        variable_item_ap_channel = nullptr;
    }
}

uint32_t WifiRouterSettings::callbackToSettings(void *context)
{
    UNUSED(context);
    return WifiRouterViewSettings;
}

uint32_t WifiRouterSettings::callbackToSubmenu(void *context)
{
    UNUSED(context);
    return WifiRouterViewSubmenu;
}

void WifiRouterSettings::freeTextInput()
{
    if (text_input && view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_remove_view(*view_dispatcher_ref, WifiRouterViewTextInput);
#ifndef FW_ORIGIN_Momentum
        uart_text_input_free(text_input);
#else
        text_input_free(text_input);
#endif
        text_input = nullptr;
    }
    text_input_buffer.reset();
    text_input_temp_buffer.reset();
}

bool WifiRouterSettings::initTextInput(uint32_t view)
{
    // check if already initialized
    if (text_input_buffer || text_input_temp_buffer)
    {
        FURI_LOG_E(TAG, "initTextInput: already initialized");
        return false;
    }

    // init buffers
    text_input_buffer_size = 128;
    if (!easy_flipper_set_buffer(reinterpret_cast<char **>(&text_input_buffer), text_input_buffer_size))
    {
        return false;
    }
    if (!easy_flipper_set_buffer(reinterpret_cast<char **>(&text_input_temp_buffer), text_input_buffer_size))
    {
        return false;
    }

    char loaded[256];

    if (view == SettingsViewAPSSID)
    {
        if (appContext->loadChar("ap_ssid", loaded, sizeof(loaded)))
        {
            strncpy(text_input_temp_buffer.get(), loaded, text_input_buffer_size);
        }
        else
        {
            text_input_temp_buffer[0] = '\0'; // Ensure empty if not loaded
        }
        text_input_temp_buffer[text_input_buffer_size - 1] = '\0'; // Ensure null-termination
#ifndef FW_ORIGIN_Momentum
        return easy_flipper_set_uart_text_input(&text_input, WifiRouterViewTextInput,
                                                "Enter AP SSID", text_input_temp_buffer.get(), text_input_buffer_size,
                                                textUpdatedSsidCallback, callbackToSettings, view_dispatcher_ref, this);
#else
        return easy_flipper_set_text_input(&text_input, WifiRouterViewTextInput,
                                           "Enter AP SSID", text_input_temp_buffer.get(), text_input_buffer_size,
                                           textUpdatedSsidCallback, callbackToSettings, view_dispatcher_ref, this);
#endif
    }
    else if (view == SettingsViewAPPass)
    {
        if (appContext->loadChar("ap_pass", loaded, sizeof(loaded)))
        {
            strncpy(text_input_temp_buffer.get(), loaded, text_input_buffer_size);
        }
        else
        {
            text_input_temp_buffer[0] = '\0'; // Ensure empty if not loaded
        }
        text_input_temp_buffer[text_input_buffer_size - 1] = '\0'; // Ensure null-termination
#ifndef FW_ORIGIN_Momentum
        return easy_flipper_set_uart_text_input(&text_input, WifiRouterViewTextInput,
                                                "Enter AP Password", text_input_temp_buffer.get(), text_input_buffer_size,
                                                textUpdatedPassCallback, callbackToSettings, view_dispatcher_ref, this);
#else
        return easy_flipper_set_text_input(&text_input, WifiRouterViewTextInput,
                                           "Enter AP Password", text_input_temp_buffer.get(), text_input_buffer_size,
                                           textUpdatedPassCallback, callbackToSettings, view_dispatcher_ref, this);
#endif
    }
    else if (view == SettingsViewAPChannel)
    {
        if (appContext->loadChar("ap_channel", loaded, sizeof(loaded)))
        {
            strncpy(text_input_temp_buffer.get(), loaded, text_input_buffer_size);
        }
        else
        {
            text_input_temp_buffer[0] = '\0';
        }
        text_input_temp_buffer[text_input_buffer_size - 1] = '\0';
#ifndef FW_ORIGIN_Momentum
        return easy_flipper_set_uart_text_input(&text_input, WifiRouterViewTextInput,
                                                "Enter Channel (1-13)", text_input_temp_buffer.get(), text_input_buffer_size,
                                                textUpdatedChannelCallback, callbackToSettings, view_dispatcher_ref, this);
#else
        return easy_flipper_set_text_input(&text_input, WifiRouterViewTextInput,
                                           "Enter Channel (1-13)", text_input_temp_buffer.get(), text_input_buffer_size,
                                           textUpdatedChannelCallback, callbackToSettings, view_dispatcher_ref, this);
#endif
    }
    return false;
}

void WifiRouterSettings::settingsItemSelected(uint32_t index)
{
    switch (index)
    {
    case SettingsViewAPSSID:
    case SettingsViewAPPass:
    case SettingsViewAPChannel:
        startTextInput(index);
        break;
    default:
        break;
    };
}

void WifiRouterSettings::settingsItemSelectedCallback(void *context, uint32_t index)
{
    WifiRouterSettings *settings = (WifiRouterSettings *)context;
    settings->settingsItemSelected(index);
}

bool WifiRouterSettings::startTextInput(uint32_t view)
{
    freeTextInput();
    if (!initTextInput(view))
    {
        FURI_LOG_E(TAG, "Failed to initialize text input for view %lu", view);
        return false;
    }
    if (view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_switch_to_view(*view_dispatcher_ref, WifiRouterViewTextInput);
        return true;
    }
    else
    {
        FURI_LOG_E(TAG, "View dispatcher reference is null or invalid");
        return false;
    }
}

void WifiRouterSettings::textUpdated(uint32_t view)
{
    // store the entered text
    strncpy(text_input_buffer.get(), text_input_temp_buffer.get(), text_input_buffer_size);

    // Ensure null-termination
    text_input_buffer[text_input_buffer_size - 1] = '\0';

    switch (view)
    {
    case SettingsViewAPSSID:
        if (variable_item_ap_ssid)
        {
            variable_item_set_current_value_text(variable_item_ap_ssid, text_input_buffer.get());
        }
        appContext->saveChar("ap_ssid", text_input_buffer.get());
        break;
    case SettingsViewAPPass:
        if (variable_item_ap_pass)
        {
            variable_item_set_current_value_text(variable_item_ap_pass, text_input_buffer.get());
        }
        appContext->saveChar("ap_pass", text_input_buffer.get());
        break;
    case SettingsViewAPChannel:
        {
            int ch = atoi(text_input_buffer.get());
            if (ch >= 1 && ch <= 13)
            {
                if (variable_item_ap_channel)
                {
                    variable_item_set_current_value_text(variable_item_ap_channel, text_input_buffer.get());
                }
                appContext->saveChar("ap_channel", text_input_buffer.get());
            }
        }
        break;
    default:
        break;
    }

    // switch to the settings view
    if (view_dispatcher_ref && *view_dispatcher_ref)
    {
        view_dispatcher_switch_to_view(*view_dispatcher_ref, WifiRouterViewSettings);
    }
}

void WifiRouterSettings::textUpdatedSsidCallback(void *context)
{
    WifiRouterSettings *settings = (WifiRouterSettings *)context;
    settings->textUpdated(SettingsViewAPSSID);
}

void WifiRouterSettings::textUpdatedPassCallback(void *context)
{
    WifiRouterSettings *settings = (WifiRouterSettings *)context;
    settings->textUpdated(SettingsViewAPPass);
}

void WifiRouterSettings::textUpdatedChannelCallback(void *context)
{
    WifiRouterSettings *settings = (WifiRouterSettings *)context;
    settings->textUpdated(SettingsViewAPChannel);
}