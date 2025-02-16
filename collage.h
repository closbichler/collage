#ifndef COLLAGE_H
#define COLLAGE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Constants */

static const int A4_HEIGHT_300PPI = 2480;
static const int A4_WIDTH_300PPI = 3508;
static const int A3_HEIGHT_300PPI = 3508;
static const int A3_WIDTH_300PPI = 4961;
static const int A2_HEIGHT_300PPI = 4961;
static const int A2_WIDTH_300PPI = 7016;
static const int A1_HEIGHT_300PPI = 7016;
static const int A1_WIDTH_300PPI = 9933;

static const float LUMINANCE_RED = 0.2126f;
static const float LUMINANCE_GREEN = 0.7152f;
static const float LUMINANCE_BLUE = 0.0722f;
static const float LUMINANCE_POWER_CURVE = 2.2f;

static const int MAX_DIMENSION = 10000;
static const int MAX_CHANNELS = 3;
static const int ALLOWED_CHANNELS = 3; // TODO: allow more channels

/* Global variables */

extern unsigned long TOTAL_MALLOC;

/* Typedefs */

typedef struct
{
    uint8_t *pix;
    int w, h, ch;
} image_t;

static const image_t image_default = {NULL, 0, 0, 0};

typedef struct
{
    float y1, y2, y3, y4;
} image_shape_t;

static const image_shape_t image_shape_default = {0, 0, 0, 0};

/* Helper methods */

void set_debug(bool debug);
void print_malloc(size_t size, bool print_always);
void print_malloc_error(size_t size);

/* Pixel manipulation */

void copy_pixel(uint8_t *image_to, uint8_t *image_from, int channels);
uint8_t *put_pixels(uint8_t *image, int amount, int r, int g, int b);

float get_point_luminance(uint8_t r, uint8_t g, uint8_t b);
float get_point_brightness(int r, int g, int b);
float get_average_luminance(image_t image);

image_shape_t get_image_shape(image_t image);
float get_shape_difference(image_shape_t s1, image_shape_t s2);
bool is_default_shape(image_shape_t s);

/* Image analysis */

bool check_image_dimensions(image_t image);
void print_image_dimensions(char *name, image_t image);
size_t get_image_size(image_t image);

int match_image_by_luminance(float Y, float *images_luminance, int count,
                             int not_allowed_1, int not_allowed_2);
int match_image_by_shape(image_shape_t S, image_shape_t *images_structure, int count,
                             int *not_allowed, int not_allowed_size);
int match_any_image_above(float Y, float *images_luminance, int count,
                          int *not_allowed, int not_allowed_size);

/* Image manipulation */

image_t shrink_image_factor(image_t image, int factor);
image_t shrink_image_size(image_t image, int shrunk_width, int shrunk_height);

bool paste_image_at_pos(image_t image_to, image_t image_from, int x, int y, float tone);

image_t collage_from_single_image(image_t base, image_t paste, int mode);
image_t collage_from_multiple_images(image_t creator,
                                     uint8_t **image_array, int image_width, int image_height, int image_count,
                                     float *images_luminance, image_shape_t *images_structure, bool mode_contour);

image_t get_contour_image(image_t image);
image_t add_border(image_t image,
                   int border_top, int border_bottom, int border_left, int border_right,
                   int border_red, int border_green, int border_blue);

#endif