#!/bin/sh
set -eu

input="${1:-.config}"
output="${2:-include/bucketos/config.h}"

serial_log=0
panic_serial=0
panic_halt=0
serial_com1_base=0x3F8
cmd_logo=0
cmd_hypervisor=0
cmd_uptime=0

while IFS= read -r line; do
    case "$line" in
        CONFIG_SERIAL_LOG=y) serial_log=1 ;;
        CONFIG_PANIC_SERIAL=y) panic_serial=1 ;;
        CONFIG_PANIC_HALT=y) panic_halt=1 ;;
        CONFIG_SERIAL_COM1_BASE=*) serial_com1_base="${line#*=}" ;;
        CONFIG_CMD_LOGO=y) cmd_logo=1 ;;
        CONFIG_CMD_HYPERVISOR=y) cmd_hypervisor=1 ;;
        CONFIG_CMD_UPTIME=y) cmd_uptime=1 ;;
        *) ;;
    esac
done < "$input"

cat > "$output" <<EOF
#ifndef BUCKETOS_CONFIG_H
#define BUCKETOS_CONFIG_H

#define CONFIG_SERIAL_LOG ${serial_log}
#define CONFIG_PANIC_SERIAL ${panic_serial}
#define CONFIG_PANIC_HALT ${panic_halt}
#define CONFIG_SERIAL_COM1_BASE ${serial_com1_base}
#define CONFIG_CMD_LOGO ${cmd_logo}
#define CONFIG_CMD_HYPERVISOR ${cmd_hypervisor}
#define CONFIG_CMD_UPTIME ${cmd_uptime}

#endif
EOF
