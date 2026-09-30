# ps - List Processes and Threads

## Description

The `ps` command displays currently running processes and their threads using the `dmosi` operating-system interface.

## Usage

```
ps
```

## Output Format

```
  PID  PPID   UID STAT    %CPU     TIME       STACK COMMAND              CMD
    8     0     0 R       11.2 00:00:00             dmell                dmell
                  R       11.2 00:00:00   2464/4608   └─ dmell
   14     0     0 R        1.0 00:00:00             automount            automount /dev/dmsdio0/0p1 dmsdio0_0p1
                  S        1.0 00:00:00   1652/2048   └─ automount
```

### Columns

**Process line:**
- `PID`     - Process identifier
- `PPID`    - Parent process identifier (0 if detached)
- `UID`     - User ID associated with the process
- `STAT`    - Process state (`I` created, `R` running, `T` suspended, `X` terminated, `Z` zombie)
- `%CPU`    - Aggregated CPU usage across the process's own threads
- `TIME`    - Aggregated runtime across the process's own threads (`HH:MM:SS`)
- `STACK`   - Empty on the process line (stacks belong to threads)
- `COMMAND` - Process name, plus the owning module name in brackets when it differs from the process name
- `CMD`     - The full command line (program plus arguments) the process was started with, as recorded by the module-start API via `dmosi_process_set_command()` - `-` if unavailable (e.g. a process not spawned through it)

**Thread line (indented):** one line per thread of the process above it, showing its `STAT`, `%CPU` and `TIME` in the same columns, then:
- `STACK`   - `<peak>/<size>` in bytes: the most stack the thread has ever used (its high-water mark) and the stack it was created with - `-` if unknown. The size includes the stack dmosi reserves for thread and module startup (`DMOSI_THREAD_STACK_OVERHEAD`) on top of the module's `DMOD_STACK_SIZE`; the peak shows how much of it a module really needs.

followed by the thread's name.

## Exit Codes

- `0` - Success
- `-ENOMEM` - Insufficient memory to allocate internal buffers

## Examples

```bash
# List all processes and threads
ps
```
