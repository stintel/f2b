#ifndef FILELIST_H_
#define FILELIST_H_

f2b_logfile_t *
f2b_filelist_from_glob(const char *pattern);

f2b_logfile_t *
f2b_filelist_append(f2b_logfile_t *list, f2b_logfile_t *file);

void
f2b_filelist_destroy(f2b_logfile_t *list);

#endif /* FILELIST_H_ */
