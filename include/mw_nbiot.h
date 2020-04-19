bool nbiot_init(void);
void nbiot_deinit(void);
void nbiot_upload_data(String host, uint16_t port, String path, uint8_t *buffer, size_t length);
void nbiot_sync_time(String host);