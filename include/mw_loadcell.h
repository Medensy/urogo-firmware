bool loadcell_init(void);
void loadcell_deinit(void);
size_t loadcell_collect_data(int32_t *buffer, size_t max_length, uint32_t timeout);