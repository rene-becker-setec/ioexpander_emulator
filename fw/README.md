# IO Expander Emulator



## Building the Firmware

```bash
cd fw
erpcgen -o app/generated ../shared/interface.erpc
west build -s app
```
