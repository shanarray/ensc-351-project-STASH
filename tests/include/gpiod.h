#ifndef TEST_GPIOD_HEADER
#define TEST_GPIOD_HEADER
struct gpiod_chip;
struct gpiod_line_request;
struct gpiod_line_settings;
struct gpiod_line_config;
struct gpiod_request_config;
enum gpiod_line_direction { GPIOD_LINE_DIRECTION_INPUT, GPIOD_LINE_DIRECTION_OUTPUT };
enum gpiod_line_value { GPIOD_LINE_VALUE_INACTIVE, GPIOD_LINE_VALUE_ACTIVE };
struct gpiod_chip *gpiod_chip_open(const char *path);
void gpiod_chip_close(struct gpiod_chip *chip);
int gpiod_chip_get_line_offset_from_name(struct gpiod_chip *chip, const char *name);
struct gpiod_line_settings *gpiod_line_settings_new(void);
void gpiod_line_settings_free(struct gpiod_line_settings *settings);
int gpiod_line_settings_set_direction(struct gpiod_line_settings *settings, enum gpiod_line_direction direction);
int gpiod_line_settings_set_output_value(struct gpiod_line_settings *settings, enum gpiod_line_value value);
struct gpiod_line_config *gpiod_line_config_new(void);
void gpiod_line_config_free(struct gpiod_line_config *config);
int gpiod_line_config_add_line_settings(struct gpiod_line_config *config, const unsigned int *offsets, unsigned int num_offsets, struct gpiod_line_settings *settings);
struct gpiod_request_config *gpiod_request_config_new(void);
void gpiod_request_config_free(struct gpiod_request_config *config);
void gpiod_request_config_set_consumer(struct gpiod_request_config *config, const char *consumer);
struct gpiod_line_request *gpiod_chip_request_lines(struct gpiod_chip *chip, struct gpiod_request_config *request_config, struct gpiod_line_config *line_config);
int gpiod_line_request_set_value(struct gpiod_line_request *request, unsigned int offset, enum gpiod_line_value value);
void gpiod_line_request_release(struct gpiod_line_request *request);
#endif
