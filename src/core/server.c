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
 * @file server.c
 * @brief
 * @author Shengyu Zhang <i@silverrainz.me>
 * @version 0.06.2
 * @date 2016-07-19
 */


#include <string.h>
#include <strings.h>
#include <gtk/gtk.h>

#include "server.h"
#include "server_cmd.h"
#include "server_cap.h"
#include "server_irc_event.h"
#include "server_ui_event.h"

#include "sirc/sirc.h"

#include "meta.h"
#include "srain.h"
#include "log.h"
#include "utils.h"
#include "prefs.h"
#include "i18n.h"

SuiAppEvents ui_app_events;
SuiEvents ui_events;
SircEvents irc_events;

void server_init_and_run(int argc, char *argv[]){
    /* UI event */
    ui_events.disconnect = server_ui_event_disconnect;
    ui_events.quit = server_ui_event_quit;
    ui_events.send = server_ui_event_send;
    ui_events.join = server_ui_event_join;
    ui_events.part = server_ui_event_part;
    ui_events.query = server_ui_event_query;
    ui_events.unquery = server_ui_event_unquery;
    ui_events.kick = server_ui_event_kick;
    ui_events.invite = server_ui_event_invite;
    ui_events.whois = server_ui_event_whois;
    ui_events.ignore = server_ui_event_ignore;
    ui_events.cutover = server_ui_event_cutover;
    ui_events.chan_list = server_ui_event_chan_list;

    ui_app_events.open = server_ui_event_open;
    ui_app_events.activate = server_ui_event_activate;
    ui_app_events.shutdown = server_ui_event_shutdown;
    ui_app_events.connect = server_ui_event_connect;
    ui_app_events.server_list = server_ui_event_server_list;

    /* IRC event */
    irc_events.connect = server_irc_event_connect;
    irc_events.connect_fail = server_irc_event_connect_fail;
    irc_events.disconnect = server_irc_event_disconnect;
    irc_events.welcome = server_irc_event_welcome;
    irc_events.nick = server_irc_event_nick;
    irc_events.quit = server_irc_event_quit;
    irc_events.join = server_irc_event_join;
    irc_events.part = server_irc_event_part;
    irc_events.mode = server_irc_event_mode;
    irc_events.umode = server_irc_event_umode;
    irc_events.topic = server_irc_event_topic;
    irc_events.kick = server_irc_event_kick;
    irc_events.channel = server_irc_event_channel;
    irc_events.privmsg = server_irc_event_privmsg;
    irc_events.notice = server_irc_event_notice;
    irc_events.channel_notice = server_irc_event_channel_notice;
    irc_events.invite = server_irc_event_invite;
    irc_events.ctcp_req = server_irc_event_ctcp_req;
    irc_events.ctcp_rsp = server_irc_event_ctcp_rsp;
    irc_events.cap = server_irc_event_cap;
    irc_events.authenticate = server_irc_event_authenticate;
    irc_events.ping = server_irc_event_ping;
    irc_events.pong = server_irc_event_pong;
    irc_events.error = server_irc_event_error;
    irc_events.numeric = server_irc_event_numeric;

    server_cmd_init();

    /* Read prefs **/
    SrnRet ret;
    SuiAppPrefs ui_app_prefs = {0};

    ret = prefs_read();
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Prefs read error"), RET_MSG(ret));
    }

    ret = log_read_prefs();
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Prefs read error"), RET_MSG(ret));
    }

    ret = prefs_read_server_prefs_list();
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Prefs read error"), RET_MSG(ret));
    }

    ret = prefs_read_sui_app_prefs(&ui_app_prefs);
    if (!RET_IS_OK(ret)){
        sui_message_box(_("Prefs read error"), RET_MSG(ret));
    }

    sui_main_loop(argc, argv, &ui_app_events, &ui_app_prefs);
}

void server_finalize(){
}

Server* server_new_from_prefs(ServerPrefs *prefs){
    Server *srv;

    g_return_val_if_fail(RET_IS_OK(server_prefs_check(prefs)), NULL);
    g_return_val_if_fail(!server_get_by_name(prefs->name), NULL);

    srv = g_malloc0(sizeof(Server));

    srv->state = SERVER_STATE_DISCONNECTED;
    srv->last_action = SERVER_ACTION_DISCONNECT; // It should be OK
    srv->negotiated = FALSE;
    srv->registered = FALSE;

    srv->prefs = prefs;
    srv->cap = server_cap_new();
    srv->cap->srv = srv;

    srv->chat = chat_new(srv, META_SERVER);
    if (!srv->chat) goto bad;

    srv->user = user_new(srv->chat,
            prefs->nickname,
            prefs->username,
            prefs->realname,
            USER_CHIGUA);
    if (!srv->user) goto bad;
    user_set_me(srv->user, TRUE);

    // FIXME: Corss-required between chat_new() and user_new()
    srv->chat->user = user_ref(srv->user);

    /* NOTE: Ping related issuses are not handled in server.c */
    srv->reconn_interval = SERVER_RECONN_INTERVAL;
    /* srv->last_pong = 0; */ // by g_malloc0()
    /* srv->delay = 0; */ // by g_malloc0()
    /* srv->ping_timer = 0; */ // by g_malloc0()
    /* srv->reconn_timer = 0; */ // by g_malloc0()

    srv->cur_chat = srv->chat;

    /* sirc */
    srv->irc = sirc_new_session(&irc_events, prefs->irc);
    if (!srv->irc) goto bad;
    sirc_set_ctx(srv->irc, srv);

    prefs->srv = srv;   // Link server to its prefs

    return srv;

bad:
    server_free(srv);
    return NULL;
}

