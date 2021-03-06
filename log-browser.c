#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <ncurses.h>
#include "utils/stdlib_utils.h"
#include "utils/stdio_utils.h"
#include "utils/string_utils.h"
#include "utils/alist.h"

size_t insize;
char *inbuf;

struct alist lines;
struct alist collapsed;

size_t screen_pos;
size_t cursor_pos;

int rows;
int cols;

static void refresh_screen() {
	getmaxyx(stdscr, rows, cols);

	int cursor_to = 0;
	int pos = 0;
	for (size_t i = screen_pos; i < lines.size; i++) {
		if (!collapsed.cdata[i]) {
			move(pos, 0);
			clrtoeol();
			addstr(lines.cptrdata[i]);

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
		if (!collapsed.cdata[pos]) {
			break;
		}
	}
	cursor_pos = pos;
}

static void cursor_down() {
	size_t pos = cursor_pos;
	while (pos < lines.size - 1) {
		pos++;
		if (!collapsed.cdata[pos]) {
			cursor_pos = pos;
			break;
		}
	}
}

static void refresh_screen_pos() {
	if (cursor_pos < screen_pos) {
		screen_pos = cursor_pos;
	} else if (cursor_pos > screen_pos) {
		getmaxyx(stdscr, rows, cols);

		size_t cnt = 0;
		size_t pos = cursor_pos;
		while (pos > screen_pos && cnt < rows - 1) {
			pos--;
			if (!collapsed.cdata[pos]) {
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
	while (pos > 0 && lines.cptrdata[pos][0] == '\t') {
		pos--;
	}

	if (collapse) {
		cursor_pos = pos;
	}

	pos++;

	while (pos < lines.size && lines.cptrdata[pos][0] == '\t') {
		collapsed.cdata[pos++] = collapse;
	}

	if (!collapse) {
		cursor_pos = pos - 1;
	}
}

static void run_gui(FILE *tty) {
	newterm(NULL, tty, tty);
	noecho();

	refresh_screen();

	int arrow_status = 0;
	while (1) {
		int arrow_status_new = 0;

		int key = getch();
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
			getmaxyx(stdscr, rows, cols);
			for (int i = 0; i < rows; i++) {
				cursor_up();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == '6' && arrow_status == 2) {
			getmaxyx(stdscr, rows, cols);
			for (int i = 0; i < rows; i++) {
				cursor_down();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 21) {
			getmaxyx(stdscr, rows, cols);
			for (int i = 0; i < rows; i += 2) {
				cursor_up();
			}
			refresh_screen_pos();
			refresh_screen();
		} else if (key == 4) {
			getmaxyx(stdscr, rows, cols);
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
}

static int run_program(char **argv) {
	char *pname = *argv++;
	if (!*argv && isatty(fileno(stdin))) {
		printf_safe("Usage: %s <log file>\n", pname);
		return 1;
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
		input = fopen(filename, "r");
		if (!input) {
			fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
			return 2;
		}
	} else {
		input = stdin;
		filename = "stdin";
	}

	int ret = 0;

	while (getline_no_eol(&inbuf, &insize, input) != -1) {
		alist_add_ptr(&lines, strdup_safe(inbuf));
		alist_add_c(&collapsed, inbuf[0] == '\t' && collapsed.size != 0);
	}
	if (!feof(input)) {
		fprintf(stderr, "Error reading file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}
	if (fclose(input)) {
		fprintf(stderr, "Error closing file %s: %s\n", filename, strerror(errno));
		ret = 2;
	}

	if (!ret) {
		FILE *tty = fopen("/dev/tty", "r+");
		if (!tty) {
			fprintf(stderr, "Error opening TTY: %s\n", strerror(errno));
			return 2;
		}

		run_gui(tty);

		if (fclose(tty)) {
			fprintf(stderr, "Error closing TTY: %s\n", strerror(errno));
			return 2;
		}
	}

	return ret;
}

int main(int argc, char **argv) {
	setlocale(LC_ALL, "");

	alist_init_ptr(&lines);
	alist_init_c(&collapsed);

	int ret = run_program(argv);

	alist_destroy_ptr(&lines);
	alist_destroy_c(&collapsed);

	return ret;
}
