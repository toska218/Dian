import os
import re
import time
import pexpect

EDITOR = "./dedit2"

KEYMAP = {
    "Enter":     "\r",
    "Backspace": "\x7f",
    "Ctrl-S":    "\x13",
    "Ctrl-Q":    "\x11",
    "Ctrl-F":    "\x06",
    "Ctrl-R":    "\x12",
    "Ctrl-Z":    "\x1a",
    "Ctrl-Y":    "\x19",
    "Esc":       "\x1b",
}

def send_script(child, script):
    for token in re.split(r"(<[^>]+>)", script):
        if not token:
            continue

        if token.startswith("<") and token.endswith(">"):
            name = token[1:-1]
            child.send(KEYMAP.get(name, name))
        else:
            for c in token:
                child.send(c)
                time.sleep(0.05)

        time.sleep(0.1)

TESTS = [
    ("insert",    "hello",                               "hello\n",   ""),
    ("backspace", "abc<Backspace>",                      "ab\n",      ""),
    ("enter",     "ab<Enter>cd",                         "ab\ncd\n",  ""),
    ("undo",      "abc<Ctrl-Z>",                         "ab\n",      ""),
    ("replace",   "<Ctrl-R>hello<Enter>hi<Enter>Y<Esc>", "hi\n",      "hello\n"),
]

def run_case(name, script, expected, initial=""):
    fname = "/tmp/case_" + name + ".txt"

    with open(fname, "w") as f:
        f.write(initial)

    child = pexpect.spawn(EDITOR, [fname], encoding="utf-8", timeout=8)

    try:
        child.expect("Saved", timeout=5)
    except Exception:
        time.sleep(1.0)

    send_script(child, script)

    time.sleep(0.3)
    child.send("\x13")
    time.sleep(0.3)
    child.send("\x11")

    try:
        child.expect(pexpect.EOF, timeout=5)
    except Exception as e:
        print("FAIL " + name + ": 程序未正常退出: " + str(e))
        child.close(force=True)
        return

    child.close()

    with open(fname) as f:
        got = f.read()
    os.remove(fname)

    if got == expected:
        print("PASS " + name)
    else:
        print("FAIL " + name)
        print("  expected:", repr(expected))
        print("  got:     ", repr(got))

def main():
    for name, script, expected, initial in TESTS:
        run_case(name, script, expected, initial)

if __name__ == "__main__":
    main()
