# 💍 Wedding Guest System — Group 3

> A full-featured wedding management application built in **C** with both a **terminal (CLI)** interface and a **GTK 4 graphical** interface. Manage guests, categories, and gifts — all backed by shared CSV persistence.

---

## 📋 Table of Contents

1. [Overview](#-overview)
2. [Repository Structure](#-repository-structure)
3. [Module Descriptions & Functions](#️-module-descriptions--functions)
   - [🧑‍🤝‍🧑 Person Module](#-module-01----person-management)
   - [🏷️ Category Module](#️-module-02----category-management)
   - [🎁 Gift Module](#-module-03----gift-management)
   - [🚀 Launchers](#-launchers)
4. [Installing MSYS2 & GTK 4](#-installing-msys2--gtk-4)
   - [Windows (MSYS2)](#-windows--msys2-ucrt64)
   - [Linux](#-linux--ubuntu--debian)
5. [Building with the Makefile](#️-building-with-the-makefile)
6. [Running the Application](#-running-the-application)
7. [Default Password](#-default-password)
8. [CSV File Formats](#-csv-file-formats)

---

## 💍 Overview

The **Wedding Guest System** is a multi-module C application for wedding organizing teams. It provides both a terminal interface and a GTK 4 GUI, with all data written to CSV files shared between both interfaces.

| Module | Status | Description |
|---|---|---|
| 🧑‍🤝‍🧑 **Person** | ✅ Complete | Register, view, update and delete wedding guests with full field validation |
| 🏷️ **Category** | ✅ Complete | Organise guests into nested linked-list groups (VIP, Family, Friends…) |
| 🎁 **Gift** | ✅ Complete | Let guests choose from a 25-item catalogue in FCFA & EUR with payment tracking |
| 🚗 **Parking** | 🚧 Planned | Parking spot management — not yet implemented |

> 🔑 **Password for all protected operations:** `group3wed!`

---

## 📁 Repository Structure

```
wedding-guest-system/
│
├── Person/
│   ├── person.h              # Guest struct, Side enum, function prototypes
│   └── person.c              # CLI: add, show, delete, update guests (CSV)
│
├── Category/
│   ├── category.h            # Category & GuestRef struct prototypes
│   └── category.c            # CLI: nested linked-list category manager
│
├── Parking/                  # 🚧 Planned — not yet implemented
│
├── GTK/
│   ├── gtkperson.c           # GTK 4 GUI — guest management
│   ├── gtkcategory.c         # GTK 4 GUI — category management
│   ├── gift_gtk03.c          # GTK 4 GUI — gift management
│   └── gtkcombinedmodules.c  # GTK 4 graphical launcher (dark-navy/gold theme)
│
├── Report/                   # Generated reports and summaries
│
├── persons.csv               # Persistent storage — guest records
├── category.csv              # Persistent storage — categories & guest assignments
├── gifts.csv                 # Persistent storage — gift registrations
│
├── combinedmodules.c         # CLI launcher linking Person & Category modules
├── gift.h                    # Gift struct and function prototypes
├── gift.c                    # CLI gift manager
│
└── Makefile                  # Build system for all targets
```

---

## ⚙️ Module Descriptions & Functions

---

### 🧑‍🤝‍🧑 Module 01 — Person Management

**Files:** `Person/person.h` · `Person/person.c` · `GTK/gtkperson.c`

#### Data Structures

| Symbol | Kind | Description |
|---|---|---|
| `Side` | `enum` | Three values: `GROOM (0)`, `BRIDE (1)`, `BOTH (2)` — identifies which side of the wedding a guest belongs to |
| `Guest` | `struct` | Holds all guest data: `id`, `name[100]`, `age`, `status[50]`, `phone[20]`, `Side side`, `parking[10]` |

#### Functions — `person.h` (Shared Prototypes)

| Function | Returns | Description |
|---|---|---|
| `side_to_string(Side s)` | `const char*` | Converts a `Side` enum value to a readable string: `"Groom"`, `"Bride"`, or `"Both"` |
| `get_next_id()` | `int` | Scans `persons.csv` for the highest existing ID and returns `max_id + 1`. Shared by CLI and GTK so IDs are always globally unique |
| `action_add_guest()` | `void` | Prompts for all guest fields, validates each one, then appends a new row to `persons.csv` |
| `action_show_guests()` | `void` | 🔑 Password-protected. Reads `persons.csv` and renders all guests as a formatted table |
| `action_delete_guest()` | `void` | 🔑 Password-protected. Removes a guest by ID via a temp-file swap, then renumbers all remaining IDs |
| `action_update_guest()` | `void` | 🔑 Password-protected. Loads the current record, lets the user overwrite any field (blank = keep original), then rewrites `persons.csv` |

#### Internal Helpers — `person.c`

| Function | Returns | Description |
|---|---|---|
| `read_line(prompt, buf, size)` | `void` | Prints a prompt and reads a trimmed line from stdin (strips trailing newline) |
| `read_int(prompt)` | `int` | Reads a line and converts it to an integer with `atoi()` |
| `check_password()` | `int` | Returns `1` if input matches `"group3wed!"`, `0` otherwise |
| `is_all_digits(s)` | `int` | Returns `1` if every character in the string is a decimal digit |
| `is_valid_phone(p)` | `int` | Validates Cameroonian format: exactly 9 digits, first digit must be 6, 7, 8 or 9 |
| `is_valid_age(a)` | `int` | Returns `1` if the age string is a number in the range 1–120 |
| `renumber_ids()` | `void` | After a deletion, rewrites `persons.csv` with fresh sequential IDs starting at 0 |
| `load_guest(target, out)` | `int` | Searches for a guest by ID; copies data into `*out`. Returns `1` if found, `0` otherwise |
| `prompt_side()` | `Side` | Presents a numbered menu and returns the chosen `Side` enum value |
| `prompt_parking(buf, size)` | `void` | Asks Yes/No for parking and stores `"Yes"` or `"No"` in the buffer |

> **Implementation notes:**
> - CSV row format: `id,name,age,status,phone,side_int,parking`
> - Both `person.c` (CLI) and `gtkperson.c` (GUI) read/write the **same** `persons.csv` — IDs stay consistent across both interfaces
> - The GTK version uses `GtkEntry`, `GtkCheckButton` (radio groups), `GtkDropDown`, and a `GtkTextView` for table display
> - Auto-refresh via `g_timeout_add()` keeps the GTK display up to date

---

### 🏷️ Module 02 — Category Management

**Files:** `Category/category.h` · `Category/category.c` · `GTK/gtkcategory.c`

#### Data Structures — Nested Linked List

| Symbol | Kind | Description |
|---|---|---|
| `GuestRef` | `struct` | **Inner list node.** Holds a `guest_id` (int) and a `*next` pointer. Forms the inner linked list of guests assigned to a category |
| `Category` | `struct` | **Outer list node.** Holds `id`, `code[50]` (name), `*guests` (head of inner GuestRef list), `guest_count`, and `*next` |

#### Menu Action Functions

| Function | Returns | Description |
|---|---|---|
| `action_create_category()` | `void` | Prompts for a category code (e.g. VIP, Family), allocates a new `Category` node, inserts it at the head of the list, and saves to `category.csv` |
| `action_assign_guest()` | `void` | Asks for category ID and guest ID, calls `assign_guest_to_category()`, and prints a precise message for each return code |
| `action_display()` | `void` | 🔑 Password-protected. Displays all categories and their nested guest lists, with total counts |
| `action_sort_display()` | `void` | 🔑 Password-protected. Sorts categories descending by guest count (insertion sort), persists the new order, then displays |
| `action_update_category()` | `void` | 🔑 Password-protected. Renames a category's code by ID and saves |
| `action_delete_category()` | `void` | 🔑 Password-protected. Removes a category node and frees its entire inner `GuestRef` list, then saves |
| `action_remove_guest()` | `void` | 🔑 Password-protected. Unlinks a single guest from a specific category's inner list and saves |

#### Persistence Functions

| Function | Returns | Description |
|---|---|---|
| `load_categories_from_csv()` | `void` | Clears in-memory data, then rebuilds the full nested linked list from `category.csv` (`CAT,…` lines → outer nodes; `ID,…` lines → inner GuestRef nodes) |
| `save_categories_to_csv()` | `void` | Writes the entire nested list to `category_tmp.csv`, then atomically renames it to `category.csv` to prevent data corruption on crash |

#### `assign_guest_to_category()` — Return Codes

| Code | Meaning |
|---|---|
| `+1` | ✅ Guest successfully assigned to the category |
| ` 0` | ⚠️ Category ID not found, or `malloc()` failed |
| `-1` | ❌ Guest ID does not exist in `persons.csv` |
| `-2` | 🚫 Guest is already assigned to this category (duplicate blocked) |

> **Implementation notes:**
> - Inner list uses `append_guest_ref()` to maintain insertion order
> - Outer list uses `insert_category()` at the head — O(1) insertion
> - `sort_categories_desc()` uses insertion sort on the outer linked list
> - After a deletion, all remaining category IDs greater than the deleted ID are decremented to keep them sequential

---

### 🎁 Module 03 — Gift Management

**Files:** `gift.h` · `gift.c` · `GTK/gift_gtk03.c`

#### Data Structures — `gift.h`

| Symbol | Kind | Description |
|---|---|---|
| `Guest` | `struct` | Lightweight linked-list node: `name[100]` and `*next`. Tracks which guests are linked to a gift |
| `Gift` | `struct` | Full gift record: `id`, `name[100]`, `price`, `max_quantity`, `taken_quantity`, `*guests` (linked list), `*next` |
| `GiftItem` | `struct` (GTK) | Catalogue entry: `id`, `name`, `price_fcfa` (double) — used in `gift_gtk03.c` |
| `GiftRecord` | `struct` (GTK) | Full persisted record: `record_id`, `guest_id`, `guest_name`, `category`, `item_id`, `item_name`, `qty`, `total_fcfa`, `total_eur`, `thanked_by` |

#### Functions — `gift.h` (Shared Prototypes)

| Function | Returns | Description |
|---|---|---|
| `create_gift(id, name, price, max_qty)` | `Gift*` | Allocates and initialises a new `Gift` node; returns `NULL` on allocation failure |
| `add_gift(head, new_gift)` | `void` | Appends `new_gift` to the end of the global gift linked list |
| `find_gift(head, id)` | `Gift*` | Walks the list and returns a pointer to the gift with matching `id`, or `NULL` if not found |
| `add_guest_to_gift(gift, guest_name)` | `void` | Allocates a new `Guest` node and appends it to the gift's inner guest list |
| `display_gifts(head)` | `void` | Iterates over all gifts and prints each item with its linked guest names |
| `initialize_predefined_gifts(list)` | `void` | Populates the list with all 25 predefined gift items and their FCFA prices |
| `check_password(input)` | `bool` | Returns `true` if the input string matches the system password |

#### CLI Functions — `gift.c`

| Function | Returns | Description |
|---|---|---|
| `showMenu()` | `void` | Prints the full 26-item gift catalogue with names and FCFA prices |
| `addGift()` | `void` | Collects guest name, gift choice, and quantity; calculates total; handles 6 payment methods; assigns a unique ID suffixed `G` (e.g. `3G`) |
| `displayAll()` | `void` | 🔑 Password-protected. Lists all gift contributions in `id – name – gift – total` format |
| `displayOne(id)` | `void` | 🔑 Password-protected. Shows the single record matching the given guest ID |
| `deleteGift(id)` | `void` | 🔑 Password-protected. Removes the record by ID and shifts remaining array entries left |
| `updateGift(id)` | `void` | 🔑 Password-protected. Shows current record, re-displays catalogue, lets user choose a new gift and quantity, recalculates total |
| `authenticate()` | `int` | Shared password check used by all protected CLI functions; returns `1` on success |

#### GTK-Only Functions — `gift_gtk03.c`

| Function | Returns | Description |
|---|---|---|
| `find_guest_by_name(name, out)` | `int` | Case-insensitive lookup in `persons.csv` by full name; fills `*out` with the `GuestBrief` record. No ID entry required from the user |
| `find_category_for_guest(guest_id, out_code, size)` | `void` | Parses `category.csv` to find which category a guest belongs to; fills `out_code` with the name or `"Unassigned"` |
| `get_next_gift_id()` | `int` | Scans `gifts.csv` for the highest record ID and returns `max + 1` |
| `write_gift_line(f, g)` | `void` | Writes one full `GiftRecord` as a CSV row to the given file pointer |

> **GTK Gift Manager — key features:**
> - Guest looked up by **name only** (case-insensitive) — no ID entry needed
> - Category resolved automatically from `category.csv`
> - Dual-currency display: FCFA and EUR at rate `1 EUR = 655.957 FCFA`
> - Atomic CSV writes: `gifts_tmp.csv` → `rename()` to prevent data corruption
> - Payment options: Cash · Mobile Money · Orange Money · Airtel Money · Yoomee · Credit Card

---

### 🚀 Launchers

| File | Interface | Description |
|---|---|---|
| `combinedmodules.c` | CLI | Text-based banner menu. Launches `./person` or `./category` via `system()`. Includes ASCII border and module feature descriptions |
| `GTK/gtkcombinedmodules.c` | GTK 4 | Graphical launcher with dark-navy (`#0d1b2e`) + gold (`#c9a84c`) CSS theme. Displays clickable module cards with hover animation. Launches `./gtkperson`, `./gtkcategory`, and `./gift_gtk03` via `system()` |

---

## 🔧 Installing MSYS2 & GTK 4

### 🪟 Windows — MSYS2 UCRT64

#### Step 1 — Download MSYS2

1. Open your browser and go to **https://www.msys2.org**
2. Click the **Download** button to get the installer (`msys2-x86_64-YYYYMMDD.exe`)

> 📸 *Screenshot: MSYS2 download page — click the green "Download" button*

#### Step 2 — Run the Installer

1. Double-click the downloaded `.exe` file
2. Keep the default installation path `C:\msys64` — **do not change this**
3. Click **Next** through the wizard, then **Finish**

> 📸 *Screenshot: MSYS2 installer welcome screen and default path selection*

#### Step 3 — Open MSYS2 UCRT64 Terminal

1. Open the **Start Menu** and search for **"MSYS2 UCRT64"**
2. Launch the **UCRT64** terminal (gold icon) — **not MINGW32 or MSYS**

> 📸 *Screenshot: Start menu showing MSYS2 UCRT64 shortcut*

#### Step 4 — Update MSYS2 Packages

```bash
pacman -Syu
```

When prompted `Proceed with installation? [Y/n]`, press **Enter**. The terminal will close — reopen it and run:

```bash
pacman -Su
```

> 📸 *Screenshot: UCRT64 terminal showing pacman update progress*

#### Step 5 — Install GTK 4, GCC, and pkg-config

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make
```

Press **Enter** when asked to confirm, then wait for all packages to install.

> 📸 *Screenshot: pacman installing GTK 4 packages*

#### Step 6 — Verify

```bash
pkg-config --modversion gtk4
gcc --version
```

You should see `4.x.x` for GTK and a GCC version line.

> 📸 *Screenshot: Terminal output confirming GTK 4 version number*

---

### 🐧 Linux — Ubuntu / Debian

#### Step 1 — Update Package Lists

```bash
sudo apt update
```

> 📸 *Screenshot: Terminal running apt update*

#### Step 2 — Install GTK 4 Development Libraries

```bash
sudo apt install libgtk-4-dev gcc make pkg-config
```

Press **Y** and Enter when prompted.

> 📸 *Screenshot: apt install downloading GTK 4 dev packages*

#### Step 3 — Verify

```bash
pkg-config --modversion gtk4
gcc --version
```

> 📸 *Screenshot: Terminal confirming GTK 4 and GCC versions*

---

## 🛠️ Building with the Makefile

### Step 1 — Navigate to the Project Directory

**MSYS2 (Windows):**
```bash
cd /c/Users/YourName/wedding-guest-system
```

**Linux:**
```bash
cd ~/wedding-guest-system
```

> 📸 *Screenshot: Terminal showing project directory with ls output*

### Step 2 — Build Everything

```bash
make
```

> 📸 *Screenshot: make output showing all targets compiled successfully*

### Step 3 — Available Targets

| Command | Type | What it builds |
|---|---|---|
| `make person` | CLI | Terminal guest manager binary |
| `make category` | CLI | Terminal category manager binary |
| `make gift` | CLI | Terminal gift manager binary |
| `make combinedmodules` | CLI | CLI launcher that opens person or category |
| `make gtkperson` | GTK 4 | GTK 4 guest management GUI |
| `make gtkcategory` | GTK 4 | GTK 4 category management GUI |
| `make gift_gtk03` | GTK 4 | GTK 4 gift management GUI |
| `make launcher` | GTK 4 | GTK 4 graphical main launcher |
| `make clean` | — | Removes all compiled binaries and temp CSV files |

> 📸 *Screenshot: Building a single target with make gtkperson*

### Step 4 — Clean

```bash
make clean
```

> 📸 *Screenshot: make clean removing all executables*

---

## ▶️ Running the Application

> ⚠️ **MSYS2 users:** Always launch from the **UCRT64** terminal — not PowerShell, CMD, or MINGW32. GTK 4 requires the UCRT64 runtime libraries.

| What to run | Command | Interface |
|---|---|---|
| GTK graphical launcher *(recommended)* | `./launcher` | GTK 4 — full graphical launcher |
| GTK guest manager only | `./gtkperson` | GTK 4 GUI |
| GTK category manager only | `./gtkcategory` | GTK 4 GUI |
| GTK gift manager only | `./gift_gtk03` | GTK 4 GUI |
| CLI launcher (person + category) | `./combinedmodules` | Terminal |
| CLI guest manager only | `./person` | Terminal |
| CLI category manager only | `./category` | Terminal |
| CLI gift manager only | `./gift` | Terminal |

### Full Quick-Start (MSYS2 UCRT64)

```bash
# 1. Navigate to the project folder
cd /c/Users/YourName/wedding-guest-system

# 2. Build everything
make

# 3. Launch the GTK interface
./launcher
```

> 📸 *Screenshot: GTK launcher running with dark-navy/gold theme and module cards*

---

## 🔑 Default Password

All protected operations (display, delete, update) across every module use the same password:

```
group3wed!
```

---

## 📄 CSV File Formats

### `persons.csv`

```
id,name,age,status,phone,side,parking
0,Talla Daniella,16,VIP,674836353,1,No
1,Matchim Celia,18,VIP,674839363,1,Yes
```

`side` values: `0` = Groom · `1` = Bride · `2` = Both

### `category.csv`

```
CAT,id,code,guest_count
ID,guest_id

CAT,1,Family,0
CAT,0,VIP,2
ID,0
ID,1
```

`CAT` lines define categories. Each `ID` line immediately below belongs to the last `CAT` entry above it.

### `gifts.csv`

```
record_id,guest_id,guest_name,category,item_id,item_name,qty,total_fcfa,total_eur,thanked_by
1,1,Matchim Celia,VIP,1,House,1,45000000.00,68602.06,Matchim Celia
```

---

*💍 Wedding Guest System — Group 3 · v1.0 · Built in C with GTK 4*
