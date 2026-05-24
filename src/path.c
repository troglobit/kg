/* ============================== Path helpers ===============================
 *
 * Self-contained filename utilities used by the minibuffer's Tab completion.
 * Kept here, separate from bufmgr.c, so unit tests can exercise them without
 * pulling in the rest of the editor's globals. */

#include "def.h"

/* Split `path` into directory part (up to and including the last '/') and
 * file part (the rest).  If no '/' is present the directory is "./" and the
 * file is the whole path.  Both outputs are NUL-terminated. */
void editor_path_split(const char *path, char *dir, int dsize, char *file, int fsize)
{
	const char *slash = strrchr(path, '/');
	int dlen;

	if (!slash) {
		snprintf(dir,  dsize, "./");
		snprintf(file, fsize, "%s", buf_basename(path));
		return;
	}
	dlen = (int)(slash - path) + 1;
	if (dlen >= dsize) dlen = dsize - 1;
	memcpy(dir, path, dlen);
	dir[dlen] = '\0';
	snprintf(file, fsize, "%s", buf_basename(path));
}

/* Scan `dir` for entries whose name starts with `prefix`.  Returns the number
 * of matches, or -1 on opendir failure.  Outputs (any of which may be NULL):
 *   - `lcp`     receives the longest common prefix of the matching names.
 *   - `is_dir`  is set to 1 iff the sole match is a directory.
 *   - `names`   receives space-separated matching names appended starting at
 *               *`names_off` (bytes used), capped at `names_size`.
 *               Names that don't fit are silently skipped; *names_off is
 *               always left ≤ names_size - 1 with a trailing NUL written. */
int editor_path_complete(const char *dir, const char *prefix,
                         char *lcp, int lcp_size, int *is_dir,
                         char *names, int *names_off, int names_size)
{
	struct dirent *de;
	DIR *dp;
	int plen = (int)strlen(prefix);
	int matches = 0;
	int lcp_len = 0;
	int names_full = 0;
	int i;

	if (is_dir) *is_dir = 0;
	if (lcp) lcp[0] = '\0';

	dp = opendir(dir[0] ? dir : ".");
	if (!dp) return -1;

	while ((de = readdir(dp)) != NULL) {
		const char *name = de->d_name;

		/* Hide dotfiles unless the user typed a leading dot; always hide
		 * the bare "." entry — listing the current dir against itself is
		 * just noise. */
		if (name[0] == '.' && (name[1] == '\0' || prefix[0] != '.'))
			continue;
		if (strncmp(name, prefix, plen) != 0) continue;

		matches++;

		if (lcp) {
			if (matches == 1) {
				snprintf(lcp, lcp_size, "%s", name);
				lcp_len = (int)strlen(lcp);
			} else {
				for (i = 0; i < lcp_len && name[i] && lcp[i] == name[i]; i++)
					;
				lcp_len = i;
				lcp[lcp_len] = '\0';
			}
		}

		if (names && !names_full) {
			int room = names_size - *names_off - 1;
			int n = snprintf(names + *names_off, room + 1, "%s ", name);
			if (n > 0 && n <= room)
				*names_off += n;
			else
				names_full = 1;   /* truncate; keep scanning for LCP/count */
		}

		/* Once LCP has collapsed to the typed prefix it cannot shrink
		 * further, the sole-match stat() below is moot (matches > 1),
		 * and the names buffer can take no more candidates — every
		 * remaining iteration would just keep counting.  Bail. */
		if (matches > 1 && lcp_len <= plen && names && names_full)
			break;
	}
	closedir(dp);

	/* stat() only the unambiguous case — that's the only path through the
	 * caller where the trailing '/' might be appended. */
	if (matches == 1 && is_dir && lcp) {
		char full[PATH_MAX];
		struct stat st;
		int n = snprintf(full, sizeof(full), "%s%s", dir[0] ? dir : "./", lcp);

		if (n < (int)sizeof(full) &&
		    stat(full, &st) == 0 && S_ISDIR(st.st_mode))
			*is_dir = 1;
	}

	return matches;
}
