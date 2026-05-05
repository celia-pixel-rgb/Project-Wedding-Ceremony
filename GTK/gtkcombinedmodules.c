/*
 * ============================================================================
 * WEDDING GUEST SYSTEM - MAIN LAUNCHER (GTK4 Graphical Interface Version)
 * ============================================================================
 * 
 * PURPOSE:
 *   This is a graphical (GUI) version of the wedding management launcher.
 *   Instead of a text menu, it displays a modern graphical interface with
 *   clickable cards for each module. It uses GTK4, a popular GUI toolkit.
 * EXECUTION:
 *   ./launcher
/* ----------------------------------------------------------------------------
   HEADER FILES
   
   gtk/gtk.h : The main GTK4 header - includes all GTK functions and types
   stdlib.h  : Standard library for system(), memory allocation, etc.
   string.h  : String manipulation functions
---------------------------------------------------------------------------- */
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
   SECTION 1: CSS STYLING
   
   CSS (Cascading Style Sheets) controls the visual appearance of the interface.
   In GTK4, you can style widgets using the same CSS you'd use for web pages.
   
   This CSS defines a dark navy background with gold accents for an elegant,
   formal wedding aesthetic.
============================================================================ */

/* ----------------------------------------------------------------------------
   CONSTANT: APP_CSS
   
   HOW GTK USES CSS:
   1. We assign CSS classes to widgets using gtk_widget_add_css_class()
   2. GTK applies the matching CSS rules to those widgets
   3. Result: styled, beautiful interface!
   
   DESIGN SYSTEM:
   - Background: Dark navy (#0d1b2e)
   - Accent: Gold (#c9a84c)
   - Text: Light cream (#f5f0e8)
   - Transparency: rgba() with alpha channel for subtle effects
---------------------------------------------------------------------------- */
static const char *APP_CSS =

/* -- Window Background -- */
"window {"
"  background-color: #0d1b2e;"  // Dark navy blue background
"}"

/* -- Top Banner Section -- */
// The banner is the header area with title and subtitle
".banner {"
"  background: linear-gradient(135deg, #091422 0%, #0d1b2e 60%, #112038 100%);"  // Gradient from dark to slightly lighter
"  padding: 36px 48px 28px 48px;"  // Space inside banner: top right bottom left
"  border-bottom: 1px solid rgba(201,168,76,0.25);"  // Subtle gold separator line
"}"

// Small text above main title (e.g., "WEDDING GUEST SYSTEM")
".banner-eyebrow {"
"  color: #c9a84c;"           // Gold color
"  font-size: 10px;"          // Small text
"  letter-spacing: 4px;"      // Wide spacing between letters
"  font-weight: bold;"        // Bold text
"}"

// Main title text (e.g., "Management Portal")
".banner-title {"
"  color: #f5f0e8;"           // Light cream color
"  font-size: 30px;"          // Large text
"  font-weight: 300;"         // Light weight (thin)
"  margin-top: 6px;"          // Space above
"}"

// Subtitle text (e.g., "Select a module to open")
".banner-sub {"
"  color: rgba(245,240,232,0.45);"  // Light cream at 45% opacity (dimmed)
"  font-size: 12px;"                // Small text
"  margin-top: 4px;"                // Small space above
"  letter-spacing: 1px;"            // Slight letter spacing
"}"

// Decorative horizontal line below banner
".gold-rule {"
"  background: linear-gradient(90deg, transparent, #c9a84c, transparent);"  // Fades in/out
"  min-height: 1px;"  // Very thin line
"  margin: 16px 0 0 0;"  // Space above, none on sides/bottom
"}"

/* -- Cards Area (Container for Module Cards) -- */
".cards-area {"
"  padding: 36px 40px 36px 40px;"  // Space around cards
"  background-color: #0d1b2e;"     // Match main background
"}"

/* -- Individual Module Card Styling -- */
// Each module (Person/Category) is displayed as a card
".module-card {"
"  background: rgba(255,255,255,0.03);"    // Very subtle white tint (3% opacity)
"  border: 1px solid rgba(201,168,76,0.22);"  // Semi-transparent gold border
"  border-radius: 6px;"                    // Rounded corners
"  padding: 28px 24px 24px 24px;"          // Inner spacing
"  transition: all 180ms ease;"            // Smooth animation on hover (180 milliseconds)
"}"

