#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <regex.h>
#include <ncurses.h>

#include "utils/stdio_utils.h"
#include "utils/stdlib_utils.h"
#include "utils/string_utils.h"
#include "utils/unistd_utils.h"
#include "utils/locale_utils.h"
#include "utils/regex_utils.h"
#include "utils/ncurses_utils.h"
#include "utils/ptrlist.h"
#include "utils/strlist.h"
#include "utils/unilist.h"

struct unilist regexes;
struct strlist regexes_type;
struct ptrlist lines;
struct strlist collapsed;

size_t screen_pos;
size_t cursor_pos;

int rows;

static void refresh_screen() {
	rows = getmaxy(stdscr);

	int cursor_to = 0;
	int pos = 0;
	for (size_t i = screen_pos; i < lines.size; i++) {
		if (!collapsed.data[i]) {
			move(pos, 0);
			clrtoeol();
			addstr(lines.data[i]);

			if (i == cursor_pos) {
				cursor_to = pos;
			}

			if (++pos == rows) {
				break;
			}
		}
	}

	while (pos < rows) {
		move(pos++, 0);
		clrtoeol();
	}

	move(cursor_to, 0);
}

static void cursor_up() {
	size_t pos = cursor_pos;
	while (pos > 0) {
		pos--;
		if (!collapsed.data[pos]) {
			break;
		}
	}
	cursor_pos = pos;
}

static void cursor_down() {
	size_t pos = cursor_pos;
	while (pos < lines.size - 1) {
		pos++;
		if (!collapsed.data[pos]) {
			cursor_pos = pos;
			break;
		}
	}
}

static void refresh_screen_pos() {
	if (cursor_pos < screen_pos) {
		screen_pos = cursor_pos;
	} else if (cursor_pos > screen_pos) {
		rows = getmaxy(stdscr);

		size_t cnt = 0;
		size_t pos = cursor_pos;
		while (pos > screen_pos && cnt < rows - 1) {
			pos--;
			if (!collapsed.data[pos]) {
				cnt++;
			}
		}
		if (pos > screen_pos) {
			screen_pos = pos;
		}
	}
}

static void set_collapsed(char collapse) {
	size_t pos = cursor_pos;
	while (pos > 0 && ((char*) lines.data[pos])[0] == '\t') {
		pos--;
	}

	if (collapse) {
		cursor_pos = pos;
	}

	pos++;

	while (pos < lines.size && ((char*) lines.data[pos])[0] == '\t') {
		collapsed.data[pos++] = collapse;
	}

	if (!collapse) {
		cursor_pos = pos - 1;
	}
}

