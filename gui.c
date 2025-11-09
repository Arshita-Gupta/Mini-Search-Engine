// gui.c — GTK3 GUI connected to your C backend
// Build (MSYS2 MINGW64):
//   gcc -std=c11 gui.c search_engine.c hash.c inverted.c text_processor.c \
//       -o mini_search_gui.exe `pkg-config --cflags --libs gtk+-3.0`

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "search_engine.h"   // createSearchEngine, indexFiles, performSearch, freeSearchEngine
#include "inverted.h"        // SearchResult, freeSearchResults, FileInfo

#ifndef G_N_ELEMENTS
#define G_N_ELEMENTS(arr) (sizeof(arr)/sizeof((arr)[0]))
#endif

// If AI_*.txt are in another folder, change this (remember trailing slash)
static const char *BASE_PATH = "./files/";

typedef struct {
    GtkWidget   *win;
    //GtkWidget   *btn_choose;
    GtkWidget   *btn_load_ai;
    GtkWidget   *btn_index;
    GtkWidget   *entry_query;
    GtkWidget   *btn_search;
    GtkWidget   *spinner;
    GtkWidget   *lbl_status;
    GtkListStore *store; // columns: rank, score, path
    GPtrArray   *files;  // array of char* (selected file paths)
    gboolean     indexed;
    SearchEngine *engine; // your backend
} App;

// Open a file with the OS default app (Notepad on Windows, etc.)
static void open_with_default_app(GtkWindow *parent, const char *filepath) {
    GError *err = NULL;

    // Make absolute & convert to URI for gtk_show_uri_on_window
    char *abs = g_canonicalize_filename(filepath, NULL);           // -> C:\...\AI_3.txt
    char *uri = g_filename_to_uri(abs, NULL, &err);                // -> file:///C:/.../AI_3.txt

    if (!uri) {
        gtk_label_set_text(GTK_LABEL(((App*)g_object_get_data(G_OBJECT(parent), "app"))->lbl_status),
                           err ? err->message : "Failed to make URI");
        if (err) g_error_free(err);
        g_free(abs);
        return;
    }

    gtk_show_uri_on_window(parent, uri, GDK_CURRENT_TIME, &err);

    if (err) {
        gtk_label_set_text(GTK_LABEL(((App*)g_object_get_data(G_OBJECT(parent), "app"))->lbl_status),
                           err->message);
        g_error_free(err);
    }
    g_free(uri);
    g_free(abs);
}

// Columns in the results list store
enum { COL_RANK=0, COL_SCORE, COL_PATH, N_COLS };

static void clear_files(App *app) {
    if (!app->files) return;
    for (guint i=0;i<app->files->len;i++) g_free(g_ptr_array_index(app->files,i));
    g_ptr_array_set_size(app->files, 0);
}

static void set_status(App *app, const char *msg) {
    gtk_label_set_text(GTK_LABEL(app->lbl_status), msg);
}

//static void on_choose(GtkButton *btn, gpointer user_data) {
//    App *app = (App*)user_data;
//    GtkWidget *dlg = gtk_file_chooser_dialog_new(
//        "Choose Text Files",
//        GTK_WINDOW(app->win),
//        GTK_FILE_CHOOSER_ACTION_OPEN,
//        "_Cancel", GTK_RESPONSE_CANCEL,
//        "_Add", GTK_RESPONSE_ACCEPT,
//        NULL
//    );
//    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), TRUE);
//
//    GtkFileFilter *ff = gtk_file_filter_new();
//    gtk_file_filter_set_name(ff, "Text files");
//    gtk_file_filter_add_pattern(ff, "*.txt");
//    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), ff);
//
//    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
//        GSList *list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
//        if (!app->files) app->files = g_ptr_array_new();
//        for (GSList *p=list; p; p=p->next) {
//            g_ptr_array_add(app->files, p->data); // takes ownership
//        }
//        g_slist_free(list);
//
//        char msg[128];
//        snprintf(msg, sizeof(msg), "Selected %u file(s). Click Build Index.", app->files->len);
//        set_status(app, msg);
//        app->indexed = FALSE;
//    }
//    gtk_widget_destroy(dlg);
//}

