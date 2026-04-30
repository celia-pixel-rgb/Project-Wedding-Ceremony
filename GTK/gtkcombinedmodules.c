/*
 * main.c  ?  Wedding Management System ? Launcher
 *
 * Compile:
 *   gcc main.c -o launcher $(pkg-config --cflags --libs gtk4) && ./launcher
 *
 * The launcher spawns the selected module as a separate process:
 *   ./gtkperson   (person management)
 *   ./gtkcategory  (category management)
 *
 * Build all three binaries first:
 *   gcc gtkperson-1.c  -o gtkperson  $(pkg-config --cflags --libs gtk4)
 *   gcc gtkcategory.c   -o gtkcategory $(pkg-config --cflags --libs gtk4)
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------
   CSS ? dark navy + gold palette
--------------------------------------------------- */
static const char *APP_CSS =
/* Window */
"window {"
"  background-color: #0d1b2e;"
"}"

/* Top banner */
".banner {"
"  background: linear-gradient(135deg, #091422 0%, #0d1b2e 60%, #112038 100%);"
"  padding: 36px 48px 28px 48px;"
"  border-bottom: 1px solid rgba(201,168,76,0.25);"
"}"
".banner-eyebrow {"
"  color: #c9a84c;"
"  font-size: 10px;"
"  letter-spacing: 4px;"
"  font-weight: bold;"
"}"
".banner-title {"
"  color: #f5f0e8;"
"  font-size: 30px;"
"  font-weight: 300;"
"  margin-top: 6px;"
"}"
".banner-sub {"
"  color: rgba(245,240,232,0.45);"
"  font-size: 12px;"
"  margin-top: 4px;"
"  letter-spacing: 1px;"
"}"
".gold-rule {"
"  background: linear-gradient(90deg, transparent, #c9a84c, transparent);"
"  min-height: 1px;"
"  margin: 16px 0 0 0;"
"}"

/* Cards area */
".cards-area {"
"  padding: 36px 40px 36px 40px;"
"  background-color: #0d1b2e;"
"}"

/* Individual card */
".module-card {"
"  background: rgba(255,255,255,0.03);"
"  border: 1px solid rgba(201,168,76,0.22);"
"  border-radius: 6px;"
"  padding: 28px 24px 24px 24px;"
"  transition: all 180ms ease;"
"}"
".module-card:hover {"
"  background: rgba(201,168,76,0.07);"
"  border-color: #c9a84c;"
"}"

/* Card text */
".card-tag {"
"  color: #c9a84c;"
"  font-size: 9px;"
"  letter-spacing: 4px;"
"  font-weight: bold;"
"  margin-bottom: 4px;"
"}"
".card-title {"
"  color: #f5f0e8;"
"  font-size: 20px;"
"  font-weight: 400;"
"  margin-bottom: 8px;"
"}"
".card-desc {"
"  color: rgba(245,240,232,0.50);"
"  font-size: 12px;"
"  line-height: 1.6;"
"}"
".card-feature {"
"  color: rgba(245,240,232,0.38);"
"  font-size: 11px;"
"  margin-top: 2px;"
"}"

/* Launch button */
".launch-btn {"
"  background: #c9a84c;"
"  color: #091422;"
"  border-radius: 3px;"
"  padding: 10px 26px;"
"  font-size: 11px;"
"  font-weight: bold;"
"  letter-spacing: 2px;"
"  border: none;"
"  margin-top: 18px;"
"  transition: all 150ms ease;"
"}"
".launch-btn:hover {"
"  background: #e8c97e;"
"}"
".launch-btn:active {"
"  background: #b8943e;"
"}"

/* Divider between cards */
".card-sep {"
"  background: rgba(201,168,76,0.15);"
"  min-width: 1px;"
"  margin: 0 20px;"
"}"

/* Footer */
".footer-bar {"
"  background: #091422;"
"  border-top: 1px solid rgba(201,168,76,0.12);"
"  padding: 10px 40px;"
"}"
".footer-label {"
"  color: rgba(245,240,232,0.18);"
"  font-size: 10px;"
"  letter-spacing: 2px;"
"}"

