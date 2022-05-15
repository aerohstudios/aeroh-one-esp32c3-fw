void initialize_wifi(void);
void connect_to_wifi(char *, char *, void (* success)(uint8_t *, uint8_t *, int), void (* failure)(void));
