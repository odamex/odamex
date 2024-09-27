#!/usr/bin/env python3

# -----------------------------------------------------------------------------
#
# $Id: $
#
# Copyright (C) 2006-2024 by The Odamex Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# DESCRIPTION:
#  A tool used to help test odamex/odasrv during development.
#
# -----------------------------------------------------------------------------

import shlex
import subprocess
from pathlib import Path
from tkinter import (
    Tk,
    Frame,
    Label,
    StringVar,
    Entry,
    Button,
    LEFT as SIDE_LEFT,
    DISABLED as STATE_DISABLED,
)

BUILD_DIR = Path(__file__).parent.parent.parent / "build"
ODAMEX_EXE = BUILD_DIR / "client" / "Debug" / "odamex.exe"
ODAMEX_CWD = ODAMEX_EXE.parent
ODASRV_EXE = BUILD_DIR / "server" / "Debug" / "odasrv.exe"
ODASRV_CWD = ODASRV_EXE.parent
CONSOLE_CMD = ["wt.exe", "--window", "-1"]

root = Tk()

odamex_params_sv = StringVar(root)
odamex_params_sv.set("+connect localhost")
odasrv_params_sv = StringVar(root)


def run_odamex():
    params = shlex.split(odamex_params_sv.get())
    subprocess.Popen([ODAMEX_EXE, *params], cwd=ODAMEX_CWD)


def run_odasrv():
    params = shlex.split(odasrv_params_sv.get())
    subprocess.Popen([*CONSOLE_CMD, ODASRV_EXE, *params], cwd=ODASRV_CWD)


paths_f = Frame(root)
paths_f.pack()
buttons_f = Frame(root)
buttons_f.pack()

odamex_l = Label(paths_f, text="Odamex")
odamex_l.grid(row=1, column=1)
odamex_sv = StringVar()
odamex_sv.set(str(ODAMEX_EXE))
odamex_e = Entry(paths_f, state=STATE_DISABLED, textvariable=odamex_sv, width=100)
odamex_e.grid(row=1, column=2)

odamex_params_e = Entry(paths_f, textvariable=odamex_params_sv, width=100)
odamex_params_e.grid(row=2, column=2)

odasrv_l = Label(paths_f, text="Odasrv")
odasrv_l.grid(row=3, column=1)
odasrv_sv = StringVar()
odasrv_sv.set(str(ODASRV_EXE))
odasrv_e = Entry(paths_f, state=STATE_DISABLED, textvariable=odasrv_sv, width=100)
odasrv_e.grid(row=3, column=2)

odasrv_params_e = Entry(paths_f, textvariable=odasrv_params_sv, width=100)
odasrv_params_e.grid(row=4, column=2)

odamex_b = Button(buttons_f, text="Run Odamex", command=run_odamex)
odamex_b.pack(side=SIDE_LEFT)
odasrv_b = Button(buttons_f, text="Run Odasrv", command=run_odasrv)
odasrv_b.pack(side=SIDE_LEFT)

root.title("Run Built Odamex")
root.mainloop()
