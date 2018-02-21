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
 * @file server_url.c
 * @brief IRC URL parse and perfrom
 * @author Shengyu Zhang <i@silverrainz.me>
 * @version 0.06.2
 * @date 2017-08-06
 */

#include <glib.h>
#include <libsoup/soup.h>

#include "server.h"

#include "ret.h"
#include "i18n.h"
#include "log.h"
#include "utils.h"
#include "prefs.h"

static void join_comma_separated_chans(Server *srv, const char *comma_chans);

SrnRet server_url_open(const char *url){
    const char *scheme;
    const char *user;
    const char *passwd;
    const char *host;
    int port;
    const char *path;
    const char *fragment;
    bool new_prefs = FALSE;
    SoupURI *suri = NULL;
    SrnRet ret = SRN_ERR;
    ServerPrefs *prefs = NULL;
    Server *srv = NULL;

    suri = soup_uri_new(url);
    if (!suri){
        ret = RET_ERR(_("Failed to parse \"%1$s\" as a URL"), url);
        goto fin;
    }

    scheme = soup_uri_get_scheme(suri);
    if (g_ascii_strcasecmp(scheme, "irc") != 0
            && g_ascii_strcasecmp(scheme, "ircs") != 0){
        ret = RET_ERR(_("Unsupported protocol: %1$s"), scheme);
        goto fin;
    }

    user = soup_uri_get_user(suri);
    passwd = soup_uri_get_password(suri);
    host = soup_uri_get_host(suri);
    port = soup_uri_get_port(suri);
    path = soup_uri_get_path(suri);
    fragment = soup_uri_get_fragment(suri);

    if (str_is_empty(host)){
        ret = RET_ERR(_("Host is empty in URL \"%1$s\""), url);
        goto fin;
    }

    /* Got ServerPrefs */
    prefs = server_prefs_get_prefs_by_host_port(host, port);
    if (!prefs){
        // If no such ServerPrefs, create one
        prefs = server_prefs_new(host);
        if (!prefs){
            ret =  RET_ERR(_("Failed to create server \"%1$s\""), host);
            goto fin;
        }
        new_prefs = TRUE;
    }
    /* Instantiate Server */
    if (!prefs->srv){
        ret = srn_config_manager_read_server_config(
                srn_application_get_default()->cfg_mgr,
                prefs,
                prefs->name);
        if (!RET_IS_OK(ret)){
            goto fin;
        }
        // FIXME: config
        // if (!str_is_empty(host)){
        //     str_assign(&prefs->host, host);
        // }
        // if (port){
        //     prefs->port = port;
        // }
        if (!str_is_empty(passwd)){
            str_assign(&prefs->passwd, passwd);
        }
        if (!str_is_empty(user)){
            str_assign(&prefs->nickname, user);
        }
        if (g_ascii_strcasecmp(scheme, "ircs") == 0){
            prefs->irc->tls = TRUE;
        }

        ret = server_prefs_check(prefs);
        if (!RET_IS_OK(ret)){
            goto fin;
        }

        // If no such Server, create one
        srv = server_new_from_prefs(prefs);
        if (!srv) {
            ret =  RET_ERR(_("Failed to instantiate server \"%1$s\""), host);
            goto fin;
        }
    } else {
        srv = prefs->srv;
    }

    DBG_FR("Server instance: %p", srv);

    if (srv->state == SERVER_STATE_DISCONNECTED){
        server_connect(srv);
    }

    server_wait_until_registered(srv);
    if (!server_is_registered(srv)){
        ret =  RET_ERR(_("Failed to register on server \"%1$s\""), prefs->name);
        goto fin;
    }

    /*  Join channels in URL */
    if (!str_is_empty(path)){
        if (path[0] == '/') {
            path++;    // Skip root of URL path
        }
        join_comma_separated_chans(srv, path);
    }

    if (!str_is_empty(fragment)){
        join_comma_separated_chans(srv, fragment);
    }

    return SRN_OK;

fin:
    if (suri){
        soup_uri_free(suri);
    }
    if (new_prefs && prefs){
        server_prefs_free(prefs);
    }

    return ret;
}

static void join_comma_separated_chans(Server *srv, const char *comma_chans){
    char *chans;
    const char *chan;

    /* TODO: how to distinguish between channel and password if channel doen't
     * start with a '#'?
     */

    chans = g_strdup(comma_chans);
    chan = strtok(chans, ",");

    do {
        if (!str_is_empty(chan)){
            DBG_FR("Get channnel: %s", chan);
            if (!sirc_is_chan(chan)){
                char *chan2 = g_strdup_printf("#%s", chan);
                sirc_cmd_join(srv->irc, chan2, NULL);
                g_free(chan2);
            } else {
                sirc_cmd_join(srv->irc, chan, NULL);
            }
        }
        chan = strtok(NULL, ",");
    } while (chan);

    g_free(chans);
}
