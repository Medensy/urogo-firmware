typedef struct {
  float m;
  int32_t a;
} loadcell_config_t;

bool loadcell_init(void);
void loadcell_deinit(void);
size_t loadcell_collect_data(int16_t *buffer, size_t max_length, uint32_t timeout, bool early_stop_enable);

void loadcell_get_config(float *m, int32_t *a);
void loadcell_set_config(float m, int32_t a);

void loadcell_set_zero(void);
void loadcell_set_weight(int32_t weight);