#pragma once
#include <sys/ioctl.h>

#include "../structures/buffer.h"
#include "utils.h"
#include "../common.h"

#define BYTE_CTRLC      '\003'      // end of text
#define BYTE_CTRLD      '\004'      // end of transmission
#define BYTE_CTRLZ      '\032'      // substitute?
#define BYTE_ESC        '\033'
//#define BYTE_ENTER      '\015'      // Carriage Return
#define BYTE_TAB        '\011'      // Tab
#define BYTE_ENTER      '\012'      // Line Feed
#define BYTE_BACKSPACE  '\177'
#define BYTE_UPARROW    '\110'
#define BYTE_LEFTARROW  '\113'
#define BYTE_RIGHTARROW '\115'
#define BYTE_DOWNARROW  '\120'
#define BYTE_DELETE     '\123'

#define CODE_DELETE     2326        // delete, not a char anymore lol

extern String* bottom_bar_info;

extern EditorMode current_mode;

extern struct winsize window_size;

extern Copy active_copy;

extern Buffer* current_buffer;
extern int current_buffer_idx;
extern Vector/*Buffer* */ buffers;
extern String* command_buffer;
extern GapBuffer active_insert;
extern size_t editor_top;
extern size_t editor_left;
extern size_t editor_bottom;
extern size_t editor_width;
extern bool editor_display;
extern bool editor_macro_mode;
extern int TAB_WIDTH;
extern bool PRESERVE_INDENT;
extern bool EXPAND_TAB;

/**
 * Process one keypress from the user.
 * (Should also work for simulated keypress, like from norm)
 */
void process_input(char input, int control);

/**
 * Callback function that gets called whenever the terminal size changes.
 * Updates the window_size variable to reflect changes made to the window size,
 * then also updates the editor_bottom variable accordingly.
 */
void editor_window_size_change();

/**
 * Start a new editor action for undo purposes.
 * Increments the undo counter.
 */
void editor_new_action();

void clear_line();      /** Clear a line of STDOUT to prep for displaying the editor. */
void clear_screen();    /** Prints an escape code that clears the entire terminal screen. */

/** DO NOT USE Gets the zero-indexed coords of the mouse cursor in the terminal. */
int get_cursor_pos(size_t *y, size_t *x);

void move_to_current();     /** Calls move_cursor with the current cursor position. */
void move_cursor(size_t y, size_t x);   /** Prints an escape code to move the location of the cursor to the given coordinates. */

/**
 * Suppose I am at position <start>, and I press tab.
 * Where should I end up?
 */
size_t tab_round_up(size_t start);

/**
 * Pointer corresponding to the spot in buf at screen pos x. (0 indexed)
 */
char* line_pos(char* buf, ssize_t x);

/**
 * Compute the 'screen length' of a buffer. Accounts for tabs.
 * TODO: unify this with `format_respect_tabspace`.
 */
size_t strlen_tab(const char* buf);

/**
 * Prepare a buffer to write out to the screen, respecting tab width.
 * Replaces tabs with spaces; Drops newlines.
 *
 * Parameters:
 *      write_buffer: String to push into to return.
 *      buf: data to write out. (C string, no control chars)
 *      start: Screenwidth position to start at.
 *      count: Number of characters in 'buf' to write out.
 * 
 * Return:
 *      New "effective screen position" (for tabbing purpoises)
 */
size_t format_respect_tabspace(String** write_buffer, const char* buf, size_t start, size_t count,
                                size_t print_start, size_t print_end);

/**
 * Creates a buffer for the given file and inserts it in the buffer vector at the given index.
 */
void editor_make_buffer(const char* filename, size_t index);

/**
 * Switch to an open buffer by index.
 */
void editor_switch_buffer(size_t n);

/**
 * Performs initial setup for the editor, including making the vector of buffers,
 * creating and pushing a buffer for the given filename to that vector,
 * and configuring the size of the editor window. Defaults to `Normal` editing mode.
 */
void editor_init(const char* filename);

/**
 * Destroys the buffer at position `idx` in the vector of buffers and updates
 * the screen to display the previous buffer in the vector, if there is one.
 */
void editor_close_buffer(int idx);

int editor_get_buffer_idx();

/**
 * Returns a pointer to the line at position y in the current buffer.
 */
String** get_line_in_buffer(size_t y);

/**
 * Creates an Edit object to prepare for changes made to the current row.
 */
void begin_insert();

/**
 * Replaces the old content of the current line with the edited content.
 * Also pushes the Edit action representing the insert action onto the undo stack.
 */ 
void end_insert();

/**
 * Deletes a character at the current cursor position.
 * Returns 1 if a character was successfully deleted, 0 otherwise.
 */
int editor_backspace();

/**
 * Writes the passed character to the file at the current cursor position.
 */
void add_chr(char c);

/**
 * Adds a line below the current line and begins insert mode.
 */
void editor_newline(int, const char* head, const char* initial);

void display_bottom_bar(char* left, char* right);

void display_top_bar();

/**
 * Display rows [start, end] inclusive, in screen coords (1-indexed).
 */
char* display_buffer_rows(ssize_t start, ssize_t end);

/**
 * Displays all rows in the current buffer by calling display_buffer_rows
 * for the entire file length.
 */
void display_current_buffer();

/**
 * Decrements the cursor column until it is aligned with the start of a tab (width 3).
 */
void left_align_tab(char* line);

/**
 * Increments the cursor column until it is aligned with the end of a tab (width 3).
 */
void right_align_tab(char* line);

void editor_move_up();
void editor_move_down();

/**
 * Only works in not insert mode.
 */
void editor_move_EOL();
void editor_move_left();
void editor_move_right();
RepaintType editor_fix_view();
void editor_align_tab();

/**
 * Moves the editor view to (row, col) in the file. 0-indexed.
 * sharp: whether to update natural col or not.
 */
RepaintType editor_move_to(ssize_t row, ssize_t col, bool sharp);

/**
 * Repaints part of the editor, depending on the action type
 * and position information contained in `ctx`.
 * NOTE: MUST TAKE A NORMALIZED CONTEXT!
 */
void editor_repaint(RepaintType repaint, EditorContext* ctx);
