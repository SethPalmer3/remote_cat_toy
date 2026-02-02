#include "storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Define the storage location as an offset from the start of flash.
// PICO_FLASH_SIZE_BYTES is typically 2MB (0x200000).
// This places storage in the last 4KB sector.
#define FLASH_STORAGE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

// A helper to get the memory-mapped address for READING
static const uint8_t *get_flash_read_ptr() {
  return (const uint8_t *)(XIP_BASE + FLASH_STORAGE_OFFSET);
}

void flash_store(char *data, int offset, unsigned int max_len) {
  // Create a buffer for the single page we will write.
  uint8_t page_buffer[FLASH_PAGE_SIZE];

  // 1. Read the ENTIRE existing page from flash into our buffer
  //    to preserve any data we are not changing.
  memcpy(page_buffer, get_flash_read_ptr(), FLASH_PAGE_SIZE);

  // 2. Modify the buffer with the new data at the correct offset.
  //    First, clear the old data in that location.
  memset(page_buffer + offset, 0, max_len);
  //    Then, copy the new data in. Use strncpy for safety.
  strncpy((char *)(page_buffer + offset), data, max_len - 1);

  // Disable interrupts for flash operations
  uint32_t interrupts = save_and_disable_interrupts();

  // 3. Erase the flash sector. Pass the OFFSET, not the XIP address.
  printf("Erasing...\n");
  flash_range_erase(FLASH_STORAGE_OFFSET, FLASH_SECTOR_SIZE);

  // 4. Program the modified page back to flash. Pass the OFFSET.
  printf("Reprogramming...\n");
  flash_range_program(FLASH_STORAGE_OFFSET, page_buffer, FLASH_PAGE_SIZE);

  // Re-enable interrupts
  restore_interrupts(interrupts);
  printf("Write complete.\n");
}

void store_ssid(char *ssid) { flash_store(ssid, 0, SSID_MAX_LEN); }

void store_password(char *password) {
  flash_store(password, SSID_MAX_LEN, MAX_PASS_LEN);
}

char *read_ssid() {
  // Read from offset 0 of our storage area
  return (char *)get_flash_read_ptr();
}

char *read_password() {
  // Read from the offset where the password begins
  return (char *)(get_flash_read_ptr() + SSID_MAX_LEN);
}