// Hover effect - card lights up when mouse is over it
".module-card:hover {"
"  background: rgba(201,168,76,0.07);"  // Stronger gold tint (7% opacity)
"  border-color: #c9a84c;"               // Full gold border
"}"

/* -- Card Text Elements -- */

// Small label above card title (e.g., "MODULE 01")
".card-tag {"
"  color: #c9a84c;"        // Gold color
"  font-size: 9px;"        // Very small
"  letter-spacing: 4px;"   // Wide letter spacing
"  font-weight: bold;"     // Bold
"  margin-bottom: 4px;"    // Space below
"}"

// Card main title (e.g., "Person Management")
".card-title {"
"  color: #f5f0e8;"        // Light cream
"  font-size: 20px;"       // Medium-large
"  font-weight: 400;"      // Normal weight
"  margin-bottom: 8px;"    // Space below
"}"

// Card description paragraph
".card-desc {"
"  color: rgba(245,240,232,0.50);"  // 50% opacity cream (dimmed)
"  font-size: 12px;"                // Small
"  line-height: 1.6;"                // Spacing between lines (1.6x font size)
"}"

// Individual feature list items
".card-feature {"
"  color: rgba(245,240,232,0.38);"  // Very dimmed (38% opacity)
"  font-size: 11px;"                // Small
"  margin-top: 2px;"                // Tiny space above each feature
"}"

/* -- Launch Button Styling -- */
// The "OPEN MODULE" button on each card
".launch-btn {"
"  background: #c9a84c;"        // Gold background
"  color: #091422;"             // Dark text for contrast
"  border-radius: 3px;"         // Slightly rounded corners
"  padding: 10px 26px;"         // Vertical and horizontal padding
"  font-size: 11px;"            // Small caps-style text
"  font-weight: bold;"          // Bold
"  letter-spacing: 2px;"        // Spaced out letters
"  border: none;"               // No border
"  margin-top: 18px;"           // Space above button
"  transition: all 150ms ease;" // Smooth color transition on hover
"}"

// Button hover effect - lighter gold
".launch-btn:hover {"
"  background: #e8c97e;"  // Lighter gold when mouse hovers
"}"

// Button pressed effect - darker gold
".launch-btn:active {"
"  background: #b8943e;"  // Darker gold when clicking
"}"

/* -- Vertical Separator Between Cards -- */
".card-sep {"
"  background: rgba(201,168,76,0.15);"  // Subtle gold line
"  min-width: 1px;"                     // Thin vertical line
"  margin: 0 20px;"                     // Space on left and right
"}"

/* -- Footer Bar -- */
".footer-bar {"
"  background: #091422;"                          // Very dark navy
"  border-top: 1px solid rgba(201,168,76,0.12);" // Subtle gold line on top
"  padding: 10px 40px;"                          // Vertical and horizontal padding
"}"

// Footer text (password info and version)
".footer-label {"
"  color: rgba(245,240,232,0.18);"  // Very dimmed text (18% opacity)
"  font-size: 10px;"                // Tiny
"  letter-spacing: 2px;"            // Spaced letters
"}"

/* -- Icon Emoji Styling -- */
// The emoji icons at top of each card
".card-icon {"
"  font-size: 32px;"      // Large emoji
"  margin-bottom: 12px;"  // Space below icon
"}";

/* ============================================================================
   SECTION 2: CALLBACK FUNCTIONS
   
   In GTK, a "callback" is a function that runs when something happens
   (like a button click). You "connect" callbacks to "signals" using
   g_signal_connect().
   
   Signal: An event (button-clicked, window-closed, etc.)
   Callback: The function to run when that event occurs
   
   These callbacks handle the button clicks on our module cards.
============================================================================ */

