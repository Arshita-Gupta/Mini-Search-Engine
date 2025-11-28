#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "search_engine.h"
#include "inverted.h"

#define SR_FILENAME(r) ((r)->filename)
#define SR_SCORE(r)    ((r)->score)

// ----------------- Application state -----------------
typedef struct {
    GtkWidget    *window;
    GtkWidget    *search_entry;
    GtkWidget    *search_button;
    GtkWidget    *index_button;
    GtkWidget    *stats_button;
    GtkWidget    *spinner;
    GtkWidget    *info_label;   
    GtkListStore *store;        
    SearchEngine *engine;
} App;

enum {
    COL_FILE = 0,
    COL_SCORE,
    COL_PREVIEW,
    N_COLS
};

// ----------------- Utilities -----------------
static void trace_status(const char *msg) {
    g_print("[STATUS] %s\n", msg ? msg : "");
}

static void set_info_text(App *app, const char *text) {
    gtk_label_set_text(GTK_LABEL(app->info_label), text ? text : "");
}

// Read first bytes as preview, collapse whitespace
static char *read_preview(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    const size_t MAX = 220;
    char *buf = g_malloc(MAX + 1);
    size_t n = fread(buf, 1, MAX, f);
    fclose(f);

    if (n == 0) {
        g_free(buf);
        return NULL;
    }
    buf[n] = '\0';

    for (size_t i = 0; i < n; ++i) {
        if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\t')
            buf[i] = ' ';
    }
    return buf;
}

static char *make_absolute(const char *p) {
    if (!p) return NULL;
    char *abs = g_canonicalize_filename(p, NULL);
    if (!abs) abs = g_strdup(p);
    return abs;
}

// Try to open file with system default; returns FALSE on error
static gboolean open_with_default(GtkWindow *parent, const char *path) {
    GError *err = NULL;
    char *abs = make_absolute(path);
    if (!abs) return FALSE;
    if (!g_file_test(abs, G_FILE_TEST_EXISTS)) {
        GtkWidget *d = gtk_message_dialog_new(parent,
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR, 
                        GTK_BUTTONS_CLOSE, "File not found:\n%s", abs);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(abs);
        return FALSE;
    }

    char *uri = g_filename_to_uri(abs, NULL, &err);
    if (!uri) {
        GtkWidget *d = gtk_message_dialog_new(parent, 
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR, 
                        GTK_BUTTONS_CLOSE,"Failed to form URI:\n%s",err ? err->message : "Unknown");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
        g_free(abs);
        return FALSE;
    }

    gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, &err);
    if (err) {
        GtkWidget *d = gtk_message_dialog_new(parent, 
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_ERROR, 
                        GTK_BUTTONS_CLOSE,"Failed to open:\n%s", err->message);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_error_free(err);
        g_free(uri);
        g_free(abs);
        return FALSE;
    }
    g_free(uri);
    g_free(abs);
    return TRUE;
}

// Reveal containing folder (best-effort)
static void reveal_containing_folder(GtkWindow *parent, const char *path) {
    char *abs = make_absolute(path);
    if (!abs) return;
    char *dir = g_path_get_dirname(abs);
    if (dir) {
        GError *err = NULL;
        char *uri = g_filename_to_uri(dir, NULL, &err);
        if (uri && !err) {
            gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, &err);
            g_free(uri);
        } else {
            GtkWidget *d = gtk_message_dialog_new(parent, 
                            GTK_DIALOG_MODAL,
                            GTK_MESSAGE_ERROR, 
                            GTK_BUTTONS_CLOSE,"Cannot open folder:\n%s",err ? err->message : dir);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            if (err) g_error_free(err);
        }
        g_free(dir);
    }
    g_free(abs);
}

