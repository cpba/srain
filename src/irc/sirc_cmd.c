/**
 * @file sirc_cmd.c
 * @brief Send IRC command to server
 * @author Shengyu Zhang <silverrainz@outlook.com>
 * @version 1.0
 * @date 2016-03-01
 *
 * Copied from <https://github.com/Themaister/simple-irc-bot>
 *
 */

#define __LOG_ON

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>

#include "socket.h"
#include "sirc_cmd.h"

#include "srain.h"
#include "log.h"

// sirc_cmd_pong: For answering pong requests...
int sirc_cmd_pong(SircSession *sirc, const char *data){
    return sirc_cmd_raw(sirc, "PONG :%s\r\n", data);
}

int sirc_cmd_user(SircSession *sirc, const char *username, const char *hostname,
        const char *servername, const char *realname){
    return sirc_cmd_raw(sirc, "USER %s %s %s :%s\r\n",
            username, hostname, servername, realname);
}

// sirc_cmd_join: For joining a chan
int sirc_cmd_join(SircSession *sirc, const char *chan){
    return sirc_cmd_raw(sirc, "JOIN %s\r\n", chan);
}

// sirc_cmd_part: For leaving a chan
int sirc_cmd_part(SircSession *sirc, const char *chan, const char *reason){
    return sirc_cmd_raw(sirc, "PART %s :%s\r\n", chan, reason);
}

// sirc_cmd_nick: For changing your nick
int sirc_cmd_nick(SircSession *sirc, const char *nick){
    return sirc_cmd_raw(sirc, "NICK %s\r\n", nick);
}

// sirc_cmd_quit: For quitting IRC
int sirc_cmd_quit(SircSession *sirc, const char *reason){
    return sirc_cmd_raw(sirc, "QUIT :%s\r\n", reason);
}


// sirc_cmd_topic: For setting or removing a topic
int sirc_cmd_topic(SircSession *sirc, const char *chan, const char *topic){
    return sirc_cmd_raw(sirc, "TOPIC %s :%s\r\n", chan, topic);
}

// sirc_cmd_action: For executing an action (.e.g /me is hungry)
int sirc_cmd_action(SircSession *sirc, const char *chan, const char *msg){
    return sirc_cmd_raw(sirc, "PRIVMSG %s :\001ACTION %s\001\r\n", chan, msg);
}

// sirc_cmd_msg: For sending a chan message or a query
int sirc_cmd_msg(SircSession *sirc, const char *chan, const char *msg){
    return sirc_cmd_raw(sirc, "PRIVMSG %s :%s\r\n", chan, msg);
}

int sirc_cmd_names(SircSession *sirc, const char *chan){
    return sirc_cmd_raw(sirc, "NAMES %s\r\n", chan);
}

int sirc_cmd_whois(SircSession *sirc, const char *who){
    return sirc_cmd_raw(sirc, "WHOIS %s\r\n", who);
}

int sirc_cmd_invite(SircSession *sirc, const char *nick, const char *chan){
    return sirc_cmd_raw(sirc, "INVITE %s %s\r\n", nick, chan);
}

int sirc_cmd_kick(SircSession *sirc, const char *nick, const char *chan,
        const char *reason){
    return sirc_cmd_raw(sirc, "KICK %s %s :%s\r\n", chan, nick, reason);
}

int sirc_cmd_mode(SircSession *sirc, const char *target, const char *mode){
    return sirc_cmd_raw(sirc, "MODE %s %s\r\n", target, mode);
}

int sirc_cmd_raw(SircSession *sirc, const char *fmt, ...){
    char buf[SIRC_BUF_LEN];
    int len = 0;
    va_list args;

    if (strlen(fmt) != 0){
        va_start(args, fmt);
        len = vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
    }

    if (len > 512){
        WARN_FR("Raw command too long");
        len = 512;
    }

    int ret = sck_send(sirc_get_fd(sirc), buf, len);
    if (ret == SRN_ERR){
        return SRN_ERR;
    }
    // TODO send it totally
    // if (ret < len) ...

    return SRN_OK;
}
