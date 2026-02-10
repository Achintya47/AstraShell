<pre align = "center">
   ▄▄▄        ██████ ▄▄▄█████▓ ██▀███   ▄▄▄           ██████  ██░ ██ ▓█████  ██▓     ██▓    
▒████▄    ▒██    ▒ ▓  ██▒ ▓▒▓██ ▒ ██▒▒████▄       ▒██    ▒ ▓██░ ██▒▓█   ▀ ▓██▒    ▓██▒    
▒██  ▀█▄  ░ ▓██▄   ▒ ▓██░ ▒░▓██ ░▄█ ▒▒██  ▀█▄     ░ ▓██▄   ▒██▀▀██░▒███   ▒██░    ▒██░    
░██▄▄▄▄██   ▒   ██▒░ ▓██▓ ░ ▒██▀▀█▄  ░██▄▄▄▄██      ▒   ██▒░▓█ ░██ ▒▓█  ▄ ▒██░    ▒██░    
 ▓█   ▓██▒▒██████▒▒  ▒██▒ ░ ░██▓ ▒██▒ ▓█   ▓██▒   ▒██████▒▒░▓█▒░██▓░▒████▒░██████▒░██████▒
 ▒▒   ▓▒█░▒ ▒▓▒ ▒ ░  ▒ ░░   ░ ▒▓ ░▒▓░ ▒▒   ▓▒█░   ▒ ▒▓▒ ▒ ░ ▒ ░░▒░▒░░ ▒░ ░░ ▒░▓  ░░ ▒░▓  ░
  ▒   ▒▒ ░░ ░▒  ░ ░    ░      ░▒ ░ ▒░  ▒   ▒▒ ░   ░ ░▒  ░ ░ ▒ ░▒░ ░ ░ ░  ░░ ░ ▒  ░░ ░ ▒  ░
  ░   ▒   ░  ░  ░    ░        ░░   ░   ░   ▒      ░  ░  ░   ░  ░░ ░   ░     ░ ░     ░ ░   
      ░  ░      ░              ░           ░  ░         ░   ░  ░  ░   ░  ░    ░  ░    ░  ░
                                                                                          
</pre>
### UNDER CONSTRUCTION
ASTRA Shell is a **Linux-compatible Unix shell written in C++** that implements
core POSIX shell features including **job control, signal handling, process groups,
foreground/background execution, and pipelines**.

This project was built incrementally to understand **how real shells interact with
the kernel**, focusing on correctness over convenience.

---

##  Features Implemented

### Core Shell
- Interactive prompt (`astra$`)
- Command execution via `fork()` + `execvp()`
- PATH-based command lookup
- Graceful shell exit

### Built-in Commands
- `cd [dir]` — change directory
- `pwd` — print current directory
- `jobs` — list active jobs
- `fg %n` — bring job to foreground
- `bg %n` — resume job in background
- `exit` — exit the shell

### Job Control (POSIX-Correct)
- Foreground and background execution (`&`)
- Process groups (PGID-based job tracking)
- Terminal ownership management via `tcsetpgrp`
- Correct handling of:
  - `Ctrl+C` → terminate foreground job
  - `Ctrl+Z` → stop foreground job
- Resume stopped jobs using `fg` / `bg`
- Zombie-free execution

### Signal Handling
- Shell ignores interactive signals (`SIGINT`, `SIGTSTP`, `SIGTTOU`, `SIGTTIN`)
- Child processes restore default signal behavior
- `SIGCHLD` handled safely (no logic inside handlers)

### Pipelines
- Arbitrary-length pipelines using `|`
- Correct stdin/stdout redirection via `pipe()` and `dup2()`
- Entire pipeline treated as **one job**
- Pipelines fully support:
  - `Ctrl+C`
  - `Ctrl+Z`
  - `fg` / `bg`
  - background execution

Example:
```bash
ls | grep cpp | wc -l
```

### Build & Run
Requirements : 
- Linux or WSL2
- C++17 compatible compiler
- POSIX environment

#### Build
```bash
mkdir build
cd buid
cmake ..
make
```
#### Run
./astra

### Not Yet Implemented
- I/O redirection (> < >> 2>)
- Environment variables (export, $VAR)
- Command history / line editing
- /proc-based memory awareness (planned)
- Scripting support

### Motivation

ASTRA Shell was built as a systems programming exercise to deeply understand:
- Unix process model
- Job control internals
- Terminal behavior
- Signal semantics

How real shells like bash actually work