// ----------------- Model handling -----------------
static void reset_results_model(App *app) {
    GtkWidget *view = g_object_get_data(G_OBJECT(app->window), "results_view");
    if (!view || !GTK_IS_TREE_VIEW(view)) return;
    if (app->store) {
        g_object_unref(app->store);
        app->store = NULL;
    }
    GtkListStore *store = gtk_list_store_new(N_COLS, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
    app->store = store;
}

static int populate_results(App *app, SearchResult *head) {
    reset_results_model(app);
    GtkTreeIter it;
    int count = 0;
    for (SearchResult *r = head; r; r = r->next) {
        const char *file = SR_FILENAME(r) ? SR_FILENAME(r) : "(unknown)";
        int score = (int)SR_SCORE(r);
        char *preview = read_preview(file);
        gtk_list_store_append(app->store, &it);
        gtk_list_store_set(app->store, &it, 
                        COL_FILE,    file, 
                        COL_SCORE,   score, 
                        COL_PREVIEW, preview ? preview : "",-1);
        if (preview) g_free(preview);
        count++;
    }
    return count;
}

// ----------------- In-app preview dialog -----------------
static void show_preview_dialog(App *app, const char *path, const char *query) {
    char *abs = make_absolute(path);
    if (!abs) return;
    if (!g_file_test(abs, G_FILE_TEST_EXISTS)) {
        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                                GTK_DIALOG_MODAL,
                                GTK_MESSAGE_ERROR,
                                GTK_BUTTONS_OK,"File not found:\n%s", abs);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(abs);
        return;
    }
    GError *err = NULL;
    gchar *contents = NULL;
    gsize len = 0;
    if (!g_file_get_contents(abs, &contents, &len, &err)) {
        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                            GTK_DIALOG_MODAL,
                            GTK_MESSAGE_ERROR,
                            GTK_BUTTONS_OK,"Cannot read file: %s",err ? err->message : "(unknown)");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        if (err) g_error_free(err);
        g_free(abs);
        return;
    }
    GtkWidget *dlg = gtk_dialog_new_with_buttons(path,
                    GTK_WINDOW(app->window),
                    GTK_DIALOG_MODAL,"_Close",
                    GTK_RESPONSE_CLOSE, NULL);
    GtkWidget *area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget *sc   = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(sc, 800, 600);
    gtk_container_add(GTK_CONTAINER(area), sc);

    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(sc), tv);

    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
    gtk_text_buffer_set_text(buf, contents, (gint)len);

    // Simple query highlight
    if (query && *query) {
        GtkTextIter start, match_start, match_end;
        gtk_text_buffer_get_start_iter(buf, &start);

        while (gtk_text_iter_forward_search(&start, query, 
                            GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_CASE_INSENSITIVE,
                            &match_start, &match_end, NULL)) {
            GtkTextTagTable *tt = gtk_text_buffer_get_tag_table(buf);
            GtkTextTag *hl = gtk_text_tag_table_lookup(tt, "highlight");
            if (!hl) {
                hl = gtk_text_tag_new("highlight");
                g_object_set(hl, "background", "yellow", NULL);
                gtk_text_tag_table_add(tt, hl);
            }
            gtk_text_buffer_apply_tag(buf, hl, &match_start, &match_end);
            start = match_end;
        }
    }
    gtk_widget_show_all(dlg);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    g_free(contents);
    g_free(abs);
}

// ----------------- Search execution -----------------
static void run_search(App *app) {
    const char *q = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    reset_results_model(app);
    if (!q || q[0] == '\0') {
        trace_status("Empty query.");
        set_info_text(app, "Type a word or phrase above and press Search.");
        return;
    }
    gtk_spinner_start(GTK_SPINNER(app->spinner));
    trace_status("Searching…");
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    SearchResult *res = performSearch(app->engine, q);
    clock_gettime(CLOCK_MONOTONIC, &b);
    double ms = (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
    int n = 0;
    if (res) {
        n = populate_results(app, res);
    }
    gtk_spinner_stop(GTK_SPINNER(app->spinner));
    char msg[256];
    if (n == 0) {
        snprintf(msg, sizeof(msg), "No results found for '%s' (%.0f ms)", q, ms);
        set_info_text(app, "No matches. Try a different keyword.");
    } else {
        snprintf(msg, sizeof(msg), "Found %d result(s) for '%s' in %.0f ms", n, q, ms);
        set_info_text(app, "Tip: Double-click a row to preview the file.");
    }
    trace_status(msg);

    GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW(app->window), 
                    GTK_DIALOG_MODAL, 
                    GTK_MESSAGE_INFO, 
                    GTK_BUTTONS_OK, "%s", msg);
    gtk_window_set_title(GTK_WINDOW(dialog), "Search Results");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (res)
        freeSearchResults(res);
}

