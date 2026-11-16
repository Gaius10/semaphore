#!/usr/bin/env python3
"""Build a self-contained HTML replay of a game recording.

Usage:
    python3 analysis/make_replay.py                          # latest recordings/game_*.jsonl
    python3 analysis/make_replay.py recordings/game_x.jsonl  # a specific recording

Writes analysis/replays/<recording name>.html: analysis/replay.html with the recording embedded,
so it opens straight from disk in any browser. replay.html also works on its own: open it and
drop a recording on it.
"""

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TEMPLATE = ROOT / "analysis" / "replay.html"
RECORDINGS_DIR = ROOT / "recordings"
REPLAYS_DIR = ROOT / "analysis" / "replays"
MARKER = "const EMBEDDED_RECORDING = null; // @@RECORDING@@"


def latest_recording() -> Path:
    files = sorted(RECORDINGS_DIR.glob("game_*.jsonl"))
    if not files:
        raise SystemExit(f"no game_*.jsonl found in {RECORDINGS_DIR} (set record_dir=recordings in config.ini)")
    return files[-1]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("recording", nargs="?", type=Path, help="recording (default: latest in recordings/)")
    args = parser.parse_args()

    path = args.recording or latest_recording()
    text = path.read_text()
    header = json.loads(text.split("\n", 1)[0])
    if header.get("format") != "semaphore-recording":
        raise SystemExit(f"{path} is not a semaphore recording")

    template = TEMPLATE.read_text()
    if MARKER not in template:
        raise SystemExit(f"marker not found in {TEMPLATE}")

    # "</" would end the <script> block early, so escape it inside the string.
    embedded = json.dumps(text).replace("</", "<\\/")
    html = template.replace(MARKER, f"const EMBEDDED_RECORDING = {embedded};", 1)
    html = html.replace("<title>Semaphore Replay</title>", f"<title>Semaphore Replay — {path.name}</title>", 1)

    REPLAYS_DIR.mkdir(parents=True, exist_ok=True)
    out = REPLAYS_DIR / f"{path.stem}.html"
    out.write_text(html)
    print(f"saved {out.relative_to(ROOT)} ({len(text.splitlines()) - 1} ticks)")


if __name__ == "__main__":
    main()