/* Icon emoji label styling */
".card-icon {"
"  font-size: 32px;"
"  margin-bottom: 12px;"
"}";

/* ---------------------------------------------------
   Callbacks
--------------------------------------------------- */

static void on_launch_person(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    /* Spawn ./gtkperson in background */
    if (g_spawn_command_line_async("./gtkperson", NULL)) {
        g_print("[Launcher] Opened Person Management (./gtkperson)\n");
    } else {
        /* Fallback: try full compile+run hint via dialog */
        GtkWidget *dlg = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Could not launch './gtkperson'.\n\n"
            "Make sure it is compiled:\n"
            "  gcc gtkperson-1.c -o gtkperson $(pkg-config --cflags --libs gtk4)"
        );
        gtk_widget_add_css_class(dlg, "window");
        g_signal_connect(dlg, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_widget_set_visible(dlg, TRUE);
    }
}

static void on_launch_category(GtkButton *btn, gpointer data)
{
    (void)btn; (void)data;
    if (g_spawn_command_line_async("./gtkcategory", NULL)) {
        g_print("[Launcher] Opened Category Management (./gtkcategory)\n");
    } else {
        GtkWidget *dlg = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Could not launch './gtkcategory'.\n\n"
            "Make sure it is compiled:\n"
            "  gcc gtkcategory.c -o gtkcategory $(pkg-config --cflags --libs gtk4)"
        );
        g_signal_connect(dlg, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_widget_set_visible(dlg, TRUE);
    }
}

/* ---------------------------------------------------
   Helper ? make one module card
--------------------------------------------------- */
static GtkWidget *make_card(
    const char *icon_utf8,
    const char *module_tag,
    const char *title,
    const char *description,
    const char *features[],   /* NULL-terminated array */
    GCallback   launch_cb)
{
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(card, "module-card");
    gtk_widget_set_hexpand(card, TRUE);

    /* Icon */
    GtkWidget *icon = gtk_label_new(icon_utf8);
    gtk_widget_add_css_class(icon, "card-icon");
    gtk_widget_set_halign(icon, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(card), icon);

    /* Module tag */
    GtkWidget *tag = gtk_label_new(module_tag);
    gtk_widget_add_css_class(tag, "card-tag");
    gtk_widget_set_halign(tag, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(card), tag);

    /* Title */
    GtkWidget *ttl = gtk_label_new(title);
    gtk_widget_add_css_class(ttl, "card-title");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(ttl), 0.0f);
    gtk_box_append(GTK_BOX(card), ttl);

    /* Thin gold divider */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(sep, "gold-rule");
    gtk_box_append(GTK_BOX(card), sep);

    /* Description */
    GtkWidget *desc = gtk_label_new(description);
    gtk_widget_add_css_class(desc, "card-desc");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(desc), 32);
    gtk_widget_set_margin_top(desc, 12);
    gtk_box_append(GTK_BOX(card), desc);

    /* Feature list */
    GtkWidget *feat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_top(feat_box, 10);
    for (int i = 0; features[i] != NULL; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "?  %s", features[i]);
        GtkWidget *fl = gtk_label_new(buf);
        gtk_widget_add_css_class(fl, "card-feature");
        gtk_widget_set_halign(fl, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(feat_box), fl);
    }
    gtk_box_append(GTK_BOX(card), feat_box);

    /* Launch button */
    char btn_label[64];
    snprintf(btn_label, sizeof(btn_label), "OPEN MODULE");
    GtkWidget *btn = gtk_button_new_with_label(btn_label);
    gtk_widget_add_css_class(btn, "launch-btn");
    gtk_widget_set_halign(btn, GTK_ALIGN_START);
    g_signal_connect(btn, "clicked", launch_cb, NULL);
    gtk_box_append(GTK_BOX(card), btn);

    return card;
}

