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
