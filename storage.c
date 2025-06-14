#include "storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#define FLASH_TARGET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
// [ssid - 32 bytes][password - 63 bytes]
void flash_store(char *data, int offset, unsigned int len) {
  const unsigned char *flash_target_ptr =
      (const unsigned char *)(XIP_BASE + FLASH_TARGET + offset);
  unsigned char write_to_data[FLASH_PAGE_SIZE];
  memcpy(write_to_data, flash_target_ptr, FLASH_PAGE_SIZE);
  memset(write_to_data + offset, 0, len);
  strncpy(((char *)write_to_data + offset), data, strlen(data));
  // Disable interrupts
  uint32_t ints = save_and_disable_interrupts();

  flash_range_erase(FLASH_TARGET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET, write_to_data, FLASH_PAGE_SIZE);

  restore_interrupts(ints);
}
char *flash_read(int offset) {
  unsigned char *flash_target_ptr =
      (unsigned char *)(XIP_BASE + FLASH_TARGET + offset);
  return flash_target_ptr;
}
void store_ssid(char *ssid) { flash_store(ssid, 0, SSID_MAX_LEN); }
void store_password(char *password) {
  flash_store(password, SSID_MAX_LEN, MAX_PASS_LEN);
}
void print_flash_data(unsigned int len) {
  unsigned char *flash_target_ptr = (unsigned char *)(XIP_BASE + FLASH_TARGET);
  printf("%.*s\n", len, flash_target_ptr);
}
char *read_ssid() { return flash_read(0); }
char *read_password() { return flash_read(SSID_MAX_LEN); }
