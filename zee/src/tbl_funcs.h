/*  Table of Zee commands */

/*
 * Add an entry to this list for declaring a new function.
 * X0 means no key binding, X1 means one key binding, etc.
 *
 * Please remember to keep in sync with the Texinfo documentation
 * `../doc/zee.texi'.
 */

X0("auto-fill-mode", auto_fill_mode)
X1("back-to-indentation", back_to_indentation,		"\\M-m")
X2("backward-char", backward_char,			"\\C-b", "\\LEFT")
X1("backward-delete-char", backward_delete_char,	"\\BS")
X1("backward-kill-word", backward_kill_word,		"\\M-\\BS")
X1("backward-paragraph", backward_paragraph,		"\\M-{")
X1("backward-sexp", backward_sexp,			"\\C-\\M-b")
X1("backward-word", backward_word,			"\\M-b")
X1("beginning-of-buffer", beginning_of_buffer,		"\\M-<")
X2("beginning-of-line", beginning_of_line,		"\\C-a", "\\HOME")
X1("call-last-kbd-macro", call_last_kbd_macro,		"\\M-e")
X1("capitalize-word", capitalize_word,			"\\M-c")
X0("cd", cd)
X1("copy-region-as-kill", copy_region_as_kill,		"\\M-w")
X2("delete-char", delete_char,				"\\C-d", "\\DEL")
X1("delete-horizontal-space", delete_horizontal_space,	"\\M-\\\\")
X1("delete-other-windows", delete_other_windows,	"\\M-1")
X1("just-one-space", just_one_space,			"\\M- ")
X1("delete-window", delete_window,			"\\M-0")
X0("describe-function", describe_function)
X0("describe-key", describe_key)
X0("describe-variable", describe_variable)
X1("downcase-word", downcase_word,			"\\M-l")
X1("end-kbd-macro", end_kbd_macro,			"\\M-)")
X1("end-of-buffer", end_of_buffer,			"\\M->")
X2("end-of-line", end_of_line,				"\\C-e", "\\END")
X1("eval-expression", eval_expression,			"\\M-:")
X0("eval-last-sexp", eval_last_sexp)
X0("exchange-point-and-mark", exchange_point_and_mark)
X1("execute-extended-command", execute_extended_command,"\\M-x")
X1("fill-paragraph", fill_paragraph,			"\\M-q")
X1("find-file", find_file,				"\\M-o")
X2("forward-char", forward_char,			"\\C-f", "\\RIGHT")
X0("forward-line", forward_line)
X1("forward-paragraph", forward_paragraph,		"\\M-}")
X1("forward-sexp", forward_sexp,			"\\C-\\M-f")
X1("forward-word", forward_word,			"\\M-f")
X0("global-set-key", global_set_key)
X1("goto-char", goto_char,				"\\C-\\M-g")
X1("goto-line", goto_line,				"\\M-g")
X1("indent-command", indent_command,			"\\TAB")
X0("insert-file", insert_file)
X1("isearch-backward-regexp", isearch_backward_regexp,	"\\C-r")
X1("isearch-forward-regexp", isearch_forward_regexp,	"\\C-s")
X1("keyboard-quit", keyboard_quit,			"\\C-g")
X0("kill-buffer", kill_buffer)
X0("kill-line", kill_line)
X1("kill-region", kill_region,				"\\C-k")
X1("kill-sexp", kill_sexp,				"\\C-\\M-k")
X2("kill-word", kill_word,				"\\M-d", "\\M-\\DEL")
X0("list-bindings", list_bindings)
X0("list-buffers", list_buffers)
X0("mark-whole-buffer", mark_whole_buffer)
X1("mark-paragraph", mark_paragraph,			"\\M-h")
X1("mark-sexp", mark_sexp,				"\\C-\\M-@")
X1("mark-word", mark_word,				"\\M-@")
X0("name-last-kbd-macro", name_last_kbd_macro)
X1("newline", newline,					"\\RET")
X1("newline-and-indent", newline_and_indent,		"\\C-j")
X2("next-line", next_line,				"\\C-n", "\\DOWN")
X1("open-line", open_line,				"\\C-o")
X1("other-window", other_window,			"\\C-\\M-o")
X2("previous-line", previous_line,			"\\C-p", "\\UP")
X1("query-replace-regexp", query_replace_regexp,	"\\M-%")
X1("quoted-insert", quoted_insert,			"\\C-q")
X1("recenter", recenter,				"\\C-l")
X0("replace-regexp", replace_regexp)
X1("save-buffer", save_buffer,				"\\M-s")
X1("save-buffers-kill-zee", save_buffers_kill_zee,	"\\C-\\M-q")
X0("save-some-buffers", save_some_buffers)
X2("scroll-down", scroll_down,				"\\M-v", "\\PGUP")
X2("scroll-up", scroll_up,				"\\C-v", "\\PGDN")
X0("search-backward-regexp", search_backward_regexp)
X0("search-forward-regexp", search_forward_regexp)
X0("self-insert-command", self_insert_command)
X0("set-fill-column", set_fill_column)
X1("set-mark-command", set_mark_command,		"\\C-@")
X0("set-variable", set_variable)
X1("shell-command", shell_command,			"\\M-!")
X1("shell-command-on-region", shell_command_on_region,	"\\M-|")
X1("split-window", split_window,			"\\M-2")
X1("start-kbd-macro", start_kbd_macro,			"\\M-(")
X1("suspend-zee", suspend_zee,				"\\C-z")
X1("switch-to-buffer", switch_to_buffer,		"\\C-\\M-x")
X1("tab-to-tab-stop", tab_to_tab_stop,			"\\M-i")
X0("toggle-read-only", toggle_read_only)
X1("undo", undo,					"\\C-_")
X1("universal-argument", universal_argument,		"\\C-u")
X1("upcase-word", upcase_word,				"\\M-u")
X0("where-is", where_is)
X0("write-file", write_file)
X1("yank", yank,					"\\C-y")
X0("zee-version", zee_version)
