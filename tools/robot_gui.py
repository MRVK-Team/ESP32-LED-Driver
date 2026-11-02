"""Test panel for the ESP32-C3 robot state controller.

Drives the board over the serial link and checks what it answers, so every
state can be walked through without typing commands by hand.

It reads the firmware sources in ../main to find out which states exist and
what each one is supposed to draw, so the state list here can never drift
away from robot_state.c - add a state to the firmware and it turns up here.
The 8x8 preview replays the same frames as led_animator.c, which is what
makes a visual check possible: compare the preview with the real panel.

    Run full test   sends SET_STATE and GET_STATE for every state, sweeps the
                    three mission brightness levels and checks that a bad
                    state name is still refused. With "ask for visual
                    verdict" ticked it stops on each state so you can look at
                    the panel and say whether the picture is right.

    A/B compare     flips between two states on a timer. Set it to LOAD_BEER
                    and UNLOAD_BEER, stand three metres back, and see whether
                    the two can be told apart - the check that matters most.

Run it with any Python that has pyserial. The ESP-IDF environment already
has one:

    C:\\Espressif\\tools\\python\\v6.1\\venv\\Scripts\\python.exe tools\\robot_gui.py

or install it yourself:

    pip install pyserial
    python tools/robot_gui.py

Only one program can hold the serial port, so close Tera Term, idf.py
monitor and robot.ps1 first.
"""

import queue
import re
import threading
import time
import tkinter as tk
from pathlib import Path
from tkinter import ttk

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    raise SystemExit(
        "pyserial is missing. Install it with:  pip install pyserial\n"
        "or run this with the ESP-IDF python:\n"
        r"  C:\Espressif\tools\python\v6.1\venv\Scripts\python.exe tools\robot_gui.py"
    )

BAUD = 115200
FIRMWARE_DIR = Path(__file__).resolve().parent.parent / "main"

# How long to wait for a reply before calling a check failed.
REPLY_TIMEOUT_S = 1.5

# The three levels the mission uses: scan, normal, confirm.
MISSION_BRIGHTNESS = (20, 80, 200)

# Used only if the firmware sources cannot be read - the GUI still works,
# just without the previews.
FALLBACK_STATES = [
    "LOAD_BEER", "AUTONOMOUS_DRIVING", "FAULT", "MISSION_COMPLETE", "SCAN_QR",
    "UNLOAD_BEER", "QR_CONFIRM", "ACCEPTED", "REJECTED", "RETURNING",
    "WAITING", "NO_SIGNAL",
]

# Mission order, so a run reads like a delivery instead of like an enum.
# Anything the firmware has that is not listed here is appended at the end,
# so a newly added state is still tested.
MISSION_ORDER = [
    "WAITING", "SCAN_QR", "QR_CONFIRM", "ACCEPTED", "REJECTED",
    "AUTONOMOUS_DRIVING", "LOAD_BEER", "UNLOAD_BEER", "RETURNING",
    "MISSION_COMPLETE", "NO_SIGNAL", "FAULT",
]

# The pair a person must never confuse. Used as the A/B defaults.
KEY_COMPARISON = ("LOAD_BEER", "UNLOAD_BEER")


# ---------------------------------------------------------------------------
# Reading the firmware sources
# ---------------------------------------------------------------------------

