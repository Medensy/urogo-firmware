bool board_init(void);
void board_deinit(void);

bool board_is_charging(void);
void board_stop_running(void);

void board_display_stop(void);
void board_display_waiting(void);
void board_display_measuring(void);
void board_display_processing(void);
void board_display_error(void);
void board_display_fatal(void);