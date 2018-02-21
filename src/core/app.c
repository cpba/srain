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
#include "app.h"
#include "server_ui_event.h"
#include "server_irc_event.h"
#include "meta.h"
#include "log.h"
#include "i18n.h"
#include "file_helper.h"
#include "server_cmd.h" // FIXME
#include "rc.h"

G_DEFINE_TYPE(SrnApplication, srn_application, G_TYPE_APPLICATION);

/* Only one SrnApplication instance in one application */
static SrnApplication *default_app = NULL;

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

static SrnRet create_window(SrnApplication *app);
static void on_activate(SrnApplication *app);
static void on_shutdown(SrnApplication *app);
static int on_handle_local_options(SrnApplication *app, GVariantDict *options,
        gpointer user_data);
static int on_command_line(SrnApplication *app,
        GApplicationCommandLine *cmdline, gpointer user_data);

static void srn_application_init(SrnApplication *self){
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
    SrnConfigManager *cfg_mgr;
    SrnApplication *app;
    SrnApplicationConfig *cfg;

    if (default_app) {
        return default_app;
    }

    ver = srn_version_new(PACKAGE_VERSION PACKAGE_BUILD);
    ret = srn_version_parse(ver);
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
    srn_config_manager_read_application_config(cfg_mgr, cfg);

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

SrnApplication* srn_application_get_default(void){
    return srn_application_new();
}

void srn_application_quit(SrnApplication *app){
    g_application_quit(G_APPLICATION(app));
}

void srn_application_run(SrnApplication *app, int argc, char *argv[]){
    /* UI event */
    app->ui_events.disconnect = server_ui_event_disconnect;
    app->ui_events.quit = server_ui_event_quit;
    app->ui_events.send = server_ui_event_send;
    app->ui_events.join = server_ui_event_join;
    app->ui_events.part = server_ui_event_part;
    app->ui_events.query = server_ui_event_query;
    app->ui_events.unquery = server_ui_event_unquery;
    app->ui_events.kick = server_ui_event_kick;
    app->ui_events.invite = server_ui_event_invite;
    app->ui_events.whois = server_ui_event_whois;
    app->ui_events.ignore = server_ui_event_ignore;
    app->ui_events.cutover = server_ui_event_cutover;
    app->ui_events.chan_list = server_ui_event_chan_list;

    app->ui_app_events.connect = server_ui_event_connect;
    app->ui_app_events.server_list = server_ui_event_server_list;

    /* IRC event */
    app->irc_events.connect = server_irc_event_connect;
    app->irc_events.connect_fail = server_irc_event_connect_fail;
    app->irc_events.disconnect = server_irc_event_disconnect;
    app->irc_events.welcome = server_irc_event_welcome;
    app->irc_events.nick = server_irc_event_nick;
    app->irc_events.quit = server_irc_event_quit;
    app->irc_events.join = server_irc_event_join;
    app->irc_events.part = server_irc_event_part;
    app->irc_events.mode = server_irc_event_mode;
    app->irc_events.umode = server_irc_event_umode;
    app->irc_events.topic = server_irc_event_topic;
    app->irc_events.kick = server_irc_event_kick;
    app->irc_events.channel = server_irc_event_channel;
    app->irc_events.privmsg = server_irc_event_privmsg;
    app->irc_events.notice = server_irc_event_notice;
    app->irc_events.channel_notice = server_irc_event_channel_notice;
    app->irc_events.invite = server_irc_event_invite;
    app->irc_events.ctcp_req = server_irc_event_ctcp_req;
    app->irc_events.ctcp_rsp = server_irc_event_ctcp_rsp;
    app->irc_events.cap = server_irc_event_cap;
    app->irc_events.authenticate = server_irc_event_authenticate;
    app->irc_events.ping = server_irc_event_ping;
    app->irc_events.pong = server_irc_event_pong;
    app->irc_events.error = server_irc_event_error;
    app->irc_events.numeric = server_irc_event_numeric;

    server_cmd_init();

    // TODO: config

    g_application_run(G_APPLICATION(app), argc, argv);
}

static SrnRet create_window(SrnApplication *app){
    SuiWindow *win;

    if (g_slist_length(app->window_list) > 0){
        return SRN_ERR;
    }

    win = sui_new_window();
    if (!win) {
        return SRN_ERR;
    }
    app->window_list = g_slist_append(app->window_list, win);

    return SRN_OK;
}

static void on_activate(SrnApplication *app){
    SrnRet ret;

    ret = create_window(app);
    if (!RET_IS_OK(ret)){
        return;
    }

    ret = rc_read();
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Error"), RET_MSG(ret));
    }
}

static void on_shutdown(SrnApplication *app){
    GSList *lst;

    lst = app->server_list;
    // FIXME: config
    // while (lst){
    //     ServerPrefs *prefs = lst->data;
    //     if (prefs->srv && prefs->srv->state == SERVER_STATE_CONNECTED){
    //         sirc_cmd_quit(prefs->srv->irc, prefs->quit_message);
    //         /* FIXME: we need a global App object in core module,
    //          * force free server for now */
    //         server_state_transfrom(prefs->srv, SERVER_ACTION_QUIT);
    //         server_state_transfrom(prefs->srv, SERVER_ACTION_DISCONNECT_FINISH);
    //     }
    //     lst = g_slist_next(lst);
    // }
}

static int on_handle_local_options(SrnApplication *app, GVariantDict *options,
        gpointer user_data){
    if (g_variant_dict_lookup(options, "version", "b", NULL)){
        g_print("%s %s\n", PACKAGE_NAME, app->ver->raw);
        return 0; // Exit
    }

    return -1; // Return -1 to let the default option processing continue.
}

static int on_command_line(SrnApplication *app,
        GApplicationCommandLine *cmdline, gpointer user_data){
    char **urls;
    GVariantDict *options;

    options = g_application_command_line_get_options_dict(cmdline);
    if (g_variant_dict_lookup(options, G_OPTION_REMAINING, "^as", &urls)){
        /* If we have URLs to open, create window firstly. */
        create_window(app);

        for (int i = 0; i < g_strv_length(urls); i++){
            SrnRet ret;

            ret = server_url_open(urls[i]);
            if (!RET_IS_OK(ret)){
                // return ret;
            }
        }
        g_strfreev(urls);
    }

    g_application_activate(G_APPLICATION(app));

    return 0;
}
