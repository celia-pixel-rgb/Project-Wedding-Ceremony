# =============================================================================
# Makefile — Wedding Guest System (Group 3)
# ALL SOURCE FILES ARE IN THE SAME FLAT DIRECTORY
# =============================================================================
# Targets:
#   all              Build every CLI and GTK binary
#   person           CLI guest manager
#   category         CLI category manager
#   gift             CLI gift manager
#   parking          CLI parking manager
#   combinedmodules  CLI launcher (person + category + gift + parking)
#   gtkperson        GTK 4 guest manager GUI
#   gtkcategory      GTK 4 category manager GUI
#   gift_gtk03       GTK 4 gift manager GUI
#   gtkparking       GTK 4 parking manager GUI
#   launcher         GTK 4 graphical main launcher (all modules)
#   clean            Remove all compiled binaries
#
# Usage:
#   make             Build all targets
#   make <target>    Build one specific target
#   make clean       Remove binaries
# =============================================================================

# ---- Compiler & base flags --------------------------------------------------
CC       = gcc
CFLAGS   = -Wall -std=c11

# GTK targets have extra flags to suppress harmless unused-parameter warnings
# and the Side* vs int* format mismatch that comes from GTK callback signatures
GTK_WARN = -Wno-unused-parameter -Wno-format

# ---- GTK 4 flags (auto-detected via pkg-config) -----------------------------
GTK_CFLAGS  = $(shell pkg-config --cflags gtk4)
GTK_LIBS    = $(shell pkg-config --libs gtk4)

# =============================================================================
# DEFAULT TARGET — build everything
# =============================================================================
.PHONY: all
all: person category gift parking combinedmodules gtkperson gtkcategory gift_gtk03 gtkparking launcher
	@echo ""
	@echo "  ========================================"
	@echo "  All targets built successfully!"
	@echo "  ========================================"

# =============================================================================
# CLI TARGETS
# =============================================================================

# -- person: terminal guest manager ------------------------------------------
person: person.c person.h
	$(CC) $(CFLAGS) -o person person.c
	@echo "  [OK] person compiled"

# -- category: terminal category manager -------------------------------------
category: category.c category.h person.h
	$(CC) $(CFLAGS) -o category category.c
	@echo "  [OK] category compiled"

# -- gift: terminal gift manager ---------------------------------------------
gift: gift.c gift.h
	$(CC) $(CFLAGS) -o gift gift.c -lm
	@echo "  [OK] gift compiled"

# -- parking: terminal parking manager ---------------------------------------
parking: parking.c parking.h
	$(CC) $(CFLAGS) -o parking parking.c
	@echo "  [OK] parking compiled"

# -- combinedmodules: CLI launcher (person + category + gift + parking) -------
combinedmodules: combinedmodules.c
	$(CC) $(CFLAGS) -o combinedmodules combinedmodules.c
	@echo "  [OK] combinedmodules compiled"

# =============================================================================
# GTK 4 TARGETS
# =============================================================================

# -- gtkperson: GTK 4 GUI for guest management --------------------------------
gtkperson: gtkperson.c person.h
	$(CC) $(CFLAGS) $(GTK_WARN) $(GTK_CFLAGS) \
	      -o gtkperson gtkperson.c \
	      $(GTK_LIBS)
	@echo "  [OK] gtkperson compiled"

# -- gtkcategory: GTK 4 GUI for category management --------------------------
gtkcategory: gtkcategory.c person.h
	$(CC) $(CFLAGS) $(GTK_WARN) $(GTK_CFLAGS) \
	      -o gtkcategory gtkcategory.c \
	      $(GTK_LIBS)
	@echo "  [OK] gtkcategory compiled"

# -- gift_gtk03: GTK 4 GUI for gift management --------------------------------
gift_gtk03: gift_gtk03.c
	$(CC) $(CFLAGS) $(GTK_WARN) $(GTK_CFLAGS) \
	      -o gift_gtk03 gift_gtk03.c \
	      $(GTK_LIBS) -lm
	@echo "  [OK] gift_gtk03 compiled"

# -- gtkparking: GTK 4 GUI for parking management ----------------------------
gtkparking: gtkparking.c parking.h
	$(CC) $(CFLAGS) $(GTK_WARN) $(GTK_CFLAGS) \
	      -o gtkparking gtkparking.c \
	      $(GTK_LIBS)
	@echo "  [OK] gtkparking compiled"

# -- launcher: GTK 4 graphical main launcher (all modules) -------------------
launcher: gtkcombinedmodules.c
	$(CC) $(CFLAGS) $(GTK_WARN) $(GTK_CFLAGS) \
	      -o launcher gtkcombinedmodules.c \
	      $(GTK_LIBS) -lm
	@echo "  [OK] launcher compiled"

# =============================================================================
# CLEAN
# =============================================================================
.PHONY: clean
clean:
	rm -f person category gift parking combinedmodules \
	      gtkperson gtkcategory gift_gtk03 gtkparking launcher \
	      temp.csv temp_renum.csv category_tmp.csv gifts_tmp.csv \
	      parking_spot.csv parking_booking.csv
	@echo "  [OK] clean done"
