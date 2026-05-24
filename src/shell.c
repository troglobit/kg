/* ============================ Shell commands ==============================
 *
 * M-!  shell-command            Run a shell command, insert stdout at point.
 * M-|  shell-command-on-region  Pipe the region through a shell command and
 *                               replace it with stdout.  The original region
 *                               is left in the kill ring as a safety net.
 *
 * The child is spawned via /bin/sh -c so the user can use pipes, redirects,
 * and shell builtins.  Stdin/stdout are connected to pipes; stderr goes to
 * /dev/null so it cannot disturb the editor display.  No tty is shared with
 * the child, so interactive commands (less, vi, ...) will not work — by
 * design, M-! is for non-interactive filters. */

#include "def.h"
#include <poll.h>
#include <sys/wait.h>

#define SHELL_INITIAL_CAP 4096

/* Concurrently write `in` (inlen bytes) to wfd and read everything from rfd
 * into a newly-malloc'd buffer.  Either fd may be -1 to skip that side.
 * Both fds are closed before return.  Returns the output buffer on success
 * (NUL-terminated, *out_len set), NULL on allocation failure. */
static char *pump_io(int rfd, int wfd, const char *in, int inlen, int *out_len)
{
	struct pollfd pfd[2];
	char *buf;
	int  buf_cap = SHELL_INITIAL_CAP;
	int  buf_len = 0;
	int  written = 0;
	int  npoll, i, n;

	buf = malloc(buf_cap);
	if (!buf) {
		if (rfd >= 0) close(rfd);
		if (wfd >= 0) close(wfd);
		*out_len = 0;
		return NULL;
	}

	if (rfd >= 0) fcntl(rfd, F_SETFL, O_NONBLOCK);
	if (wfd >= 0) fcntl(wfd, F_SETFL, O_NONBLOCK);

	/* If there's no input to send, close the write side immediately so the
	 * child sees EOF on stdin and isn't left blocking on a read. */
	if (wfd >= 0 && (in == NULL || inlen <= 0)) {
		close(wfd);
		wfd = -1;
	}

	while (rfd >= 0 || wfd >= 0) {
		npoll = 0;
		if (rfd >= 0) {
			pfd[npoll].fd     = rfd;
			pfd[npoll].events = POLLIN;
			npoll++;
		}
		if (wfd >= 0) {
			pfd[npoll].fd     = wfd;
			pfd[npoll].events = POLLOUT;
			npoll++;
		}

		if (poll(pfd, npoll, -1) < 0) {
			if (errno == EINTR) continue;
			break;
		}

		for (i = 0; i < npoll; i++) {
			if (pfd[i].fd == rfd && (pfd[i].revents & (POLLIN|POLLHUP|POLLERR))) {
				if (buf_len + 1024 >= buf_cap) {
					char *nb;
					buf_cap *= 2;
					nb = realloc(buf, buf_cap);
					if (!nb) {
						free(buf);
						if (rfd >= 0) close(rfd);
						if (wfd >= 0) close(wfd);
						*out_len = 0;
						return NULL;
					}
					buf = nb;
				}
				n = read(rfd, buf + buf_len, buf_cap - buf_len - 1);
				if (n > 0)
					buf_len += n;
				else {
					close(rfd);
					rfd = -1;
				}
			}
			if (pfd[i].fd == wfd && (pfd[i].revents & (POLLOUT|POLLHUP|POLLERR))) {
				n = write(wfd, in + written, inlen - written);
				if (n > 0) {
					written += n;
					if (written >= inlen) {
						close(wfd);
						wfd = -1;
					}
				} else if (n < 0 && errno == EAGAIN) {
					;
				} else {
					close(wfd);
					wfd = -1;
				}
			}
		}
	}

	buf[buf_len] = '\0';
	*out_len = buf_len;
	return buf;
}

/* Run `cmd` via /bin/sh -c.  Optionally write `in` (inlen bytes) to its stdin.
 * On success returns a malloc'd buffer of the child's stdout and sets *out_len.
 * Returns NULL on fork/pipe failure.  Exposed (rather than static) so the
 * test suite can exercise it without driving the editor through a PTY. */