// ----------------- Callbacks -----------------
static void on_search_activate(GtkEntry *entry, gpointer ud) {
    (void)entry;
    run_search((App *)ud);
}

static void on_search_button(GtkButton *b, gpointer ud) {
    (void)b;
    run_search((App *)ud);
}

static void on_index_files(GtkButton *b, gpointer ud) {
    (void)b;
    App *app = (App *)ud;

    // Build search pattern: files\*.txt
    char searchPattern[MAX_PATH];
    snprintf(searchPattern, sizeof(searchPattern), "files\\*.txt");

    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA(searchPattern, &data);

    if (hFind == INVALID_HANDLE_VALUE) {
        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), 
                    GTK_DIALOG_MODAL, 
                    GTK_MESSAGE_ERROR, 
                    GTK_BUTTONS_OK, "Could not find any .txt files in 'files' folder.\n" "Make sure the folder exists next to the executable.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    char **arr = NULL;
    int count = 0;

    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        arr = realloc(arr, sizeof(char*) * (count + 1));
        char fullpath[MAX_PATH];
        snprintf(fullpath, sizeof(fullpath), "files\\%s", data.cFileName);
        arr[count] = _strdup(fullpath);
        count++;
    } while (FindNextFileA(hFind, &data));

    FindClose(hFind);

    if (count == 0) {
        GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_WARNING,
                        GTK_BUTTONS_OK, "No .txt files found in 'files' folder.");
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    gtk_spinner_start(GTK_SPINNER(app->spinner));
    indexFiles(app->engine, arr, count);
    gtk_spinner_stop(GTK_SPINNER(app->spinner));
    for (int i = 0; i < count; ++i)
        free(arr[i]);
    free(arr);
    char msg[150];
    snprintf(msg, sizeof(msg),"%d file(s) automatically indexed from 'files/'", count);
    trace_status(msg);
    set_info_text(app, msg);

    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                    GTK_DIALOG_MODAL,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,"%s", msg);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static void on_stats(GtkButton *b, gpointer ud) {
    (void)b;
    App *app = (App *)ud;
    printEngineStats(app->engine);
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app->window), 
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_INFO, 
                GTK_BUTTONS_OK, "Engine statistics printed to the terminal.");
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

// Context menu actions
static void on_context_open(GtkMenuItem *mi, gpointer ud) {
    (void)mi;
    App *app = (App *)ud;
    GtkTreeView *view = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(app->window), "results_view"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection(view);
    GtkTreeModel *m;
    GtkTreeIter it;
    if (gtk_tree_selection_get_selected(sel, &m, &it)) {
        char *path = NULL;
        gtk_tree_model_get(m, &it, COL_FILE, &path, -1);
        if (path)
            open_with_default(GTK_WINDOW(app->window), path);
        g_free(path);
    }
}

static void on_context_reveal(GtkMenuItem *mi, gpointer ud) {
    (void)mi;
    App *app = (App *)ud;
    GtkTreeView *view = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(app->window), "results_view"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection(view);
    GtkTreeModel *m;
    GtkTreeIter it;
    if (gtk_tree_selection_get_selected(sel, &m, &it)) {
        char *path = NULL;
        gtk_tree_model_get(m, &it, COL_FILE, &path, -1);
        if (path)
            reveal_containing_folder(GTK_WINDOW(app->window), path);
        g_free(path);
    }
}

