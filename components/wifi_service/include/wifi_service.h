void initialize_wifi(void);
void connect_to_wifi_with_options(char *, char *, void (* success)(uint8_t *, uint8_t *, int), void (* failure)(void));
void connect_to_wifi();
bool is_wifi_connected();
