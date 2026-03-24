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

/* Save the current file on disk. Return 0 on success, 1 on error.
 * Special buffers (filename is NULL or starts with '*') prompt for a name. */
int editorSave(int fd)
{
	char *buf;
	int len;
	int filefd;

	if (isSpecialBuffer(E.filename)) {
		char newname[256];
		if (editorReadLine(fd, "Write file: ", newname, sizeof(newname)) < 0
		    || !newname[0])
			return 1;
		free(E.filename);
		E.filename = strdup(newname);
		editorSelectSyntaxHighlight(E.filename);
	}

	buf = editorRowsToString(E.row, E.numrows, &len);
	filefd = open(E.filename, O_RDWR|O_CREAT, 0644);
	if (filefd == -1) goto writeerr;

	/* Use truncate + a single write(2) call in order to make saving
	 * a bit safer, under the limits of what we can do in a small editor. */
	if (ftruncate(filefd, len) == -1) goto writeerr;
	if (write(filefd, buf, len) != len) goto writeerr;

	close(filefd);
	free(buf);
	E.dirty = 0;
	undoMarkClean();  /* Mark this state as clean for undo tracking */
	editorSetStatusMessage("Wrote %s (%d bytes)", E.filename, len);
	return 0;

writeerr:
	free(buf);
	if (filefd != -1)
		close(filefd);

	editorSetStatusMessage("Error writing %s: %s", E.filename, strerror(errno));
	return 1;
}
