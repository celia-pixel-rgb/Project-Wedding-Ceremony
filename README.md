| `print_all_spots(num_spots)` | `void` | Displays `FREE` / `OCCUPIED` status for every configured spot |
| `print_guest_bookings(guest_name)` | `void` | Lists all bookings in `parking_booking.csv` for a guest |
| `run_admin_menu()` | `void` | 🔑 Configure spots & max hours, view occupancy, cancel bookings by ID |
| `run_guest_menu(guest_name, num_spots, max_hours)` | `void` | 🔓 Book a slot, view own bookings, cancel own booking |

#### GTK Parking Panels — `gtkparking.c`

| Panel | Access | Features |
|---|:---:|---|
| **Admin Panel** | 🔑 | Set spot count & max hours · real-time occupancy via `g_timeout_add()` · cancel any booking by ID |
| **Guest Panel** | 🔓 | Name entry → verified vs `persons.csv` → choose start time & duration → auto-assigned spot → confirmed |

---

### 🚀 Launchers

| File | Interface | Theme / Style | Behaviour |
|---|:---:|---|---|
| `combinedmodules.c` | CLI | ASCII border + banner | `system()` calls `./person`, `./category`, `./parking` |
| `gtkcombinedmodules.c` | GTK 4 | Dark-navy `#0d1b2e` + gold `#c9a84c` | Clickable module cards with hover animation → launches all 4 GTK GUIs |

---

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

| Step | Module | Action | Output file |
|:---:|---|---|---|
| **1** | 🧑‍🤝‍🧑 **Person** | Register all wedding guests | `persons.csv` |
| **2** | 🏷️ **Category** | Create groups, assign guests by ID | `category.csv` |
| **3** | 🎁 **Gift** | Record contributions *(requires steps 1 & 2)* | `gifts.csv` |
| **4** | 🚗 **Parking** | Set parking flags in Person → configure spots → accept bookings | `parking_spot.csv` · `parking_booking.csv` |

> ⚠️ **Important:** The Gift module will fail to look up a guest if they are not yet registered **and** categorised. Complete steps 1 and 2 first.

---

## 🔧 Installation

### 🪟 Windows — MSYS2 UCRT64

| # | Step | Action |
|:---:|---|---|
| 1 | **Download** | Visit [msys2.org](https://www.msys2.org) → download `msys2-x86_64-YYYYMMDD.exe` |
| 2 | **Install** | Run the installer — keep default path `C:\msys64` |
| 3 | **Open terminal** | Start Menu → **MSYS2 UCRT64** (gold icon) — *not* MINGW32 or MSYS |
| 4 | **Update** | Run `pacman -Syu` → terminal closes → reopen → run `pacman -Su` |
| 5 | **Install GTK 4** | Run the command below |
| 6 | **Verify** | `pkg-config --modversion gtk4` should print `4.x.x` · `gcc --version` shows GCC |

```bash
pacman -S mingw-w64-ucrt-x86_64-gtk4 \
          mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-pkg-config \
          make
```

<img width="900" height="230" alt="MSYS2 installer download page" src="https://github.com/user-attachments/assets/081d6c21-92ee-41fa-aa08-53c1891eb530" />

<img width="832" height="170" alt="MSYS2 UCRT64 terminal" src="https://github.com/user-attachments/assets/0e2686f2-aad5-4d58-b8b4-fdd96a9f5854" />

<img width="832" height="170" alt="GTK 4 version verified" src="https://github.com/user-attachments/assets/a3f8ed5a-f82b-4b33-8ba4-76bc9b6dad46" />

---

### 🐧 Linux — Ubuntu / Debian

| # | Step | Command |
|:---:|---|---|
| 1 | **Update package lists** | `sudo apt update` |
| 2 | **Install GTK 4 + toolchain** | `sudo apt install libgtk-4-dev gcc make pkg-config` |
| 3 | **Verify** | `pkg-config --modversion gtk4` · `gcc --version` |

<img width="793" height="196" alt="apt install GTK 4" src="https://github.com/user-attachments/assets/ed4c485a-c227-41ab-9d3d-f14fd6fd1888" />

<img width="793" height="196" alt="GTK 4 version verified on Linux" src="https://github.com/user-attachments/assets/cb22e8a9-a5ec-4ec0-87e7-dec7d2b60ddd" />

---

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

| Command | Type | What it builds |
|---|:---:|---|
| `make person` | CLI | Terminal guest manager binary |
| `make category` | CLI | Terminal category manager binary |
| `make gift` | CLI | Terminal gift manager binary |
| `make combinedmodules` | CLI | CLI launcher — person · category · parking |
| `make gtkperson` | GTK 4 | GUI guest management window |
| `make gtkcategory` | GTK 4 | GUI category management window |
| `make gift_gtk03` | GTK 4 | GUI gift management window |
| `make gtkparking` | GTK 4 | GUI parking management window |
| `make launcher` | GTK 4 | Main graphical launcher (all modules) |
| `make clean` | — | Removes all binaries and temp CSV files |

### Run Commands

| Command | Interface | Description |
|---|:---:|---|
| `./launcher` ⭐ | GTK 4 | **Recommended** — graphical portal to all four module GUIs |
| `./gtkperson` | GTK 4 | Guest management GUI |
| `./gtkcategory` | GTK 4 | Category management GUI |
| `./gift_gtk03` | GTK 4 | Gift management GUI |
| `./gtkparking` | GTK 4 | Parking management GUI (admin + guest panels) |
| `./combinedmodules` | CLI | Text-based launcher — opens person / category / parking |
| `./person` | CLI | Full terminal guest management |
| `./category` | CLI | Full terminal category management |
| `./gift` | CLI | Full terminal gift management |
| `./parking` | CLI | Full terminal parking management |

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

---

## 📄 CSV File Formats

All CSV files live in the `csv/` folder.

| File | Row Format |
|---|---|
| `persons.csv` | `id, name, age, status, phone, side, parking` |
| `gifts.csv` | `record_id, guest_id, guest_name, category, item_id, item_name, qty, total_fcfa, total_eur, thanked_by` |
| `parking_spot.csv` | `spot_id, max_hours` |
| `parking_booking.csv` | `booking_id, spot_id, guest_name, start_hour, start_min, duration_hours, end_hour, end_min` |

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

---

<div align="center">

💍 **Wedding Guest System** · Group 3 · v1.0 · Built in C with GTK 4

*Manage your wedding, one CSV row at a time.*

</div>
