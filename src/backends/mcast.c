/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>

#include "../strlcpy.h"
#include "../commands.h"
#include "../cmsg.h"
#include "../csocket.h"

#include "backend.h"
#include "shared.c"

#define DEFAULT_MCAST_ADDR "239.255.186.1"
#define DEFAULT_MCAST_PORT "3370"

struct _config {
  char name[ID_MAX + 1];
  char error[256];
  bool shared;
  char maddr[INET_ADDRSTRLEN];  /**< multicast address */
  char mport[6];                /**< multicast port */
  char iface[IF_NAMESIZE];      /**< bind interface */
  int sock;
};

cfg_t *
create(const char *id) {
  cfg_t *cfg = NULL;

  assert(id != NULL);

  if ((cfg = calloc(1, sizeof(cfg_t))) == NULL)
    return NULL;
  strlcpy(cfg->name, id, sizeof(cfg->name));
  strlcpy(cfg->maddr, DEFAULT_MCAST_ADDR, sizeof(cfg->maddr));
  strlcpy(cfg->mport, DEFAULT_MCAST_PORT, sizeof(cfg->mport));

  return cfg;
}

bool
config(cfg_t *cfg, const char *key, const char *value) {
  assert(cfg != NULL);
  assert(key != NULL);
  assert(value != NULL);

  if (strcmp(key, "group") == 0) {
    if (strncmp(value, "239.255.", 8) != 0) {
      strlcpy(cfg->error, "mcast group address should be inside 239.255.0.0/16 block", sizeof(cfg->error));
      return false;
    }
    strlcpy(cfg->maddr, value, sizeof(cfg->maddr));
    return true;
  }
  if (strcmp(key, "port") == 0) {
    strlcpy(cfg->mport, value, sizeof(cfg->mport));
    return true;
  }
  if (strcmp(key, "iface") == 0) {
    strlcpy(cfg->iface, value, sizeof(cfg->iface));
    return true;
  }

  return false;
}

bool
ready(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->maddr[0] && cfg->mport[0])
    return true;

  return false;
}

char *
error(cfg_t *cfg) {
  assert(cfg != NULL);

  return cfg->error;
}

bool
start(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_inc(cfg->name) > 1)
    return true;

  /* TODO */

  return true;
}

bool
stop(cfg_t *cfg) {
  assert(cfg != NULL);

  if (cfg->shared && usage_dec(cfg->name) > 0)
    return true;

  close(cfg->sock);
  cfg->sock = -1;

  return true;
}

bool
ban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  /* TODO */

  return false;
}

bool
unban(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  (void)(ip); /* suppress warning for unused variable 'ip' */
  return true;
}

bool
check(cfg_t *cfg, const char *ip) {
  assert(cfg != NULL);

  (void)(ip); /* suppress warning for unused variable 'ip' */
  return false;
}

bool
ping(cfg_t *cfg) {
  assert(cfg != NULL);

  /* TODO */

  return false;
}

void
destroy(cfg_t *cfg) {
  assert(cfg != NULL);

  free(cfg);
}