static void on_context_copy(GtkMenuItem *mi, gpointer ud) {
    (void)mi;
    App *app = (App *)ud;
    GtkTreeView *view = GTK_TREE_VIEW(g_object_get_data(G_OBJECT(app->window), "results_view"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection(view);
    GtkTreeModel *m;
    GtkTreeIter it;
    if (gtk_tree_selection_get_selected(sel, &m, &it)) {
        char *path = NULL;
        gtk_tree_model_get(m, &it, COL_FILE, &path, -1);
        if (path) {
            GtkClipboard *clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gtk_clipboard_set_text(clip, path, -1);
        }
        g_free(path);
    }
}

// Row activated (double-click or Enter) -> preview dialog
static gboolean on_row_activated_cb(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer ud) {
    (void)col;
    App *app = (App *)ud;
    GtkTreeModel *m = gtk_tree_view_get_model(tv);
    GtkTreeIter it;
    if (gtk_tree_model_get_iter(m, &it, path)) {
        char *file = NULL;
        gtk_tree_model_get(m, &it, COL_FILE, &file, -1);
        const char *q =
            gtk_entry_get_text(GTK_ENTRY(app->search_entry));
        if (file)
            show_preview_dialog(app, file, q);
        g_free(file);
    }
    return TRUE;
}

// Build a context menu and show it at pointer
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer ud) {
    (void)widget;
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        App *app = (App *)ud;
        GtkWidget *menu = gtk_menu_new();
        GtkWidget *mi_open   = gtk_menu_item_new_with_label("Open");
        GtkWidget *mi_reveal = gtk_menu_item_new_with_label("Open Folder");
        GtkWidget *mi_copy   = gtk_menu_item_new_with_label("Copy Path");

        g_signal_connect(mi_open,   "activate", G_CALLBACK(on_context_open),   app);
        g_signal_connect(mi_reveal, "activate", G_CALLBACK(on_context_reveal), app);
        g_signal_connect(mi_copy,   "activate", G_CALLBACK(on_context_copy),   app);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_open);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_reveal);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_copy);
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
        return TRUE;
    }
    return FALSE;
}

// ----------------- UI construction -----------------
static void apply_css(void) {
    const char *css =
        "headerbar {"
        "  padding: 8px;"
        "  background-image: linear-gradient(to right, #4e73df, #1cc88a);"
        "  color: white;"
        "}"
        "headerbar .title, headerbar .subtitle { color: white; }"
        "entry {"
        "  padding: 8px;"
        "  border-radius: 10px;"
        "  font-size: 14px;"
        "}"
        "button {"
        "  border-radius: 10px;"
        "  padding: 6px 12px;"
        "  font-size: 14px;"
        "}"
        "button:hover {"
        "  background: #e2e6ea;"
        "}"
        "treeview {"
        "  font-family: 'Segoe UI', 'Cantarell', sans-serif;"
        "  font-size: 13px;"
        "}"
        "label.info-label {"
        "  color: #555;"
        "  font-style: italic;"
        "}";
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

static GtkWidget *build_results_view(App *app) {
    app->store = NULL;

    GtkWidget *view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(view), TRUE);

    g_signal_connect(view, "row-activated", G_CALLBACK(on_row_activated_cb), app);
    g_signal_connect(view, "button-press-event", G_CALLBACK(on_button_press), app);

    GtkCellRenderer *r1 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c1 = gtk_tree_view_column_new_with_attributes("File", r1, "text", COL_FILE, NULL);
    gtk_tree_view_column_set_expand(c1, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c1);

    GtkCellRenderer *r2 = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c2 = gtk_tree_view_column_new_with_attributes("Score", r2, "text", COL_SCORE, NULL);
    gtk_tree_view_column_set_sort_column_id(c2, COL_SCORE);
    gtk_tree_view_column_set_min_width(c2, 80);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c2);

    GtkCellRenderer *r3 = gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(r3), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    GtkTreeViewColumn *c3 = gtk_tree_view_column_new_with_attributes("Preview", r3, "text", COL_PREVIEW, NULL);
    gtk_tree_view_column_set_min_width(c3, 350);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), c3);

    g_object_set_data(G_OBJECT(app->window), "results_view", view);
    return view;
}

