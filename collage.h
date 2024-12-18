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


/* Global variables */

extern unsigned long TOTAL_MALLOC;


/* Typedefs */

struct image_structure_t { float y1, y2, y3, y4; };


/* Helper methods */

void set_debug(bool debug);
void print_malloc(size_t size, bool print_always);


/* Baisc pixel manipulation */

void copy_pixel(uint8_t* image_to, uint8_t* image_from, int channels);
uint8_t* put_pixels(uint8_t* image, int amount, int r, int g, int b);

float get_point_luminance(uint8_t r, uint8_t g, uint8_t b);
float get_point_brightness(int r, int g, int b);
float get_average_luminance(uint8_t* image, int width, int height, int channels);

struct image_structure_t get_image_structure(uint8_t* image, int width, int height, int channels);
float get_structure_difference(struct image_structure_t s1, struct image_structure_t s2);
struct image_structure_t get_null_structure();
bool is_null_structure(struct image_structure_t s);

int match_image_by_luminance(float Y, float* images_luminance, int count,
                             int not_allowed_1, int not_allowed_2);
int match_image_by_structure(struct image_structure_t S, struct image_structure_t* images_structure, int count,
                             int* not_allowed, int not_allowed_size);
int match_any_image_above(float Y, float* images_luminance, int count,
                          int* not_allowed, int not_allowed_size);


/* Whole image manipulation */

uint8_t* shrink_image_factor(uint8_t* image, int width, int height, int* shrunk_width, int* shrunk_height, int factor);
uint8_t* shrink_image_size(uint8_t* image, int width, int height, int channels, int shrunk_width, int shrunk_height);
uint8_t* get_contour_image(uint8_t* image, int width, int height, int channels);

bool paste_image_at_pos(uint8_t *image_to, int width1, int height1, int channels1, 
                        uint8_t *image_from, int width2, int height2, int channels2, 
                        int x, int y, float tone);

uint8_t* collage_from_single_image(uint8_t *image, int width, int height, int channels,
                                   int *collage_width, int *collage_height, int mode);
uint8_t* collage_from_multiple_images(
    uint8_t *creator_image, int creator_width, int creator_height, int channels,
    uint8_t **image_array, int image_width, int image_height, int image_count,
    float *images_luminance, struct image_structure_t* images_structure, bool mode_contour);

uint8_t* add_border(
    uint8_t* image, int width, int height, int channels,
    int border_top, int border_bottom, int border_left, int border_right,
    int border_red, int border_green, int border_blue);

#endif 