def _strip_comments(text):
    """Comments hide old pattern rows that are still in the file - dropping
    them first keeps commented-out frames out of the preview."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


class Firmware:
    """What the sources in main/ say about states, pictures and timing."""

    def __init__(self, main_dir):
        self.states = []            # wire names, in enum order
        self.patterns = {}          # PAT_NAME -> 8 rows of 8 (r, g, b)
        self.animations = {}        # state name -> [(PAT_NAME or None, hold_ms)]
        self.problem = None

        try:
            self._load(Path(main_dir))
        except Exception as exc:            # unreadable, moved, renamed
            self.problem = f"{type(exc).__name__}: {exc}"

        if not self.states:
            self.states = list(FALLBACK_STATES)
            if self.problem is None:
                self.problem = "no states found in robot_state.c"

    # --- loading --------------------------------------------------------

    def _load(self, main_dir):
        self.states = self._parse_states(main_dir / "robot_state.c")
        defines = self._parse_defines(main_dir / "config.h")
        self.patterns = self._parse_patterns(main_dir / "patterns.c")
        self.animations = self._parse_animations(main_dir / "led_animator.c",
                                                 defines)

    @classmethod
    def _source(cls, path):
        return _strip_comments(path.read_text(encoding="utf-8", errors="replace"))

    @classmethod
    def _parse_states(cls, path):
        """[ROBOT_STATE_LOAD_BEER] = "LOAD_BEER","""
        src = cls._source(path)
        return [
            m.group(2)
            for m in re.finditer(r"\[ROBOT_STATE_(\w+)\]\s*=\s*\"([^\"]+)\"", src)
        ]

    @classmethod
    def _parse_defines(cls, path):
        """The frame tables hold times by name, e.g. LOAD_BEER_FILL_MS."""
        src = cls._source(path)
        return {
            m.group(1): int(m.group(2))
            for m in re.finditer(r"^#define\s+(\w+)\s+(\d+)\s*$", src, re.M)
        }

    @classmethod
    def _parse_patterns(cls, path):
        src = cls._source(path)

        palette = {
            m.group(1): (int(m.group(2)), int(m.group(3)), int(m.group(4)))
            for m in re.finditer(
                r"\[(CLR_\w+)\]\s*=\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", src
            )
        }
        # The one-letter macros the pattern tables are written in.
        letters = {
            m.group(1): m.group(2)
            for m in re.finditer(r"^#define\s+(\w+)\s+(CLR_\w+)", src, re.M)
        }

        off = palette.get("CLR_OFF", (0, 0, 0))
        patterns = {}
        for m in re.finditer(
            r"const\s+pattern_t\s+(PAT_\w+)\s*=\s*\{(.*?)\n\};", src, re.S
        ):
            name, body = m.group(1), m.group(2)
            rows = re.findall(r"\{([^{}]*)\}", body)[:8]
            if len(rows) != 8:
                continue
            grid = []
            for row in rows:
                cells = [c.strip() for c in row.split(",") if c.strip()]
                if len(cells) != 8:
                    grid = []
                    break
                grid.append([palette.get(letters.get(c, c), off) for c in cells])
            if grid:
                patterns[name] = grid
        return patterns

    @classmethod
    def _parse_animations(cls, path, defines):
        src = cls._source(path)

        # static const frame_t s_load_beer[] = { { &PAT_BEER, 900 }, ... };
        tables = {}
        for m in re.finditer(
            r"static\s+const\s+frame_t\s+(s_\w+)\s*\[\s*\]\s*=\s*\{(.*?)\n\};",
            src, re.S,
        ):
            frames = []
            for f in re.finditer(
                r"\{\s*(?:&(PAT_\w+)|NULL)\s*,\s*(\w+)\s*\}", m.group(2)
            ):
                hold = f.group(2)
                hold_ms = int(hold) if hold.isdigit() else defines.get(hold, 0)
                frames.append((f.group(1), hold_ms))
            if frames:
                tables[m.group(1)] = frames

        # [ROBOT_STATE_LOAD_BEER] = { s_load_beer, sizeof(...) },
        animations = {}
        for m in re.finditer(r"\[ROBOT_STATE_(\w+)\]\s*=\s*\{\s*(s_\w+)", src):
            table = tables.get(m.group(2))
            if table:
                animations[m.group(1)] = table
        return animations

    # --- use ------------------------------------------------------------

    def ordered_states(self):
        """Mission order first, then anything else the firmware knows."""
        known = set(self.states)
        ordered = [s for s in MISSION_ORDER if s in known]
        ordered += [s for s in self.states if s not in MISSION_ORDER]
        return ordered

    def frames(self, state):
        """[(grid or None, hold_ms)] for a state, [] if it has no pictures."""
        return [
            (self.patterns.get(pat) if pat else None, hold)
            for pat, hold in self.animations.get(state, [])
        ]


# ---------------------------------------------------------------------------
# Serial link
# ---------------------------------------------------------------------------

class RobotLink:
    """Serial connection with a reader thread.

    Received lines are put on a queue; the GUI drains it from the main
    thread, so no tkinter call ever happens off-thread.
    """

    def __init__(self):
        self.serial = None
        self.thread = None
        self.stop = threading.Event()
        self.lines = queue.Queue()

    @property
    def connected(self):
        return self.serial is not None and self.serial.is_open

    def open(self, port):
        # dsrdtr/rtscts left off: asserting RTS and DTR together resets the
        # C3 into its ROM bootloader.
        self.serial = serial.Serial(port, BAUD, timeout=0.1)
        self.serial.dtr = True
        self.serial.rts = False
        self.stop.clear()
        self.thread = threading.Thread(target=self._reader, daemon=True)
        self.thread.start()

    def close(self):
        self.stop.set()
        if self.thread is not None:
            self.thread.join(timeout=1.0)
            self.thread = None
        if self.serial is not None:
            try:
                self.serial.close()
            finally:
                self.serial = None

    def send(self, command):
        if not self.connected:
            return False
        # The firmware accepts CR, LF or CRLF as end of line.
        self.serial.write((command + "\r").encode("ascii", "ignore"))
        return True

    def _reader(self):
        buf = b""
        while not self.stop.is_set():
            try:
                chunk = self.serial.read(256)
            except Exception as exc:  # port yanked out, board reset, ...
                self.lines.put(("error", str(exc)))
                return
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf or b"\r" in buf:
                idx = min(
                    (buf.index(b) for b in (b"\n", b"\r") if b in buf),
                    default=-1,
                )
                line, buf = buf[:idx], buf[idx + 1:]
                text = line.decode("utf-8", "replace").strip()
                if text:
                    self.lines.put(("line", text))


# ---------------------------------------------------------------------------
# 8x8 preview
# ---------------------------------------------------------------------------

class Preview(tk.Canvas):
    """Replays a state's frames the way led_animator.c would.

    This is what the panel is supposed to show. A real panel that shows
    something else means a wrong pattern table, wrong wiring macros in
    matrix.h, or a wrong animation table.
    """

    CELL = 30
    GAP = 2

    def __init__(self, master):
        size = 8 * self.CELL + 9 * self.GAP
        super().__init__(master, width=size, height=size, bg="#0a0a0a",
                         highlightthickness=0)
        self.frames = []
        self.started = time.monotonic()
        self.brightness = 255
        self.cells = []
        for y in range(8):
            row = []
            for x in range(8):
                x0 = self.GAP + x * (self.CELL + self.GAP)
                y0 = self.GAP + y * (self.CELL + self.GAP)
                row.append(self.create_rectangle(
                    x0, y0, x0 + self.CELL, y0 + self.CELL,
                    fill="#000000", outline="#1a1a1a",
                ))
            self.cells.append(row)
        self._shown = None

    def show(self, frames):
        self.frames = frames or []
        self.started = time.monotonic()
        self._shown = None
        self.tick()

    def set_brightness(self, value):
        if value != self.brightness:
            self.brightness = value
            self._shown = None
            self.tick()

    def _current_index(self):
        """Which frame led_animator.c would be on by now."""
        if len(self.frames) <= 1:
            return 0                    # a still picture never advances
        total = sum(hold for _, hold in self.frames)
        if total <= 0:
            return 0
        t = ((time.monotonic() - self.started) * 1000.0) % total
        for i, (_, hold) in enumerate(self.frames):
            if t < hold:
                return i
            t -= hold
        return len(self.frames) - 1

    def tick(self):
        if not self.frames:
            if self._shown != "blank":
                self._paint(None)
                self._shown = "blank"
            return
        index = self._current_index()
        if index != self._shown:
            self._paint(self.frames[index][0])
            self._shown = index

    def _paint(self, grid):
        # Same scaling as matrix.c: value * (brightness + 1) >> 8.
        scale = self.brightness + 1
        for y in range(8):
            for x in range(8):
                if grid is None:
                    r = g = b = 0
                else:
                    r, g, b = grid[y][x]
                    r = (r * scale) >> 8
                    g = (g * scale) >> 8
                    b = (b * scale) >> 8
                self.itemconfig(self.cells[y][x], fill=f"#{r:02x}{g:02x}{b:02x}")


# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Robot state controller - test panel")
        self.geometry("1040x780")
        self.minsize(920, 680)

        self.firmware = Firmware(FIRMWARE_DIR)
        self.states = self.firmware.ordered_states()

        self.link = RobotLink()
        self.board_state = None         # what the board last reported

        # Test run bookkeeping.
        self.steps = []
        self.step_index = 0
        self.pending = None             # the check waiting for a reply
        self.pending_deadline = 0.0
        self.running = False
        self.awaiting_verdict = None    # state the user is judging

        # A/B compare.
        self.ab_running = False
        self.ab_which = 0

        # Closing while the repeating timers are queued makes Tk complain
        # about callbacks on a dead window, so they are cancelled by hand.
        self.closing = False
        self.timers = []

        self._build_ui()
        self._refresh_ports()
        self._later(50, self._drain)
        self._later(50, self._animate)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

        if self.firmware.problem:
            self._log(f"firmware sources: {self.firmware.problem}", "error")
            self._log("previews are off; the rest of the panel still works", "error")
        else:
            self._log(f"read {len(self.states)} states and "
                      f"{len(self.firmware.patterns)} patterns from {FIRMWARE_DIR}",
                      "sent")

    def _later(self, ms, func):
        """after(), but remembered so it can be cancelled on the way out."""
        if self.closing:
            return None
        handle = self.after(ms, func)
        self.timers.append(handle)
        if len(self.timers) > 64:
            # Handles of timers that have already fired are harmless to
            # cancel, but the list should not grow all session.
            self.timers = self.timers[-32:]
        return handle

    # --- layout ---------------------------------------------------------

    def _build_ui(self):
        bar = ttk.Frame(self)
        bar.pack(fill="x", padx=8, pady=6)
        ttk.Label(bar, text="Port").pack(side="left")
        self.port_box = ttk.Combobox(bar, width=12, state="readonly")
        self.port_box.pack(side="left", padx=6)
        ttk.Button(bar, text="Refresh", command=self._refresh_ports).pack(side="left")
        self.connect_btn = ttk.Button(bar, text="Connect", command=self._toggle)
        self.connect_btn.pack(side="left", padx=6)
        ttk.Button(bar, text="Ping",
                   command=lambda: self._send("PING")).pack(side="left")
        self.summary = ttk.Label(bar, text="not connected")
        self.summary.pack(side="right")

        body = ttk.Frame(self)
        body.pack(fill="both", expand=True, padx=8)
        left = ttk.Frame(body)
        left.pack(side="left", fill="y")
        right = ttk.Frame(body)
        right.pack(side="left", fill="both", expand=True, padx=(10, 0))

        self._build_preview(left)
        self._build_compare(left)
        self._build_states(right)
        self._build_protocol(right)
        self._build_log(right)

        # Fills the state rows, which needs both trees to exist.
        self._clear_results()
        if self.states:
            self.tree.selection_set(self.states[0])

    def _build_preview(self, parent):
        box = ttk.LabelFrame(parent, text="What the panel should show")
        box.pack(fill="x")

        self.preview = Preview(box)
        self.preview.pack(padx=8, pady=8)

        self.preview_title = tk.Label(box, text="—", font=("Segoe UI", 15, "bold"),
                                      bg="#202020", fg="#808080")
        self.preview_title.pack(fill="x", padx=8)
        self.preview_note = ttk.Label(box, text="")
        self.preview_note.pack(fill="x", padx=8, pady=(2, 6))

        bright = ttk.Frame(box)
        bright.pack(fill="x", padx=8, pady=(0, 6))
        ttk.Label(bright, text="Brightness").grid(row=0, column=0, sticky="w")
        self.bright = tk.IntVar(value=80)
        self.bright_label = ttk.Label(bright, text="80", width=4)
        self.bright_label.grid(row=0, column=2, sticky="e")
        ttk.Scale(bright, from_=0, to=255, variable=self.bright,
                  command=lambda _: self._on_brightness()).grid(
            row=0, column=1, sticky="ew", padx=6)
        bright.columnconfigure(1, weight=1)

        levels = ttk.Frame(box)
        levels.pack(fill="x", padx=8, pady=(0, 6))
        for i, value in enumerate(MISSION_BRIGHTNESS):
            ttk.Button(levels, text=str(value), width=6,
                       command=lambda v=value: self._set_brightness(v)).grid(
                row=0, column=i, padx=2, sticky="ew")
            levels.columnconfigure(i, weight=1)
        last = len(MISSION_BRIGHTNESS)
        ttk.Button(levels, text="Apply",
                   command=lambda: self._set_brightness(self.bright.get())).grid(
            row=0, column=last, padx=2, sticky="ew")
        levels.columnconfigure(last, weight=1)

        # The preview is a shape reference. A real panel at 20 is still
        # readable, a monitor pixel at 8% is not, so dimming is opt-in.
        self.dim_preview = tk.BooleanVar(value=False)
        ttk.Checkbutton(box, text="dim the preview too",
                        variable=self.dim_preview,
                        command=self._on_brightness).pack(anchor="w", padx=8,
                                                          pady=(0, 8))

    def _build_compare(self, parent):
        box = ttk.LabelFrame(parent, text="A/B compare")
        box.pack(fill="x", pady=(10, 0))

        first, second = KEY_COMPARISON
        self.ab_a = tk.StringVar(
            value=first if first in self.states else self.states[0])
        self.ab_b = tk.StringVar(
            value=second if second in self.states else self.states[-1])

        for row, var in ((0, self.ab_a), (1, self.ab_b)):
            ttk.Combobox(box, textvariable=var, values=self.states,
                         state="readonly", width=24).grid(
                row=row, column=0, columnspan=2, padx=8, sticky="ew",
                pady=(8 if row == 0 else 2, 2))

        ttk.Label(box, text="swap every (ms)").grid(row=2, column=0, padx=8,
                                                    sticky="w")
        self.ab_interval = tk.IntVar(value=1500)
        ttk.Spinbox(box, from_=300, to=10000, increment=100, width=8,
                    textvariable=self.ab_interval).grid(row=2, column=1, padx=8,
                                                        sticky="e")

        self.ab_btn = ttk.Button(box, text="Start flipping", command=self._toggle_ab)
        self.ab_btn.grid(row=3, column=0, columnspan=2, padx=8, pady=8, sticky="ew")
        box.columnconfigure(1, weight=1)

    def _build_states(self, parent):
        box = ttk.LabelFrame(parent, text="States")
        box.pack(fill="both", expand=True)

        controls = ttk.Frame(box)
        controls.pack(fill="x", padx=6, pady=6)
        self.run_btn = ttk.Button(controls, text="Run full test",
                                  command=self._toggle_test)
        self.run_btn.pack(side="left")
        ttk.Button(controls, text="Set selected",
                   command=self._set_selected).pack(side="left", padx=6)
        ttk.Button(controls, text="Clear results",
                   command=self._clear_results).pack(side="left")

        ttk.Label(controls, text="hold ms").pack(side="left", padx=(12, 2))
        self.dwell = tk.IntVar(value=1200)
        ttk.Spinbox(controls, from_=0, to=20000, increment=100, width=7,
                    textvariable=self.dwell).pack(side="left")

        self.ask_visual = tk.BooleanVar(value=True)
        ttk.Checkbutton(controls, text="ask for visual verdict",
                        variable=self.ask_visual).pack(side="left", padx=10)

        self.tree = ttk.Treeview(box, columns=("set", "get", "visual"),
                                 height=13, selectmode="browse")
        self.tree.heading("#0", text="State")
        self.tree.heading("set", text="SET_STATE")
        self.tree.heading("get", text="GET_STATE")
        self.tree.heading("visual", text="Looks right")
        self.tree.column("#0", width=200, anchor="w")
        for col in ("set", "get", "visual"):
            self.tree.column(col, width=100, anchor="center")
        self.tree.pack(fill="both", expand=True, padx=6, pady=(0, 6))
        self.tree.tag_configure("pass", foreground="#20a040")
        self.tree.tag_configure("fail", foreground="#d02020")
        self.tree.bind("<<TreeviewSelect>>", lambda _e: self._preview_selected())
        self.tree.bind("<Double-1>", lambda _e: self._set_selected())

        # Packed only while a run is paused waiting for a human verdict.
        self.verdict = ttk.Frame(box)
        self.verdict_label = ttk.Label(self.verdict, text="")
        self.verdict_label.pack(side="left", padx=6)
        ttk.Button(self.verdict, text="Looks right",
                   command=lambda: self._give_verdict(True)).pack(side="left", padx=3)
        ttk.Button(self.verdict, text="Not right",
                   command=lambda: self._give_verdict(False)).pack(side="left", padx=3)

    def _build_protocol(self, parent):
        box = ttk.LabelFrame(parent, text="Protocol checks")
        box.pack(fill="x", pady=(10, 0))
        self.checks = ttk.Treeview(box, columns=("result",), height=6,
                                   selectmode="none")
        self.checks.heading("#0", text="Check")
        self.checks.heading("result", text="Result")
        self.checks.column("#0", width=240, anchor="w")
        self.checks.column("result", width=260, anchor="w")
        self.checks.pack(fill="x", padx=6, pady=6)
        self.checks.tag_configure("pass", foreground="#20a040")
        self.checks.tag_configure("fail", foreground="#d02020")

    def _build_log(self, parent):
        box = ttk.LabelFrame(parent,
                             text="Traffic (button presses arrive as EVENT lines)")
        box.pack(fill="both", expand=True, pady=(10, 8))
        self.log = tk.Text(box, height=8, bg="#101010", fg="#d0d0d0",
                           font=("Consolas", 10), state="disabled", wrap="none")
        self.log.pack(side="left", fill="both", expand=True)
        bar = ttk.Scrollbar(box, command=self.log.yview)
        bar.pack(side="right", fill="y")
        self.log.config(yscrollcommand=bar.set)
        self.log.tag_config("event", foreground="#50d0ff")
        self.log.tag_config("sent", foreground="#909090")
        self.log.tag_config("nack", foreground="#ff8080")
        self.log.tag_config("state", foreground="#80ff80")
        self.log.tag_config("error", foreground="#ff6060")

    # --- connection -----------------------------------------------------

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_box["values"] = ports
        if ports and not self.port_box.get():
            self.port_box.set(ports[0])

    def _toggle(self):
        if self.link.connected:
            self._stop_test("disconnected")
            self._stop_ab()
            self.link.close()
            self.connect_btn.config(text="Connect")
            self.summary.config(text="not connected")
            self._log("disconnected", "sent")
            return

        port = self.port_box.get()
        if not port:
            self._log("no port selected", "error")
            return
        try:
            self.link.open(port)
        except Exception as exc:
            self._log(f"cannot open {port}: {exc}", "error")
            self._log("close Tera Term / idf.py monitor and try again", "error")
            return

        self.connect_btn.config(text="Disconnect")
        self.summary.config(text=f"connected to {port}")
        self._log(f"connected to {port}", "sent")
        self._later(300, lambda: self._send("GET_STATE"))

    def _send(self, command):
        if not self.link.send(command):
            self._log("not connected", "error")
            return False
        self._log(f"> {command}", "sent")
        return True

    # --- manual control -------------------------------------------------

    def _selected_state(self):
        sel = self.tree.selection()
        return sel[0] if sel else None

    def _set_selected(self):
        state = self._selected_state()
        if state:
            self._send(f"SET_STATE,{state}")

    def _preview_selected(self):
        """Selecting a row previews it without touching the board, so the
        pictures can be looked through with nothing connected."""
        state = self._selected_state()
        if state:
            self._show_preview(state, live=(state == self.board_state))

    def _set_brightness(self, value):
        value = max(0, min(255, int(value)))
        self.bright.set(value)
        self._on_brightness()
        self._send(f"SET_BRIGHTNESS,{value}")

    def _on_brightness(self):
        value = self.bright.get()
        self.bright_label.config(text=str(value))
        self.preview.set_brightness(value if self.dim_preview.get() else 255)

    def _show_preview(self, state, live):
        frames = self.firmware.frames(state)
        self.preview.show(frames)
        self.preview_title.config(text=state,
                                  fg="#d0d0d0" if live else "#808080")
        if not frames:
            note = "no pattern found in the sources"
        elif len(frames) == 1:
            note = "still picture"
        else:
            note = f"{len(frames)} frames, {sum(h for _, h in frames)} ms loop"
        if not live:
            note += "   (not what the panel is showing)"
        self.preview_note.config(text=note)

    # --- A/B compare ----------------------------------------------------

    def _toggle_ab(self):
        if self.ab_running:
            self._stop_ab()
            return
        if not self.link.connected:
            self._log("not connected", "error")
            return
        self.ab_running = True
        self.ab_which = 0
        self.ab_btn.config(text="Stop flipping")
        self._ab_step()

    def _stop_ab(self):
        if self.ab_running:
            self.ab_running = False
            self.ab_btn.config(text="Start flipping")

    def _ab_step(self):
        if not self.ab_running or not self.link.connected:
            self._stop_ab()
            return
        state = self.ab_a.get() if self.ab_which == 0 else self.ab_b.get()
        self.ab_which ^= 1
        self._send(f"SET_STATE,{state}")
        self._later(max(300, self.ab_interval.get()), self._ab_step)

    # --- results --------------------------------------------------------

    def _clear_results(self):
        for item in self.tree.get_children():
            self.tree.delete(item)
        for state in self.states:
            self.tree.insert("", "end", iid=state, text=state,
                             values=("—", "—", "—"))
        for item in self.checks.get_children():
            self.checks.delete(item)
        self.summary.config(
            text="connected" if self.link.connected else "not connected")

    def _mark(self, state, column, ok, text=None):
        if not self.tree.exists(state):
            return
        values = list(self.tree.item(state, "values"))
        values[{"set": 0, "get": 1, "visual": 2}[column]] = (
            text or ("ok" if ok else "FAIL"))
        # A row is green only while nothing in it has failed.
        failed = "fail" in self.tree.item(state, "tags") or not ok
        self.tree.item(state, values=values, tags=("fail",) if failed else ("pass",))
        self.tree.see(state)

    def _mark_check(self, label, ok, detail):
        tags = ("pass" if ok else "fail",)
        if self.checks.exists(label):
            self.checks.item(label, values=(detail,), tags=tags)
        else:
            self.checks.insert("", "end", iid=label, text=label,
                               values=(detail,), tags=tags)
        self.checks.see(label)

    # --- the test run ---------------------------------------------------

    def _toggle_test(self):
        if self.running:
            self._stop_test("stopped")
        else:
            self._start_test()

    def _start_test(self):
        if not self.link.connected:
            self._log("not connected", "error")
            return
        self._stop_ab()
        self._clear_results()

        # (kind, command, reply prefix, expected reply, (state, column), check)
        steps = [("cmd", "PING", "PONG", "PONG", None, "PING answered")]

        for state in self.states:
            steps.append(("cmd", f"SET_STATE,{state}", "ACK,", "ACK,SET_STATE",
                          (state, "set"), None))
            steps.append(("cmd", "GET_STATE", "STATE,", f"STATE,{state}",
                          (state, "get"), None))
            steps.append(("dwell", None, None, None, None, None))
            steps.append(("ask", None, None, None, (state, "visual"), None))

        # Every level, on whatever state the run ended on.
        for value in MISSION_BRIGHTNESS:
            steps.append(("cmd", f"SET_BRIGHTNESS,{value}", "ACK,",
                          "ACK,SET_BRIGHTNESS", None, f"SET_BRIGHTNESS,{value}"))
            steps.append(("dwell", None, None, None, None, None))

        steps += [
            ("cmd", "SET_STATE,NOT_A_STATE", "NACK", "NACK,BAD_STATE", None,
             "unknown state refused"),
            ("cmd", "SET_BRIGHTNESS,999", "NACK", "NACK,BAD_ARGUMENT", None,
             "brightness out of range refused"),
            ("cmd", "NOT_A_COMMAND", "NACK", "NACK,UNKNOWN_COMMAND", None,
             "unknown command refused"),
            # Leave the panel somewhere sane.
            ("cmd", f"SET_BRIGHTNESS,{MISSION_BRIGHTNESS[1]}", "ACK,",
             "ACK,SET_BRIGHTNESS", None, None),
        ]

        self.steps = steps
        self.step_index = 0
        self.pending = None
        self.running = True
        self.run_btn.config(text="Stop test")
        self.summary.config(text="testing...")
        self._log(f"test run started, {len(self.states)} states", "sent")
        self._next_step()

    def _stop_test(self, why):
        if not self.running:
            return
        self.running = False
        self.pending = None
        self.awaiting_verdict = None
        self.verdict.pack_forget()
        self.run_btn.config(text="Run full test")
        self._log(f"test run {why}", "sent")
        self._report()

    def _next_step(self):
        if not self.running:
            return
        if self.step_index >= len(self.steps):
            self.running = False
            self.run_btn.config(text="Run full test")
            self._log("test run finished", "sent")
            self._report()
            return

        kind, cmd, prefix, expected, target, label = self.steps[self.step_index]
        self.step_index += 1

        if kind == "cmd":
            if not self._send(cmd):
                self._stop_test("aborted, link gone")
                return
            self.pending = (prefix, expected, target, label)
            self.pending_deadline = time.monotonic() + REPLY_TIMEOUT_S
            self._later(50, self._poll_pending)

        elif kind == "dwell":
            self._later(max(0, self.dwell.get()), self._next_step)

        elif kind == "ask":
            if not self.ask_visual.get():
                self._next_step()
                return
            state = target[0]
            self.awaiting_verdict = state
            self.verdict_label.config(
                text=f"Does the panel show {state} the way the preview does?")
            self.verdict.pack(fill="x", padx=6, pady=(0, 6))

    def _poll_pending(self):
        """A reply either arrives in _on_line, or it never does."""
        if self.closing or not self.running or self.pending is None:
            return
        if time.monotonic() >= self.pending_deadline:
            _prefix, expected, target, label = self.pending
            self.pending = None
            self._record(target, label, False, f"no reply, wanted {expected}")
            self._next_step()
            return
        self._later(50, self._poll_pending)

    def _resolve(self, text):
        """Does this line answer the check the run is waiting for?"""
        if self.pending is None:
            return
        prefix, expected, target, label = self.pending
        if not text.startswith(prefix):
            return
        self.pending = None
        ok = (text == expected)
        self._record(target, label, ok,
                     text if ok else f"wanted {expected}, got {text}")
        self._next_step()

    def _record(self, target, label, ok, detail):
        if target is not None:
            state, column = target
            self._mark(state, column, ok)
            if not ok:
                self._log(f"{state} {column}: {detail}", "nack")
        if label is not None:
            self._mark_check(label, ok, detail)

    def _give_verdict(self, ok):
        state = self.awaiting_verdict
        self.awaiting_verdict = None
        self.verdict.pack_forget()
        if state is not None:
            self._mark(state, "visual", ok, "ok" if ok else "WRONG")
            if not ok:
                self._log(f"{state}: marked wrong by eye", "nack")
        self._next_step()

    def _report(self):
        good = sum(1 for s in self.states
                   if self.tree.exists(s)
                   and "fail" not in self.tree.item(s, "tags")
                   and self.tree.item(s, "values")[0] == "ok")
        checks = self.checks.get_children()
        checks_ok = sum(1 for c in checks if "pass" in self.checks.item(c, "tags"))
        text = (f"{good}/{len(self.states)} states ok, "
                f"{checks_ok}/{len(checks)} checks ok")
        self.summary.config(text=text)
        self._log(text, "state" if good == len(self.states) else "nack")

    # --- incoming -------------------------------------------------------

    def _drain(self):
        if self.closing:
            return
        try:
            while True:
                kind, text = self.link.lines.get_nowait()
                if kind == "error":
                    self._log(f"serial error: {text}", "error")
                    self._stop_test("aborted, serial error")
                    self._stop_ab()
                    self.link.close()
                    self.connect_btn.config(text="Connect")
                    self.summary.config(text="not connected")
                else:
                    self._on_line(text)
        except queue.Empty:
            pass
        self._later(50, self._drain)

    def _on_line(self, text):
        tag = None
        if text.startswith("EVENT,"):
            tag = "event"
        elif text.startswith("NACK"):
            tag = "nack"
        elif text.startswith("STATE,"):
            tag = "state"
            self._board_state(text.split(",", 1)[1])
        elif " state: " in text and " -> " in text:
            # Log line: "I (123) state: LOAD_BEER -> FAULT"
            self._board_state(text.rsplit(" -> ", 1)[1])
        elif text.startswith("ACK,SET_STATE") and not self.running:
            # Outside a run, follow up so the preview tracks the panel. A run
            # sends its own GET_STATE and must not get extra replies.
            self._later(100, lambda: self.link.send("GET_STATE"))

        self._log(text, tag)
        self._resolve(text)

    def _board_state(self, name):
        name = name.strip()
        self.board_state = name
        if self.tree.exists(name):
            self.tree.selection_set(name)
        self._show_preview(name, live=True)

    def _animate(self):
        if self.closing:
            return
        self.preview.tick()
        self._later(40, self._animate)

    def _log(self, text, tag=None):
        stamp = time.strftime("%H:%M:%S")
        self.log.config(state="normal")
        self.log.insert("end", f"{stamp}  {text}\n", tag or ())
        self.log.see("end")
        self.log.config(state="disabled")

    def _on_close(self):
        self.closing = True
        self.running = False
        self.ab_running = False
        for handle in self.timers:
            try:
                self.after_cancel(handle)
            except Exception:       # already fired, nothing to cancel
                pass
        self.timers.clear()
        self.link.close()
        self.destroy()


if __name__ == "__main__":
    App().mainloop()