static void activate(GtkApplication *gapp, gpointer user_data) {
    (void)user_data;
    App *app = g_new0(App, 1);
    app->engine = createSearchEngine();

    app->window = gtk_application_window_new(gapp);
    gtk_window_set_title(GTK_WINDOW(app->window), "Mini Search Engine");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 980, 640);

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Mini Search Engine");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    // Left: Index button with icon
    app->index_button = gtk_button_new_with_label(" Index Files");
    GtkWidget *idx_icon = gtk_image_new_from_icon_name("document-open", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(app->index_button), idx_icon);
    gtk_button_set_always_show_image(GTK_BUTTON(app->index_button), TRUE);
    g_signal_connect(app->index_button, "clicked", G_CALLBACK(on_index_files), app);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), app->index_button);

    // Right: Stats button with icon
    app->stats_button = gtk_button_new_with_label(" Stats");
    GtkWidget *stats_icon = gtk_image_new_from_icon_name("system-run", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(app->stats_button), stats_icon);
    gtk_button_set_always_show_image(GTK_BUTTON(app->stats_button), TRUE);
    g_signal_connect(app->stats_button, "clicked", G_CALLBACK(on_stats), app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), app->stats_button);

    // Main layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(app->window), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);

    // Title + subtitle
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>Mini Search Engine</span>\n"
                                "<span size='large' foreground='#555555'>Search across your indexed text files instantly</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), title, FALSE, FALSE, 4);

    // Top search row
    GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), top, FALSE, FALSE, 0);

    app->search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->search_entry), "Type search query and press Enter or Search...");
    g_signal_connect(app->search_entry, "activate", G_CALLBACK(on_search_activate), app);
    gtk_box_pack_start(GTK_BOX(top), app->search_entry, TRUE, TRUE, 0);

    app->search_button = gtk_button_new_with_label(" Search");
    GtkWidget *search_icon = gtk_image_new_from_icon_name("system-search", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(app->search_button), search_icon);
    gtk_button_set_always_show_image(GTK_BUTTON(app->search_button), TRUE);
    g_signal_connect(app->search_button, "clicked", G_CALLBACK(on_search_button), app);
    gtk_box_pack_start(GTK_BOX(top), app->search_button, FALSE, FALSE, 0);

    app->spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(top), app->spinner, FALSE, FALSE, 0);

    // Results area
    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scroller, TRUE, TRUE, 0);

    GtkWidget *results = build_results_view(app);
    gtk_container_add(GTK_CONTAINER(scroller), results);

    // Bottom info label
    GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bottom, FALSE, FALSE, 0);

    GtkWidget *left_spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(bottom), left_spacer, TRUE, TRUE, 0);

    app->info_label = gtk_label_new("Ready. Click 'Index Files' to load documents.");
    gtk_widget_set_halign(app->info_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(app->info_label, FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->info_label), "info-label");
    gtk_box_pack_start(GTK_BOX(bottom), app->info_label, FALSE, FALSE, 0);

    GtkWidget *right_spacer = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(bottom), right_spacer, TRUE, TRUE, 0);

    apply_css();
    gtk_widget_show_all(app->window);

    // Keyboard accelerators (Ctrl+O for index)
    GtkAccelGroup *acc = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(app->window), acc);
    gtk_widget_add_accelerator(app->index_button, "clicked", acc, GDK_KEY_o, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_object_set_data(G_OBJECT(app->window), "app", app);
    reset_results_model(app);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.mini.search", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}