/**
 * @brief server_free Free a disconnected server, NEVER use this function
 *      directly on server which has been initialized
 *
 * @param srv
 */
void server_free(Server *srv){
    g_return_if_fail(server_is_valid(srv));
    g_return_if_fail(srv->state == SERVER_STATE_DISCONNECTED);

    // srv->prefs is held by server_prefs_list, just unlink it
    srv->prefs->srv = NULL;
    if (srv->cap) {
        server_cap_free(srv->cap);
        srv->cap = NULL;
    }
    if (srv->user != NULL){
        user_free(srv->user);
        srv->user = NULL;
    }
    if (srv->irc != NULL){
        sirc_free_session(srv->irc);
        srv->irc= NULL;
    }

    GSList *lst;
    /* Free chat list */
    if (srv->chat_list){
        lst = srv->chat_list;
        while (lst){
            if (lst->data){
                chat_free(lst->data);
                lst->data = NULL;
            }
            lst = g_slist_next(lst);
        }
        g_slist_free(srv->chat_list);
        srv->chat_list = NULL;
    }

    /* Server's chat should be freed after all chat in chat list are freed */
    if (srv->chat != NULL){
        chat_free(srv->chat);
        srv->chat = NULL;
    }

    g_free(srv);
}

bool server_is_valid(Server *srv){
    return server_prefs_is_server_valid(srv);
}

Server *server_get_by_name(const char *name){
    ServerPrefs *prefs;

    prefs =  server_prefs_get_prefs(name);
    if (prefs) {
        return prefs->srv;
    }

    return NULL;
}

/**
 * @brief server_connect Just an intuitive alias of a connect action
 *
 * @param srv
 *
 * @return
 */
SrnRet server_connect(Server *srv){
    return server_state_transfrom(srv, SERVER_ACTION_CONNECT);
}

/**
 * @brief server_disconnect Just an intuitive alias of a disconnect action
 *
 * @param srv
 *
 * @return
 */
SrnRet server_disconnect(Server *srv){
    return server_state_transfrom(srv, SERVER_ACTION_DISCONNECT);
}

/**
 * @brief server_is_registered Whether this server registered
 *
 * @param srv
 *
 * @return TRUE if registered
 */
bool server_is_registered(Server *srv){
    g_return_val_if_fail(server_is_valid(srv), FALSE);

    return srv->state == SERVER_STATE_CONNECTED && srv->registered == TRUE;
}

void server_wait_until_registered(Server *srv){
    g_return_if_fail(server_is_valid(srv));

    /* Waiting for connection established */
    while (srv->state == SERVER_STATE_CONNECTING) sui_proc_pending_event();
    /* Waiting until server registered */
    while (srv->state == SERVER_STATE_CONNECTED && srv->registered == FALSE)
        sui_proc_pending_event();
}

int server_add_chat(Server *srv, const char *name){
    GSList *lst;
    Chat *chat;

    g_return_val_if_fail(server_is_valid(srv), SRN_ERR);

    lst = srv->chat_list;
    while (lst) {
        chat = lst->data;
        if (strcasecmp(chat->name, name) == 0){
            return SRN_ERR;
        }
        lst = g_slist_next(lst);
    }

    chat = chat_new(srv, name);
    g_return_val_if_fail(chat, SRN_ERR);

    srv->chat_list = g_slist_append(srv->chat_list, chat);

    return SRN_OK;
}

int server_rm_chat(Server *srv, const char *name){
    GSList *lst;
    Chat *chat;

    g_return_val_if_fail(server_is_valid(srv), SRN_ERR);

    lst = srv->chat_list;
    while (lst) {
        chat = lst->data;
        if (strcasecmp(chat->name, name) == 0){
            g_return_val_if_fail(!chat->joined, SRN_ERR);

            if (srv->cur_chat == chat){
                srv->cur_chat = srv->chat;
            }
            chat_free(chat);
            srv->chat_list = g_slist_delete_link(srv->chat_list, lst);

            return SRN_OK;
        }
        lst = g_slist_next(lst);
    }

    return SRN_ERR;
}

Chat* server_get_chat(Server *srv, const char *name) {
    GSList *lst;
    Chat *chat;

    g_return_val_if_fail(server_is_valid(srv), NULL);

    lst = srv->chat_list;
    while (lst) {
        chat = lst->data;
        if (strcasecmp(chat->name, name) == 0){
            return chat;
        }
        lst = g_slist_next(lst);
    }

    return NULL;
}

/**
 * @brief server_get_chat_fallback
 *        This function never fail, if name is NULL or not chat found, return
 *        `srv->chat` instead.
 *
 * @param srv
 * @param name
 *
 * @return A instance of Chat
 */
Chat* server_get_chat_fallback(Server *srv, const char *name) {
    Chat *chat;

    g_return_val_if_fail(server_is_valid(srv), NULL);

    if (!name || !(chat = server_get_chat(srv, name))){
        chat = srv->chat;
    }

    return chat;
}
