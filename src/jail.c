#include "common.h"
#include "jail.h"

#define DEFAULT_STATE   true
#define DEFAULT_BANTIME 3600 /* in seconds, 1 hour */
#define DEFAULT_TRIES   5

static f2b_jail_t defaults = {
  .enabled  = DEFAULT_STATE,
  .bantime  = DEFAULT_BANTIME,
  .tries    = DEFAULT_TRIES,
};

static void
f2b_jail_parse_compound_value(const char *value, char *name, char *init) {
  size_t len = 0;
  char *p = NULL;

  if ((p = strchr(value, ':')) == NULL) {
    /* param = name */
    strncpy(name, value, CONFIG_KEY_MAX);
    return;
  }

  /* param = name:init_string */
  len = p - value;
  if (len >= CONFIG_KEY_MAX) {
    f2b_log_msg(log_warn, "'name' part of value exceeds max length %d bytes: %s", CONFIG_KEY_MAX, value);
    return;
  }

  strncpy(name, value, len);
  strncpy(init, (p + 1), CONFIG_VAL_MAX);
  return;
}

void
f2b_jail_apply_config(f2b_jail_t *jail, f2b_config_section_t *section) {
  f2b_config_param_t *param = NULL;

  assert(jail != NULL);
  assert(section != NULL);
  assert(section->type != t_jail || section->type == t_defaults);

  for (param = section->param; param != NULL; param = param->next) {
    if (strcmp(param->name, "enabled") == 0) {
      if (strcmp(param->value, "yes") == 0)
        jail->enabled = true;
      continue;
    }
    if (strcmp(param->name, "bantime") == 0) {
      jail->bantime = atoi(param->value);
      if (jail->bantime <= 0)
        jail->bantime = DEFAULT_BANTIME;
      continue;
    }
    if (strcmp(param->name, "tries") == 0) {
      jail->bantime = atoi(param->value);
      if (jail->tries <= 0)
        jail->tries = DEFAULT_TRIES;
      continue;
    }
    if (strcmp(param->name, "source") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->source_name, jail->source_init);
      continue;
    }
    if (strcmp(param->name, "filter") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->filter_name, jail->filter_init);
      continue;
    }
    if (strcmp(param->name, "backend") == 0) {
      f2b_jail_parse_compound_value(param->value, jail->backend_name, jail->backend_init);
      continue;
    }
  }

  return;
}

void
f2b_jail_set_defaults(f2b_config_section_t *section) {
  assert(section != NULL);
  assert(section->type == t_defaults);

  f2b_jail_apply_config(&defaults, section);

  return;
}

bool
f2b_jail_ban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  assert(jail != NULL);
  assert(addr != NULL);

  addr->matches.used = 0;
  addr->banned  = true;
  addr->bantime = addr->lastseen;

  if (f2b_backend_ban(jail->backend, addr->text)) {
    f2b_log_msg(log_info, "banned ip in jail '%s': %s", jail->name, addr->text);
    return true;
  }

  f2b_log_msg(log_error, "can't ban ip in jail '%s': backend failure for '%s'", jail->name, addr->text);
  return false;
}

bool
f2b_jail_unban(f2b_jail_t *jail, f2b_ipaddr_t *addr) {
  assert(jail != NULL);
  assert(addr != NULL);

  addr->banned  = false;
  addr->bantime = 0;

  if (f2b_backend_unban(jail->backend, addr->text)) {
    f2b_log_msg(log_info, "released ip in jail '%s': %s", jail->name, addr->text);
    return true;
  }

  f2b_log_msg(log_error, "can't release ip in jail '%s': backend failure for '%s'", jail->name, addr->text);
  return false;
}