char *shell_run(const char *cmd, const char *in, int inlen, int *out_len)
{
	void (*old_sigpipe)(int);
	int in_pipe[2]  = {-1, -1};
	int out_pipe[2] = {-1, -1};
	int saved_errno;
	char *out;
	pid_t pid;

	if (pipe(in_pipe)  < 0) goto fail;
	if (pipe(out_pipe) < 0) goto fail;

	pid = fork();
	if (pid < 0) goto fail;

	if (pid == 0) {
		int devnull = open("/dev/null", O_WRONLY);

		dup2(in_pipe[0],  STDIN_FILENO);
		dup2(out_pipe[1], STDOUT_FILENO);
		if (devnull >= 0) {
			dup2(devnull, STDERR_FILENO);
			close(devnull);
		}
		close(in_pipe[0]);
		close(in_pipe[1]);
		close(out_pipe[0]);
		close(out_pipe[1]);

		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		_exit(127);
	}

	close(in_pipe[0]);
	close(out_pipe[1]);

	/* If the child exits early we don't want EPIPE to kill us. */
	old_sigpipe = signal(SIGPIPE, SIG_IGN);

	out = pump_io(out_pipe[0], in_pipe[1], in, inlen, out_len);
	saved_errno = errno;
	signal(SIGPIPE, old_sigpipe);
	waitpid(pid, NULL, 0);
	errno = saved_errno;
	return out;

fail:
	saved_errno = errno;
	if (in_pipe[0]  >= 0) close(in_pipe[0]);
	if (in_pipe[1]  >= 0) close(in_pipe[1]);
	if (out_pipe[0] >= 0) close(out_pipe[0]);
	if (out_pipe[1] >= 0) close(out_pipe[1]);
	*out_len = 0;
	errno = saved_errno;
	return NULL;
}

/* Insert text at point as a single undoable yank. */
static void insert_as_yank(const char *text, int len)
{
	int filerow = editor.rowoff + editor.cy;
	int filecol = editor.coloff + editor.cx;

	if (len <= 0) return;

	undo_push(UNDO_YANK_TEXT, filerow, filecol, 0, (char *)text, len);
	editor_insert_text_raw(text, len);
}

/* M-! shell-command: prompt, run, insert stdout at point. */
void editor_shell_command(int fd)
{
	char cmd[256];
	char *out;
	int   out_len = 0;

	if (editor.readonly) {
		editor_set_status_message("Buffer is read-only");
		return;
	}
	if (editor_read_line(fd, "Shell command: ", cmd, sizeof(cmd)) < 0 || !cmd[0])
		return;

	out = shell_run(cmd, NULL, 0, &out_len);
	if (!out) {
		editor_set_status_message("Shell command failed: %s", strerror(errno));
		return;
	}

	insert_as_yank(out, out_len);
	free(out);
	editor_set_status_message("Inserted %d byte%s", out_len, out_len == 1 ? "" : "s");
}

/* M-| shell-command-on-region: pipe region through cmd, replace it with stdout. */
void editor_shell_command_on_region(int fd)
{
	char cmd[256];
	char *region, *out;
	int region_len = 0, out_len = 0;

	if (editor.readonly) {
		editor_set_status_message("Buffer is read-only");
		return;
	}
	if (!editor.mark_set) {
		editor_set_status_message("No mark set");
		return;
	}

	region = editor_get_region_text(&region_len);
	if (!region) {
		editor_set_status_message("Empty region");
		return;
	}

	if (editor_read_line(fd, "Shell command on region: ", cmd, sizeof(cmd)) < 0 || !cmd[0]) {
		free(region);
		return;
	}

	out = shell_run(cmd, region, region_len, &out_len);
	free(region);
	if (!out) {
		editor_set_status_message("Shell command failed: %s", strerror(errno));
		return;
	}

	/* Delete the region (cursor lands at the start, region goes to kill ring). */
	editor_kill_region();
	insert_as_yank(out, out_len);
	free(out);
	editor_set_status_message("Replaced region (%d byte%s out)",
	                          out_len, out_len == 1 ? "" : "s");
}
