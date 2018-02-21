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

#ifndef __APP_H
#define __APP_H

#include <glib.h>

#include "sui/sui.h"
#include "sirc/sirc.h"
#include "prefs.h"
#include "version.h"

#define SRN_TYPE_APPLICATION (srn_application_get_type())
#define SRN_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SRN_TYPE_APPLICATION, SrnApplication))

typedef struct _SrnApplication SrnApplication;
typedef struct _SrnApplicationClass SrnApplicationClass;
typedef struct _SrnApplicationConfig SrnApplicationConfig;

struct _SrnApplication {
    GApplication parent;

    SrnVersion *ver;
    SrnConfigManager *cfg_mgr;
    SrnApplicationConfig *cfg;

    SuiAppEvents ui_app_events; // TODO: rename
    SuiEvents ui_events;
    SircEvents irc_events;

    GSList *window_list;
    GSList *server_list;
};

struct _SrnApplicationClass {
    GApplicationClass parent_class;
};

struct _SrnApplicationConfig {
    bool prompt_on_quit; // TODO
    bool create_user_file; // TODO
    char *id;
    char *theme;
    GSList *server_cfg_list;
};

GType srn_application_get_type(void);
SrnApplication *srn_application_new(void);
SrnApplication* srn_application_get_default(void);
void srn_application_run(SrnApplication *app, int argc, char *argv[]);
void srn_application_quit(SrnApplication *app);

SrnApplicationConfig *srn_application_config_new(void);
SrnRet srn_application_config_check(SrnApplicationConfig *cfg);
void srn_application_config_free(SrnApplicationConfig *cfg);

// FIXME: config
#include "log.h"
#include "server.h"
SrnRet srn_config_manager_read_log_config(SrnConfigManager *mgr, LogPrefs *cfg);
SrnRet srn_config_manager_read_application_config(SrnConfigManager *mgr, SrnApplicationConfig *cfg);
SrnRet srn_config_manager_read_server_config(SrnConfigManager *mgr, ServerPrefs *cfg, const char *srv_name);
SrnRet srn_config_manager_read_chat_config(SrnConfigManager *mgr, ChatPrefs *cfg, const char *srv_name, const char *chat_name);

#endif /* __APP_H */
