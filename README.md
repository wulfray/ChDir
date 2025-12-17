Author: wulfray.

Readme Author: Hardbean.

Modified By: wulray.

Overview
-
`ChDir` changes the current working directory of the process to a
requested path and then launches an interactive shell there. It keeps a
history file at `$HOME/.chdir_history` so that after you first use a
full path you can later use a short alias (or the directory basename)
to return to that location.

Key behaviors
-
- On first use with a full path, `ChDir` resolves the path and appends
	an entry to `$HOME/.chdir_history` in the form `key<TAB>resolved_path`.
- When invoked with a key (single argument), `ChDir` checks the history
	file for the most recent matching entry and uses the stored path.
- If invoked with no argument, `ChDir` falls back to `$HOME`.
- After successfully changing directories, `ChDir` execs the user's
	login shell (from `$SHELL`, or `/bin/bash` by default), so you get
	an interactive shell whose current directory is the requested path.

Build
-
A `Makefile` is included to build the `ChDir` binary:

```sh
make
```

This produces the `ChDir` executable in the project directory.

Install
-
To make `ChDir` available system-wide you can copy or symlink the
binary to a directory in your `PATH`, e.g.:

```sh
sudo cp ChDir /usr/local/bin/
# or
sudo ln -s /path/to/ChDir /usr/local/bin/ChDir
```
I know we like to type less, so you can also use a symlink with a shorter name.

Usage
-
Basic usage:

*On **FIRST USE** you **MUST** provide the full path:*

```sh
ChDir </home/user/test>
```
After this the utility will remember the full path to the ```test``` directory. So all you have to do is type:

```ChDir test```

That will automatically take you to the needed directory.

Examples
-
- First time: provide a full path. This records the alias in the
	history file.

```sh
ChDir /opt/projects/awesome
```

- Subsequent uses: use the saved key or the basename as a shorthand.

```sh
ChDir awesome
```

Notes and limitations
-
- Running `ChDir` from your interactive shell will spawn a new subshell
	in the target directory. It does not change the working directory of
	the parent process. If you want to change your current shell's
	directory, use a shell function or source wrapper that calls `cd`.
  *Note: This is NOT intended and will be fixed soon*
- History location: `$HOME/.chdir_history` (tab-separated `key<TAB>path`).
- When the same key is recorded multiple times, the program prefers the
	most recent match.

History file format
-
Each line in the history file is written as:

```
key<TAB>resolved_absolute_path
```

Tools and behavior
-
- Resolves provided paths with `realpath` before storing them.
- Remembers both the provided key and the path's basename as aliases
	(unless they are identical).

Troubleshooting
-
- If `HOME` is unset, invoking `ChDir` without arguments will fail.
- If the history file cannot be read or written, warnings are printed
	to stderr but `ChDir` still attempts to operate.

PRs
-

You're welcome to do whatever you wish for this utility, but I'm not accepting PRs at the moment.
