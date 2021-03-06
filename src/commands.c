/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "common.h"
#include "commands.h"

struct f2b_cmd_t {
  const short int argc;
  const short int tokenc;
  const char *help;
  const char *tokens[CMD_TOKENS_MAX];
} commands[CMD_MAX_NUMBER] = {
  [CMD_NONE] = {
    .argc = 0, .tokenc = 0,
    .tokens = { NULL },
    .help = "Unspecified command"
  },
  [CMD_RESP] = {
    .argc = 1, .tokenc = 0,
    .tokens = { NULL },
    .help = "Command response, used internally",
  },
  [CMD_HELP] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "help", NULL },
    .help = "Show available commands",
  },
  [CMD_PING] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "ping", NULL },
    .help = "Check the connection",
  },
  [CMD_STATUS] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "status", NULL },
    .help = "Show general stats and jails list",
  },
  [CMD_ROTATE] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "rotate", NULL },
    .help = "Reopen daemon's own log file",
  },
  [CMD_RELOAD] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "reload", NULL },
    .help = "Reload own config, all jails will be reset.",
  },
  [CMD_SHUTDOWN] = {
    .argc = 0, .tokenc = 1,
    .tokens = { "shutdown", NULL },
    .help = "Gracefully terminate f2b daemon",
  },
  [CMD_JAIL_STATUS] = {
    .argc = 1, .tokenc = 3,
    .tokens = { "jail", "<jailname>", "status", NULL },
    .help = "Show status and stats of given jail",
  },
  [CMD_JAIL_SET] = {
    .argc = 3, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "set", "<param>", "<value>", NULL },
    .help = "Set parameter of given jail",
  },
  [CMD_JAIL_IP_STATUS] = {
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "status", "<ip>", NULL },
    .help = "Show ip status in given jail",
  },
  [CMD_JAIL_IP_BAN] = {
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "ban", "<ip>", NULL },
    .help = "Forcefully ban some ip in given jail",
  },
  [CMD_JAIL_IP_RELEASE] = {
    .argc = 2, .tokenc = 5,
    .tokens = { "jail", "<jailname>", "ip", "release", "<ip>", NULL },
    .help = "Forcefully release some ip in given jail",
  },
  [CMD_JAIL_FILTER_STATS] = {
    .argc = 1, .tokenc = 4,
    .tokens = { "jail", "<jailname>", "filter", "stats", NULL },
    .help = "Show matches stats for jail regexps",
  },
  [CMD_JAIL_FILTER_RELOAD] = {
    .argc = 1, .tokenc = 4,
    .tokens = { "jail", "<jailname>", "filter", "reload", NULL },
    .help = "Reload regexps for given jail",
  },
};

void
f2b_cmd_help() {
  struct f2b_cmd_t *cmd = NULL;
  const char **p = NULL;

  fputs("Available commands:\n\n", stdout);
  for (size_t i = CMD_PING; i < CMD_MAX_NUMBER; i++) {
    cmd = &commands[i];
    if (cmd->tokens[0] == NULL)
      continue;
    for (p = cmd->tokens; *p != NULL; p++)
      fprintf(stdout, "%s ", *p);
    fprintf(stdout, "\n\t%s\n\n", cmd->help);
  }

  return;
}

void
f2b_cmd_append_arg(char *buf, size_t bufsize, const char *arg) {
  assert(buf != NULL);
  assert(arg != NULL);
  strlcat(buf, arg,  bufsize);
  strlcat(buf, "\n", bufsize);
}

/**
 * @brief Parse command from line
 * @param buf  Buffer for command parameters
 * @param bufsize SSize of buffer above
 * @param src  Line taken from user input
 * @return Type of parsed command or CMD_NONE if no matches
 */
enum f2b_cmd_type
f2b_cmd_parse(char *buf, size_t bufsize, const char *src) {
  size_t tokenc = 0; /* tokens count */
  char *tokens[CMD_TOKENS_MAX] = { NULL };
  char line[INPUT_LINE_MAX];
  char *p;

  assert(line != NULL);
  /* strip leading spaces */
  while (isblank(*line))
    src++;
  /* strip trailing spaces, newlines, etc */
  strlcpy(line, src, sizeof(line));
  for (size_t l = strlen(line); l >= 1 && isspace(line[l - 1]); l--)
    line[l - 1] = '\0';

  if (line[0] == '\0')
    return CMD_NONE; /* empty string */

  /* simple commands without args */
  if      (strcmp(line, "ping")     == 0) { return CMD_PING;     }
  else if (strcmp(line, "help")     == 0) { return CMD_HELP;     }
  else if (strcmp(line, "status")   == 0) { return CMD_STATUS;   }
  else if (strcmp(line, "rotate")   == 0) { return CMD_ROTATE;   }
  else if (strcmp(line, "reload")   == 0) { return CMD_RELOAD;   }
  else if (strcmp(line, "shutdown") == 0) { return CMD_SHUTDOWN; }

  /* split string to tokens */
  tokenc = 1; /* we has at least one token */
  tokens[0] = strtok_r(line, " \t", &p);
  for (size_t i = 1; i < CMD_TOKENS_MAX; i++) {
    tokens[i] = strtok_r(NULL, " \t", &p);
    if (tokens[i] == NULL)
      break;
    tokenc++;
  }

  buf[0] = '\0';
  if (strcmp(line, "jail") == 0 && tokenc > 1) {
    /* commands for jail */
    f2b_cmd_append_arg(buf, bufsize, tokens[1]);
    if (tokenc == 3 && strcmp(tokens[2], "status") == 0) {
      return CMD_JAIL_STATUS;
    }
    if (tokenc == 5 && strcmp(tokens[2], "set") == 0) {
      f2b_cmd_append_arg(buf, bufsize, tokens[3]);
      f2b_cmd_append_arg(buf, bufsize, tokens[4]);
      return CMD_JAIL_SET;
    }
    if (tokenc == 5 && strcmp(tokens[2], "ip") == 0 && strcmp(tokens[3], "status") == 0) {
      f2b_cmd_append_arg(buf, bufsize, tokens[4]);
      return CMD_JAIL_IP_STATUS;
    }
    if (tokenc == 5 && strcmp(tokens[2], "ip") == 0 && strcmp(tokens[3], "ban") == 0) {
      f2b_cmd_append_arg(buf, bufsize, tokens[4]);
      return CMD_JAIL_IP_BAN;
    }
    if (tokenc == 5 && strcmp(tokens[2], "ip") == 0 && strcmp(tokens[3], "release") == 0) {
      f2b_cmd_append_arg(buf, bufsize, tokens[4]);
      return CMD_JAIL_IP_RELEASE;
    }
    if (tokenc == 4 && strcmp(tokens[2], "filter") == 0 && strcmp(tokens[3], "stats") == 0) {
      return CMD_JAIL_FILTER_STATS;
    }
    if (tokenc == 4 && strcmp(tokens[2], "filter") == 0 && strcmp(tokens[3], "reload") == 0) {
      return CMD_JAIL_FILTER_RELOAD;
    }
  }

  return CMD_NONE;
}

bool
f2b_cmd_check_argc(enum f2b_cmd_type type, int argc) {
  if (commands[type].argc == argc)
    return true;
  return false;
}
