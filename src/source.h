/* Copyright 2016 Alex 'AdUser' Z (ad_user@runbox.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef F2B_SOURCE_H_
#define F2B_SOURCE_H_

#include "config.h"
#include "log.h"

/**
 * @file
 * This header describes source module definition and related routines
 */

/** source module definition */
typedef struct f2b_source_t {
  void *h;   /**< dlopen handler */
  void *cfg; /**< opaque pointer of module config */
  /* handlers */
  /** dlsym pointer to handler of @a create command */
  void *(*create)  (const char *init);
  /** dlsym pointer to handler of @a config command */
  bool  (*config)  (void *cfg, const char *key, const char *value);
  /** dlsym pointer to handler of @a ready command */
  bool  (*ready)   (void *cfg);
  /** dlsym pointer to handler of @a error command */
  char *(*error)   (void *cfg);
  /** dlsym pointer to handler of @a errcb command */
  void  (*errcb)   (void *cfg, void (*cb)(const char *errstr));
  /** dlsym pointer to handler of @a start command */
  bool  (*start)   (void *cfg);
  /** dlsym pointer to handler of @a next command */
  bool  (*next)    (void *cfg, char *buf, size_t bufsize, bool reset);
  /** dlsym pointer to handler of @a stop command */
  bool  (*stop)    (void *cfg);
  /** dlsym pointer to handler of @a destroy command */
  void  (*destroy) (void *cfg);
} f2b_source_t;

/**
 * @brief Create module from config
 * @param config Pointer to config section with module description
 * @param init   Module init string
 * @param errcb  Error callback
 * @returns Pointer to allocated module struct or NULL on error
 */
f2b_source_t * f2b_source_create  (f2b_config_section_t *config, const char *init, void (*errcb)(const char *));
/**
 * @brief Free module metadata
 * @param s Pointer to module struct
 */
void f2b_source_destroy (f2b_source_t *s);

/**
 * @brief Start given source
 * @param s Pointer to source struct
 * @returns true on success, false on error with setting last error
 */
bool f2b_source_start (f2b_source_t *s);
/**
 * @brief Get next line of data from given source
 * @param s Pointer to source struct
 * @param buf Buffer for data
 * @param bufsize Size of buffer for data
 * @param reset Reset source internals
 * @returns false of no data available, true otherwise with setting @a buf
 */
bool f2b_source_next (f2b_source_t *s, char *buf, size_t bufsize, bool reset);
/**
 * @brief Stop given source
 * @param s Pointer to source struct
 * @returns true on success, false on error with setting last error
 */
bool f2b_source_stop (f2b_source_t *s);
/**
 * @brief Get last source error
 * @param s Pointer to source struct
 * @returns Pointer to string with description of last error
 */
const char * f2b_source_error (f2b_source_t *s);

#endif /* F2B_SOURCE_H_ */
