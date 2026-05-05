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
4. [How the Modules Are Interconnected](#-how-the-modules-are-interconnected)
5. [Installing MSYS2 & GTK 4](#-installing-msys2--gtk-4)
   - [Windows (MSYS2)](#-windows--msys2-ucrt64)
   - [Linux](#-linux--ubuntu--debian)
6. [Building with the Makefile](#️-building-with-the-makefile)
7. [Running the Application](#-running-the-application)
8. [Default Password](#-default-password)
9. [CSV File Formats](#-csv-file-formats)

---

## 💍 Overview

The **Wedding Guest System** is a multi-module C application for wedding organising teams. It provides both a terminal interface and a GTK 4 GUI, with all data written to CSV files that are shared between both interfaces, ensuring perfect data consistency regardless of which interface you use.

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
├── Gifts/
│   ├── gift.h                # Gift struct and function prototypes
│   └── gift.c                # CLI gift manager (25-item catalogue)
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
├── csv/
│   ├── persons.csv           # Persistent storage — guest records
│   ├── category.csv          # Persistent storage — categories & guest assignments
│   └── gifts.csv             # Persistent storage — gift registrations
│
├── combinedmodules.c         # CLI launcher linking Person & Category modules
│
└── Makefile                  # Build system for all targets
```

---

## ⚙️ Module Descriptions & Functions

---

### 🧑‍🤝‍🧑 Module 01 — Person Management

**Files:** `Person/person.h` · `Person/person.c` · `GTK/gtkperson.c`

**What it does:** The Person module is the foundation of the entire system. It allows the wedding team to register every guest, storing their name, age, marital status, phone number, which side of the wedding they belong to (Groom / Bride / Both), and whether they need a parking spot. All guest data is persisted to `csv/persons.csv` and is shared with the Category and Gift modules.

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

> **Implementation notes:**
> - CSV row format: `id,name,age,status,phone,side_int,parking`
> - Both `person.c` (CLI) and `gtkperson.c` (GUI) read/write the **same** `csv/persons.csv` — IDs stay consistent across both interfaces
> - The GTK version uses `GtkEntry`, `GtkCheckButton` (radio groups), `GtkDropDown`, and a `GtkTextView` for table display
> - Auto-refresh via `g_timeout_add()` keeps the GTK display up to date

---

### 🏷️ Module 02 — Category Management

**Files:** `Category/category.h` · `Category/category.c` · `GTK/gtkcategory.c`

**What it does:** The Category module organises registered guests into named groups (e.g. VIP, Family, Friends, Colleagues). It reads guest IDs from `csv/persons.csv` to validate that a guest exists before assigning them to a category. All category data — including the list of guests in each category — is stored in `csv/category.csv`. This module depends directly on the Person module, so guests must be registered first.

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

**Files:** `Gifts/gift.h` · `Gifts/gift.c` · `GTK/gift_gtk03.c`

**What it does:** The Gift module allows guests to select a gift from a catalogue of 25 predefined items with prices in both FCFA and EUR. When a guest makes a gift contribution, the system looks them up by name in `csv/persons.csv`, automatically resolves their category from `csv/category.csv`, calculates the total cost, and saves a complete record to `csv/gifts.csv`. This module depends on both the Person and Category modules — a guest must be registered and categorised before a gift entry can be created.

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

## 🔗 How the Modules Are Interconnected

Understanding the relationships between modules is key to using the system correctly. The three modules are **not independent** — they share data through the CSV files and must be used in the right order.

```
┌─────────────────────────────────────────────────────────────────┐
│                    WEDDING GUEST SYSTEM                         │
│                                                                 │
│   ┌──────────────┐     reads      ┌──────────────────────┐     │
│   │   PERSON     │ ─────────────► │   CATEGORY MODULE    │     │
│   │   MODULE     │  persons.csv   │  (assigns guests     │     │
│   │              │                │   to groups)         │     │
│   │ persons.csv  │                │  category.csv        │     │
│   └──────────────┘                └──────────────────────┘     │
│          │                                    │                 │
│          │ reads                              │ reads           │
│          ▼                                    ▼                 │
│   ┌────────────────────────────────────────────────────┐       │
│   │                   GIFT MODULE                      │       │
│   │  (looks up guest by name from persons.csv,         │       │
│   │   auto-resolves their category from category.csv,  │       │
│   │   saves gift record to gifts.csv)                  │       │
│   └────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────┘
```

### Recommended Workflow

Follow this order when using the system for the first time:

**Step 1 — Register Guests (Person Module)**
> Before anything else, all wedding guests must be registered. The Person module writes each guest with a unique numeric ID to `csv/persons.csv`. This file is the master registry used by all other modules.

**Step 2 — Create Categories and Assign Guests (Category Module)**
> Once guests are registered, create category groups (e.g. VIP, Family, Friends). Then assign each guest to a category using their guest ID. The Category module validates that the guest ID exists in `persons.csv` before accepting the assignment. Results are saved to `csv/category.csv`.

**Step 3 — Record Gift Contributions (Gift Module)**
> With guests registered and categorised, the Gift module can now be used. A guest is looked up by name in `persons.csv`, their category is automatically read from `category.csv`, and the gift record (item, quantity, total in FCFA and EUR, payment method) is saved to `csv/gifts.csv`.

### How `combinedmodules` Ties It Together

The **CLI launcher** (`./combinedmodules`) presents a unified text menu that lets the user choose to open the Person module or the Category module from a single entry point — without having to remember separate binary names. Under the hood it calls `system("./person")` or `system("./category")`.

The **GTK launcher** (`./launcher`) does the same graphically — it displays three clickable module cards (Person Management, Category Management and Gift Management) in a dark-navy/gold themed window. Clicking a card launches the corresponding GTK GUI (`./gtkperson` or `./gtkcategory`) as a separate process. T

> ⚠️ **Important:** Because the Gift module reads both `persons.csv` and `category.csv`, you must have at least one registered guest and one assigned category before creating a gift record. Attempting to add a gift for an unregistered guest name will result in a lookup failure.

---

## 🔧 Installing MSYS2 & GTK 4

### 🪟 Windows — MSYS2 UCRT64

#### Step 1 — Download MSYS2

1. Open your browser and go to **https://www.msys2.org**
2. Click the **Download** button to get the installer (`msys2-x86_64-YYYYMMDD.exe`)

<img width="900" height="230" alt="image" src="https://github.com/user-attachments/assets/081d6c21-92ee-41fa-aa08-53c1891eb530" />

#### Step 2 — Run the Installer

1. Double-click the downloaded `.exe` file
2. Keep the default installation path `C:\msys64` — **do not change this**
   <img width="900" height="230" alt="specify path" src="https://github.com/user-attachments/assets/da96d271-510a-4863-bc70-3c8b14ed0c66" />

3. Click **Next** through the wizard, then **Finish**

<img width="900" height="230" alt="image" src="https://github.com/user-attachments/assets/d9b3e6c1-be47-40d7-878c-11b9564663cd" />

#### Step 3 — Open MSYS2 UCRT64 Terminal

1. Open the **Start Menu** and search for **"MSYS2 UCRT64"**
   
2. Launch the **UCRT64** terminal (gold icon) — **not MINGW32 or MSYS**

<img width="832" height="170" alt="first msys2" src="https://github.com/user-attachments/assets/0e2686f2-aad5-4d58-b8b4-fdd96a9f5854" />


#### Step 4 — Update MSYS2 Packages

```bash
pacman -Syu
```
<img width="832" height="170" alt="image" src="https://github.com/user-attachments/assets/44afd957-6fcb-4f4c-b7f7-92ec4145f66f" />

When prompted `Proceed with installation? [Y/n]`, press **Enter**. The terminal will close — reopen it and run:

```bash
pacman -Su
```

<img width="832" height="170" alt="update pacman" src="https://github.com/user-attachments/assets/2950db45-e52b-4fb8-b167-f5d55ea4e619" />

#### Step 5 — Install GTK 4, GCC, and pkg-config

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make
```

Press **Enter** when asked to confirm, then wait for all packages to install.

<img width="832" height="170" alt="image" src="https://github.com/user-attachments/assets/08345744-7d54-4ab7-89a3-d178bed58ec2" />

#### Step 6 — Verify

```bash
pkg-config --modversion gtk4
gcc --version
```
You should see `4.x.x` for GTK and a GCC version line.

<img width="832" height="170" alt="image" src="https://github.com/user-attachments/assets/a3f8ed5a-f82b-4b33-8ba4-76bc9b6dad46" />

---

### 🐧 Linux — Ubuntu / Debian

#### Step 1 — Update Package Lists

```bash
sudo apt update
```

<img width="793" height="196" alt="image" src="https://github.com/user-attachments/assets/1dd26c48-406a-4028-9f0e-254c7bb24dd1" />


#### Step 2 — Install GTK 4 Development Libraries

```bash
sudo apt install libgtk-4-dev gcc make pkg-config
```

Press **Y** and Enter when prompted.
<img width="793" height="196" alt="image" src="https://github.com/user-attachments/assets/ed4c485a-c227-41ab-9d3d-f14fd6fd1888" />

<img width="793" height="196" alt="image" src="https://github.com/user-attachments/assets/314a4228-5816-4e66-9c8f-5366075ea831" />

#### Step 3 — Verify

```bash
pkg-config --modversion gtk4
gcc --version
```
<img width="793" height="196" alt="image" src="https://github.com/user-attachments/assets/cb22e8a9-a5ec-4ec0-87e7-dec7d2b60ddd" />

---

## 🛠️ Building with the Makefile

### Step 1 — Clone the Repository

```bash
git clone https://github.com/your-username/wedding-guest-system.git
cd wedding-guest-system
```

> 📸 *Screenshot: Terminal after cloning and entering the project directory*

### Step 2 — Navigate to the Project Directory

**MSYS2 (Windows):**
```bash
cd /c/Users/YourName/wedding-guest-system
```

**Linux:**
```bash
cd ~/wedding-guest-system
```

> 📸 *Screenshot: Terminal showing project directory with ls output*

### Step 3 — Confirm the Structure

Run `ls` (Linux/MSYS2) to confirm folders are present:

```
Person/   Category/   Gifts/   GTK/   Parking/   Report/   csv/   Makefile
```

> 📸 *Screenshot: ls output showing correct folder layout*

### Step 4 — Build Everything

```bash
make
```

This compiles all CLI binaries and all GTK 4 GUI binaries in one step.

> 📸 *Screenshot: make output showing all targets compiled successfully*

### Step 5 — Available Targets

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

### Step 6 — Clean Build Artifacts

```bash
make clean
```

> 📸 *Screenshot: make clean removing all executables*

---

## ▶️ Running the Application

> ⚠️ **MSYS2 users:** Always launch from the **UCRT64** terminal — not PowerShell, CMD, or MINGW32. GTK 4 requires the UCRT64 runtime libraries.

### 🖥️ Recommended: GTK Graphical Launcher

The easiest way to use the system is to launch the GTK main launcher, which gives you a graphical portal to all modules:

```bash
./launcher
```

This opens a dark-navy/gold themed window with two clickable cards:
- **MODULE 01 — Person Management** → launches `./gtkperson`
- **MODULE 02 — Category Management** → launches `./gtkcategory`
- **MODULE 03 — Gift Management** → launches `./gift_gtk03`

> 📸 *Screenshot: GTK launcher running with dark-navy/gold theme and module cards*

To open the Gift module separately:

```bash
./gift_gtk03
```

---

### 🖥️ All Run Commands at a Glance

| What to run | Command | Interface | What it does |
|---|---|---|---|
| GTK graphical launcher *(recommended)* | `./launcher` | GTK 4 | Opens the main portal; from here launch Person or Category GUI |
| GTK guest manager | `./gtkperson` | GTK 4 | Register, update, delete and view guests graphically |
| GTK category manager | `./gtkcategory` | GTK 4 | Create categories, assign guests, manage groups graphically |
| GTK gift manager | `./gift_gtk03` | GTK 4 | Select gifts from catalogue, track payments in FCFA & EUR |
| CLI launcher (person + category) | `./combinedmodules` | Terminal | Text menu to open person or category CLI |
| CLI guest manager | `./person` | Terminal | Full terminal guest management |
| CLI category manager | `./category` | Terminal | Full terminal category management |
| CLI gift manager | `./gift` | Terminal | Full terminal gift management |

---

### ⚡ Full Quick-Start (MSYS2 UCRT64)

```bash
# 1. Navigate to the project folder
cd /c/Users/YourName/wedding-guest-system

# 2. Build everything
make

# 3. Launch the GTK graphical interface
./launcher

# 4. Or run any individual module
./gtkperson
./gtkcategory
./gift_gtk03

# 5. Or use the CLI
./combinedmodules
```

> 📸 *Screenshot: MSYS2 UCRT64 terminal showing make output and ./launcher command*

---

### ⚡ Full Quick-Start (Linux)

```bash
# 1. Navigate to the project folder
cd ~/wedding-guest-system

# 2. Build everything
make

# 3. Launch the GTK graphical interface
./launcher
```

> 📸 *Screenshot: Linux terminal showing GTK launcher running*

---

## 🔑 Default Password

All protected operations (display, delete, update) across every module use the same password:

```
group3wed!
```

---

## 📄 CSV File Formats

All CSV files are stored in the `csv/` folder at the root of the project.

### `csv/persons.csv`

Each row represents one registered wedding guest.

```
id,name,age,status,phone,side,parking
```

`side` values: `0` = Groom · `1` = Bride · `2` = Both

---

### `csv/category.csv`

`CAT` lines define category groups. Each `ID` line immediately below a `CAT` line is a guest assigned to that category.

```
CAT,id,code,guest_count
ID,guest_id
```

---

### `csv/gifts.csv`

Each row represents one gift contribution made by a guest.

```
record_id,guest_id,guest_name,category,item_id,item_name,qty,total_fcfa,total_eur,thanked_by
```

---

*💍 Wedding Guest System — Group 3 · v1.0 · Built in C with GTK 4*
