/**
 * @file srv_session.c
 * @brief Server sessions manager
 * @author Shengyu Zhang <lastavengers@outlook.com>
 * @version 1.0
 * @date 2016-07-19
 */

// #define __DBG_ON
#define __LOG_ON

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "srv_session.h"
#include "srv_event.h"
#include "srv_hdr.h"

#include "log.h"
#include "i18n.h"
#include "meta.h"

srv_session_t sessions[MAX_SESSIONS] = { 0 };
irc_callbacks_t cbs;

static void srv_session_err_hdr(srv_session_t *session);

static int srv_session_get_index(srv_session_t *session){
    int i;
    if (!session){
        WARN_FR("Session is NULL");
        return -1;
    }

    // TODO: access data from another thread?
    srv_session_t *session2;
    for (i = 0; i < MAX_SESSIONS; i++){
        session2 = &sessions[i];
        if (session2 == session){
            if (session2->stat == SESS_NOINUSE){
                return -1;
            } else {
                return i;
            }
        }
    }

    return -1;
}

static int srv_session_reconnect(srv_session_t *session){
    int res;
    char msg[512];

    WARN_FR("Reconnecting, session: %s", session->host);

    snprintf(msg, sizeof(msg), _("Reconnecting to %s ..."), session->host);
    srv_hdr_ui_sys_msg(session->host, META_SERVER, msg, SYS_MSG_NORMAL);

    session->stat = SESS_INUSE;
    irc_disconnect(session->irc_session);
    res = irc_connect(session->irc_session,
            session->prefix[0] ? session->prefix : session->host,
            session->port,
            strlen(session->passwd) ? session->passwd : NULL,
            session->nickname, session->username, session->realname);

    if (res){
        srv_session_err_hdr(session);
    }

    return 0;
}

static void srv_session_err_hdr(srv_session_t *session){
    int errno;
    char msg[512];
    const char *errmsg = "";

    errno = irc_errno(session->irc_session);
    errmsg = irc_strerror(irc_errno(session->irc_session));

    /* Ignore it */
    if (errno == LIBIRC_ERR_OK) return;

    /* Error message should be reported */
    ERR_FR("session: %s, errno: %d, errmsg: %s",
            session->host, errno, errmsg);

    snprintf(msg, sizeof(msg), _("ERROR: %s"), errmsg);
    srv_hdr_ui_sys_msg(session->host, META_SERVER, msg, SYS_MSG_ERROR);

    /* Error occurs when session is not connnect yet */
    if (session->stat == SESS_INUSE){
        srv_session_free(session);
        return;
    }
    /* Connection should be closed: you receive a QUIT message */
    if (session->stat == SESS_CLOSE){
        return;
    }

    switch (errno){
        /* Nothing to do */
        case LIBIRC_ERR_INVAL:
        case LIBIRC_ERR_NODCCSEND:
        case LIBIRC_ERR_READ:
        case LIBIRC_ERR_WRITE:
        case LIBIRC_ERR_OPENFILE:
            break;
        /* Terminate the session */
        case LIBIRC_ERR_RESOLV:
        case LIBIRC_ERR_SOCKET:
        case LIBIRC_ERR_CONNECT:
        case LIBIRC_ERR_NOMEM:
        case LIBIRC_ERR_NOIPV6:
        case LIBIRC_ERR_SSL_NOT_SUPPORTED:
        case LIBIRC_ERR_SSL_INIT_FAILED:
        case LIBIRC_ERR_CONNECT_SSL_FAILED:
        case LIBIRC_ERR_SSL_CERT_VERIFY_FAILED:
            srv_session_free(session);
            break;
        case LIBIRC_ERR_CLOSED:
        case LIBIRC_ERR_ACCEPT:
        case LIBIRC_ERR_TIMEOUT:
        case LIBIRC_ERR_TERMINATED:
            srv_session_reconnect(session);
            sleep(5);
            break;
        case LIBIRC_ERR_STATE:
            break;
    }
}

/**
 * @brief Proecss all sessions' event, This function runs on a separate thread
 * and never return, there is only one thread in a application.
 */
