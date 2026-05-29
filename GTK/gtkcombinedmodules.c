/* ============================================================================
 * WEDDING GUEST SYSTEM - LOGIN / SPLASH WINDOW (GTK4)
 * ============================================================================
 * PURPOSE:
 *   Displays a full-screen splash / login window whose background is the
 *   WedPlanner image (mainwindow_bg.jpg).  No password is required – the
 *   user just clicks the "Enter" button (or anywhere on the overlay) to
 *   proceed.  After clicking, this window is destroyed and the main
 *   launcher (./launcher) is spawned as a separate process.
 *
 * BUILD:
 *   gcc gtklogin.c -o gtklogin $(pkg-config --cflags --libs gtk4)
 *
 * RUN:
 *   ./gtklogin
 *
 * IMAGE:
 *   Place  mainwindow_bg.jpg  in the same directory as the executable.
 * ========================================================================== */

#include <gtk/gtk.h>
#include <stdlib.h>

/* ============================================================================
   SECTION 1: CSS
   ========================================================================== */
static const char *LOGIN_CSS =

/* ── Transparent root window so the GtkPicture shows through ── */
"window {"
"  background-color: transparent;"
"}"

/* ── Dark translucent scrim that dims the photo slightly ── */
".bg-scrim {"
"  background: rgba(5, 12, 25, 0.45);"
"}"

/* ── Centered glass card ── */
".login-card {"
"  background: rgba(9, 20, 34, 0.72);"
"  border: 1px solid rgba(201, 168, 76, 0.55);"
"  border-radius: 10px;"
"  padding: 52px 60px 44px 60px;"
"}"

/* ── Gold decorative rule inside the card ── */
".card-rule {"
"  background: linear-gradient(90deg, transparent, #c9a84c, transparent);"
"  min-height: 1px;"
"  margin: 18px 0 24px 0;"
"}"

/* ── Eyebrow label ── */
".login-eyebrow {"
"  color: #c9a84c;"
"  font-size: 10px;"
"  letter-spacing: 4px;"
"  font-weight: bold;"
"}"

/* ── Main title ── */
".login-title {"
"  color: #f5f0e8;"
"  font-size: 34px;"
"  font-weight: 300;"
"  margin-top: 6px;"
"}"

/* ── Subtitle / tagline ── */
".login-sub {"
"  color: rgba(245, 240, 232, 0.55);"
"  font-size: 13px;"
"  letter-spacing: 1px;"
"  margin-top: 4px;"
"}"

/* ── "Click to Enter" button ── */
".enter-btn {"
"  background: #c9a84c;"
"  color: #091422;"
"  border-radius: 4px;"
"  padding: 14px 48px;"
"  font-size: 12px;"
"  font-weight: bold;"
"  letter-spacing: 3px;"
"  border: none;"
"  margin-top: 32px;"
"  transition: all 160ms ease;"
"}"
".enter-btn:hover {"
"  background: #e8c97e;"
"}"
".enter-btn:active {"
"  background: #b8943e;"
"}"

/* ── Small version stamp at bottom of card ── */
".login-version {"
"  color: rgba(245, 240, 232, 0.22);"
"  font-size: 10px;"
"  letter-spacing: 2px;"
"  margin-top: 24px;"
"}";

/* ============================================================================
   SECTION 2: CALLBACK – Enter button / gesture clicked
   ========================================================================== */

/*
 * on_enter_clicked
 *
 * Called when the user clicks the "CLICK TO ENTER" button.
 * Launches ./launcher in the background and destroys this login window.
 */
static void on_enter_clicked(GtkButton *btn, gpointer win)
{
    (void)btn;

    /* Attempt to spawn the main launcher */
    GError *err = NULL;
    if (!g_spawn_command_line_async("./launcher", &err)) {
        /* If the launcher isn't found, show a brief warning but still close */
        g_printerr("[Login] Could not start ./launcher: %s\n",
                   err ? err->message : "unknown error");
        if (err) g_error_free(err);
    } else {
        g_print("[Login] Launched ./launcher – closing login window.\n");
    }

    gtk_window_destroy(GTK_WINDOW(win));
}

/* ============================================================================
   SECTION 3: APPLICATION ACTIVATE
   ========================================================================== */

static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    /* ── CSS ── */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, LOGIN_CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    /* ── Main window ── */
    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "WedPlanner – Welcome");
    gtk_window_set_default_size(GTK_WINDOW(win), 1100, 700);
    gtk_window_set_resizable(GTK_WINDOW(win), TRUE);

    /* ── GtkOverlay: lets us layer the scrim + card on top of the photo ── */
    GtkWidget *overlay = gtk_overlay_new();
    gtk_window_set_child(GTK_WINDOW(win), overlay);

    /* ── Background image (mainwindow_bg.jpg) ── */
    GtkWidget *bg_picture = gtk_picture_new_for_filename("mainwindow_bg.jpg");
    /*
     * CONTENT_FIT_COVER: scales the image so it fills the whole window,
     * cropping the edges if the aspect ratio differs – just like CSS cover.
     */
    gtk_picture_set_content_fit(GTK_PICTURE(bg_picture),
                                GTK_CONTENT_FIT_COVER);
    gtk_widget_set_hexpand(bg_picture, TRUE);
    gtk_widget_set_vexpand(bg_picture, TRUE);
    gtk_overlay_set_child(GTK_OVERLAY(overlay), bg_picture);   /* bottom layer */

    /* ── Semi-transparent scrim overlay ── */
    GtkWidget *scrim = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(scrim, "bg-scrim");
    gtk_widget_set_hexpand(scrim, TRUE);
    gtk_widget_set_vexpand(scrim, TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), scrim);

    /* ── Centered login card ── */
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(card, "login-card");
    gtk_widget_set_halign(card, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(card, GTK_ALIGN_CENTER);

    /* Eyebrow */
    GtkWidget *eyebrow = gtk_label_new("WEDDING GUEST SYSTEM  ·  GROUP 3");
    gtk_widget_add_css_class(eyebrow, "login-eyebrow");
    gtk_widget_set_halign(eyebrow, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), eyebrow);

    /* Title */
    GtkWidget *title_lbl = gtk_label_new("WedPlanner");
    gtk_widget_add_css_class(title_lbl, "login-title");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), title_lbl);

    /* Subtitle */
    GtkWidget *sub_lbl = gtk_label_new("Gifts & Invites Delivered");
    gtk_widget_add_css_class(sub_lbl, "login-sub");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), sub_lbl);

    /* Gold decorative rule */
    GtkWidget *rule = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(rule, "card-rule");
    gtk_box_append(GTK_BOX(card), rule);

    /* "Click to Enter" button */
    GtkWidget *enter_btn = gtk_button_new_with_label("CLICK TO ENTER");
    gtk_widget_add_css_class(enter_btn, "enter-btn");
    gtk_widget_set_halign(enter_btn, GTK_ALIGN_CENTER);
    g_signal_connect(enter_btn, "clicked", G_CALLBACK(on_enter_clicked), win);
    gtk_box_append(GTK_BOX(card), enter_btn);

    /* Version stamp */
    GtkWidget *version = gtk_label_new("Wedding Guest System  v1.0");
    gtk_widget_add_css_class(version, "login-version");
    gtk_widget_set_halign(version, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(card), version);

    /* Add card as an overlay on top of scrim */
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), card);

    /* ── Present ── */
    gtk_window_present(GTK_WINDOW(win));
}

/* ============================================================================
   SECTION 4: MAIN
   ========================================================================== */

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "org.group3.wedding.login",
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    return status;
}
