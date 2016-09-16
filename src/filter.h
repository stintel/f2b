/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_FILTER_H_
#define F2B_FILTER_H_

#include "config.h"
#include "log.h"

typedef struct f2b_filter_t {
  void *h;
  void *cfg;
  void *(*create)  (const char *id);
  bool  (*config)  (void *cfg, const char *key, const char *value);
  bool  (*append)  (void *cfg, const char *pattern);
  char *(*error)   (void *cfg);
  bool  (*ready)   (void *cfg);
  bool  (*stats)   (void *cfg, int *matches, char **pattern, bool reset);
  bool  (*match)   (void *cfg, const char *line, char *buf, size_t buf_size);
  void  (*destroy) (void *cfg);
} f2b_filter_t;

f2b_filter_t * f2b_filter_create  (f2b_config_section_t *config, const char *id);
const char *   f2b_filter_error   (f2b_filter_t *f);
bool           f2b_filter_append  (f2b_filter_t *f, const char *pattern);
bool           f2b_filter_match   (f2b_filter_t *f, const char *line, char *buf, size_t buf_size);
void           f2b_filter_stats   (f2b_filter_t *f, char *res, size_t ressize);
void           f2b_filter_destroy (f2b_filter_t *f);

#endif /* F2B_FILTER_H_ */
