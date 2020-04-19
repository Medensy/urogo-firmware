typedef struct 
{
  char serial_number[11]; //serial number
  float loadcell_m;
  int32_t loadcell_a;
} board_stored_data_t;

bool board_init(void);
void board_deinit(void);

bool board_is_charging(void);
void board_stop_running(void);

void board_save_data(board_stored_data_t stored_data);
board_stored_data_t board_get_data(void);

void board_display_stop(void);
void board_display_waiting(void);
void board_display_measuring(void);
void board_display_processing(void);
void board_display_error(void);
void board_display_fatal(void);