static void run_gui(FILE *tty) {
	SCREEN *screen = newterm_safe(NULL, tty, tty);
	noecho();

	refresh_screen();

	int arrow_status = 0;
	while (1) {
		int arrow_status_new = 0;

		int key = getch_safe();
		if (key == 'q' || key == ERR) {
			break;
		} else if (key == 27) {
			arrow_status_new = 1;
		} else if (key == '[' && arrow_status == 1) {
			arrow_status_new = 2;
		} else if (key == 'k' || (key == 'A' && arrow_status == 2)) {
			cursor_up();
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 'j' || (key == 'B' && arrow_status == 2)) {
			cursor_down();
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 'l' || (key == 'C' && arrow_status == 2)) {
			set_collapsed(0);
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 'h' || (key == 'D' && arrow_status == 2)) {
			set_collapsed(1);
			refresh_screen_pos();
			refresh_screen();
		} else if (key == '5' && arrow_status == 2) {
			rows = getmaxy(stdscr);
			for (int i = 0; i < rows; i++) {
				cursor_up();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == '6' && arrow_status == 2) {
			rows = getmaxy(stdscr);
			for (int i = 0; i < rows; i++) {
				cursor_down();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 21) {
			rows = getmaxy(stdscr);
			for (int i = 0; i < rows; i += 2) {
				cursor_up();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 4) {
			rows = getmaxy(stdscr);
			for (int i = 0; i < rows; i += 2) {
				cursor_down();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 'g' || (key == 'H' && arrow_status == 2)) {
			cursor_pos = 0;
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 'G' || (key == 'F' && arrow_status == 2)) {
			cursor_pos = lines.size;
			cursor_up();
			refresh_screen_pos();
			refresh_screen();
		} else if (key == KEY_RESIZE) {
			refresh_screen();
		}

		arrow_status = arrow_status_new;
	}

	endwin();
	delscreen_trace(screen);
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv && isatty_safe(fileno_safe(stdin))) {
		printf_safe("Usage: %s [arguments] <log file>\n"
			"\n"
			"arguments:\n"
			"    -in <regex>    regular expression to include (optional, can be specified multiple times)\n"
			"    -ex <regex>    regular expression to exclude (optional, can be specified multiple times)\n", pname);
		return 1;
	}

	while (*argv && *argv[0] == '-') {
		char *arg = *argv++;

		if (!strcmp(arg, "--")) {
			break;
		} else if ((!strcmp(arg, "-in") || !strcmp(arg, "-ex")) && *argv) {
			regex_t compiled;
			int ret = regcomp_trace(&compiled, *argv++, REG_EXTENDED | REG_NOSUB);
			if (ret) {
				size_t bufsize = regerror(ret, &compiled, NULL, 0);
				char *buf = malloc_safe(bufsize);
				regerror(ret, &compiled, buf, bufsize);
				fprintf(stderr, "Invalid regular expression: %s\n", buf);
				free_trace(buf);
				return 2;
			}
			unilist_add(&regexes, &compiled);
			strlist_add(&regexes_type, !strcmp(arg, "-in"));
		} else {
			fprintf(stderr, "Invalid argument: %s\n", arg);
			return 2;
		}
	}

	const char *filename = NULL;
	if (*argv) {
		filename = *argv++;
	}

	if (*argv) {
		fprintf(stderr, "Too many arguments\n");
		return 2;
	}

	FILE *input;
	if (filename) {
		input = fopen_trace(filename, "r");
		if (!input) {
			fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
			return 2;
		}
	} else {
		input = stdin;
		filename = "stdin";
	}

	int ret = 0;

	struct strlist inbuf;
	strlist_init(&inbuf);

	size_t last_mark = 0;
	while (1) {
		strlist_resize(&inbuf, 0);
		int res = strlist_readline(&inbuf, input);

		char additional = res != -1 && inbuf.data[0] == '\t' && lines.size;

		if (!additional && lines.size) {
			int include = 1;
			if (regexes_type.size && regexes_type.data[regexes_type.size - 1]) {
				include = 0;
			}

			regex_t *regexes_data = regexes.data;
			for (size_t i = 0; i < regexes.size; i++) {
				int match = 0;
				for (size_t j = last_mark; j < lines.size; j++) {
					if (!regexec_safe(regexes_data + i, lines.data[j], 0, NULL, 0)) {
						match = 1;
						break;
					}
				}
				if (match) {
					include = regexes_type.data[i];
					break;
				}
			}

			if (!include) {
				ptrlist_resize(&lines, last_mark);
				strlist_resize(&collapsed, last_mark);
			}

			last_mark = lines.size;
		}

		if (res == -1) {
			break;
		}

		size_t newsize = 0;
		for (size_t i = 0; i < inbuf.size; i++) {
			char c = inbuf.data[i];
			if (c && c != '\n') {
				inbuf.data[newsize++] = c;
			}
		}
		strlist_resize(&inbuf, newsize);

		ptrlist_add(&lines, memdup_safe(inbuf.data, inbuf.size + 1));
		strlist_add(&collapsed, additional);
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}
	if (fclose_trace(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	strlist_destroy(&inbuf);

	if (!ret) {
		if (isatty_safe(fileno_safe(stdout))) {
			FILE *tty = fopen_trace("/dev/tty", "r+");
			if (!tty) {
				fprintf(stderr, "Error opening TTY: %s\n", strerror(errno));
				return 2;
			}

			run_gui(tty);

			if (fclose_trace(tty)) {
				fprintf(stderr, "Error closing TTY: %s\n", strerror(errno));
				return 2;
			}
		} else {
			for (size_t i = 0; i < lines.size; i++) {
				printf_safe("%s\n", (char*) lines.data[i]);
			}
		}
	}

	return ret;
}

int main(int argc, char **argv) {
	setlocale_safe(LC_ALL, "");

	unilist_init(&regexes, sizeof(regex_t));
	strlist_init(&regexes_type);
	ptrlist_init(&lines);
	strlist_init(&collapsed);

	int ret = run_program(argv);

	regex_t *regexes_data = regexes.data;
	for (size_t i = 0; i < regexes.size; i++) {
		regfree_trace(regexes_data + i);
	}

	unilist_destroy(&regexes);
	strlist_destroy(&regexes_type);
	ptrlist_destroy(&lines);
	strlist_destroy(&collapsed);

	return ret;
}
