// Includes

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// Defines

#define KILO_VERSION "0.0.1"
#define KILO_TAB_STOP 8
#define KILO_QUIT_TIMES 3

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN,
};

// Data

// Editor row of text
typedef struct erow {
    int size;
    int rsize;
    char* chars;
    char* render;
} erow;

// A struct for holding the original termios configuration
// so it can easily be restored on exit
struct editorConfig {
    int cx, cy; // cursor position
    int rx; // index into renders
    int rowoff; // row offset
    int coloff; // column offset

    int screenrows;
    int screencols;

    int numrows;
    erow* row;
    int dirty; // flag indicating whether or not text has changed after loading

    char* filename;

    char statusmsg[80];
    time_t statusmsg_time;

    struct termios orig_termios;
};

struct editorConfig E;

// Prototypes

void editorSetStatusMessage(const char* fmt, ...);
void editorRefreshScreen();
char* editorPrompt(char* prompt);

// Terminal

void die(const char* s) {
    // Clear the screen
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
    // Store the original config in orig_termios
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
        die("tcgetattr");
    }
    // Restore the original termios at exit
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    // ICRNL: Disable C-m (translating CR to newline)
    // IXON: Disable C-s, C-q
    raw.c_iflag &= ~(ICRNL | IXON);

    // OPOST: Disable output post-processing (NL -> CRNL)
    raw.c_oflag &= ~(OPOST);

    // ECHO: Disable echoing
    // ICANON: Disable canonical mode (read byte-by-byte)
    // IEXTEN: Disable C-v
    // ISIG: Disable C-c, C-z
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISTRIP | ISIG);

    raw.c_cflag |= (CS8);

    // Prevent read from blocking indefinitely
    // Minimum number of bytes that must be read
    raw.c_cc[VMIN] = 0;
    // Maximum time for read to block (1/10 second)
    raw.c_cc[VTIME] = 1;

    // Enable raw mode
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}

// Wait for one keypress, and return it
int editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }

    // If we read an escape key, we read two more characters
    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }

        // The arrow keys are <esc>(A|B|C|D)
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return '\x1b';
                }

                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }

            else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int* rows, int* cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') {
        return -1;
    }
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return 1;
    }

    return 0;
}

// Set the window size by dereferencing row and col pointers
// Also, nasty hacks in case ioctl fails
int getWindowSize(int* rows, int* cols) {
    struct winsize ws;

    // If ioctl fails, fallback to positioning the cursor and getting its position
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

// Row Operations

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row->chars[j] == '\t') {
            rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
        }
        rx++;
    }
    return rx;
}

void editorUpdateRow(erow* row){
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % KILO_TAB_STOP != 0) {
                row->render[idx++] = ' ';
            }
        } else {
            row->render[idx++] = row->chars[j];
        }
    }

    row->render[idx] = '\0';
    row->rsize = idx;
}

void editorInsertRow(int at, char* s, size_t len) {
    // Validate at
    if (at < 0 || at > E.numrows) {
        return;
    }

    // Allocate memory for another row
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    // Make room at the specified index
    memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rsize = 0;
    E.row[at].render = NULL;
    editorUpdateRow(&E.row[at]);

    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow* row) {
    free(row->render);
    free(row->chars);
}

void editorDelRow(int at) {
    if (at < 0 || at >= E.numrows) {
        return;
    }

    editorFreeRow(&E.row[at]);
    // Overwrite the deleted row with those that come after it
    memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
    E.numrows--;
    E.dirty++;
}

void editorRowInsertChar(erow* row, int at, int c) {
    // Validate the cursor position
    if (at < 0 || at > row->size) {
        at = row->size;
    }
    // Make room for an additional charater plus the null byte
    row->chars = realloc(row->chars, row->size + 2);
    // Make room for the new character
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row);

    E.dirty++;
}

void editorRowAppendString(erow* row, char* s, size_t len) {
    // Allocate enough space and then copy s into place, adding the null terminator
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';

    editorUpdateRow(row);
    E.dirty++;
}

void editorRowDelChar(erow* row, int at) {
    // Check the cursove position
    if (at < 0 || at >= row->size) {
        return;
    }

    memmove(&row->chars[at], &row->chars[at+1], row->size - at);
    row->size--;
    editorUpdateRow(row);
    E.dirty++;
}

// Editor Operations