/* ----------------------------------------------------------------------------
   FUNCTION: on_launch_person()
   
   PURPOSE:
     This callback runs when user clicks the "OPEN MODULE" button on the
     Person Management card.
   
   PARAMETERS:
     GtkButton *btn - Pointer to the button that was clicked
     gpointer data  - User data (not used here, so we ignore it)
   
   RETURN VALUE:
     void - No return value
   
   HOW IT WORKS:
     1. Uses g_spawn_command_line_async() to launch ./gtkperson
     2. This is a GTK function that runs a program in the background
     3. "async" means it doesn't wait - program runs independently
     4. If launch fails, shows an error dialog box with instructions
---------------------------------------------------------------------------- */
static void on_launch_person(GtkButton *btn, gpointer data)
{
    // Suppress compiler warnings about unused parameters
    // We don't need to use these parameters, but GTK requires this signature
    (void)btn;   // Mark as intentionally unused
    (void)data;  // Mark as intentionally unused
    
    /* -- Attempt to Launch Person Management Program -- */
    
    // g_spawn_command_line_async() launches a program without waiting
    // Returns TRUE if successful, FALSE if it failed
    // NULL means we don't need the error details
    if (g_spawn_command_line_async("./gtkperson", NULL)) {
        // Success! Print confirmation to terminal (for debugging)
        g_print("[Launcher] Opened Person Management (./gtkperson)\n");
    } 
    else {
        /* -- Launch Failed - Show Error Dialog -- */
        
        // Create a message dialog (popup window)
        // Parameters:
        //   NULL: No parent window
        //   GTK_DIALOG_MODAL: Blocks interaction with other windows
        //   GTK_MESSAGE_WARNING: Warning icon
        //   GTK_BUTTONS_OK: Single "OK" button
        //   "message text...": The text to display
        GtkWidget *dlg = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Could not launch './gtkperson'.\n\n"
            "Make sure it is compiled:\n"
            "  gcc gtkperson-1.c -o gtkperson $(pkg-config --cflags --libs gtk4)"
        );
        
        // Apply our CSS styling to the dialog
        gtk_widget_add_css_class(dlg, "window");
        
        // Connect the "response" signal (when user clicks OK)
        // G_CALLBACK casts our function pointer to the right type
        // gtk_window_destroy will close the dialog when OK is clicked
        g_signal_connect(dlg, "response", G_CALLBACK(gtk_window_destroy), NULL);
        
        // Make the dialog visible
        gtk_widget_set_visible(dlg, TRUE);
    }
}

/* ----------------------------------------------------------------------------
   FUNCTION: on_launch_category()
   
   PURPOSE:
     This callback runs when user clicks the "OPEN MODULE" button on the
     Category Management card.
   
   PARAMETERS:
     GtkButton *btn - Pointer to the button that was clicked
     gpointer data  - User data (unused)
   
   RETURN VALUE:
     void
   
   HOW IT WORKS:
     Identical to on_launch_person() but launches ./gtkcategory instead.
     Same error handling logic with dialog popup if launch fails.
---------------------------------------------------------------------------- */
static void on_launch_category(GtkButton *btn, gpointer data)
{
    // Mark parameters as intentionally unused
    (void)btn;
    (void)data;
    
    /* -- Attempt to Launch Category Management Program -- */
    if (g_spawn_command_line_async("./gtkcategory", NULL)) {
        // Success - print confirmation
        g_print("[Launcher] Opened Category Management (./gtkcategory)\n");
    } 
    else {
        /* -- Launch Failed - Show Error Dialog -- */
        GtkWidget *dlg = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "Could not launch './gtkcategory'.\n\n"
            "Make sure it is compiled:\n"
            "  gcc gtkcategory.c -o gtkcategory $(pkg-config --cflags --libs gtk4)"
        );
        
        // Auto-destroy dialog when user clicks OK
        g_signal_connect(dlg, "response", G_CALLBACK(gtk_window_destroy), NULL);
        
        // Show the dialog
        gtk_widget_set_visible(dlg, TRUE);
    }
}

/* ============================================================================
   SECTION 3: UI CONSTRUCTION HELPER
   
   This function builds a complete module card with all its components.
   It's a "helper" because it encapsulates repetitive card-building logic
   into a single reusable function.
============================================================================ */

