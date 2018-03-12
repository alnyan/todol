todol
=====

About
-----

todol is a simple to-do list application with (currently limited to) command-line interface.


Building
--------

```bash
    $ mkdir build && cd build
    $ cmake ..
    $ make && sudo make install
```

Usage
-----

```bash
    $ todol <command> ...
```

Possible commands:

* add <Text here> — adds a to-do entry to list
* do <N> — marks entry #N as _DONE_
* undo <N> — reverse for "do", removes _DONE_ status from entry #N
* ls (or no command) — shows list of current tasks
* clear — removes all current tasks
* help — prints help message __NYI__

Also, aliasing (like `alias t=todol`) is recommended for easier usage.
