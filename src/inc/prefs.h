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

#ifndef __PREFS_H
#define __PREFS_H

#include "srain.h"
#include "server.h"
#include "ret.h"
#include "version.h"

typedef struct _SrnConfigManager SrnConfigManager;

SrnConfigManager* srn_config_manager_new(SrnVersion *ver);
void srn_config_manager_free(SrnConfigManager *mgr);
SrnRet srn_config_manager_read_user_config(SrnConfigManager *mgr, const char *file);
SrnRet srn_config_manager_read_system_config(SrnConfigManager *mgr, const char *file);

SrnRet srn_config_manager_read_log_config(SrnConfigManager *mgr, LogPrefs *cfg);
SrnRet srn_config_manager_read_application_config(SrnConfigManager *mgr, SrnApplicationConfig *cfg);
SrnRet srn_config_manager_read_server_config(SrnConfigManager *mgr, ServerPrefs *cfg, const char *srv_name);
SrnRet srn_config_manager_read_chat_config(SrnConfigManager *mgr, ChatPrefs *cfg, const char *srv_name, const char *chat_name);

#endif /*__PREFS_H */