/* ---------------------------------------------------
   activate()
--------------------------------------------------- */
static void activate(GtkApplication *app, gpointer user_data)
{
    (void)user_data;

    /* -- Load CSS -- */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_string(provider, APP_CSS);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    /* -- Main window -- */
    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "Wedding Guest System ? Launcher");
    gtk_window_set_default_size(GTK_WINDOW(win), 760, 500);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    /* Root vertical box */
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_window_set_child(GTK_WINDOW(win), root);

    /* ------ BANNER ------ */
    GtkWidget *banner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(banner, "banner");

    GtkWidget *eyebrow = gtk_label_new("WEDDING GUEST SYSTEM  ?  GROUP 3");
    gtk_widget_add_css_class(eyebrow, "banner-eyebrow");
    gtk_widget_set_halign(eyebrow, GTK_ALIGN_START);

    GtkWidget *title_lbl = gtk_label_new("Management Portal");
    gtk_widget_add_css_class(title_lbl, "banner-title");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);

    GtkWidget *sub_lbl = gtk_label_new("Select a module to open");
    gtk_widget_add_css_class(sub_lbl, "banner-sub");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);

    GtkWidget *gold_rule = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(gold_rule, "gold-rule");

    gtk_box_append(GTK_BOX(banner), eyebrow);
    gtk_box_append(GTK_BOX(banner), title_lbl);
    gtk_box_append(GTK_BOX(banner), sub_lbl);
    gtk_box_append(GTK_BOX(banner), gold_rule);
    gtk_box_append(GTK_BOX(root), banner);

    /* ------ CARDS AREA ------ */
    GtkWidget *cards_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(cards_area, "cards-area");
    gtk_widget_set_vexpand(cards_area, TRUE);

    /* Person card */
    const char *person_feats[] = {
        "Add & save guests (CSV)",
        "Live field validation",
        "Update guest information",
        "Password-protected delete",
        "Auto-refresh display",
        NULL
    };
    GtkWidget *card_person = make_card(
        "??",
        "MODULE 01",
        "Person Management",
        "Register, update and manage\nevery wedding guest with full\nvalidation and CSV persistence.",
        person_feats,
        G_CALLBACK(on_launch_person)
    );

    /* Vertical separator between cards */
    GtkWidget *vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_add_css_class(vsep, "card-sep");

    /* Category card */
    const char *cat_feats[] = {
        "Create named categories",
        "Assign guests (nested list)",
        "Sort by guest count",
        "Update & delete categories",
        "Remove guests from category",
        NULL
    };
    GtkWidget *card_cat = make_card(
        "???",
        "MODULE 02",
        "Category Management",
        "Organise guests into nested\nlinked-list categories such as\nVIP, Family, Friends and more.",
        cat_feats,
        G_CALLBACK(on_launch_category)
    );

    gtk_box_append(GTK_BOX(cards_area), card_person);
    gtk_box_append(GTK_BOX(cards_area), vsep);
    gtk_box_append(GTK_BOX(cards_area), card_cat);
    gtk_box_append(GTK_BOX(root), cards_area);

    /* ------ FOOTER ------ */
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(footer, "footer-bar");

    GtkWidget *foot_l = gtk_label_new("PASSWORD PROTECTED  ?  group3wed!");
    gtk_widget_add_css_class(foot_l, "footer-label");
    gtk_widget_set_halign(foot_l, GTK_ALIGN_START);
    gtk_widget_set_hexpand(foot_l, TRUE);

    GtkWidget *foot_r = gtk_label_new("Wedding Guest System  v1.0");
    gtk_widget_add_css_class(foot_r, "footer-label");
    gtk_widget_set_halign(foot_r, GTK_ALIGN_END);

    gtk_box_append(GTK_BOX(footer), foot_l);
    gtk_box_append(GTK_BOX(footer), foot_r);
    gtk_box_append(GTK_BOX(root), footer);

    gtk_window_present(GTK_WINDOW(win));
}

/* ---------------------------------------------------
   main()
--------------------------------------------------- */
int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new(
        "org.group3.wedding.launcher",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
