bool sdcard_init(void);
void sdcard_deinit(void);
void sdcard_save_data(String file_name, uint8_t *buffer, size_t length);
void sdcard_log(String file_name, String str);