/* ----------------------------------------------------------------------------
   FUNCTION: make_card()
   
   PURPOSE:
     Creates a complete module card widget with icon, title, description,
     features list, and launch button.
   
   PARAMETERS:
     icon_utf8    : Unicode emoji to display (e.g., "??")
     module_tag   : Small label text (e.g., "MODULE 01")
     title        : Main card title (e.g., "Person Management")
     description  : Paragraph describing the module
     features[]   : Array of feature strings, must end with NULL
     launch_cb    : Callback function to run when button is clicked
   
   RETURN VALUE:
     GtkWidget* - Pointer to the complete card widget (a GtkBox container)
---------------------------------------------------------------------------- */
static GtkWidget *make_card(
    const char *icon_utf8,     // Unicode emoji string
    const char *module_tag,    // "MODULE 01" or "MODULE 02"
    const char *title,         // Card title
    const char *description,   // Description paragraph
    const char *features[],    // NULL-terminated array of features
    GCallback   launch_cb)     // Function to call on button click
{
    /* -- Create Main Card Container -- */
    
    // GtkBox is a container that arranges children vertically or horizontally
    // GTK_ORIENTATION_VERTICAL: stack children from top to bottom
    // 0: no spacing between children (we'll use margins/padding instead)
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Add CSS class so our stylesheet applies to this widget
    gtk_widget_add_css_class(card, "module-card");
    
    // Allow card to expand horizontally to fill available space
    gtk_widget_set_hexpand(card, TRUE);
    
    /* -- Add Icon Emoji -- */
    
    // GtkLabel displays text (or in this case, an emoji)
    GtkWidget *icon = gtk_label_new(icon_utf8);
    gtk_widget_add_css_class(icon, "card-icon");
    
    // GTK_ALIGN_START: align to the left (start of horizontal axis)
    gtk_widget_set_halign(icon, GTK_ALIGN_START);
    
    // Add icon to card container
    // In GTK4, gtk_box_append adds a child to the end of the box
    gtk_box_append(GTK_BOX(card), icon);
    
    /* -- Add Module Tag (e.g., "MODULE 01") -- */
    
    GtkWidget *tag = gtk_label_new(module_tag);
    gtk_widget_add_css_class(tag, "card-tag");
    gtk_widget_set_halign(tag, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(card), tag);
    
    /* -- Add Title -- */
    
    GtkWidget *ttl = gtk_label_new(title);
    gtk_widget_add_css_class(ttl, "card-title");
    gtk_widget_set_halign(ttl, GTK_ALIGN_START);
    
    // gtk_label_set_xalign sets horizontal alignment of text within label
    // 0.0 = left-aligned, 0.5 = centered, 1.0 = right-aligned
    gtk_label_set_xalign(GTK_LABEL(ttl), 0.0f);
    gtk_box_append(GTK_BOX(card), ttl);
    
    /* -- Add Decorative Horizontal Line -- */
    
    // GtkSeparator is a visual divider line
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(sep, "gold-rule");
    gtk_box_append(GTK_BOX(card), sep);
    
    /* -- Add Description Paragraph -- */
    
    GtkWidget *desc = gtk_label_new(description);
    gtk_widget_add_css_class(desc, "card-desc");
    gtk_widget_set_halign(desc, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.0f);
    
    // Enable text wrapping so long descriptions wrap to multiple lines
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    
    // Limit text width to 32 characters before wrapping
    gtk_label_set_max_width_chars(GTK_LABEL(desc), 32);
    
    // Add spacing above description
    gtk_widget_set_margin_top(desc, 12);
    gtk_box_append(GTK_BOX(card), desc);
    
    /* -- Build Feature List -- */
    
    // Create a container for the feature list items
    GtkWidget *feat_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);  // 2px spacing between items
    gtk_widget_set_margin_top(feat_box, 10);
    
    // Loop through features array until we hit NULL (end marker)
    for (int i = 0; features[i] != NULL; i++) {
        // Build feature string with bullet point
        char buf[128];  // Temporary buffer for formatting
        snprintf(buf, sizeof(buf), "?  %s", features[i]);
        
        // Create label for this feature
        GtkWidget *fl = gtk_label_new(buf);
        gtk_widget_add_css_class(fl, "card-feature");
        gtk_widget_set_halign(fl, GTK_ALIGN_START);
        
        // Add feature to features box
        gtk_box_append(GTK_BOX(feat_box), fl);
    }
    
    // Add the entire features box to the card
    gtk_box_append(GTK_BOX(card), feat_box);
    
    /* -- Add Launch Button -- */
    
    // Format button label
    char btn_label[64];
    snprintf(btn_label, sizeof(btn_label), "OPEN MODULE");
    
    // Create button with label
    GtkWidget *btn = gtk_button_new_with_label(btn_label);
    gtk_widget_add_css_class(btn, "launch-btn");
    gtk_widget_set_halign(btn, GTK_ALIGN_START);
    
    // Connect the button's "clicked" signal to the provided callback
    // When button is clicked, launch_cb function will be called
    // G_CALLBACK casts the function pointer to the correct type
    g_signal_connect(btn, "clicked", launch_cb, NULL);
    
    // Add button to card
    gtk_box_append(GTK_BOX(card), btn);
    
    /* -- Return Complete Card -- */
    return card;  // Return pointer to the fully constructed card widget
}

