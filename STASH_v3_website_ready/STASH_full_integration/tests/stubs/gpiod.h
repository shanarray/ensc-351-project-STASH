#ifndef GPIOD_H_TEST_STUB
#define GPIOD_H_TEST_STUB
struct gpiod_line_request;
enum gpiod_line_value { GPIOD_LINE_VALUE_INACTIVE = 0, GPIOD_LINE_VALUE_ACTIVE = 1, GPIOD_LINE_VALUE_ERROR = -1 };
enum gpiod_line_value gpiod_line_request_get_value(struct gpiod_line_request *request, unsigned int offset);
#endif