void editorInsertChar(int c) {
    // If on tilde after last row, append a line first
    if (E.cy == E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    }

    editorRowInsertChar(&E.row[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline() {
    // If we're at the beginning of the line, just a blank row gets inserted before
    if (E.cx == 0) {
        editorInsertRow(E.cy, "", 0);
    } else {
        // Get a reference to teh current row
        erow *row = &E.row[E.cy];
        // Insert a row, grabbing text starting at the current cursor position
        editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        // Update the size of the truncated row, and set the null terminator
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';

        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar() {
    // If the editor is past the end of the file, there is nothing to delete
    if (E.cy == E.numrows) {
        return;
    }
    // Same if we're at the start of the file
    if (E.cx == 0 && E.cy == 0) {
        return;
    }

    // Delete if there's a character to the left of the cursor
    erow* row = &E.row[E.cy];
    if (E.cx > 0) {
        editorRowDelChar(row, E.cx - 1);
        E.cx--;
    } else {
        // Move to the end of the previous line and append the string
        E.cx = E.row[E.cy - 1].size;
        editorRowAppendString(&E.row[E.cy-1], row->chars, row->size);
        editorDelRow(E.cy);
        E.cy--;
    }
}

// File io

char* editorRowsToString(int *buflen) {
    // Determine how long the full string will be
    int totlen = 0;
    int j;
    for (j = 0; j < E.numrows; j++) {
        // Add one for newline
        totlen += E.row[j].size + 1;
    }
    *buflen = totlen;

    char* buf = malloc(totlen);
    char* p = buf;
    for (j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void editorOpen(char* filename) {
    free(E.filename);
    E.filename = strdup(filename);

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        die("fopen");
    }

    char* line = NULL;
    size_t linecap = 0;

    ssize_t linelen;
    while((linelen = getline(&line, &linecap, fp)) != -1) {
        // Get rid of trailing NL and CR, since we know it's a line
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                    line[linelen - 1] == '\r')) {
            linelen--;
        }

        // Copy from the buffer allocated by getline to the row
        editorInsertRow(E.numrows, line, linelen);
    }

    free(line);
    fclose(fp);
    E.dirty = 0;
}

// If we're set to save to a file, write the rows into a string
// and then write it to disk
void editorSave() {
    if (E.filename == NULL) {
        E.filename = editorPrompt("Save as: %s");
        if (E.filename == NULL) {
            editorSetStatusMessage("Save aborted");
            return;
        }
    }

    int len;
    char* buf = editorRowsToString(&len);

    // 0644 gives the owner write permission and everyone else read permission
    int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) != -1) {
                close(fd);
                free(buf);
                E.dirty = 0;
                editorSetStatusMessage("%d bytes written to disk", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

// Append Buffer

struct abuf {
    char* b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char* s, int len) {
    char* new = realloc(ab->b, ab->len + len);

    if (new == NULL) {
        return;
    }

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

// Output

void editorScroll() {
    E.rx = 0;
    if (E.cy < E.numrows) {
        E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.rx < E.coloff) {
        E.coloff = E.cx;
    }
    if (E.rx >= E.coloff + E.screencols) {
        E.coloff = E.rx - E.screencols + 1;
    }
}

void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                        "Kilo editor -- version %s", KILO_VERSION);
                if (welcomelen > E.screencols) {
                    welcomelen = E.screencols;
                }

                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }

                while(padding--) {
                    abAppend(ab, " ", 1);
                }

                abAppend(ab, welcome, welcomelen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            // Print the characters from the rows buffer to the screen
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) {
                len = 0;
            }
            if (len > E.screencols) {
                len = E.screencols;
            }
            abAppend(ab, &E.row[filerow].render[E.coloff], len);
        }

        // Clear the line
        abAppend(ab, "\x1b[K", 3);

        abAppend(ab, "\r\n", 2);
    }
}

void editorDrawStatusBar(struct abuf* ab) {
    // Use white background
    abAppend(ab, "\x1b[7m", 4);

    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
            E.filename ? E.filename : "[No Name]", E.numrows,
            E.dirty ? "(modified)" : "");
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
            E.cy + 1, E.numrows);
    if (len > E.screencols) {
        len = E.screencols;
    }
    abAppend(ab, status, len);
    while (len < E.screencols) {
        // If there's enough space and we're at the right position, append
        if (E.screencols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }

    // Reset background to normal
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf* ab) {
    // Clear the message bar row
    abAppend(ab, "\x1b[K", 3);

    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) {
        msglen = E.screencols;
    }
    // Display the message, if it's less than 5 seconds old
    if (msglen && time(NULL) - E.statusmsg_time < 5) {
        abAppend(ab, E.statusmsg, msglen);
    }
}

void editorRefreshScreen() {
    editorScroll();

    struct abuf ab = ABUF_INIT;

    // Hide the cursor (to avoid flashing)
    abAppend(&ab, "\x1b[?25l", 6);
    // Reposition the cursor at the top left corner
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);
    editorDrawStatusBar(&ab);
    editorDrawMessageBar(&ab);

    // Move the cursor to E.cx, E.cy
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                              (E.rx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    // Show the cursor again
    abAppend(&ab, "\x1b[?25h", 6);

    // Write the full buffer, then clear it
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorSetStatusMessage(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

// Input

char* editorPrompt(char* prompt) {
    size_t bufsize = 128;
    char* buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(prompt, buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        } else if (c == '\x1b') {
            editorSetStatusMessage("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }

            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
    }
}

void editorMoveCursor(int key) {
    erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
            }
            break;
    }

    row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

// Read a keypress and then determine the action
void editorProcessKeypress() {
    static int quit_times = KILO_QUIT_TIMES;

    int c = editorReadKey();

    switch (c) {
        case '\r':
            editorInsertNewline();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0) {
                editorSetStatusMessage("WARNING!!! File has unsaved changes. Press Ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            // Clear the screen
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);

            exit(0);
            break;

        case CTRL_KEY('s'):
            editorSave();
            break;

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            if (E.cy < E.numrows) {
                E.cx = E.row[E.cy].size;
            }
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            // Delete just simulates moving one to the right first
            if (c == DEL_KEY) {
                editorMoveCursor(ARROW_RIGHT);
            }
            editorDelChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E.cy = E.rowoff;
                } else if (c == PAGE_DOWN) {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.numrows) {
                        E.cy = E.numrows;
                    }
                }
                int times = E.screenrows;
                while(times--) {
                    editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
                }
            }
            break;

        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_UP:
        case ARROW_DOWN:
            editorMoveCursor(c);
            break;

        // Don't insert keypresses on escape and C-l
        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            editorInsertChar(c);
            break;
    }

    quit_times = KILO_QUIT_TIMES;
}

// Init

void initEditor() {
    // Initially position the cursor at the top-left
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }

    // Make room for a status bar
    E.screenrows -= 2;
}

int main(int argc, char* argv[]) {
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        editorOpen(argv[1]);
    }

    editorSetStatusMessage("HELP: Ctrl-Q = quit | Ctrl-Q = quit");

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}