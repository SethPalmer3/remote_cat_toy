#include "storage.h"
#include "pico/flash.h"
#include "pico/stdlib.h"
#include "pico/sync.h"
#include <stdio.h>
#include <string.h>
#define FLASH_TARGET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
// [ssid - 32 bytes][password - 63 bytes]
void store_ssid(char *ssid) {
  const unsigned char *flash_target_ptr =
      (const unsigned char *)(XIP_BASE + FLASH_TARGET);
  unsigned char write_to_data[FLASH_PAGE_SIZE];
  memset(write_to_data, 0, FLASH_PAGE_SIZE);
  memcpy(write_to_data, ssid, strlen(ssid));
  // Disable interrupts
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET, FLASH_SECTOR_SIZE);

  flash_range_program(FLASH_TARGET, write_to_data, FLASH_PAGE_SIZE);

  restore_interrupts(ints);
}
void store_password(char *password) {
  const unsigned char *flash_target_ptr =
      (const unsigned char *)(XIP_BASE + FLASH_TARGET + SSID_MAX_LEN);
  unsigned char write_to_data[FLASH_PAGE_SIZE];
  memset(write_to_data, 0, FLASH_PAGE_SIZE);
  memcpy(write_to_data, password, strlen(password));
  // Disable interrupts
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET + SSID_MAX_LEN, FLASH_SECTOR_SIZE);

  flash_range_program(FLASH_TARGET + SSID_MAX_LEN, write_to_data,
                      FLASH_PAGE_SIZE);

  restore_interrupts(ints);
}
char *read_ssid() {}
char *read_password() {}
