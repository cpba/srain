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
 * @file regex.c
 * @brief Regex message filter
 * @author Z.Wind.L <zwindl@protonmail.com>
 *         Shengyu Zhang <i@silverrainz.me>
 * @version 0.06.2
 * @date 2017-03-16
 */

#include <glib.h>
#include <strings.h>

#include "server.h"

#include "srain.h"
#include "filter.h"
#include "i18n.h"
#include "log.h"

typedef struct _NamedPattern {
    char *name;
    char *pattern;
} NamedPattern;

static void named_pattern_free(NamedPattern *np);
static bool regex(const Message *msg, const char *content);

Filter regex_filter = {
    .name = "regex",
    .func = regex,
};

int regex_filter_add_pattern(Chat *chat, const char *name, const char *pattern){
    GSList *lst;
    GError *err;
    GRegex *regex;
    NamedPattern *np;

    lst = chat->ignore_regex_list;

    while(lst){
        if (lst->data){
            np = lst->data;
            if (strcasecmp(np->name, name) == 0){
                chat_add_error_message_fmt(chat->srv->cur_chat, chat->user->nick,
                        _("\"%1$s\" already exists in %2$s 's regex list"),
                        np->name, chat->name);
                return SRN_ERR;
            }
        }
        lst = g_slist_next(lst);
    }

    err = NULL;
    regex = g_regex_new(pattern, 0, 0, &err);
    if (!regex){
        chat_add_error_message_fmt(chat->srv->cur_chat, chat->user->nick,
                _("Invail pattern: %1$s"), err->message);
        return SRN_ERR;
    }

    g_regex_unref(regex);

    np = g_malloc0(sizeof(NamedPattern));
    np->name = g_strdup(name);
    np->pattern = g_strdup(pattern);

    chat->ignore_regex_list = g_slist_append(chat->ignore_regex_list, np);

    chat_add_misc_message_fmt(chat->srv->cur_chat, chat->user->nick,
            _("\"%1$s\" has added to %2$s 's regex list"), np->name, chat->name);

    return SRN_OK;
}

int regex_filter_rm_pattern(Chat *chat, const char *name){
    GSList *lst;
    NamedPattern *np;

    lst = chat->ignore_regex_list;

    while(lst){
        if (lst->data){
            np = lst->data;
            if (strcasecmp(np->name, name) == 0){
                chat_add_misc_message_fmt(chat->srv->cur_chat, chat->user->nick,
                        _("\"%1$s\" is removed from %2$s 's ignore list"),
                        name, chat->name);

                named_pattern_free(np);
                chat->ignore_regex_list = g_slist_delete_link(chat->ignore_regex_list, lst);

                return SRN_OK;
            }
        }
        lst = g_slist_next(lst);
    }

    chat_add_error_message_fmt(chat->srv->cur_chat, chat->user->nick,
            _("\"%1$s\" not found in %2$s 's ignore list"),
            name, chat->name);

    return SRN_ERR;
}

void regex_filter_free_list(Chat *chat){
    g_slist_free_full(chat->ignore_regex_list,
            (GDestroyNotify)named_pattern_free);
    chat->ignore_regex_list = NULL;
}

static bool regex(const Message *msg, const char *content){
    GSList *lst;
    NamedPattern *np;

    g_return_val_if_fail(msg->chat, TRUE);
    g_return_val_if_fail(server_is_valid(msg->chat->srv), TRUE);
    g_return_val_if_fail(msg->chat->srv->chat, TRUE);

    lst = msg->chat->ignore_regex_list;
    while (lst){
        if (lst->data){
            np = lst->data;
            if(g_regex_match_simple(np->pattern, content, 0, 0)){
                return FALSE;
            }
        }
        lst = g_slist_next(lst);
    }

    lst = msg->chat->srv->chat->ignore_regex_list;
    while (lst){
        if (lst->data){
            np = lst->data;
            if(g_regex_match_simple(np->pattern, content, 0, 0)){
                return FALSE;
            }
        }
        lst = g_slist_next(lst);
    }

    return TRUE;
}

static void named_pattern_free(NamedPattern *np){
    g_free(np->name);
    g_free(np->pattern);
    g_free(np);
}
