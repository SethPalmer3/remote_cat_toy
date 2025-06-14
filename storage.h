#ifndef STORAGE_H
#define STORAGE_H
#define SSID_MAX_LEN 32
#define MAX_PASS_LEN 63

void print_flash_data(unsigned int len);
void store_ssid(char *ssid);
void store_password(char *password);
char *read_ssid();
char *read_password();
#endif // !STORAGE_H
