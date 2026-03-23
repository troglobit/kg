/* =============================== File I/O ================================= */

#include "def.h"

/* Load the specified program in the editor memory and returns 0 on success
 * or 1 on error. */
int editorOpen(char *filename)
{
	ssize_t linelen;
	size_t linecap = 0;
	size_t fnlen = strlen(filename) + 1;
	char *line = NULL;
	FILE *fp;

	E.dirty = 0;
	free(E.filename);
	E.filename = malloc(fnlen);
	memcpy(E.filename, filename, fnlen);

	fp = fopen(filename, "r");
	if (!fp) {
		if (errno != ENOENT) {
			perror("Opening file");
			exit(1);
		}
		return 1;
	}

	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		if (linelen && (line[linelen-1] == '\n' || line[linelen-1] == '\r'))
			line[--linelen] = '\0';
		editorInsertRow(E.numrows, line, linelen);
	}
	free(line);
	fclose(fp);
	E.dirty = 0;
	undoMarkClean();  /* Mark initial file state as clean */
	return 0;
}

/* Save the current file on disk. Return 0 on success, 1 on error. */
int editorSave(void)
{
	char *buf;
	int len;
	int fd;

	buf = editorRowsToString(&len);
	fd = open(E.filename, O_RDWR|O_CREAT, 0644);
	if (fd == -1) goto writeerr;

	/* Use truncate + a single write(2) call in order to make saving
	 * a bit safer, under the limits of what we can do in a small editor. */
	if (ftruncate(fd, len) == -1) goto writeerr;
	if (write(fd, buf, len) != len) goto writeerr;

	close(fd);
	free(buf);
	E.dirty = 0;
	undoMarkClean();  /* Mark this state as clean for undo tracking */
	editorSetStatusMessage("Wrote %s (%d bytes)", E.filename, len);
	return 0;

writeerr:
	free(buf);
	if (fd != -1)
		close(fd);

	editorSetStatusMessage("Error writing %s: %s", E.filename, strerror(errno));
	return 1;
}
