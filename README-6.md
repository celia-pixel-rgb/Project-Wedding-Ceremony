# 💍 Wedding Guest System — Group 3

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![GTK](https://img.shields.io/badge/GTK_4-4A90D9?style=for-the-badge&logo=gtk&logoColor=white)
![CSV](https://img.shields.io/badge/CSV-Storage-217346?style=for-the-badge&logo=files&logoColor=white)
![MSYS2](https://img.shields.io/badge/MSYS2-UCRT64-1C1C1C?style=for-the-badge&logo=windows-terminal&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-Ubuntu%2FDebian-E95420?style=for-the-badge&logo=ubuntu&logoColor=white)

> Multi-module wedding management app in **C** — CLI + GTK 4 GUI, all data shared via CSV.

> 🔑 **Password:** `group3wed!`

---

## 📋 Table of Contents

1. [Overview](#-overview)
2. [Repository Structure](#-repository-structure)
3. [Module Functions](#️-module-functions)
4. [Module Interconnections](#-module-interconnections)
5. [Installation](#-installation)
6. [Build & Run](#️-build--run)
7. [CSV File Formats](#-csv-file-formats)

---

## 💍 Overview

| Module | Status | Description |
|---|---|---|
| 🧑‍🤝‍🧑 **Person** | ✅ Complete | Register, view, update, delete guests with full field validation |
| 🏷️ **Category** | ✅ Complete | Organise guests into nested linked-list groups (VIP, Family, Friends…) |
| 🎁 **Gift** | ✅ Complete | 25-item catalogue in FCFA & EUR with 6 payment methods |
| 🚗 **Parking** | ✅ Complete | Spot booking with admin config, overlap detection, guest verification |

---

## 📁 Repository Structure

```
wedding-guest-system/
│
├── person.h / person.c           # Guest struct, CLI guest management
├── category.h / category.c       # Nested linked-list category manager
├── gift.h / gift.c               # CLI gift manager (25-item catalogue)
├── parking.h / parking.c         # CLI parking: config, booking, overlap detection
│
├── gtkperson.c                   # GTK 4 — guest management
├── gtkcategory.c                 # GTK 4 — category management
├── gift_gtk03.c                  # GTK 4 — gift management
├── gtkparking.c                  # GTK 4 — parking management
├── gtkcombinedmodules.c          # GTK 4 — main launcher (dark-navy/gold theme)
│
├── combinedmodules.c             # CLI launcher
│
├── csv/
│   ├── persons.csv               # Guest records
│   ├── category.csv              # Categories & assignments
│   ├── gifts.csv                 # Gift registrations
│   ├── parking_spot.csv          # Admin spot config
│   └── parking_booking.csv       # Active bookings
│
└── Makefile
```

---

## ⚙️ Module Functions

### 🧑‍🤝‍🧑 Module 01 — Person

**Files:** `person.h` · `person.c` · `gtkperson.c`

#### Data Structures

| Symbol | Kind | Description |
|---|---|---|
| `Side` | `enum` | `GROOM (0)`, `BRIDE (1)`, `BOTH (2)` |
| `Guest` | `struct` | `id`, `name[100]`, `age`, `status[50]`, `phone[20]`, `Side side`, `parking[10]` |

#### Functions

| Function | Returns | Access | Description |
|---|---|---|---|
| `side_to_string(Side s)` | `const char*` | Public | Converts `Side` to `"Groom"` / `"Bride"` / `"Both"` |
| `get_next_id()` | `int` | Public | Returns `max_id + 1` from `persons.csv` |
| `action_add_guest()` | `void` | Open | Prompts, validates, appends new guest row |
| `action_show_guests()` | `void` | 🔑 | Reads and renders all guests as a table |
| `action_delete_guest()` | `void` | 🔑 | Removes guest by ID, renumbers remaining IDs |
| `action_update_guest()` | `void` | 🔑 | Overwrites any field (blank = keep original) |

---

### 🏷️ Module 02 — Category

**Files:** `category.h` · `category.c` · `gtkcategory.c`

#### Data Structures

| Symbol | Kind | Description |
|---|---|---|
| `GuestRef` | `struct` | Inner list node: `guest_id`, `*next` |
| `Category` | `struct` | Outer list node: `id`, `code[50]`, `*guests`, `guest_count`, `*next` |

#### Menu Actions

| Function | Returns | Access | Description |
|---|---|---|---|
| `action_create_category()` | `void` | Open | Creates new category node, saves to `category.csv` |
| `action_assign_guest()` | `void` | Open | Assigns guest to category; prints return-code message |
| `action_display()` | `void` | 🔑 | Shows all categories with nested guest lists |
| `action_sort_display()` | `void` | 🔑 | Sorts by guest count desc, persists, displays |
| `action_update_category()` | `void` | 🔑 | Renames category code by ID |
| `action_delete_category()` | `void` | 🔑 | Removes category and frees its GuestRef list |
| `action_remove_guest()` | `void` | 🔑 | Unlinks a single guest from a specific category |

#### Persistence

| Function | Returns | Description |
|---|---|---|
| `load_categories_from_csv()` | `void` | Rebuilds full nested linked list from `category.csv` |
| `save_categories_to_csv()` | `void` | Atomic write via `category_tmp.csv` → rename |

#### `assign_guest_to_category()` Return Codes

| Code | Meaning |
|---|---|
| `+1` | ✅ Guest successfully assigned |
| `0` | ⚠️ Category ID not found or `malloc()` failed |
| `-1` | ❌ Guest ID not in `persons.csv` |
| `-2` | 🚫 Guest already assigned (duplicate blocked) |

---

### 🎁 Module 03 — Gift

**Files:** `gift.h` · `gift.c` · `gift_gtk03.c`

#### Data Structures

| Symbol | Kind | Description |
|---|---|---|
| `Guest` | `struct` | Linked-list node: `name[100]`, `*next` |
| `Gift` | `struct` | `id`, `name[100]`, `price`, `max_quantity`, `taken_quantity`, `*guests`, `*next` |
| `GiftItem` | `struct` (GTK) | Catalogue entry: `id`, `name`, `price_fcfa` |
| `GiftRecord` | `struct` (GTK) | `record_id`, `guest_id`, `guest_name`, `category`, `item_id`, `item_name`, `qty`, `total_fcfa`, `total_eur`, `thanked_by` |

#### Shared Functions — `gift.h`

| Function | Returns | Description |
|---|---|---|
| `create_gift(id, name, price, max_qty)` | `Gift*` | Allocates new Gift node; `NULL` on failure |
| `add_gift(head, new_gift)` | `void` | Appends to global gift list |
| `find_gift(head, id)` | `Gift*` | Returns pointer to matching gift or `NULL` |
| `add_guest_to_gift(gift, guest_name)` | `void` | Appends Guest node to gift's inner list |
| `display_gifts(head)` | `void` | Prints all gifts with linked guest names |
| `initialize_predefined_gifts(list)` | `void` | Populates list with 25 predefined items |
| `check_password(input)` | `bool` | Returns `true` if password matches |

#### CLI Functions — `gift.c`

| Function | Returns | Access | Description |
|---|---|---|---|
| `showMenu()` | `void` | Open | Prints 26-item catalogue with FCFA prices |
| `addGift()` | `void` | Open | Collects guest name, gift choice, qty; assigns ID suffixed `G` (e.g. `3G`) |
| `displayAll()` | `void` | 🔑 | Lists all gift contributions |
| `displayOne(id)` | `void` | 🔑 | Shows single record by guest ID |
| `deleteGift(id)` | `void` | 🔑 | Removes record, shifts array left |
| `updateGift(id)` | `void` | 🔑 | Re-displays catalogue, recalculates total |
| `authenticate()` | `int` | — | Returns `1` on success |

#### GTK-Only Functions — `gift_gtk03.c`

| Function | Returns | Description |
|---|---|---|
| `find_guest_by_name(name, out)` | `int` | Case-insensitive lookup in `persons.csv` |
| `find_category_for_guest(guest_id, out_code, size)` | `void` | Resolves category from `category.csv`; fills `"Unassigned"` if none |
| `get_next_gift_id()` | `int` | Returns `max_record_id + 1` from `gifts.csv` |
| `write_gift_line(f, g)` | `void` | Writes one `GiftRecord` as CSV row |

> **Payment methods:** Cash · Mobile Money · Orange Money · Airtel Money · Yoomee · Credit Card  
> **Rate:** `1 EUR = 655.957 FCFA`

---

### 🚗 Module 04 — Parking

**Files:** `parking.h` · `parking.c` · `gtkparking.c`

#### Data Structures

| Symbol | Kind | Description |
|---|---|---|
| `SpotConfig` | `struct` | `spot_id` (1-based), `max_hours` |
| `Booking` | `struct` | `booking_id`, `spot_id`, `guest_name[64]`, `start_hour`, `start_min`, `duration_hours`, `end_hour`, `end_min` |

#### `check_person_parking()` Return Codes

| Code | Constant | Meaning |
|---|---|---|
| `0` | `PERSON_NOT_FOUND` | Name not in `persons.csv` |
| `1` | `PERSON_NO_PARKING` | Guest exists but parking = `"No"` |
| `2` | `PERSON_HAS_PARKING` | Guest exists with parking = `"Yes"` |

#### CLI Functions — `parking.c`

| Function | Returns | Description |
|---|---|---|
| `to_minutes(h, m)` | `int` | Hour:minute → minutes since midnight |
| `add_hours(sh, sm, dur_h, *eh, *em)` | `void` | Adds hours to start time, wraps past midnight |
| `intervals_overlap(as, ae, bs, be)` | `int` | Returns `1` if `[as,ae)` overlaps `[bs,be)` |
| `save_spot_config(num_spots, max_hours)` | `void` | Writes admin config to `parking_spot.csv` |
| `load_spot_config(*num_spots, *max_hours)` | `int` | Reads config; returns `1` on success |
| `append_booking(b)` | `void` | Appends booking row to `parking_booking.csv` |
| `next_booking_id()` | `int` | Returns `max_id + 1` (or `1` for empty file) |
| `load_bookings(*out_count)` | `Booking*` | Loads all rows into dynamic array (caller must `free()`) |
| `cancel_booking_by_id(booking_id)` | `int` | Removes row; returns `1` on success |
| `cancel_bookings_for_guest(guest_name)` | `int` | Removes all bookings for guest; returns count removed |
| `check_person_parking(name)` | `int` | Case-insensitive lookup; returns `PERSON_*` code |
| `find_available_spot(req_start, req_end, num_spots)` | `int` | Returns first non-overlapping spot or `-1` |
| `guest_has_overlapping_booking(name, *out, req_start, req_end)` | `int` | Returns `1` if conflict found; fills `*out` |
| `print_all_spots(num_spots)` | `void` | Shows `FREE` / `OCCUPIED` status for all spots |
| `print_guest_bookings(guest_name)` | `void` | Lists all bookings for a guest |
| `run_admin_menu()` | `void` 🔑 | Configure spots, view occupancy, cancel bookings |
| `run_guest_menu(guest_name, num_spots, max_hours)` | `void` | Book, view, or cancel own booking |

#### GTK Parking Panels

| Panel | Access | Description |
|---|---|---|
| **Admin Panel** | 🔑 | Configure spots & max hours; real-time occupancy with auto-refresh; cancel bookings |
| **Guest Panel** | Open | Enter name → verified against `persons.csv` → choose time & duration → auto-assigned spot |

---

### 🚀 Launchers

| File | Interface | Description |
|---|---|---|
| `combinedmodules.c` | CLI | ASCII banner menu; launches `./person`, `./category`, `./parking` via `system()` |
| `gtkcombinedmodules.c` | GTK 4 | Dark-navy `#0d1b2e` + gold `#c9a84c` theme; clickable module cards with hover animation |

---

## 🔗 Module Interconnections

```
┌──────────────────────────────────────────────────────────────────────┐
│                      WEDDING GUEST SYSTEM                            │
│                                                                      │
│   ┌──────────────┐     reads      ┌──────────────────────┐          │
│   │   PERSON     │ ─────────────► │   CATEGORY MODULE    │          │
│   │   MODULE     │  persons.csv   │  (assigns guests     │          │
│   │ persons.csv  │                │   to groups)         │          │
│   └──────┬───────┘                └──────────┬───────────┘          │
│          │ reads                             │ reads                 │
│          ▼                                   ▼                       │
│   ┌──────────────────────────────────────────────────────┐          │
│   │                    GIFT MODULE                       │          │
│   │  persons.csv → guest lookup                          │          │
│   │  category.csv → auto-resolves category               │          │
│   │  gifts.csv → output                                  │          │
│   └──────────────────────────────────────────────────────┘          │
│                                                                      │
│   ┌──────────────┐     reads      ┌──────────────────────┐          │
│   │   PERSON     │ ─────────────► │   PARKING MODULE     │          │
│   │  (sets       │  parking flag  │  parking_spot.csv    │          │
│   │ parking flag)│                │  parking_booking.csv │          │
│   └──────────────┘                └──────────────────────┘          │
└──────────────────────────────────────────────────────────────────────┘
```

#### Recommended Workflow

| Step | Module | Action |
|---|---|---|
| 1 | 🧑‍🤝‍🧑 Person | Register all guests → writes `persons.csv` |
| 2 | 🏷️ Category | Create groups, assign guests → writes `category.csv` |
| 3 | 🎁 Gift | Record contributions (requires steps 1 & 2) → writes `gifts.csv` |
| 4 | 🚗 Parking | Set parking flag in Person, configure spots, accept bookings |

> ⚠️ Gift module requires at least one registered guest **and** one assigned category before creating records.

---

## 🔧 Installation

### 🪟 Windows — MSYS2 UCRT64

| Step | Command / Action |
|---|---|
| 1. Download | [msys2.org](https://www.msys2.org) → Download installer |
| 2. Install | Keep default path `C:\msys64` |
| 3. Open terminal | Start Menu → **MSYS2 UCRT64** (gold icon) — not MINGW32 or MSYS |
| 4. Update | `pacman -Syu` → reopen terminal → `pacman -Su` |
| 5. Install GTK 4 | See command below |
| 6. Verify | `pkg-config --modversion gtk4` → `gcc --version` |

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make
```

<img width="900" height="230" alt="MSYS2 download" src="https://github.com/user-attachments/assets/081d6c21-92ee-41fa-aa08-53c1891eb530" />
<img width="832" height="170" alt="UCRT64 terminal" src="https://github.com/user-attachments/assets/0e2686f2-aad5-4d58-b8b4-fdd96a9f5854" />
<img width="832" height="170" alt="GTK verify" src="https://github.com/user-attachments/assets/a3f8ed5a-f82b-4b33-8ba4-76bc9b6dad46" />

---

### 🐧 Linux — Ubuntu / Debian

| Step | Command |
|---|---|
| 1. Update | `sudo apt update` |
| 2. Install | `sudo apt install libgtk-4-dev gcc make pkg-config` |
| 3. Verify | `pkg-config --modversion gtk4` → `gcc --version` |

<img width="793" height="196" alt="Linux install" src="https://github.com/user-attachments/assets/ed4c485a-c227-41ab-9d3d-f14fd6fd1888" />
<img width="793" height="196" alt="Linux verify" src="https://github.com/user-attachments/assets/cb22e8a9-a5ec-4ec0-87e7-dec7d2b60ddd" />

---

## 🛠️ Build & Run

### Build

```bash
git clone https://github.com/your-username/wedding-guest-system.git
cd wedding-guest-system
make          # builds all CLI + GTK binaries
make clean    # removes binaries and temp CSV files
```

<img width="594" height="173" alt="make build" src="https://github.com/user-attachments/assets/c48d1a40-d1eb-4548-a1ac-fcbfc4247899" />
<img width="594" height="173" alt="make clean" src="https://github.com/user-attachments/assets/f3cc2343-5c8f-4be3-afa5-0307c5177834" />

### Make Targets

| Command | Type | Binary |
|---|---|---|
| `make person` | CLI | Terminal guest manager |
| `make category` | CLI | Terminal category manager |
| `make gift` | CLI | Terminal gift manager |
| `make combinedmodules` | CLI | CLI launcher (person + category + parking) |
| `make gtkperson` | GTK 4 | GUI guest manager |
| `make gtkcategory` | GTK 4 | GUI category manager |
| `make gift_gtk03` | GTK 4 | GUI gift manager |
| `make gtkparking` | GTK 4 | GUI parking manager |
| `make launcher` | GTK 4 | GTK main launcher |
| `make clean` | — | Remove all binaries & temp CSVs |

### Run Commands

| Command | Interface | Description |
|---|---|---|
| `./launcher` ⭐ | GTK 4 | Main portal — opens all module GUIs |
| `./gtkperson` | GTK 4 | Guest management GUI |
| `./gtkcategory` | GTK 4 | Category management GUI |
| `./gift_gtk03` | GTK 4 | Gift management GUI |
| `./gtkparking` | GTK 4 | Parking management GUI |
| `./combinedmodules` | CLI | Text menu → person / category / parking |
| `./person` | CLI | Terminal guest management |
| `./category` | CLI | Terminal category management |
| `./gift` | CLI | Terminal gift management |
| `./parking` | CLI | Terminal parking management |

> ⚠️ **MSYS2:** Always launch from the **UCRT64** terminal.

---

## 📄 CSV File Formats

| File | Row Format |
|---|---|
| `persons.csv` | `id, name, age, status, phone, side (0/1/2), parking` |
| `gifts.csv` | `record_id, guest_id, guest_name, category, item_id, item_name, qty, total_fcfa, total_eur, thanked_by` |
| `parking_spot.csv` | `spot_id, max_hours` |
| `parking_booking.csv` | `booking_id, spot_id, guest_name, start_hour, start_min, duration_hours, end_hour, end_min` |

`category.csv` uses a two-line format — `CAT` lines define groups, `ID` lines assign guests:
```
CAT,id,code,guest_count
ID,guest_id
```

`side` values: `0` = Groom · `1` = Bride · `2` = Both  
Hours are 24-hour format (0–23). `end_hour`/`end_min` computed from start + duration.

---

*💍 Wedding Guest System — Group 3 · v1.0 · Built in C with GTK 4*
