# ps - List Processes and Threads

## Description

The `ps` command displays currently running processes and their threads using the `dmosi` operating-system interface.

## Usage

```
ps
```

## Output Format

```
  PID  PPID   UID STAT   %CPU     TIME COMMAND              CMD
    1     0     0 R       5.3    00:00:12 main [dmell]          dmell
    └─ main_thread
    2     1     0 R       1.0    00:00:01 sensor               sensor_mod --port 12
    └─ sensor_reader
```

### Columns

**Process line:**
- `PID`     - Process identifier
- `PPID`    - Parent process identifier (0 if detached)
- `UID`     - User ID associated with the process
- `STAT`    - Process state (`I` created, `R` running, `T` suspended, `X` terminated, `Z` zombie)
- `%CPU`    - Aggregated CPU usage across the process's own threads
- `TIME`    - Aggregated runtime across the process's own threads (`HH:MM:SS`)
- `COMMAND` - Process name, plus the owning module name in brackets when it differs from the process name
- `CMD`     - The full command line (program plus arguments) the process was started with, as recorded by the module-start API via `dmosi_process_set_command()` - `-` if unavailable (e.g. a process not spawned through it)

**Thread line (indented):** one line per thread of the process above it, showing its `STAT`, `%CPU` and `TIME` in the same columns, followed by the thread's name.

## Exit Codes

- `0` - Success
- `-ENOMEM` - Insufficient memory to allocate internal buffers

## Examples

```bash
# List all processes and threads
ps
```