static void _srv_session_proc(){
    int i;
    int maxfd;
    struct timeval tv;
    fd_set in_set, out_set;
    irc_session_t *isess;

    LOG_FR("Networking loop started");

loop:
    maxfd = 0;
    tv.tv_usec = 0;
    tv.tv_sec = 1;

    FD_ZERO(&in_set);
    FD_ZERO(&out_set);

    for (i = 0; i < MAX_SESSIONS; i++){
        if (sessions[i].stat == SESS_NOINUSE) continue;
        // if (sessions[i].stat == SESS_CONNECT
                // && !irc_is_connected(sessions[i].irc_session)){
                // continue;
        // }
        isess = sessions[i].irc_session;

        irc_add_select_descriptors(isess, &in_set, &out_set, &maxfd);
    }

    if (select(maxfd + 1, &in_set, &out_set, 0, &tv) < 0 ){
        ERR_FR("select() error");
        return;
    }

    for (i = 0; i < MAX_SESSIONS; i++){
        if (sessions[i].stat == SESS_NOINUSE) continue;
        isess = sessions[i].irc_session;

        if (irc_process_select_descriptors(isess, &in_set, &out_set)){
            /* Some error occurred */
            srv_session_err_hdr(&sessions[i]);
        }
    }

    goto loop;
}

void srv_session_init(){
    LOG_FR("...");

    memset(sessions, 0, sizeof(sessions));
    memset (&cbs, 0, sizeof(cbs));

    // Set up the mandatory events
    cbs.event_connect  = srv_event_connect;
    cbs.event_nick = srv_event_nick;
    cbs.event_quit = srv_event_quit;
    cbs.event_join = srv_event_join;
    cbs.event_part = srv_event_part;
    cbs.event_mode = srv_event_mode;
    cbs.event_umode = srv_event_umode;
    cbs.event_topic = srv_event_topic;
    cbs.event_kick = srv_event_kick;
    cbs.event_channel = srv_event_channel;
    cbs.event_privmsg = srv_event_privmsg;
    cbs.event_notice = srv_event_notice;
    cbs.event_channel_notice = srv_event_channel_notice;
    cbs.event_invite = srv_event_invite;
    cbs.event_ctcp_action = srv_event_ctcp_action;
    cbs.event_numeric = srv_event_numeric;
}

void srv_session_proc(){
    // Start the networking loop
    g_thread_new(NULL, (GThreadFunc)_srv_session_proc, NULL);
}

/**
 * @brief Create a srv_session
 *
 * @param host
 * @param port Can be 0, fallback to 6667
 * @param passwd Can be NULL
 * @param nickname Can NOT be NULL
 * @param username Can be NULL
 * @param realname Can be NULL
 * @param ssl
 *
 * @return NULL or srv_session_t
 */
srv_session_t* srv_session_new(const char *host, int port, const char *passwd,
        const char *nickname, const char *username, const char *realname, ssl_opt_t ssl){
    int i;
    srv_session_t *sess;

    if (!port) port = 6667;
    if (!passwd) passwd = "";
    if (!username) username = nickname;
    if (!realname) realname = nickname;

    if (srv_session_get_by_host(host) != NULL){
        WARN_FR("Session %s already exist", host);
        return NULL;
    }

    LOG_FR("host: %s, port: %d, nickname: %s, username: %s, realname: %s, ssl: %d",
            host, port, nickname, username, realname, ssl);

    sess = NULL;
    for (i = 0; i < MAX_SESSIONS; i++){
        if (sessions[i].stat == SESS_NOINUSE){
            sess = &sessions[i];
            break;
        } else {
            continue;
        }
    }
    if (!sess){
        WARN_FR("MAX_SESSIONS count limit");
        return NULL;
    }

    sess->irc_session = irc_create_session(&cbs);

    if (!(sess->irc_session)){
        ERR_FR("Failed to create IRC session");
        return NULL;
    }

    sess->stat = SESS_INUSE;
    sess->port = port;
    sess->chans = NULL;
    if (ssl != SSL_OFF)
        sess->prefix[0] = '#';
    else
        sess->prefix[0] = 0;
    strncpy(sess->host, host, HOST_LEN);
    strncpy(sess->passwd, passwd, PASSWD_LEN);
    strncpy(sess->realname, realname, NICK_LEN);
    strncpy(sess->username, username, NICK_LEN);
    strncpy(sess->nickname, nickname, NICK_LEN);

    irc_set_ctx(sess->irc_session, sess);
    irc_option_set(sess->irc_session, LIBIRC_OPTION_STRIPNICKS);
    if (ssl == SSL_NO_VERIFY)
        irc_option_set(sess->irc_session, LIBIRC_OPTION_SSL_NO_VERIFY);

    if (irc_connect(sess->irc_session,
                sess->prefix[0] ? sess->prefix : sess->host,
                port,
                strlen(sess->passwd) ? sess->passwd : NULL,
                sess->nickname, sess->username, sess->realname)){
        srv_session_err_hdr(sess);
        return NULL;
    }

    DBG_FR("Alloc session: %p for %s", sess, sess->host);

    srv_hdr_ui_add_chan(host, META_SERVER);
    return sess;
}