static void on_row_activated(GtkTreeView *view, GtkTreePath *tpath,
    GtkTreeViewColumn *col, gpointer user_data) {
    App *app = (App*)user_data;

    GtkTreeModel *model = gtk_tree_view_get_model(view);
    GtkTreeIter it;
    if (!gtk_tree_model_get_iter(model, &it, tpath)) return;

    gchar *path = NULL;
    gtk_tree_model_get(model, &it, COL_PATH, &path, -1);
    if (path && *path) {
        open_with_default_app(GTK_WINDOW(app->win), path);
    }
    g_free(path);
}

static void on_load_ai(GtkButton *btn, gpointer user_data) {
    App *app = (App*)user_data;
    if (!app->files) app->files = g_ptr_array_new();

    // Add AI_1.txt ... AI_10.txt using BASE_PATH
    //for (int i=1; i<=10; i++) {
    //char rel[1024];
    //snprintf(rel, sizeof(rel), "%sAI_%d.txt", BASE_PATH, i);

    // Make absolute now, so the model stores openable paths
    //char *abs = g_canonicalize_filename(rel, NULL);
    //g_ptr_array_add(app->files, abs);  // take ownership
//}
    set_status(app, "Loaded AI_1.txt … AI_10.txt. Click Build Index.");
    app->indexed = FALSE;
}

static void on_index(GtkButton *btn, gpointer user_data) {
    App *app = (App*)user_data;
    if (!app->files || app->files->len == 0) {
        set_status(app, "Please add files first (Choose or Load AI_*.txt).");
        return;
    }

    // Convert GPtrArray -> char** for your API
    int n = (int)app->files->len;
    char **list = (char**)malloc(sizeof(char*)*n);
    for (int i=0; i<n; i++) list[i] = (char*)g_ptr_array_index(app->files, i);

    gtk_spinner_start(GTK_SPINNER(app->spinner));
    set_status(app, "Indexing…");

    indexFiles(app->engine, list, n);

    gtk_spinner_stop(GTK_SPINNER(app->spinner));
    app->indexed = TRUE;

    // Optional: quick stats (sum totalWords from FileInfo list)
    int totalWords = 0, filesCount = 0;
    for (FileInfo *f = app->engine->files; f; f = f->next) {
        totalWords += f->totalWords;
        filesCount++;
    }
    char msg[200];
    snprintf(msg, sizeof(msg), "Indexed %d file(s), total words: %d", filesCount, totalWords);
    set_status(app, msg);

    free(list);
}

static void fill_results(App *app, SearchResult *res_head, const char *q) {
    gtk_list_store_clear(app->store);
    int count = 0;
    for (SearchResult *p = res_head; p; p = p->next) {
        GtkTreeIter it;
        gtk_list_store_append(app->store, &it);
        gtk_list_store_set(app->store, &it,
            COL_RANK,  ++count,
            COL_SCORE, (double)p->score,
            COL_PATH,  p->filename,
            -1);
    }
    char msg[256];
    snprintf(msg, sizeof(msg), "Found %d result(s) for “%s”", count, q);
    set_status(app, msg);
}

static void start_search(App *app) {
    const gchar *q = gtk_entry_get_text(GTK_ENTRY(app->entry_query));
    if (!q || !*q) { set_status(app, "Type a query."); return; }
    if (!app->indexed) { set_status(app, "Please build the index first."); return; }

    gtk_spinner_start(GTK_SPINNER(app->spinner));
    set_status(app, "Searching…");

    // Call your real backend
    SearchResult *results = performSearch(app->engine, q);

    gtk_spinner_stop(GTK_SPINNER(app->spinner));

    if (!results) { set_status(app, "No results (or invalid query)."); return; }

    fill_results(app, results, q);
    freeSearchResults(results); // IMPORTANT: avoid leaks
}

