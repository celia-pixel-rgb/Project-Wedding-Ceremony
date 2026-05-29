<div align="center">

# 💍 Wedding Guest System

### Group 3 · Built in C with GTK 4

<br/>

[![C](https://img.shields.io/badge/Language-C-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![GTK 4](https://img.shields.io/badge/GUI-GTK_4-4A90D9?style=for-the-badge&logo=gtk&logoColor=white)](https://gtk.org/)
[![CSV](https://img.shields.io/badge/Storage-CSV-217346?style=for-the-badge&logoColor=white)](#-csv-file-formats)
[![MSYS2](https://img.shields.io/badge/Windows-MSYS2_UCRT64-1C1C1C?style=for-the-badge&logo=windows-terminal&logoColor=white)](#-windows--msys2-ucrt64)
[![Linux](https://img.shields.io/badge/Linux-Ubuntu%2FDebian-E95420?style=for-the-badge&logo=ubuntu&logoColor=white)](#-linux--ubuntu--debian)
[![License](https://img.shields.io/badge/License-Academic-gold?style=for-the-badge)](#)

<br/>


> *A full-featured wedding management application — dual CLI & GTK 4 GUI interfaces,*
> *four interconnected modules, all data shared through CSV persistence.*

<br/>

|🧑‍🤝‍🧑 Guests         |🏷️ Categories |🎁 Gifts          |🚗 Parking        |
|:---------------:|:-----------:|:---------------:|:---------------:|
|Register & manage|Nested groups|25-item catalogue|Slot booking     |
|Full validation  |Linked lists |FCFA & EUR       |Overlap detection|

<br/>


> 🔑 **Default password for all protected operations:**  `group3wed!`

</div>

-----

## 📋 Table of Contents

1. [Overview](#-overview)
1. [Repository Structure](#-repository-structure)
1. [GTK Graphical Interface](#️-gtk-graphical-interface)
1. [Background Image Management](#-background-image-management)
1. [Module Functions](#️-module-functions)
- [🧑‍🤝‍🧑 Person Module](#-module-01--person-management)
- [🏷️ Category Module](#️-module-02--category-management)
- [🎁 Gift Module](#-module-03--gift-management)
- [🪡 Wedding Fabrics](#-wedding-fabrics--gift-module-extension)
- [🚗 Parking Module](#-module-04--parking-management)
- [💌 Invitation Module](#-module-05--invitation-manager-gtkinvitationsc)
- [🚀 Launchers](#-launchers)
1. [Module Interconnections](#-module-interconnections)
- [Dependency Diagram](#dependency-diagram)
- [Recommended Workflow](#recommended-workflow)
1. [Installation](#-installation)
- [🪟 Windows — MSYS2 UCRT64](#-windows--msys2-ucrt64)
- [🐧 Linux — Ubuntu / Debian](#-linux--ubuntu--debian)
1. [Build & Run](#️-build--run)
- [Clone & Build](#clone--build)
- [Make Targets](#make-targets)
- [Run Commands](#run-commands)
- [Quick-Start Cheatsheet](#-quick-start-cheatsheet)
1. [CSV File Formats](#-csv-file-formats)

-----

## 💍 Overview

|Module          |Status    |Interface  |Description                                                                         |
|:--------------:|:--------:|:---------:|------------------------------------------------------------------------------------|
|🧑‍🤝‍🧑 **Person**    |✅ Complete|CLI + GTK 4|Register, view, update & delete wedding guests with full field validation           |
|🏷️ **Category**  |✅ Complete|CLI + GTK 4|Organise guests into nested linked-list groups — VIP, Family, Friends…              |
|🎁 **Gift**      |✅ Complete|CLI + GTK 4|25-item catalogue in FCFA & EUR, 6 payment methods, auto category resolution        |
|🚗 **Parking**   |✅ Complete|CLI + GTK 4|Admin-configurable spots, time-slot booking, midnight-wrap overlap detection        |
|💌 **Invitation**|✅ Complete|GTK 4      |Design, preview, save & e-mail personalised wedding invitations with 5 colour themes|


> 💡 Both interfaces (CLI and GTK) read and write the **same CSV files** — data stays consistent regardless of which you use.

-----

## 📁 Repository Structure

```
wedding-guest-system/               ← all source files in one flat directory
│
├── 🧩 Core Modules
│   ├── person.h / person.c         # Guest struct · Side enum · CRUD operations
│   ├── category.h / category.c     # Nested linked-list category manager
│   ├── gift.h / gift.c             # CLI gift manager — 25-item catalogue
│   └── parking.h / parking.c       # Spot config · booking · overlap detection
│
├── 🖥️  GTK 4 GUI
│   ├── gtkperson.c                 # Guest management GUI
│   ├── gtkcategory.c               # Category management GUI
│   ├── gift_gtk03.c                # Gift management GUI
│   ├── gtkparking.c                # Parking management GUI (admin + guest panels)
│   ├── gtkcombinedmodules.c        # Main launcher — dark-navy/gold theme
│   ├── gtkinvitations.c            # Invitation manager GUI (login · create · send · delete)
│   │
│   └── 🪡 fabrics/                 # Wedding fabric assets — lives here, under gtk/
│       ├── fabrics.csv             # Fabric catalogue (id · name · price_fcfa · price_eur · colour · origin)
│       └── fabric_images/          # (optional) fabric preview thumbnails — PNG/JPG
│           ├── fabric_01.png
│           ├── fabric_02.png
│           └── ...
│
├── 🚀 Launchers
│   └── combinedmodules.c           # CLI launcher — person · category · parking
│
├── 📂 csv/
│   ├── persons.csv                 # Master guest registry
│   ├── category.csv                # Category groups & guest assignments
│   ├── gifts.csv                   # Gift contribution records
│   ├── parking_spot.csv            # Admin spot configuration
│   ├── parking_booking.csv         # Active parking reservations
│   └── invitations.csv             # Saved invitation records
│
└── 🔨 Makefile                     # Builds all CLI + GTK targets
```

-----

## 🖥️ GTK Graphical Interface

GTK 4 *(GIMP Toolkit 4)* was integrated into this project to provide a fully featured **Graphical User Interface (GUI)** for the Wedding Invitation and Gift Management System. Rather than relying solely on the terminal-based CLI, the GTK layer delivers an interactive, visually polished experience accessible to all users regardless of their comfort with the command line.

Each of the four core modules — **Person Management**, **Category Management**, **Gift Management**, and **Parking Management** — has a dedicated GTK 4 window. These windows are tied together through a central **Main Launcher** (`gtkcombinedmodules.c`) styled in a dark-navy and gold wedding theme, from which all module GUIs can be opened individually.

### Key GTK Design Decisions

|Aspect               |Implementation                                                                                  |
|---------------------|------------------------------------------------------------------------------------------------|
|**Toolkit**          |GTK 4 — cross-platform, native rendering on Windows (MSYS2 UCRT64) and Linux                    |
|**Theme**            |Dark-navy `#0d1b2e` with gold accent `#c9a84c` — consistent across all windows                  |
|**Data binding**     |All GTK windows read and write the **same CSV files** as the CLI — no data divergence           |
|**Auto-refresh**     |Live data updates via `g_timeout_add()` — no manual reload required                             |
|**Access control**   |Password-protected admin actions use `GtkPasswordEntry` with the shared system password         |
|**Background images**|Each module window loads a dedicated background image dynamically at runtime (see section below)|


> 💡 The GTK interface and the CLI interface are fully interoperable — you may switch between them freely at any time. All data modifications made through one interface are immediately visible in the other.

-----

## 🖼️ Background Image Management

To enhance the visual quality and user experience of the application, a dedicated **image management system** has been implemented for the GTK interface. Each GTK window loads a unique background image at runtime, giving every module a distinct and professional appearance.

### Background Folder

A folder named **`Backgrounds`** has been created inside the GTK project directory. This folder serves as the centralised repository for all background images used across the application’s windows.

> ⚠️ **Important:** All background images must be stored in the **same source directory as their corresponding GTK window source files**. The application loads images using relative paths at runtime — if an image is missing or placed in the wrong location, the window will fail to render its background correctly.

### Background Images — Module Mapping

The following images are included in the `Backgrounds` folder. Each image is dynamically loaded by its corresponding GTK window during application startup:

|Image File         |Module / Window            |Description                                                                                      |
|-------------------|---------------------------|-------------------------------------------------------------------------------------------------|
|`mainwindow_bg.jpg`|**Main Window / Home Page**|Background for the central launcher (`gtkcombinedmodules.c`) — the entry point of the application|
|`person_bg.jpg`    |**Person Management**      |Background for the guest registration and management window (`gtkperson.c`)                      |
|`gift_bg.jpg`      |**Gift Management**        |Background for the gift catalogue and contribution recording window (`gift_gtk03.c`)             |
|`category_bg.jpg`  |**Category Management**    |Background for the guest grouping and category assignment window (`gtkcategory.c`)               |
|`parking_bg.jpg`   |**Parking Management**     |Background for the admin and guest parking panels window (`gtkparking.c`)                        |
|`invitation_bg.jpg`|**Invitation Manager**     |Background for the invitation login screen (`gtkinvitations.c`)                                  |

Each image is loaded **dynamically** inside its corresponding GTK window using GTK 4’s image-loading API. This approach ensures that the application’s graphical appearance remains consistent and visually appealing without hard-coding any visual assets into the compiled binary.

### Project Structure

The following diagram illustrates the expected folder layout, including the `Backgrounds` subfolder and its image assets:

```
Project Folder
├── src
├── GTK Windows
│   ├── gtkcombinedmodules.c
│   ├── gtkperson.c
│   ├── gift_gtk03.c
│   ├── gtkcategory.c
│   ├── gtkparking.c
│   │
│   └── Backgrounds
│       ├── mainwindow_bg.jpg
│       ├── person_bg.jpg
│       ├── gift_bg.jpg
│       ├── category_bg.jpg
│       ├── parking_bg.jpg
│       └── invitation_bg.jpg
```

> 🔴 **Do not relocate the `Backgrounds` folder.** The GTK source files reference background images using paths relative to their own source directory. Moving the folder — or running the application binary from a different working directory — will cause background images to fail to load silently at runtime.

### ✅ Background Image Checklist

Before building and running the GTK interface, verify the following:

- [ ] The `Backgrounds` folder exists inside the GTK Windows source directory
- [ ] All six image files (`mainwindow_bg.jpg`, `person_bg.jpg`, `gift_bg.jpg`, `category_bg.jpg`, `parking_bg.jpg`, `invitation_bg.jpg`) are present
- [ ] Images are in `.jpg` format and are not corrupted or renamed
- [ ] The application binary is launched from the correct working directory so relative paths resolve correctly

-----

## ⚙️ Module Functions

-----

### 🧑‍🤝‍🧑 Module 01 — Person Management

**Files:** `person.h`  ·  `person.c`  ·  `gtkperson.c`

The **foundation** of the system. Every other module reads `persons.csv` to validate guest identity.

#### Data Structures

|Symbol |Kind    |Fields                                                                               |
|-------|:------:|-------------------------------------------------------------------------------------|
|`Side` |`enum`  |`GROOM (0)` · `BRIDE (1)` · `BOTH (2)`                                               |
|`Guest`|`struct`|`id` · `name[100]` · `age` · `status[50]` · `phone[20]` · `Side side` · `parking[10]`|

#### Functions

|Function                |Returns      |Access|Description                                                         |
|------------------------|:-----------:|:----:|--------------------------------------------------------------------|
|`side_to_string(Side s)`|`const char*`|🌐     |Converts `Side` enum → `"Groom"` / `"Bride"` / `"Both"`             |
|`get_next_id()`         |`int`        |🌐     |Scans `persons.csv`, returns `max_id + 1` — shared by CLI & GTK     |
|`action_add_guest()`    |`void`       |🔓     |Prompts all fields, validates each, appends new row to `persons.csv`|
|`action_show_guests()`  |`void`       |🔑     |Reads and renders all guests as a formatted table                   |
|`action_delete_guest()` |`void`       |🔑     |Removes guest by ID via temp-file swap; renumbers remaining IDs     |
|`action_update_guest()` |`void`       |🔑     |Overwrites any field — blank input keeps the original value         |


> 🔵 **GTK extras:** `GtkEntry`, radio `GtkCheckButton` groups, `GtkDropDown`, `GtkTextView` for table display · auto-refresh via `g_timeout_add()`

-----

### 🏷️ Module 02 — Category Management

**Files:** `category.h`  ·  `category.c`  ·  `gtkcategory.c`

Organises guests into named groups using a **nested linked list**. Validates guest IDs against `persons.csv` before assignment.

#### Data Structures

|Symbol    |Kind    |Description                                                             |
|----------|:------:|------------------------------------------------------------------------|
|`GuestRef`|`struct`|**Inner node** — `guest_id` (int) · `*next`                             |
|`Category`|`struct`|**Outer node** — `id` · `code[50]` · `*guests` · `guest_count` · `*next`|

#### Menu Actions

|Function                  |Returns|Access|Description                                                      |
|--------------------------|:-----:|:----:|-----------------------------------------------------------------|
|`action_create_category()`|`void` |🔓     |Allocates new `Category` node, inserts at head O(1), saves       |
|`action_assign_guest()`   |`void` |🔓     |Assigns guest to category; prints a message per return code      |
|`action_display()`        |`void` |🔑     |Shows all categories with nested guest lists and total counts    |
|`action_sort_display()`   |`void` |🔑     |Insertion-sorts by guest count desc, persists new order, displays|
|`action_update_category()`|`void` |🔑     |Renames a category code by ID                                    |
|`action_delete_category()`|`void` |🔑     |Removes outer node; frees entire inner `GuestRef` list           |
|`action_remove_guest()`   |`void` |🔑     |Unlinks a single guest from a specific category’s inner list     |

#### Persistence

|Function                    |Returns|Description                                                                        |
|----------------------------|:-----:|-----------------------------------------------------------------------------------|
|`load_categories_from_csv()`|`void` |Clears memory, rebuilds full nested list (`CAT` → outer nodes · `ID` → inner nodes)|
|`save_categories_to_csv()`  |`void` |Atomic write: `category_tmp.csv` → `rename()` — prevents corruption on crash       |

#### `assign_guest_to_category()` — Return Codes

|Code|Status   |Meaning                                    |
|:--:|:-------:|-------------------------------------------|
|`+1`|✅ Success|Guest successfully assigned to the category|
|`0` |⚠️ Warning|Category ID not found, or `malloc()` failed|
|`-1`|❌ Error  |Guest ID does not exist in `persons.csv`   |
|`-2`|🚫 Blocked|Guest is already assigned to this category |

-----

### 🎁 Module 03 — Gift Management

**Files:** `gift.h`  ·  `gift.c`  ·  `gift_gtk03.c`

Guests choose from a **25-item catalogue** priced in FCFA & EUR. Depends on both Person and Category modules — guests must be registered and categorised first.

#### Data Structures

|Symbol      |Kind            |Description                                                                                                                                    |
|------------|:--------------:|-----------------------------------------------------------------------------------------------------------------------------------------------|
|`Guest`     |`struct`        |Linked-list node: `name[100]` · `*next`                                                                                                        |
|`Gift`      |`struct`        |`id` · `name[100]` · `price` · `max_quantity` · `taken_quantity` · `*guests` · `*next`                                                         |
|`GiftItem`  |`struct` *(GTK)*|Catalogue entry: `id` · `name` · `price_fcfa`                                                                                                  |
|`GiftRecord`|`struct` *(GTK)*|Full record: `record_id` · `guest_id` · `guest_name` · `category` · `item_id` · `item_name` · `qty` · `total_fcfa` · `total_eur` · `thanked_by`|

#### Shared Functions — `gift.h`

|Function                               |Returns|Description                                                   |
|---------------------------------------|:-----:|--------------------------------------------------------------|
|`create_gift(id, name, price, max_qty)`|`Gift*`|Allocates & initialises a new Gift node; `NULL` on failure    |
|`add_gift(head, new_gift)`             |`void` |Appends to the global gift linked list                        |
|`find_gift(head, id)`                  |`Gift*`|Returns pointer to matching gift, or `NULL`                   |
|`add_guest_to_gift(gift, guest_name)`  |`void` |Appends a Guest node to the gift’s inner list                 |
|`display_gifts(head)`                  |`void` |Prints all gifts with their linked guest names                |
|`initialize_predefined_gifts(list)`    |`void` |Populates the list with all 25 catalogue items and FCFA prices|
|`check_password(input)`                |`bool` |Returns `true` if input matches system password               |

#### CLI Functions — `gift.c`

|Function        |Returns|Access|Description                                                                              |
|----------------|:-----:|:----:|-----------------------------------------------------------------------------------------|
|`showMenu()`    |`void` |🔓     |Prints the full 26-item catalogue with FCFA prices                                       |
|`addGift()`     |`void` |🔓     |Collects guest, gift, qty; handles 6 payment methods; assigns `G`-suffixed ID (e.g. `3G`)|
|`displayAll()`  |`void` |🔑     |Lists all gift contributions                                                             |
|`displayOne(id)`|`void` |🔑     |Shows the single record for a given guest ID                                             |
|`deleteGift(id)`|`void` |🔑     |Removes record by ID, shifts remaining entries left                                      |
|`updateGift(id)`|`void` |🔑     |Re-displays catalogue, lets user pick a new gift, recalculates total                     |
|`authenticate()`|`int`  |—     |Shared password check; returns `1` on success                                            |

#### GTK-Only Functions — `gift_gtk03.c`

|Function                                           |Returns|Description                                                              |
|---------------------------------------------------|:-----:|-------------------------------------------------------------------------|
|`find_guest_by_name(name, out)`                    |`int`  |Case-insensitive name lookup in `persons.csv`; no ID entry needed        |
|`find_category_for_guest(guest_id, out_code, size)`|`void` |Resolves category from `category.csv`; fills `"Unassigned"` if none found|
|`get_next_gift_id()`                               |`int`  |Scans `gifts.csv` for highest ID; returns `max + 1`                      |
|`write_gift_line(f, g)`                            |`void` |Writes one full `GiftRecord` as a CSV row                                |


> 💳 **Payment methods:** Cash · Mobile Money · Orange Money · Airtel Money · Yoomee · Credit Card
> 
> 💱 **Exchange rate:** `1 EUR = 655.957 FCFA`

-----

### 🪡 Wedding Fabrics — Gift Module Extension

<div align="center">

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║   ✦  W E D D I N G   F A B R I C S   C A T A L O G U E  ✦                  ║
║                                                                              ║
║        A curated selection of premium bridal & ceremonial textiles           ║
║              woven into the Gift Module's offering system                    ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

</div>

The Gift module has been extended with a **Wedding Fabrics** section — a dedicated catalogue of ceremonial and bridal textiles that guests may offer as a gift contribution. Each fabric is listed with its FCFA & EUR pricing and integrates seamlessly with the existing gift recording system.

#### 📂 Critical — Fabric Files Location

> ⚠️ **This is mandatory. The application will not find the fabrics without it.**

Fabric assets (images, data files, or resource files related to the fabric catalogue) are located inside the **`gtk/fabrics/`** subfolder of the repository. This is their **canonical location** — the GTK gift GUI (`gift_gtk03.c`) loads them from this path at runtime:

```
wedding-guest-system/
│
├── 🧩 Source & Headers
│   ├── gift.h / gift.c             # ← fabric functions declared & implemented here
│   ├── gift_gtk03.c                # ← fabric GTK rendering — opens gtk/fabrics/ at runtime
│   ├── person.h / person.c
│   ├── category.h / category.c
│   └── parking.h / parking.c
│
├── 🖥️  gtk/                        ← GTK 4 GUI folder
│   ├── gtkcombinedmodules.c        # Main launcher — dark-navy/gold theme
│   ├── gtkperson.c
│   ├── gtkcategory.c
│   ├── gtkparking.c
│   │
│   └── 🪡 fabrics/                 ← ✅ FABRIC ASSETS LIVE HERE — inside gtk/
│       ├── fabrics.csv             # Fabric catalogue (id · name · price_fcfa · price_eur · colour · origin)
│       └── fabric_images/          # Fabric preview thumbnails — PNG/JPG
│           ├── fabric_01.png
│           ├── fabric_02.png
│           └── ...
│
├── 📂 csv/
│   ├── persons.csv
│   ├── category.csv
│   ├── gifts.csv
│   ├── parking_spot.csv
│   └── parking_booking.csv
│
└── 🔨 Makefile
```

> 🔴 **The `fabrics/` folder must remain inside `gtk/` — do not move it to the project root or any other location.** The C code opens fabric resources using the **relative path `gtk/fabrics/fabrics.csv`** from the working directory. Launching the binary from outside the project root, or relocating the `fabrics/` folder, will cause the fabric catalogue to silently fail to load.

#### 🧵 Why Co-location Matters

The Gift module opens `fabrics.csv` with a **relative file path**:

```c
FILE *f = fopen("gtk/fabrics/fabrics.csv", "r");  // path relative to project root
```

This means:

|Scenario                                                    |Result                                                         |
|------------------------------------------------------------|:-------------------------------------------------------------:|
|`gtk/fabrics/fabrics.csv` exists at project root level      |✅ Loads correctly                                              |
|`fabrics/` folder moved out of `gtk/` (e.g. to project root)|❌ `NULL` — path broken                                         |
|Binary launched from outside the project root               |❌ `NULL` — relative path broken                                |
|`gtk/fabrics/fabric_images/` folder missing (GTK mode)      |⚠️ Fabric images will not render, but catalogue text still works|


> 💡 **Best practice:** always `cd` into the project root before running `make` or any binary, so relative paths like `gtk/fabrics/fabrics.csv` resolve correctly. The **MSYS2 UCRT64** terminal and standard Linux terminals both support this naturally.

#### 🌸 Fabric Catalogue — Available Items

The fabric catalogue ships with the following ceremonial textiles (editable via `fabrics.csv`):

|ID |Fabric Name            |Colour / Style         |Price (FCFA)|Price (EUR)|
|:-:|-----------------------|-----------------------|-----------:|----------:|
|F01|**Kente Cloth**        |Gold & Green · Royal   |85 000      |129.58     |
|F02|**Bogolan (Mud Cloth)**|Earth tones · Artisanal|45 000      |68.59      |
|F03|**Ankara Print**       |Vivid multicolour      |22 000      |33.54      |
|F04|**Silk Bridal Lace**   |Ivory · Off-white      |120 000     |182.93     |
|F05|**Organza Sheer**      |Blush Pink · Delicate  |38 000      |57.94      |
|F06|**Damask Brocade**     |Champagne Gold         |95 000      |144.83     |
|F07|**Aso-Oke**            |Cream & Burgundy       |67 000      |102.14     |
|F08|**Chiffon Veil Fabric**|Pure White · Flowing   |30 000      |45.73      |


> 🎨 Fabric items are appended to the main gift catalogue as items **F01–F08**, fully compatible with the existing `GiftRecord` struct and `gifts.csv` schema.

#### 🔧 Fabric-Specific Functions — `gift.h` / `gift.c`

|Function                           |Returns|Description                                                                   |
|-----------------------------------|:-----:|------------------------------------------------------------------------------|
|`initialize_fabric_catalogue(list)`|`void` |Loads fabric items from `fabrics.csv` and appends them to the gift linked list|
|`display_fabric_menu()`            |`void` |Prints the fabric sub-catalogue with FCFA & EUR prices                        |
|`is_fabric_item(item_id)`          |`bool` |Returns `true` if the item ID starts with `"F"`                               |

#### 🖥️ GTK Integration — `gift_gtk03.c`

In the GTK interface, fabric items appear as a **dedicated tab** within the Gift Management window:

- A scrollable `GtkListBox` renders each fabric with name, price, and colour swatch
- Fabric images are loaded from `gtk/fabrics/fabric_images/` (relative to the project root) using `gtk_picture_new_for_filename()`
- If `gtk/fabrics/fabric_images/` is absent, a **placeholder tile** is shown — the rest of the GUI is unaffected

> 🪟 **MSYS2 UCRT64 users:** GTK 4 and all required libraries (`mingw-w64-ucrt-x86_64-gtk4`, `gcc`, `pkg-config`) are installed into `C:\msys64\ucrt64\`. When you run binaries from the UCRT64 terminal, these libraries are automatically on the `PATH` — **you never need to copy DLLs manually**. Fabric assets, however, are loaded at runtime via relative paths and are therefore **your responsibility** to keep co-located in the project root. A missing DLL breaks the whole program; a missing `gtk/fabrics/fabrics.csv` silently disables only the fabric tab.

#### ✅ Pre-launch Checklist for Fabric Features

Before running `./launcher` or `./gift_gtk03`, confirm:

- [ ] `gtk/fabrics/fabrics.csv` exists inside the `gtk/fabrics/` subfolder
- [ ] You are running the binary **from the project root** (`cd wedding-guest-system && ./launcher`)
- [ ] MSYS2 UCRT64 terminal is used on Windows (not PowerShell, CMD, or MINGW32)
- [ ] GTK 4 libraries verified: `pkg-config --modversion gtk4` prints `4.x.x`
- [ ] *(Optional)* `gtk/fabrics/fabric_images/` folder is present for image previews in GTK mode

-----

### 🚗 Module 04 — Parking Management

**Files:** `parking.h`  ·  `parking.c`  ·  `gtkparking.c`

Admin configures up to **200 spots**. Guests with `parking = "Yes"` book time slots — the system detects overlaps and wraps correctly past midnight.

#### Data Structures

|Symbol      |Kind    |Fields                                                                                                              |
|------------|:------:|--------------------------------------------------------------------------------------------------------------------|
|`SpotConfig`|`struct`|`spot_id` (1-based) · `max_hours`                                                                                   |
|`Booking`   |`struct`|`booking_id` · `spot_id` · `guest_name[64]` · `start_hour` · `start_min` · `duration_hours` · `end_hour` · `end_min`|

#### `check_person_parking()` — Return Codes

|Code|Constant            |Meaning                              |
|:--:|--------------------|-------------------------------------|
|`0` |`PERSON_NOT_FOUND`  |Name not found in `persons.csv`      |
|`1` |`PERSON_NO_PARKING` |Guest exists, but parking = `"No"`   |
|`2` |`PERSON_HAS_PARKING`|Guest exists with parking = `"Yes"` ✅|

#### CLI Functions — `parking.c`

|Function                                                       |Returns   |Description                                                         |
|---------------------------------------------------------------|:--------:|--------------------------------------------------------------------|
|`to_minutes(h, m)`                                             |`int`     |Converts hour:minute → minutes since midnight (e.g. `9:30 → 570`)   |
|`add_hours(sh, sm, dur_h, *eh, *em)`                           |`void`    |Adds whole hours to start time, wraps correctly past midnight       |
|`intervals_overlap(as, ae, bs, be)`                            |`int`     |Returns `1` if `[as, ae)` overlaps `[bs, be)`                       |
|`save_spot_config(num_spots, max_hours)`                       |`void`    |Writes admin config to `parking_spot.csv`                           |
|`load_spot_config(*num_spots, *max_hours)`                     |`int`     |Reads config; returns `1` on success, `0` if not found              |
|`append_booking(b)`                                            |`void`    |Appends one `Booking` row to `parking_booking.csv`                  |
|`next_booking_id()`                                            |`int`     |Returns `max_id + 1`; returns `1` for empty file                    |
|`load_bookings(*out_count)`                                    |`Booking*`|Loads all rows into dynamic array — caller must `free()`            |
|`cancel_booking_by_id(booking_id)`                             |`int`     |Removes matching row; returns `1` on success                        |
|`cancel_bookings_for_guest(guest_name)`                        |`int`     |Removes **all** bookings for guest; returns count removed           |
|`check_person_parking(name)`                                   |`int`     |Case-insensitive lookup; returns `PERSON_*` code                    |
|`find_available_spot(req_start, req_end, num_spots)`           |`int`     |Returns first non-overlapping spot number, or `-1` if full          |
|`guest_has_overlapping_booking(name, *out, req_start, req_end)`|`int`     |Returns `1` if conflict found; fills `*out` with conflicting booking|
|`print_all_spots(num_spots)`                                   |`void`    |Displays `FREE` / `OCCUPIED` status for every configured spot       |
|`print_guest_bookings(guest_name)`                             |`void`    |Lists all bookings in `parking_booking.csv` for a guest             |
|`run_admin_menu()`                                             |`void`    |🔑 Configure spots & max hours, view occupancy, cancel bookings by ID|
|`run_guest_menu(guest_name, num_spots, max_hours)`             |`void`    |🔓 Book a slot, view own bookings, cancel own booking                |

#### GTK Parking Panels — `gtkparking.c`

|Panel          |Access|Features                                                                                              |
|---------------|:----:|------------------------------------------------------------------------------------------------------|
|**Admin Panel**|🔑     |Set spot count & max hours · real-time occupancy via `g_timeout_add()` · cancel any booking by ID     |
|**Guest Panel**|🔓     |Name entry → verified vs `persons.csv` → choose start time & duration → auto-assigned spot → confirmed|

-----

### 💌 Module 05 — Invitation Manager (`gtkinvitations.c`)

**File:** `gtkinvitations.c`

**Dependencies:** `person.h`  ·  `libcurl`  ·  `libcairo`  ·  `libm`

The **Invitation Manager** is a fully self-contained GTK 4 module that allows users to design, preview, save, and send personalised wedding invitations. Unlike the other modules — which offer both a CLI and a GTK interface — this module is **GTK 4 only**, making full use of Cairo vector graphics to render elegant, print-ready invitation cards directly inside the application.

The module opens with a **password-protected login screen** (`login_bg.jpg` background) styled in the system’s dark-navy and gold theme. After authentication, a four-tab main window becomes available.

-----

#### Login Screen

|Feature          |Detail                                                             |
|-----------------|-------------------------------------------------------------------|
|Background image |`invitation_bg.jpg` — loaded from the `Backgrounds` folder        |
|Authentication   |System password `group3wed!` — shared with all protected operations|
|Fullscreen toggle|Button to switch between fullscreen and windowed mode              |
|CSS theme        |Dark navy `rgba(10,18,40,0.88)` card with gold `#c8a850` accents   |


> ⚠️ `invitation_bg.jpg` must be placed in the `Backgrounds` folder alongside the other module backgrounds. If it is missing, the login window opens without a background but remains fully functional.

-----

#### Data Structures

|Symbol       |Kind    |Fields                                                                                                                                                                                              |
|-------------|:------:|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|`Theme`      |`struct`|`name` · `bg_r/g/b` · `accent_r/g/b` · `text_r/g/b` · `header_r/g/b`                                                                                                                                |
|`InvData`    |`struct`|`husband[100]` · `h_parents[120]` · `wife[100]` · `w_parents[120]` · `venue[200]` · `date[30]` · `dresscode[80]` · `t_council[40]` · `t_church[40]` · `t_reception[40]` · `msg[500]` · `theme` (int)|
|`MailPayload`|`struct`|`data` · `pos` — used internally by the libcurl read callback                                                                                                                                       |

-----

#### Colour Themes

Five built-in themes are available via a dropdown on the Create Invitation tab. The selected theme is applied **live** to the card preview and saved with the invitation record.

|Index|Theme Name       |Background   |Accent       |Notes                                 |
|:---:|-----------------|-------------|-------------|--------------------------------------|
|0    |**Royal Gold**   |Cream ivory  |Gold         |Default theme — dark navy header      |
|1    |**Rose Pink**    |Blush        |Deep rose    |Ideal for romantic/feminine aesthetics|
|2    |**Midnight Blue**|Ice blue     |Midnight blue|Elegant and formal                    |
|3    |**Emerald Green**|Mint         |Emerald      |Fresh, garden-party style             |
|4    |**Lavender**     |Pale lavender|Violet       |Soft and contemporary                 |

-----

#### Cairo Card Renderer — `draw_card_from_data()`

The card preview is rendered entirely using the **Cairo 2D graphics library** at a portrait A5 aspect ratio (595 × 840). Every element is drawn programmatically — no image assets are required for the card itself.

|Layer             |Description                                                     |
|------------------|----------------------------------------------------------------|
|Card background   |Themed cream/ivory fill with a soft radial warm wash            |
|Drop shadow       |Semi-transparent grey offset rectangle for depth                |
|Gold border       |Double-line rounded rectangle in the accent colour              |
|Mandala medallion |Subtle watermark lace pattern at the top centre                 |
|Marigold garland  |Curved vine with orange and red blooms across the top           |
|Side hanging vines|Botanical sprigs with leaves running down both sides            |
|Diya lamps        |Terracotta oil lamps with animated flame at the bottom corners  |
|Pink lotus        |Decorative flower at the bottom centre                          |
|Calligraphic text |Couple names in large italic serif, parents lines, event details|
|Decorative rules  |Thin gold horizontal dividers between text sections             |

-----

#### Functions

**Utility Functions**

|Function                                      |Returns|Description                                                        |
|----------------------------------------------|:-----:|-------------------------------------------------------------------|
|`invitation_get_next_id()`                    |`int`  |Scans `invitations.csv` for the highest ID; returns `max + 1`      |
|`load_guests_from_csv()`                      |`void` |Loads up to 500 guest names and e-mail addresses from `persons.csv`|
|`rebuild_guest_dropdown()`                    |`void` |Repopulates the guest `GtkDropDown` from the in-memory name array  |
|`get_textview_text(tv, buf, size)`            |`void` |Extracts plain text from a `GtkTextView` into a C string buffer    |
|`card_line(buf, bufsize, inner, text)`        |`void` |Builds a UTF-8–safe bordered card line with correct column padding |
|`card_line_centred(buf, bufsize, inner, text)`|`void` |Same as `card_line()` but centres the text within the column width |

**Cairo Drawing Helpers**

|Function                                              |Returns|Description                                                     |
|------------------------------------------------------|:-----:|----------------------------------------------------------------|
|`draw_rounded_rect(cr, x, y, w, h, r)`                |`void` |Draws a rounded-corner rectangle path                           |
|`draw_text_centred(cr, cx, y, text)`                  |`void` |Renders text horizontally centred at the given coordinates      |
|`draw_thin_rule(cr, cx, y, half_len, r, g, b)`        |`void` |Draws a thin decorative horizontal rule                         |
|`draw_leaf(cr, ox, oy, angle, scale, r, g, b, alpha)` |`void` |Draws a single stylised leaf using a cubic Bézier curve         |
|`draw_floral_sprig(cr, bx, by, scale, r, g, b, alpha)`|`void` |Draws a three-leaf botanical sprig on a centre stem             |
|`draw_oval_frame(cr, cx, cy, rw, rh, ar, ag, ab)`     |`void` |Draws the double-line oval/rounded-rectangle decorative border  |
|`draw_card_from_data(cr, width, height, inv)`         |`void` |Master card renderer — draws all layers from an `InvData` struct|

**GTK Callbacks**

|Function                   |Access|Description                                                                                                    |
|---------------------------|:----:|---------------------------------------------------------------------------------------------------------------|
|`on_preview_draw()`        |🔓     |Live preview draw callback — populates `InvData` from form fields and calls `draw_card_from_data()`            |
|`on_field_changed()`       |🔓     |Triggers a preview redraw whenever any `GtkEntry` field is modified                                            |
|`on_message_changed()`     |🔓     |Triggers a preview redraw when the personal message `GtkTextView` changes                                      |
|`on_theme_changed()`       |🔓     |Updates `current_theme` and redraws the preview when the theme dropdown changes                                |
|`on_save_invitation()`     |🔓     |Validates form fields and appends one record to `invitations.csv`; clears the form on success                  |
|`on_refresh_display()`     |🔑     |Password-protected — reads `invitations.csv` and renders each saved invitation as a card inside the Display tab|
|`on_disp_card_draw()`      |🔑     |Draw callback for individual saved invitation cards in the Display tab                                         |
|`on_search_guest_emails()` |🔓     |Reloads `persons.csv` and populates a scrollable guest list showing names and e-mail addresses                 |
|`on_guest_row_activated()` |🔓     |Fills the recipient e-mail entry when a guest row is clicked in the Send tab                                   |
|`on_send_email()`          |🔑     |Password-protected — looks up an invitation by ID and sends it to a single recipient via Gmail SMTP            |
|`on_send_to_all_guests()`  |🔑     |Password-protected — sends the same invitation to every guest in `persons.csv` that has an e-mail address      |
|`on_delete_invitation()`   |🔑     |Password-protected — removes an invitation record by ID using a temp-file swap                                 |
|`on_login_button_clicked()`|—     |Validates the password on the login screen; opens the main window on success                                   |
|`show_login_window()`      |—     |Builds and presents the login overlay window; entry point called by `GApplication::activate`                   |

**E-mail Functions (libcurl)**

|Function                                                    |Returns   |Description                                                                     |
|------------------------------------------------------------|:--------:|--------------------------------------------------------------------------------|
|`curl_read_cb(ptr, size, nmemb, userp)`                     |`size_t`  |libcurl read callback — streams the RFC 2822 message body to the SMTP connection|
|`send_gmail(sender, app_password, recipient, subject, body)`|`CURLcode`|Sends one e-mail via Gmail SMTP over TLS/SSL (`smtps://smtp.gmail.com:465`)     |

-----

#### Application Tabs

|Tab                    |Access|Description                                                                          |
|-----------------------|:----:|-------------------------------------------------------------------------------------|
|**Create Invitation**  |🔓     |Form with all invitation fields + live Cairo card preview on the right               |
|**Display Invitations**|🔑     |Scrollable gallery of all saved invitations rendered as decorated cards              |
|**Send E-mail**        |🔑     |Send a saved invitation by ID to one recipient or to all guests with e-mail addresses|
|**Delete Invitation**  |🔑     |Remove a saved invitation record by its ID                                           |

-----

#### CSV File Format — `invitations.csv`

Each saved invitation is stored as a single CSV row:

```
id, husband, h_parents, wife, w_parents, venue, date, theme_index,
dresscode, time_council, time_church, time_reception, message
```

**Field notes:**

- `theme_index` — integer 0–4 corresponding to the five built-in colour themes
- `message` — personal message; internal newlines are encoded as `|` to keep the row single-line
- `h_parents` / `w_parents` — internal commas are replaced with `;` to prevent CSV breakage
- Optional fields (`dresscode`, `time_council`, `time_church`, `time_reception`) are stored as `"-"` when left blank

-----

#### Compile Command

```bash
gcc gtkinvitations.c -o invitation \
    $(pkg-config --cflags --libs gtk4) -lcurl -lm
```

> 📦 **Required libraries:** GTK 4 (`libgtk-4-dev`), libcurl (`libcurl4-openssl-dev`), and libm (standard C math — included by default on Linux). On Windows via MSYS2 UCRT64, install `mingw-w64-ucrt-x86_64-curl` in addition to the standard GTK 4 packages.

-----

#### E-mail Setup Notes

The Send E-mail tab uses **libcurl with Gmail SMTP over TLS/SSL**. To use it:

1. Enable **2-Step Verification** on the sender Gmail account.
1. Generate a **16-character App Password** (Google Account → Security → App Passwords).
1. Enter the Gmail address and App Password in the Send E-mail tab at runtime — credentials are **never stored** to disk.

|Scenario                                            |Result                              |
|----------------------------------------------------|:----------------------------------:|
|Valid Gmail address + App Password, guest has e-mail|✅ E-mail sent                       |
|Incorrect password field                            |❌ Blocked — “Incorrect password!”   |
|Recipient e-mail field empty                        |❌ Blocked — prompts to enter address|
|No invitation found for given ID                    |❌ “Invitation ID not found.”        |
|`persons.csv` has no guests with e-mail             |⚠️ “Send to All” reports 0 sent      |


> 🔒 **Security note:** Gmail credentials are entered at runtime and never written to `invitations.csv` or any other file. The App Password grants access only to Gmail SMTP — it does not expose the full Google account.

-----

### 🚀 Launchers

|File                  |Interface|Theme / Style                       |Behaviour                                                            |
|----------------------|:-------:|------------------------------------|---------------------------------------------------------------------|
|`combinedmodules.c`   |CLI      |ASCII border + banner               |`system()` calls `./person`, `./category`, `./parking`               |
|`gtkcombinedmodules.c`|GTK 4    |Dark-navy `#0d1b2e` + gold `#c9a84c`|Clickable module cards with hover animation → launches all 4 GTK GUIs|

-----

## 🔗 Module Interconnections

### Dependency Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                        WEDDING GUEST SYSTEM                              │
│                                                                          │
│   ┌────────────────┐      reads        ┌──────────────────────┐         │
│   │  PERSON MODULE │ ────────────────► │   CATEGORY MODULE    │         │
│   │                │   persons.csv     │  (assigns guests     │         │
│   │  persons.csv   │                   │   to named groups)   │         │
│   └───────┬────────┘                   └──────────┬───────────┘         │
│           │ reads                                 │ reads               │
│           ▼                                       ▼                     │
│   ┌──────────────────────────────────────────────────────────┐         │
│   │                      GIFT MODULE                         │         │
│   │   persons.csv  ──►  guest lookup by name                 │         │
│   │   category.csv ──►  auto-resolve guest category          │         │
│   │                ──►  gifts.csv  (output)                  │         │
│   └──────────────────────────────────────────────────────────┘         │
│                                                                          │
│   ┌────────────────┐      reads        ┌──────────────────────┐         │
│   │  PERSON MODULE │ ────────────────► │   PARKING MODULE     │         │
│   │  (sets parking │   parking flag    │  parking_spot.csv    │         │
│   │     flag)      │                   │  parking_booking.csv │         │
│   └────────────────┘                   └──────────────────────┘         │
└──────────────────────────────────────────────────────────────────────────┘
```

### Recommended Workflow

|Step |Module        |Action                                                         |Output file                               |
|:---:|--------------|---------------------------------------------------------------|------------------------------------------|
|**1**|🧑‍🤝‍🧑 **Person**  |Register all wedding guests                                    |`persons.csv`                             |
|**2**|🏷️ **Category**|Create groups, assign guests by ID                             |`category.csv`                            |
|**3**|🎁 **Gift**    |Record contributions *(requires steps 1 & 2)*                  |`gifts.csv`                               |
|**4**|🚗 **Parking** |Set parking flags in Person → configure spots → accept bookings|`parking_spot.csv` · `parking_booking.csv`|


> ⚠️ **Important:** The Gift module will fail to look up a guest if they are not yet registered **and** categorised. Complete steps 1 and 2 first.

-----

## 🔧 Installation

### 🪟 Windows — MSYS2 UCRT64

|#  |Step             |Action                                                                         |
|:-:|-----------------|-------------------------------------------------------------------------------|
|1  |**Download**     |Visit [msys2.org](https://www.msys2.org) → download `msys2-x86_64-YYYYMMDD.exe`|
|2  |**Install**      |Run the installer — keep default path `C:\msys64`                              |
|3  |**Open terminal**|Start Menu → **MSYS2 UCRT64** (gold icon) — *not* MINGW32 or MSYS              |
|4  |**Update**       |Run `pacman -Syu` → terminal closes → reopen → run `pacman -Su`                |
|5  |**Install GTK 4**|Run the command below                                                          |
|6  |**Verify**       |`pkg-config --modversion gtk4` should print `4.x.x` · `gcc --version` shows GCC|

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make
```

<img width="900" height="230" alt="MSYS2 installer download page" src="https://github.com/user-attachments/assets/081d6c21-92ee-41fa-aa08-53c1891eb530" />

<img width="832" height="170" alt="MSYS2 UCRT64 terminal" src="https://github.com/user-attachments/assets/0e2686f2-aad5-4d58-b8b4-fdd96a9f5854" />

<img width="832" height="170" alt="GTK 4 version verified" src="https://github.com/user-attachments/assets/a3f8ed5a-f82b-4b33-8ba4-76bc9b6dad46" />

-----

### 🐧 Linux — Ubuntu / Debian

|#  |Step                         |Command                                            |
|:-:|-----------------------------|---------------------------------------------------|
|1  |**Update package lists**     |`sudo apt update`                                  |
|2  |**Install GTK 4 + toolchain**|`sudo apt install libgtk-4-dev gcc make pkg-config`|
|3  |**Verify**                   |`pkg-config --modversion gtk4` · `gcc --version`   |

<img width="793" height="196" alt="apt install GTK 4" src="https://github.com/user-attachments/assets/ed4c485a-c227-41ab-9d3d-f14fd6fd1888" />

<img width="793" height="196" alt="GTK 4 version verified on Linux" src="https://github.com/user-attachments/assets/cb22e8a9-a5ec-4ec0-87e7-dec7d2b60ddd" />

-----

## 🛠️ Build & Run

### Clone & Build

```bash
git clone https://github.com/your-username/wedding-guest-system.git
cd wedding-guest-system
make          # compiles all CLI + GTK 4 binaries in one step
make clean    # removes all compiled binaries and temp CSV files
```

<img width="594" height="173" alt="make build output" src="https://github.com/user-attachments/assets/c48d1a40-d1eb-4548-a1ac-fcbfc4247899" />

<img width="594" height="173" alt="make clean output" src="https://github.com/user-attachments/assets/f3cc2343-5c8f-4be3-afa5-0307c5177834" />

### Make Targets

|Command               |Type |What it builds                            |
|----------------------|:---:|------------------------------------------|
|`make person`         |CLI  |Terminal guest manager binary             |
|`make category`       |CLI  |Terminal category manager binary          |
|`make gift`           |CLI  |Terminal gift manager binary              |
|`make combinedmodules`|CLI  |CLI launcher — person · category · parking|
|`make gtkperson`      |GTK 4|GUI guest management window               |
|`make gtkcategory`    |GTK 4|GUI category management window            |
|`make gift_gtk03`     |GTK 4|GUI gift management window                |
|`make gtkparking`     |GTK 4|GUI parking management window             |
|`make launcher`       |GTK 4|Main graphical launcher (all modules)     |
|`make clean`          |—    |Removes all binaries and temp CSV files   |

### Run Commands

|Command            |Interface|Description                                               |
|-------------------|:-------:|----------------------------------------------------------|
|`./launcher` ⭐     |GTK 4    |**Recommended** — graphical portal to all four module GUIs|
|`./gtkperson`      |GTK 4    |Guest management GUI                                      |
|`./gtkcategory`    |GTK 4    |Category management GUI                                   |
|`./gift_gtk03`     |GTK 4    |Gift management GUI                                       |
|`./gtkparking`     |GTK 4    |Parking management GUI (admin + guest panels)             |
|`./combinedmodules`|CLI      |Text-based launcher — opens person / category / parking   |
|`./person`         |CLI      |Full terminal guest management                            |
|`./category`       |CLI      |Full terminal category management                         |
|`./gift`           |CLI      |Full terminal gift management                             |
|`./parking`        |CLI      |Full terminal parking management                          |


> ⚠️ **MSYS2 users:** always launch from the **UCRT64** terminal — not PowerShell, CMD, or MINGW32.

### ⚡ Quick-Start Cheatsheet

```bash
# ── Windows (MSYS2 UCRT64) ──────────────────────────────────────────────
cd /c/Users/YourName/wedding-guest-system
make
./launcher          # GTK graphical interface (recommended)

# ── Linux ────────────────────────────────────────────────────────────────
cd ~/wedding-guest-system
make
./launcher          # GTK graphical interface (recommended)

# ── Run individual modules ───────────────────────────────────────────────
./gtkperson         # Guest GUI
./gtkcategory       # Category GUI
./gift_gtk03        # Gift GUI
./gtkparking        # Parking GUI

# ── CLI alternative ──────────────────────────────────────────────────────
./combinedmodules   # CLI launcher
./parking           # CLI parking only
```

-----

## 📄 CSV File Formats

All CSV files live in the `csv/` folder.

|File                 |Row Format                                                                                                                        |
|---------------------|----------------------------------------------------------------------------------------------------------------------------------|
|`persons.csv`        |`id, name, age, status, phone, side, parking`                                                                                     |
|`gifts.csv`          |`record_id, guest_id, guest_name, category, item_id, item_name, qty, total_fcfa, total_eur, thanked_by`                           |
|`parking_spot.csv`   |`spot_id, max_hours`                                                                                                              |
|`parking_booking.csv`|`booking_id, spot_id, guest_name, start_hour, start_min, duration_hours, end_hour, end_min`                                       |
|`invitations.csv`    |`id, husband, h_parents, wife, w_parents, venue, date, theme_index, dresscode, time_council, time_church, time_reception, message`|

`category.csv` uses a **two-line format** — each `CAT` block is followed by its `ID` members:

```
CAT,id,code,guest_count
ID,guest_id
ID,guest_id
...
```

**Field notes:**

- `side` — `0` = Groom · `1` = Bride · `2` = Both
- `start_hour` / `end_hour` — 24-hour format (0–23)
- `end_hour` / `end_min` — computed automatically from `start + duration`
- `parking_spot.csv` — one row per configurable spot; supports up to 200 spots

-----

<div align="center">

💍 **Wedding Guest System** · Group 3 · v1.0 · Built in C with GTK 4

*Manage your wedding, one CSV row at a time.*

</div>