size_t
f2b_jail_process(f2b_jail_t *jail) {
  f2b_logfile_t *file = NULL;
  f2b_ipaddr_t  *addr = NULL;
  size_t processed = 0;
  char logline[LOGLINE_MAX] = "";
  char matchbuf[IPADDR_MAX] = "";
  time_t now  = time(NULL);
  time_t release_time = 0;

  assert(jail != NULL);

  for (file = jail->logfiles; file != NULL; file = file->next) {
    while (f2b_logfile_getline(file, logline, sizeof(logline))) {
      if (!f2b_regexlist_match(jail->regexps, logline, matchbuf, sizeof(matchbuf)))
        continue;
      /* some regex matches the line */
      addr = f2b_addrlist_lookup(jail->ipaddrs, matchbuf);
      if (!addr) {
        /* new ip */
        addr = f2b_ipaddr_create(matchbuf, jail->tries);
        addr->lastseen = now;
        f2b_matches_append(&addr->matches, now);
        jail->ipaddrs = f2b_addrlist_append(jail->ipaddrs, addr);
        f2b_log_msg(log_debug, "new ip found by jail '%s': %s", jail->name, matchbuf);
        continue;
      }
      /* this ip was seen before */
      addr->lastseen = now;
      if (addr->banned) {
        f2b_log_msg(log_warn, "found ip that was already banned by jail '%s': %s", jail->name, matchbuf);
        continue;
      }
      f2b_matches_expire(&addr->matches, now - jail->bantime);
      f2b_matches_append(&addr->matches, now);
      if (addr->matches.used < jail->tries) {
        f2b_log_msg(log_debug, "new match in jail '%s': %s (%d/%d)", jail->name, matchbuf, addr->matches.used, addr->matches.max);
        continue;
      }
      /* limit reached, ban ip */
      f2b_jail_ban(jail, addr);
     } /* while(lines) */
  } /* for(files) */

  for (addr = jail->ipaddrs; addr != NULL; addr = addr->next) {
    if (!addr->banned)
      continue;
    release_time = addr->bantime + jail->bantime;
    if (now < release_time) {
      f2b_log_msg(log_debug, "skipping banned ip in jail '%s': %s (%.1fh remains)", jail->name, addr->text, (now - release_time) / 3600);
      continue;
    }
    f2b_jail_unban(jail, addr);
  }

  return processed;
}

bool
f2b_jail_init(f2b_jail_t *jail, f2b_config_t *config) {
  f2b_config_section_t * b_section = NULL;

  assert(jail   != NULL);
  assert(config != NULL);

  /* source */
  if (jail->source_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'source' parameter", jail->name);
    return false;
  }
  /* TODO: temp stub */
  if (strcmp(jail->source_name, "files") != 0) {
    f2b_log_msg(log_error, "jail '%s': 'source' supports only 'files' for now", jail->name);
    return false;
  }
  if (jail->source_init == '\0') {
    f2b_log_msg(log_error, "jail '%s': 'source' requires file or files pattern", jail->name);
    return false;
  }

  /* filter */
  if (jail->filter_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'filter' parameter", jail->name);
    return false;
  }
  /* TODO: temp stub */
  if (strcmp(jail->filter_name, "preg") != 0) {
    f2b_log_msg(log_error, "jail '%s': 'filter' supports only 'preg' for now", jail->name);
    return false;
  }
  if (jail->filter_init == '\0') {
    f2b_log_msg(log_error, "jail '%s': 'filter' requires path to file with regexps", jail->name);
    return false;
  }

  /* backend */
  if (jail->backend_name[0] == '\0') {
    f2b_log_msg(log_error, "jail '%s': missing 'backend' parameter", jail->name);
    return false;
  }
  if ((b_section = f2b_config_section_find(config->backends, jail->backend_name)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no filter with name '%s'", jail->name, jail->backend_name);
    return false;
  }

  /* init all */
  if ((jail->logfiles = f2b_filelist_from_glob(jail->source_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no files matching '%s' pattern", jail->name, jail->source_init);
    goto cleanup;
  }

  if ((jail->regexps = f2b_regexlist_from_file(jail->filter_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': no regexps loaded from '%s'", jail->name, jail->filter_init);
    goto cleanup;
  }

  if ((jail->backend = f2b_backend_create(b_section, jail->backend_init)) == NULL) {
    f2b_log_msg(log_error, "jail '%s': can't init backend '%s' with %s",
      jail->name, jail->backend_name, jail->backend_init);
    goto cleanup;
  }

  return true;

  cleanup:
  if (jail->logfiles)
    f2b_filelist_destroy(jail->logfiles);
  if (jail->regexps)
    f2b_regexlist_destroy(jail->regexps);
  if (jail->backend)
    f2b_backend_destroy(jail->backend);
  return false;
}
