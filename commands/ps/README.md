# ps - List Processes and Threads

## Description

The `ps` command displays currently running processes and their threads using the `dmosi` operating-system interface.

## Usage

```
ps
```

## Output Format

```
PID    PROCESS              MODULE           STATE
  THREAD                 STATE      CPU%   STACK(cur/tot)
1      main               dmell            RUNNING
  [main_thread]          RUNNING    5.2%   256/2048
  [io_thread]            BLOCKED    0.1%   128/1024
2      sensor             sensor_mod       RUNNING
  [sensor_reader]        RUNNING    1.0%   128/1024
```

### Columns

**Process line:**
- `PID`     - Process identifier
- `PROCESS` - Process name
- `MODULE`  - DMOD module name associated with the process
- `STATE`   - Process state (`CREATED`, `RUNNING`, `SUSPENDED`, `TERMINATED`, `ZOMBIE`)

**Thread line (indented):**
- `THREAD`         - Thread name
- `STATE`          - Thread state (`CREATED`, `READY`, `RUNNING`, `BLOCKED`, `SUSPENDED`, `TERMINATED`)
- `CPU%`           - CPU usage as a percentage
- `STACK(cur/tot)` - Current and total stack usage in bytes

## Exit Codes

- `0` - Success
- `-ENOMEM` - Insufficient memory to allocate internal buffers

## Examples

```bash
# List all processes and threads
ps
```
