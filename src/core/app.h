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

#include <gtk/gtk.h>

#define SRN_TYPE_APPLICATION (srn_application_get_type())
#define SRN_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SRN_TYPE_APPLICATION, SrnApplication))

struct _SrnApplication {
    GtkApplication parent;

    SrnVersion *ver;
    SrnPrefs *srn_prefs;
    SrnApplicationPrefs *prefs;

    GSList *window_list;
    GSList *server_list;
    GSList *server_prefs_list;
};

struct _SrnApplicationClass {
    GtkApplicationClass parent_class;
};

struct _SrnApplicationPrefs {
    bool prompt_on_quit; // TODO
    bool create_user_file; // TODO
    char *id;
    char *theme;
}
typedef struct _SrnApplication SrnApplication;
typedef struct _SrnApplicationClass SrnApplicationClass;

GType srn_application_get_type(void);
SrnApplication *srn_application_new(SrnPrefs *srn_prefs);
void srn_application_run(void);
void srn_application_quit(SrnApplication *app);

/* Only one SrnApplication instance in one application */
extern SrnApplication* const srn_app;

#endif /* __SRN_APPLICATION_H */