/* ============================================================================
   SECTION 4: APPLICATION ACTIVATION
   
   In GTK, "activation" occurs when the application starts up and is ready
   to show its UI. The activate() function is where we build the entire
   interface: windows, widgets, layouts, styling.
============================================================================ */

/* ----------------------------------------------------------------------------
   FUNCTION: activate()
   
   PURPOSE:
     Builds and displays the main application window with all its contents.
     This function is automatically called by GTK when the application starts.
   
   PARAMETERS:
     GtkApplication *app - The application instance
     gpointer user_data  - User data (unused)
   
   RETURN VALUE:
     void
---------------------------------------------------------------------------- */
static void activate(GtkApplication *app, gpointer user_data)
{
    // Mark unused parameter
    (void)user_data;
    
    /* ========================================================================
       STEP 1: LOAD CSS STYLESHEET
       
       GTK4 uses CSS for styling. We load our CSS string and apply it to
       all widgets in the application.
    ======================================================================== */
    
    // Create a CSS provider (object that holds CSS rules)
    GtkCssProvider *provider = gtk_css_provider_new();
    
    // Load CSS from our APP_CSS string constant
    gtk_css_provider_load_from_string(provider, APP_CSS);
    
    // Apply the CSS provider to the default display (screen)
    // This makes our styles available to all widgets
    // GTK_STYLE_PROVIDER_PRIORITY_APPLICATION: our styles override defaults
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),                      // Get the screen
        GTK_STYLE_PROVIDER(provider),                   // Our CSS provider
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);       // Priority level
    
    // Decrease reference count - GTK now manages the provider's memory
    // When no one is using it, GTK will automatically free it
    g_object_unref(provider);
    
    /* ========================================================================
       STEP 2: CREATE MAIN WINDOW
       
       GtkApplicationWindow is the top-level window of our app.
    ======================================================================== */
    
    // Create application window tied to our GtkApplication
    GtkWidget *win = gtk_application_window_new(app);
    
    // Set window title (shown in title bar and taskbar)
    gtk_window_set_title(GTK_WINDOW(win), "Wedding Guest System ? Launcher");
    
    // Set default window size (width x height in pixels)
    gtk_window_set_default_size(GTK_WINDOW(win), 760, 500);
    
    // Make window non-resizable (fixed size)
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
    
    /* ========================================================================
       STEP 3: CREATE ROOT LAYOUT CONTAINER
       
       The root container holds all sections: banner, cards, footer.
       We use a vertical box so sections stack top-to-bottom.
    ======================================================================== */
    
    // Create vertical box with 0 spacing (we'll use padding instead)
    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Set this box as the window's main content
    gtk_window_set_child(GTK_WINDOW(win), root);
    
    /* ========================================================================
       STEP 4: BUILD BANNER SECTION (HEADER)
       
       The banner contains:
       - Eyebrow text: "WEDDING GUEST SYSTEM - GROUP 3"
       - Title: "Management Portal"
       - Subtitle: "Select a module to open"
       - Decorative gold line
    ======================================================================== */
    
    // Create container for banner elements
    GtkWidget *banner = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(banner, "banner");
    
    /* -- Eyebrow Label (Small Text Above Title) -- */
    GtkWidget *eyebrow = gtk_label_new("WEDDING GUEST SYSTEM  ?  GROUP 3");
    gtk_widget_add_css_class(eyebrow, "banner-eyebrow");
    gtk_widget_set_halign(eyebrow, GTK_ALIGN_START);  // Align left
    
    /* -- Main Title -- */
    GtkWidget *title_lbl = gtk_label_new("Management Portal");
    gtk_widget_add_css_class(title_lbl, "banner-title");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    
    /* -- Subtitle -- */
    GtkWidget *sub_lbl = gtk_label_new("Select a module to open");
    gtk_widget_add_css_class(sub_lbl, "banner-sub");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);
    
    /* -- Decorative Horizontal Line -- */
    GtkWidget *gold_rule = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_add_css_class(gold_rule, "gold-rule");
    
    /* -- Add All Banner Elements to Banner Container -- */
    gtk_box_append(GTK_BOX(banner), eyebrow);
    gtk_box_append(GTK_BOX(banner), title_lbl);
    gtk_box_append(GTK_BOX(banner), sub_lbl);
    gtk_box_append(GTK_BOX(banner), gold_rule);
    
    /* -- Add Complete Banner to Root Container -- */
    gtk_box_append(GTK_BOX(root), banner);
    
    /* ========================================================================
       STEP 5: BUILD CARDS SECTION (MAIN CONTENT)
       
       This section contains two module cards side-by-side.
       Cards are separated by a vertical divider line.
    ======================================================================== */
    
    // Create horizontal container for cards
    GtkWidget *cards_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(cards_area, "cards-area");
    
    // Allow this section to expand vertically to fill available space
    gtk_widget_set_vexpand(cards_area, TRUE);
    
    /* -- Build Person Management Card -- */
    
    // Define features array (must end with NULL)
    const char *person_feats[] = {
        "Add & save guests (CSV)",
        "Live field validation",
        "Update guest information",
        "Password-protected delete",
        "Auto-refresh display",
        NULL  // Array terminator
    };
    
    // Create complete card using our helper function
    GtkWidget *card_person = make_card(
        "??",                                              // Icon emoji
        "MODULE 01",                                       // Tag
        "Person Management",                               // Title
        "Register, update and manage\n"                   // Description (multi-line)
        "every wedding guest with full\n"
        "validation and CSV persistence.",
        person_feats,                                      // Features array
        G_CALLBACK(on_launch_person)                       // Button callback
    );
    
    /* -- Create Vertical Separator Between Cards -- */
    GtkWidget *vsep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_widget_add_css_class(vsep, "card-sep");
    
    /* -- Build Category Management Card -- */
    
    const char *cat_feats[] = {
        "Create named categories",
        "Assign guests (nested list)",
        "Sort by guest count",
        "Update & delete categories",
        "Remove guests from category",
        NULL  // Array terminator
    };
    
    GtkWidget *card_cat = make_card(
        "???",                                              // Icon emoji
        "MODULE 02",                                       // Tag
        "Category Management",                             // Title
        "Organise guests into nested\n"                   // Description
        "linked-list categories such as\n"
        "VIP, Family, Friends and more.",
        cat_feats,                                         // Features array
        G_CALLBACK(on_launch_category)                     // Button callback
    );
    
    /* -- Add All Elements to Cards Area -- */
    gtk_box_append(GTK_BOX(cards_area), card_person);
    gtk_box_append(GTK_BOX(cards_area), vsep);
    gtk_box_append(GTK_BOX(cards_area), card_cat);
    
    /* -- Add Cards Section to Root Container -- */
    gtk_box_append(GTK_BOX(root), cards_area);
    
    /* ========================================================================
       STEP 6: BUILD FOOTER SECTION
       
       Footer displays system information:
       - Left side: Password notice
       - Right side: Version number
    ======================================================================== */
    
    // Create horizontal container for footer
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(footer, "footer-bar");
    
    /* -- Left Footer Label (Password) -- */
    GtkWidget *foot_l = gtk_label_new("PASSWORD PROTECTED  ?  group3wed!");
    gtk_widget_add_css_class(foot_l, "footer-label");
    gtk_widget_set_halign(foot_l, GTK_ALIGN_START);  // Align left
    gtk_widget_set_hexpand(foot_l, TRUE);            // Expand to push right label right
    
    /* -- Right Footer Label (Version) -- */
    GtkWidget *foot_r = gtk_label_new("Wedding Guest System  v1.0");
    gtk_widget_add_css_class(foot_r, "footer-label");
    gtk_widget_set_halign(foot_r, GTK_ALIGN_END);    // Align right
    
    /* -- Add Labels to Footer -- */
    gtk_box_append(GTK_BOX(footer), foot_l);
    gtk_box_append(GTK_BOX(footer), foot_r);
    
    /* -- Add Footer to Root Container -- */
    gtk_box_append(GTK_BOX(root), footer);
    
    /* ========================================================================
       STEP 7: SHOW THE WINDOW
       
       All widgets are hidden by default. gtk_window_present() makes the
       window visible and brings it to the front.
    ======================================================================== */
    
    gtk_window_present(GTK_WINDOW(win));
}

