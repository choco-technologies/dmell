# reboot

`reboot` immediately restarts the system through the platform's `Dmod_Reboot()`
SAL implementation. Save pending work first: this command does not stop services,
flush filesystems, or perform an orderly shutdown.

- `reboot`: request a restart; success does not return.
- `reboot -h` or `reboot --help`: show usage without restarting.
- Other arguments fail without requesting a restart.

Install with `dmf-get reboot`. Requires a DMOD host exporting `Dmod_Reboot`.
Hosts using the default SAL implementation return `-ENOSYS` without restarting;
the command reports the error. A host without the API cannot load the command.
