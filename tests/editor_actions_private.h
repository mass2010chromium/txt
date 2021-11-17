ActionType resolve_action_stack(Buffer*);

// basic_actions.c
EditorAction* make_h_action(int control);  // Move left. (repeatable, no line wrap)
EditorAction* make_j_action(int control);  // Move down. (repeatable)
EditorAction* make_k_action(int control);  // Move up. (repeatable)
EditorAction* make_l_action(int control);  // Move right. (repeatable, no line wrap)
EditorAction* make_i_action(int control);  // Enter insert mode.
EditorAction* make_ESC_action(int control);    // Pop cmdstack, exit visual, etc etc.


// editing_actions.c
EditorAction* make_0_action(int control);  // Go to start of line.
EditorAction* make_DOLLAR_action(int control); // Go to end of line.

EditorAction* make_o_action(int control);  // Create a line below, and enter insert mode.
EditorAction* make_A_action(int control);  // Go to end of line and enter insert mode.

EditorAction* make_g_action(int control);  // "Go" command. (`gg` goes to start, `gt` / `gT` move between tabs)
EditorAction* make_G_action(int control);  // Go to end of buffer. (first col)

EditorAction* make_x_action(int control);  // Delete a character (repeatable), or delete the visual selection.
EditorAction* make_r_action(int control);  // Replace character(s) under cursor.
EditorAction* make_d_action(int control);  // 'dd' to delete line (repeatable), or delete based on the result of a move command.
EditorAction* make_D_action(int control);  // Shortcut for `d$`.


// visual_actions.c
EditorAction* make_v_action(int control);  // Enter visual select mode (char).
EditorAction* make_V_action(int control);  // Enter visual select mode (line).


EditorAction* make_u_action(int control);  // Undo an action. (repeatable)
EditorAction* make_p_action(int control);  // Paste copied/cut text. (repeatable, only partially implemented)


// search_actions.c
EditorAction* make_f_action(int control);  // Find a character forwards in line.
EditorAction* make_F_action(int control);  // Find a character backwards in line.
EditorAction* make_w_action(int control);  // Move forward by one "word", breaking on punctuation and space. (repeatable)
EditorAction* make_W_action(int control);  // Move forward by one "word", breaking on punctuation. (repeatable)

EditorAction* make_slash_action(int control);  // Search for a word across lines, forward.
EditorAction* make_question_action(int control);   // Search for a word across lines, backward.
EditorAction* make_n_action(int control);  // Repeat previous word search.
EditorAction* make_N_action(int control);  // Repeat previous word search, in the reverse direction.

EditorAction* make_m_action(int control);  // Set mark.
EditorAction* make_BACKTICK_action(int control);   // Go to mark.


// command_action.c
EditorAction* make_COLON_action(int control);  // command mode.

EditorAction* make_q_action(int control);  // Record macro.
EditorAction* make_AT_action(int control); // Execute macro.
