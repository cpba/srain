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

#ifndef __SRN_APPLICATION_H
#define __SRN_APPLICATION_H

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
    GSList *server_list;
};

GType srn_application_get_type(void);
SrnApplication *srn_application_new(void);
void srn_application_run(SrnApplication *app, int argc, char *argv[]);
void srn_application_quit(SrnApplication *app);

/* Only one SrnApplication instance in one application */
extern SrnApplication* const srn_app;

#endif /* __SRN_APPLICATION_H */