int srv_session_is_session(srv_session_t *session){
    return srv_session_get_index(session) == -1 ? 0 : 1;
}

int srv_session_free(srv_session_t *session){
    int i;

    i = srv_session_get_index(session);
    if (i == -1) return -1;
    if (session->stat == SESS_NOINUSE) return -1;

    session->stat = SESS_NOINUSE;

    LOG_FR("Free session %s", session->host);

    irc_disconnect(session->irc_session);
    irc_destroy_session(session->irc_session);

    GList *list = sessions[i].chans;
    while (list){
        free(list->data);
        list = g_list_next(list);
    }

    sessions[i] = (srv_session_t) { 0 };
    sessions[i].stat = SESS_NOINUSE;
    sessions[i].chans = NULL;

    return 0;
}

srv_session_t* srv_session_get_by_host(const char *host){
    int i;

    if (!host) return NULL;

    for (i = 0; i < MAX_SESSIONS; i++){
        if (sessions[i].stat == SESS_NOINUSE) continue;
        if (strcmp(sessions[i].host, host) == 0)
            return &sessions[i];
    }

    return NULL;
}

int srv_session_send(srv_session_t *session,
        const char *target, const char *msg){
    int res;
    if ((res = irc_cmd_msg(session->irc_session, target, msg)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_me(srv_session_t *session,
        const char *target, const char *msg){
    int res;
    if ((res = irc_cmd_me(session->irc_session, target, msg)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_join(srv_session_t *session,
        const char *chan, const char *passwd){
    int res;
    if ((res = irc_cmd_join(session->irc_session, chan, passwd)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_part(srv_session_t *session, const char *chan){
    int res;
    if ((res = irc_cmd_part(session->irc_session, chan)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_quit(srv_session_t *session, const char *reason){
    int res;
    if ((res = irc_cmd_quit(session->irc_session, reason)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

void srv_session_quit_all(){
    int i;

    for (i = 0; i < MAX_SESSIONS; i++){
        if (sessions[i].stat == SESS_CONNECT) {
            srv_session_quit(&sessions[i], PACKAGE_NAME PACKAGE_VERSION " close.");
        }
    }

    /* Wait 1 second for QUIT message */
    sleep(1);
    /* FIXME:
     * It seems that ngircd doesn't return you a QUIT message when you send QUIT
     * message...
     */
}

int srv_session_nick(srv_session_t *session, const char *new_nick){
    int res;
    if ((res = irc_cmd_nick(session->irc_session, new_nick)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_whois(srv_session_t *session, const char *nick){
    int res;
    if ((res = irc_cmd_whois(session->irc_session, nick)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_invite(srv_session_t *session, const char *nick, const char *chan){
    int res;
    if ((res = irc_cmd_invite(session->irc_session, nick, chan)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_kick(srv_session_t *session, const char *nick,
        const char *chan, const char *reason){
    int res;
    if ((res = irc_cmd_kick(session->irc_session, nick, chan, reason)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}

int srv_session_mode(srv_session_t *session, const char *target, const char *mode){
    int res;
    if (target)
        res = irc_cmd_user_mode(session->irc_session, mode);
    else
        res = irc_cmd_channel_mode(session->irc_session, target, mode);

    if (res < 0){
        srv_session_err_hdr(session);
    }

    return res;
}

int srv_session_topic(srv_session_t *session, const char *chan, const char *topic){
    int res;
    if ((res = irc_cmd_topic(session->irc_session, chan, topic)) < 0){
        srv_session_err_hdr(session);
    }
    return res;
}