/* ============================================================================
   SECTION 5: MAIN FUNCTION
   
   The entry point of the program. Creates a GtkApplication and runs it.
============================================================================ */

/* ----------------------------------------------------------------------------
   FUNCTION: main()
   
   PURPOSE:
     Initializes the GTK application and starts the event loop.
   
   PARAMETERS:
     argc - Number of command-line arguments
     argv - Array of command-line argument strings
   
   RETURN VALUE:
     int - Exit status (0 = success)
   
   HOW GTK APPLICATIONS WORK:
     1. Create GtkApplication object
     2. Connect "activate" signal to activate() function
     3. Call g_application_run() which starts the GTK main loop
     4. Main loop handles events (mouse clicks, keyboard, redraws, etc.)
     5. When application quits, main loop exits and returns status code
     6. Clean up memory and return
   
   THE EVENT LOOP:
     GTK uses an "event loop" (also called "main loop"):
     - Loop continuously checks for events
     - When event occurs (button click, etc.), calls appropriate callback
     - Continues until application is told to quit
     - This is how all GUI programs work!
---------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    /* -- Create GTK Application Object -- */
    
    // gtk_application_new() creates a new application
    // Parameters:
    //   "org.group3.wedding.launcher": Unique application ID (reverse domain style)
    //   G_APPLICATION_DEFAULT_FLAGS: Standard application behavior
    GtkApplication *app = gtk_application_new(
        "org.group3.wedding.launcher",
        G_APPLICATION_DEFAULT_FLAGS);
    
    /* -- Connect Activate Signal -- */
    
    // When the application is ready to show UI, call our activate() function
    // "activate" is a signal name (predefined by GTK)
    // activate is our callback function
    // NULL: no extra data to pass to callback
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    
    /* -- Run Application -- */
    
    // Start the GTK main loop
    // This function blocks (doesn't return) until application quits
    // Returns exit status code
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    
    /* -- Clean Up -- */
    
    // Decrease reference count on application object
    // When reference count reaches 0, GTK frees the memory
    g_object_unref(app);
    
    /* -- Return Exit Status -- */
    
    // Return status to operating system
    // 0 typically means success
    return status;
}

/* ============================================================================
   END OF FILE
   
   SUMMARY:
   This GTK4 application demonstrates modern GUI programming concepts:
   
   1. WIDGETS: Building blocks of the interface (buttons, labels, boxes)
   2. CONTAINERS: Widgets that hold other widgets (GtkBox)
   3. SIGNALS & CALLBACKS: Event-driven programming model
   4. CSS STYLING: Separating appearance from structure
   5. LAYOUT: Using boxes and alignment for positioning
   6. MEMORY MANAGEMENT: Reference counting with GObject
   
   KEY DIFFERENCES FROM CLI VERSION:
   - Event-driven (responds to clicks) vs. loop-driven (asks for input)
   - Visual/graphical vs. text-based
   - Asynchronous module launching (doesn't wait) vs. synchronous (waits)
   - More complex but more user-friendly*/
