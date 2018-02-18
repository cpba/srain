/* Copyright (C) 2016-2017 Shengyu Zhang <i@silverrainz.me>
 *
 * This file is part of Srain.
 *
 * Srain is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file app.c
 * @brief Srain's application class implementation
 * @author Shengyu Zhang <i@silverrainz.me>
 * @version 0.06.2
 * @date 2016-03-01
 */

#include <glib.h>

#include "sui/sui.h"
#include "meta.h"
#include "log.h"
#include "i18n.h"

G_DEFINE_TYPE(SrnApplication, srn_application, G_TYPE_APPLICATION);

/* Only one SrnApplication instance in one application */
SrnApplication *srn_app = NULL;

static GOptionEntry option_entries[] = {
    {
        .long_name = "version",
        .short_name = 'v',
        .flags = 0,
        .arg = G_OPTION_ARG_NONE,
        .arg_data = NULL,
        .description = N_("Show version information"),
        .arg_description = NULL,
    },
    {
        .long_name = G_OPTION_REMAINING,
        .short_name = '\0',
        .flags = 0,
        .arg = G_OPTION_ARG_STRING_ARRAY,
        .arg_data = NULL,
        .description = N_("Open one or more IRC URLs"),
        .arg_description = N_("[URL…]")
    },
    {NULL}
};

static SrnRet create_window(GApplication *app);
static void on_activate(GApplication *app);
static void on_shutdown(GApplication *app);
static int on_handle_local_options(GApplication *app, GVariantDict *options,
        gpointer user_data);
static int on_command_line(GApplication *app,
        GApplicationCommandLine *cmdline, gpointer user_data);

static void srn_application_init(SrnApplication *self){
    if (srn_app) return;
    srn_app = self;

    g_application_add_main_option_entries(G_APPLICATION(self), option_entries);

    g_signal_connect(self, "activate", G_CALLBACK(on_activate), NULL);
    g_signal_connect(self, "shutdown", G_CALLBACK(on_shutdown), NULL);
    g_signal_connect(self, "command-line", G_CALLBACK(on_command_line), NULL);
    g_signal_connect(self, "handle-local-options", G_CALLBACK(on_handle_local_options), NULL);
}

static void srn_application_class_init(SrnApplicationClass *class){
}

SrnApplication* srn_application_new(void){
    char *path;
    SrnRet ret;
    SrnVersion *ver;
    SrnPrefs *ver;
    SrnConfigManager *cfg_mgr;
    SrnApplication *app;
    SrnApplicationConfig *cfg;

    ver = srn_version_new();
    ret = srn_version_parse(PACKAGE_VERSION PACKAGE_BUILD);
    if (!RET_IS_OK(ret)){
        ERR_FR("Failed to parse " PACKAGE_VERSION PACKAGE_BUILD
                "as application version: %s", RET_MSG(ret));
        return NULL;
    }

    cfg_mgr = srn_config_manager_new(ver);
    path = get_system_config_file("builtin.cfg");
    if (path){
        ret = srn_config_manager_read_system_config(cfg_mgr, path);
        g_free(path);
        if (!RET_IS_OK(ret)){
            sui_message_box(_("Error"), RET_MSG(ret));
        }
    }
    path = get_config_file("srain.cfg");
    if (path){
        ret = srn_config_manager_read_user_config(cfg_mgr, path);
        g_free(path);
        if (!RET_IS_OK(ret)){
            sui_message_box(_("Error"), RET_MSG(ret));
        }
    }

    cfg = srn_application_config_new();
    srn_config_mananger_read_application_config(cfg_mgr, cfg);

    app = g_object_new(SRN_TYPE_APPLICATION,
            "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
            "application-id", cfg->id ? cfg->id : "im.srain.Srain",
            NULL);

    // TODO: impl as gobject properties?
    app->ver = ver;
    app->cfg = cfg;
    app->cfg_mgr = cfg_mgr;

    return app;
}

void srn_application_quit(SrnApplication *app){
    g_application_quit(G_APPLICATION(app));
}

static SrnRet create_window(GApplication *app){
    if (srain_win){
        gtk_window_present(GTK_WINDOW(srain_win));
        return SRN_ERR;
    };

    srain_win = srain_window_new(SRAIN_APP(app));
    gtk_window_present(GTK_WINDOW(srain_win));

    return SRN_OK;
}

static void on_activate(GApplication *app){
    SrnRet ret;

    ret = create_window(app);
    if (!RET_IS_OK(ret)){
        return;
    }

    // ret = sui_event_hdr(NULL, SUI_EVENT_ACTIVATE, NULL);
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Error"), RET_MSG(ret));
    }
}

static void on_shutdown(GApplication *app){
    // sui_event_hdr(NULL, SUI_EVENT_SHUTDOWN, NULL);
}

static int on_handle_local_options(GApplication *app, GVariantDict *options,
        gpointer user_data){
    if (g_variant_dict_lookup(options, "version", "b", NULL)){
        g_print("%s %s%s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUILD);
        return 0; // Exit
    }

    return -1; // Return -1 to let the default option processing continue.
}

static int on_command_line(GApplication *app,
        GApplicationCommandLine *cmdline, gpointer user_data){
    char **urls;
    GVariantDict *options;
    GVariantDict* params;

    options = g_application_command_line_get_options_dict(cmdline);
    if (g_variant_dict_lookup(options, G_OPTION_REMAINING, "^as", &urls)){
        /* If we have URLs to open, create window firstly. */
        create_window(app);

        params = g_variant_dict_new(NULL);
        g_variant_dict_insert(params, "urls", SUI_EVENT_PARAM_STRINGS,
                urls, g_strv_length(urls));

        // sui_event_hdr(NULL, SUI_EVENT_OPEN, params);

        g_variant_dict_unref(params);
        g_strfreev(urls);
    }

    g_application_activate(app);

    return 0;
}
