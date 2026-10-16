# tree - Display a directory tree

```text
tree [-a] [-d] [-L level] [path ...]
```

With no path, `tree` shows the current directory. Entries are sorted by name.
Hidden entries are omitted unless `-a` is given; `.` and `..` are never traversed.
`-d` shows directories only. `-L level` limits traversal to a positive number
of levels below each supplied path. The final line reports directory and file
counts, excluding the starting path. Symbolic links are displayed but not
followed when the filesystem reports their type.

Example:

```text
tree -L 2 /etc
```