static void on_search(GtkButton *btn, gpointer user_data) { start_search((App*)user_data); }
static void on_entry_activate(GtkEntry *e, gpointer user_data) { start_search((App*)user_data); }

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    App app = {0};
    app.engine = createSearchEngine();
    if (!app.engine) {
        g_printerr("Failed to create search engine.\n");
        return 1;
    }

    // Window
    app.win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.win), "Mini Search Engine — GUI");
    gtk_window_set_default_size(GTK_WINDOW(app.win), 900, 600);
    g_signal_connect(app.win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(app.win), root);

    GtkWidget *row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(root), row1, FALSE, FALSE, 0);

    //app.btn_choose = gtk_button_new_with_label("Add Files…");
    //g_signal_connect(app.btn_choose, "clicked", G_CALLBACK(on_choose), &app);
    //gtk_box_pack_start(GTK_BOX(row1), app.btn_choose, FALSE, FALSE, 0);

    app.btn_load_ai = gtk_button_new_with_label("Load AI_*.txt");
    g_signal_connect(app.btn_load_ai, "clicked", G_CALLBACK(on_load_ai), &app);
    gtk_box_pack_start(GTK_BOX(row1), app.btn_load_ai, FALSE, FALSE, 0);

    app.btn_index = gtk_button_new_with_label("Build Index");
    g_signal_connect(app.btn_index, "clicked", G_CALLBACK(on_index), &app);
    gtk_box_pack_start(GTK_BOX(row1), app.btn_index, FALSE, FALSE, 0);

    GtkWidget *row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(root), row2, FALSE, FALSE, 0);

    app.entry_query = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.entry_query), "Enter query keywords");
    g_signal_connect(app.entry_query, "activate", G_CALLBACK(on_entry_activate), &app);
    gtk_box_pack_start(GTK_BOX(row2), app.entry_query, TRUE, TRUE, 0);

    app.btn_search = gtk_button_new_with_label("Search");
    g_signal_connect(app.btn_search, "clicked", G_CALLBACK(on_search), &app);
    gtk_box_pack_start(GTK_BOX(row2), app.btn_search, FALSE, FALSE, 0);

    app.spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(row2), app.spinner, FALSE, FALSE, 0);

    app.store = gtk_list_store_new(N_COLS, G_TYPE_INT, G_TYPE_DOUBLE, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));

    g_signal_connect(tree, "row-activated", G_CALLBACK(on_row_activated), &app);
    g_object_set_data(G_OBJECT(app.win), "app", &app);    

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);

    GtkCellRenderer *r;

    r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes("Rank", r, "text", COL_RANK, NULL));

    r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes("Score", r, "text", COL_SCORE, NULL));

    r = gtk_cell_renderer_text_new();
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree),
        gtk_tree_view_column_new_with_attributes("Document", r, "text", COL_PATH, NULL));

    GtkWidget *scr = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scr), tree);
    gtk_box_pack_start(GTK_BOX(root), scr, TRUE, TRUE, 0);

    app.lbl_status = gtk_label_new("Load files and click Build Index.");
    gtk_box_pack_start(GTK_BOX(root), app.lbl_status, FALSE, FALSE, 0);

    // Auto-load AI_1..AI_10 on startup
    app.files = g_ptr_array_new();
    for (int i=1; i<=10; i++) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%sAI_%d.txt", BASE_PATH, i);
        g_ptr_array_add(app.files, g_strdup(buf));
    }
    gtk_label_set_text(GTK_LABEL(app.lbl_status), "AI_*.txt preloaded. Click Build Index.");

    gtk_widget_show_all(app.win);
    gtk_main();

    clear_files(&app);
    if (app.files) g_ptr_array_free(app.files, TRUE);
    freeSearchEngine(app.engine);
    return 0;
}
