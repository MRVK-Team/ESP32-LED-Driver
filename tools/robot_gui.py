"""Desktop panel for the ESP32-C3 robot state controller.

Shows the current robot state, lets you command a new one, and prints every
line the board sends - including the EVENT lines a button press produces.

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
import threading
import time
import tkinter as tk
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

STATES = [
    ("Load beer", "LOAD_BEER"),
    ("Autonomous driving", "AUTONOMOUS_DRIVING"),
    ("Fault", "FAULT"),
    ("Mission complete", "MISSION_COMPLETE"),
    ("Scan QR", "SCAN_QR"),
]

# Colour per state, roughly matching what the matrix shows.
STATE_COLOR = {
    "LOAD_BEER": "#e0a000",
    "AUTONOMOUS_DRIVING": "#ff7e00",
    "FAULT": "#d02020",
    "MISSION_COMPLETE": "#20a040",
    "SCAN_QR": "#d0d0d0",
}


class RobotLink:
    """Serial connection with a reader thread.

    Received lines are put on a queue; the GUI drains it from the main
    thread, so no tkinter call ever happens off-thread.
    """

    def __init__(self, on_line):
        self.on_line = on_line
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


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Robot state controller")
        self.geometry("620x520")
        self.minsize(520, 420)

        self.link = RobotLink(self._on_line)
        self.state_name = tk.StringVar(value="—")

        self._build_ui()
        self._refresh_ports()
        self.after(50, self._drain)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # --- layout ---------------------------------------------------------

    def _build_ui(self):
        pad = {"padx": 8, "pady": 6}

        top = ttk.Frame(self)
        top.pack(fill="x", **pad)

        ttk.Label(top, text="Port").pack(side="left")
        self.port_box = ttk.Combobox(top, width=12, state="readonly")
        self.port_box.pack(side="left", padx=6)
        ttk.Button(top, text="Refresh", command=self._refresh_ports).pack(side="left")
        self.connect_btn = ttk.Button(top, text="Connect", command=self._toggle)
        self.connect_btn.pack(side="left", padx=6)
        ttk.Button(top, text="Ping", command=lambda: self._send("PING")).pack(side="left")

        # Current state, large and colour coded.
        self.state_label = tk.Label(
            self,
            textvariable=self.state_name,
            font=("Segoe UI", 22, "bold"),
            bg="#202020",
            fg="#808080",
            height=2,
        )
        self.state_label.pack(fill="x", **pad)

        cmds = ttk.LabelFrame(self, text="Set state")
        cmds.pack(fill="x", **pad)
        row = ttk.Frame(cmds)
        row.pack(fill="x", padx=6, pady=6)
        for label, name in STATES:
            ttk.Button(
                row,
                text=label,
                width=18,
                command=lambda n=name: self._send(f"SET_STATE,{n}"),
            ).pack(side="left", padx=3)

        bright = ttk.Frame(cmds)
        bright.pack(fill="x", padx=6, pady=(0, 6))
        ttk.Label(bright, text="Brightness").pack(side="left")
        self.bright = tk.IntVar(value=80)
        ttk.Scale(
            bright,
            from_=0,
            to=255,
            variable=self.bright,
            command=lambda _: self.bright_label.config(text=str(self.bright.get())),
        ).pack(side="left", fill="x", expand=True, padx=6)
        self.bright_label = ttk.Label(bright, text="80", width=4)
        self.bright_label.pack(side="left")
        ttk.Button(
            bright,
            text="Apply",
            command=lambda: self._send(f"SET_BRIGHTNESS,{self.bright.get()}"),
        ).pack(side="left", padx=4)

        log_frame = ttk.LabelFrame(self, text="Traffic (button presses arrive as EVENT lines)")
        log_frame.pack(fill="both", expand=True, **pad)

        self.log = tk.Text(log_frame, height=12, bg="#101010", fg="#d0d0d0",
                           font=("Consolas", 10), state="disabled", wrap="none")
        self.log.pack(side="left", fill="both", expand=True)
        bar = ttk.Scrollbar(log_frame, command=self.log.yview)
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
            self.link.close()
            self.connect_btn.config(text="Connect")
            self.state_name.set("—")
            self.state_label.config(fg="#808080")
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
        self._log(f"connected to {port}", "sent")
        # Ask what state it is in right now.
        self.after(300, lambda: self._send("GET_STATE"))

    def _send(self, command):
        if not self.link.send(command):
            self._log("not connected", "error")
            return
        self._log(f"> {command}", "sent")

    # --- incoming -------------------------------------------------------

    def _drain(self):
        try:
            while True:
                kind, text = self.link.lines.get_nowait()
                if kind == "error":
                    self._log(f"serial error: {text}", "error")
                    self.link.close()
                    self.connect_btn.config(text="Connect")
                else:
                    self._on_line(text)
        except queue.Empty:
            pass
        self.after(50, self._drain)

    def _on_line(self, text):
        tag = None
        if text.startswith("EVENT,"):
            tag = "event"
        elif text.startswith("NACK"):
            tag = "nack"
        elif text.startswith("STATE,"):
            tag = "state"
            self._set_state(text.split(",", 1)[1])
        elif text.startswith("ACK,SET_STATE"):
            # The board confirmed a change; ask what it settled on.
            self.after(100, lambda: self.link.send("GET_STATE"))
        elif " state: " in text and " -> " in text:
            # Log line: "I (123) state: LOAD_BEER -> FAULT"
            self._set_state(text.rsplit(" -> ", 1)[1].strip())

        self._log(text, tag)

    def _set_state(self, name):
        name = name.strip()
        self.state_name.set(name)
        self.state_label.config(fg=STATE_COLOR.get(name, "#d0d0d0"))

    def _log(self, text, tag=None):
        stamp = time.strftime("%H:%M:%S")
        self.log.config(state="normal")
        self.log.insert("end", f"{stamp}  {text}\n", tag or ())
        self.log.see("end")
        self.log.config(state="disabled")

    def _on_close(self):
        self.link.close()
        self.destroy()


if __name__ == "__main__":
    App().mainloop()
