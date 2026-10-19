#!/usr/bin/env bash
set -euo pipefail

CONFIG_FLASH_OFFSET="0x101FF000"
CONFIG_FLASH_LENGTH=4096
CONFIG_FLASH_MAGICN=0x5049434F
TEMP_BINARY_FILE="config.bin"

SSID=$(sed -n 's/^ssid[[:space:]]*=[[:space:]]*//p' .env | head -n 1 | tr -d '\r')
PASSWORD=$(sed -n 's/^password[[:space:]]*=[[:space:]]*//p' .env | head -n 1 | tr -d '\r')

if [[ -z "$SSID" ]]; then
    echo "Error: missing Wi-Fi SSID in config.cfg" >&2
    exit 1
fi

if [[ ${#SSID} -gt 32 ]]; then
    echo "Error: SSID exceeds 32 bytes" >&2
    exit 1
fi

if [[ -z "$PASSWORD" ]]; then
    echo "Error: missing Wi-Fi password in config.cfg" >&2
    exit 1
fi

if [[ ${#PASSWORD} -gt 64 ]]; then
    echo "Error: password exceeds 64 bytes" >&2
    exit 1
fi

MAGIC_HEX=$(printf '%08x' "$CONFIG_FLASH_MAGICN")
MAGIC_LE=$(echo "$MAGIC_HEX" | sed -E 's/(..)(..)(..)(..)/\4\3\2\1/')

{
    printf '%s' "$MAGIC_LE" | xxd -r -p
    printf '%s' "$SSID"
    printf '%0.s\0' $(seq 1 $((32 - ${#SSID})))
    printf '%s' "$PASSWORD"
    printf '%0.s\0' $(seq 1 $((64 - ${#PASSWORD})))
    HEADER_SIZE=$((4 + 32 + 64)) # Pad remaining sector bytes with 0xFF
    printf '%0.s\xFF' $(seq 1 $((CONFIG_FLASH_LENGTH - HEADER_SIZE)))
} > "$TEMP_BINARY_FILE"

ACTUAL_SIZE=$(wc -c < "$TEMP_BINARY_FILE" | tr -d ' ')

if [[ "$ACTUAL_SIZE" -ne "$CONFIG_FLASH_LENGTH" ]]; then
    echo "Error: generated image is $ACTUAL_SIZE bytes, expected $CONFIG_FLASH_LENGTH" >&2
    rm -f "$TEMP_BINARY_FILE"
    exit 1
fi

echo "Writing $ACTUAL_SIZE bytes to $CONFIG_FLASH_OFFSET..."

picotool load "$TEMP_BINARY_FILE" -t bin -o "$CONFIG_FLASH_OFFSET" -v

echo "Configuration written and verified."

rm -f "$TEMP_BINARY_FILE"