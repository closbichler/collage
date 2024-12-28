#include "collage.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>


static bool DEBUG = false;
static bool DEBUG_MALLOC = false;
unsigned long TOTAL_MALLOC = 0;


void print_malloc(size_t size, bool print_always)
{   
    TOTAL_MALLOC += size;

    if (DEBUG_MALLOC || print_always)
    {
        printf("MALLOC: ");
        if (size < 1000)
        {
            printf("%lu B", size);
        }
        else if (size < 1000000)
        {
            double kb = (size) / 1000.0d;
            printf("%.4f KB", kb);
        }
        else
        {
            double mb = (size) / 1000000.0d;
            printf("%.4f MB", mb);
        }
        printf("\n");
    }
}

void print_malloc_error(size_t size)
{
    fprintf(stderr, "ERR: malloc failed with size %lu\n", size);
}

void set_debug(bool debug)
{
    DEBUG = debug;
}

bool check_image_dimensions(image_t image) 
{
    return (image.width > 0 && image.height > 0 && image.channels > 0 &&
        image.width <= MAX_DIMENSION && image.height <= MAX_DIMENSION && image.channels <= MAX_CHANNELS);
}

void print_image_dimensions(char* name, int width, int height, int channels)
{
    printf("%s: %dx%dx%d\n", name, width, height, channels);
}

float get_point_luminance(uint8_t r, uint8_t g, uint8_t b)
{
    float Y = 0;
    float red_normalized = r / 256.0f;
    float green_normalized = g / 256.0f;
    float blue_normalized = b / 256.0f;
    Y += pow(red_normalized, LUMINANCE_POWER_CURVE) * LUMINANCE_RED;
    Y += pow(green_normalized, LUMINANCE_POWER_CURVE) * LUMINANCE_GREEN;
    Y += pow(blue_normalized, LUMINANCE_POWER_CURVE) * LUMINANCE_BLUE;
    return Y;
}

float get_point_brightness(int r, int g, int b)
{
    return (r + g + b) / (255 * 3.f);
}

float get_average_luminance(uint8_t* image, int width, int height, int channels)
{   
    float Y_sum = 0;
    
    int image_size = width * height * channels;
    uint8_t *img_i = image;
    while (img_i < image + image_size)
    {
        Y_sum += get_point_luminance(*img_i, *(img_i+1), *(img_i+2));
        img_i += channels;
    }

    return Y_sum / (width*height);
}

struct image_structure_t get_image_structure(uint8_t* image, int width, int height, int channels)
{
    struct image_structure_t Y;
    int image_size = width * height * channels;
    int quarter_area = width * height / 4;
    Y.y1 = 0; Y.y2 = 0; Y.y3 = 0; Y.y4 = 0;

    int p = 0;
    uint8_t* img_i = image;
    while (p < quarter_area && img_i < image + image_size)
    {
        Y.y1 += get_point_brightness(*img_i, *(img_i+1), *(img_i+2));
        img_i += channels;
        p++;

        if (p % (width/2) == 0 && p != 0) 
            img_i += channels * (width/2);
    }

    p = 0;
    img_i = image + (channels * width/2);
    while (p < quarter_area && img_i < image + image_size)
    {
        Y.y2 += get_point_luminance(*img_i, *(img_i+1), *(img_i+2));
        img_i += channels;
        p++;

        if (p % (width/2) == 0 && p != 0) 
            img_i += channels * (width/2);
    }

    p = 0;
    img_i = image + (channels * width * height/2);
    while (p < quarter_area && img_i < image + image_size)
    {
        Y.y3 += get_point_luminance(*img_i, *(img_i+1), *(img_i+2));
        img_i += channels;
        p++;

        if (p % (width/2) == 0 && p != 0) 
            img_i += channels * (width/2);
    }

    p = 0;
    img_i = image + (channels * width * height/2) + (channels * width/2);
    while (p < quarter_area && img_i < image + image_size)
    {
        Y.y4 += get_point_luminance(*img_i, *(img_i+1), *(img_i+2));
        img_i += channels;
        p++;

        if (p % (width/2) == 0 && p != 0)
            img_i += channels * (width/2);
    }

    Y.y1 /= quarter_area;
    Y.y2 /= quarter_area;
    Y.y3 /= quarter_area;
    Y.y4 /= quarter_area;

    return Y;
}

float get_structure_difference(struct image_structure_t s1, struct image_structure_t s2)
{
    return (fabs(s1.y1 - s2.y1) + fabs(s1.y2 - s2.y2) + 
            fabs(s1.y3 - s2.y3) + fabs(s1.y4 - s2.y4));
}

struct image_structure_t get_null_structure()
{
    struct image_structure_t s = { -1.0f, -1.0f, -1.0f, -1.0f };
    return  s;
}

bool is_null_structure(struct image_structure_t s)
{
    return (s.y1 == -1.0f && s.y2 == -1.0f && s.y3 == -1.0f && s.y4 == -1.0f);
}

void copy_pixel(uint8_t* image_to, uint8_t* image_from, int channels)
{
    for (int i=0; i<channels; i++)
    {
        *(image_to + i) = (uint8_t) *(image_from + i);
    }
}

uint8_t* put_pixels(uint8_t* image, int amount, int r, int g, int b)
{
    for (int i=0; i<amount; i++)
    {
        *image = r;
        *(image+1) = g;
        *(image+2) = b;
        image += 3;
    }
    return image;
}

uint8_t* shrink_image_factor(
    uint8_t* image, int width, int height,
    int* shrunk_width, int* shrunk_height, int factor) 
{
    uint channels = 3;
    uint line_size = width * channels;
    uint image_size = line_size * height;
    
    // divide scale and floor (int division = "truncate to zero")
    *shrunk_width = width / factor;
    *shrunk_height = height / factor;
    uint shrunk_image_size = *shrunk_width * *shrunk_height * 3;
    int diff_width = width - *shrunk_width * factor;

    print_malloc(shrunk_image_size, false);
    uint8_t* shrunk_image = malloc(shrunk_image_size);

    uint8_t *img_i = image, *shrunk_img_i = shrunk_image;
    int p=0;
    while (img_i < image + image_size && 
        shrunk_img_i < shrunk_image + shrunk_image_size)
    {
        // copy pixel
        *shrunk_img_i = (uint8_t) *img_i;
        *(shrunk_img_i+1) = (uint8_t) *(img_i+1);
        *(shrunk_img_i+2) = (uint8_t) *(img_i+2);

        // jump over factor-lines or factor-pixels
        if (p % width >= width - factor)
            p += width * (factor - 1) + factor + diff_width;
        else 
            p += factor; 

        // go to next pixel
        img_i = image + channels * p;
        shrunk_img_i += channels;
    }

    return shrunk_image;
}

uint8_t* shrink_image_size(
    uint8_t* image, int width, int height, int channels,
    int shrunk_width, int shrunk_height) 
{
    if (shrunk_width > width || shrunk_height > height)
    {
        fprintf(stderr, "ERR: Shrunk has to be smaller\n");
        return (uint8_t*) NULL;
    }

    size_t shrunk_image_size = shrunk_width * shrunk_height * channels;

    double width_factor = width / (double)shrunk_width,
           height_factor = height / (double)shrunk_height;
    double diff_width = 0,
           diff_height = 0;
    double shrink_factor;

    if (width_factor <= height_factor)
    {
        shrink_factor = width_factor;
        diff_height = height/shrink_factor - shrunk_height;
    }
    else
    {
        shrink_factor = height_factor;
        diff_width = width/shrink_factor - shrunk_width;
    }

    print_malloc(shrunk_image_size, false);
    uint8_t* shrunk_image = malloc(shrunk_image_size);

    for (int y=0; y<shrunk_height; y++)
    {
        for (int x=0; x<shrunk_width; x++)
        {
            double x_src = (x * shrink_factor + diff_width), 
                   y_src = (y * shrink_factor + diff_height);
            int x1 = (int) x_src, x2 = x1 + 1;
            int y1 = (int) y_src, y2 = y1 + 1;

            double x_weight = x_src - x1;
            double y_weight = y_src - y1;

            uint8_t* p1 = image + (y1 * width + x1) * channels;
            uint8_t* p2 = image + (y1 * width + x2) * channels;
            uint8_t* p3 = image + (y2 * width + x1) * channels;
            uint8_t* p4 = image + (y2 * width + x2) * channels;

            for (int c = 0; c < channels; ++c) {
                double value = (1 - x_weight) * (1 - y_weight) * p1[c] +
                                x_weight * (1 - y_weight) * p2[c] +
                               (1 - x_weight) * y_weight * p3[c] +
                                x_weight * y_weight * p4[c];
                shrunk_image[(y * shrunk_width + x) * channels + c] = (uint8_t)value;
            }
        }
    }

    return shrunk_image;
}

bool paste_image_at_pos(
    uint8_t *image_to, int width1, int height1, int channels1, 
    uint8_t *image_from, int width2, int height2, int channels2, 
    int x, int y, float tone) 
{
    int image_to_size = channels1 * width1 * height1;
    int image_from_size = channels2 * width2 * height2;

    if (image_to_size < image_from_size)
    {
        if (DEBUG) fprintf(stderr, "ERR: Not posible to paste onto smaller image\n");
        return false;
    }
    else if ((x + width2 > width1) || (y + height2 > height1))
    {
        if (DEBUG) fprintf(stderr, "ERR: Paste coordinates out of bounds %d,%d\n", x, y);
        return false;
    }

    uint8_t *img_to_i = image_to,
            *img_from_i = image_from;

    // go to x, y on image_to
    img_to_i += channels1 * width1 * y + channels1 * x;

    int p=1;
    while (img_from_i < image_from + image_from_size && 
            img_to_i < image_to + image_to_size)
    {
        // copy pixel
        *img_to_i = tone * (uint8_t) *img_from_i;
        *(img_to_i + 1) = tone * (uint8_t) *(img_from_i+1);
        *(img_to_i + 2) = tone * (uint8_t) *(img_from_i+2);

        // jump over every other line
        if (p % width2 == 0)
        {
            img_to_i += channels1 * (width1 - width2);
        }

        // go to next pixel
        img_to_i += channels1;
        img_from_i += channels2;
        p++; 
    }

    return true;
}

uint8_t* collage_from_single_image(
    uint8_t *base_image, int base_width, int base_height, int base_channels,
    uint8_t *paste_image, int paste_width, int paste_height, int paste_channels,
    int *collage_width, int *collage_height, int *collage_channels, 
    int mode) 
{
    image_t base_image_s, paste_image_s;
    base_image_s.width = base_width;
    base_image_s.height = base_height;
    base_image_s.channels = base_channels;
    paste_image_s.width = paste_width;
    paste_image_s.height = paste_height;
    paste_image_s.channels = paste_channels;
    if (!check_image_dimensions(base_image_s) ||
        !check_image_dimensions(paste_image_s) ||
        !(mode == 0 || mode == 1))
    {
        fprintf(stderr, "ERR: invalid input image dimensions or wrong function call\n");
        return NULL;
    }

    *collage_width = (base_width * paste_width);
    *collage_height = (base_height * paste_height);
    *collage_channels = paste_channels;
    
    size_t collage_size = (*collage_width) * (*collage_height) * (*collage_channels);
    print_malloc(collage_size, false);
    uint8_t* collage = malloc(collage_size);
    if (collage == NULL)
    {
        print_malloc_error(collage_size);
        return NULL;
    }

    for (int x=0; x<base_width; x++) 
    {
        for (int y=0; y<base_height; y++)
        {
            uint8_t* base_pixel = base_image + (y * base_width + x) * base_channels;
            float Y;
            switch (mode)
            {
                default: 
                case 0: Y = get_point_luminance(*base_pixel, *(base_pixel+1), *(base_pixel+2)); break;
                case 1: Y = fabs(sinf(x*M_PI / base_width) * sinf(y*M_PI / base_height)); break;
            }

            paste_image_at_pos( 
                collage, *collage_width, *collage_height, *collage_channels,
                paste_image, paste_width, paste_height, paste_channels, 
                x*paste_width, y*paste_height, Y
            );
        }
    }
    
    return collage;
}

int match_image_by_luminance(
    float Y, float* images_luminance, int count,
    int not_allowed_1, int not_allowed_2)
{
    int best_image = 0;
    float best_distance = 1;
    for (int k=0; k<count; k++)
    {
        if (images_luminance[k] == 0 || 
            k == not_allowed_1 || 
            k == not_allowed_2)
                continue;

        float d = fabs(Y - images_luminance[k]);

        if (d < best_distance)
        {       
            best_distance = d;
            best_image = k;
        }
    }
    return best_image;
}

bool int_array_contains(int* array, int size, int item)
{
    for (int i=0; i<size; i++)
    {
        if (array[i] == item)
            return true;
    }
    return false;
}

int match_image_by_structure(
    struct image_structure_t S, struct image_structure_t* images_structure, int count,
    int* not_allowed, int not_allowed_count)
{
    int best_image = 0;
    float best_distance = 1000;
    for (int k=0; k<count; k++)
    {
        if (is_null_structure(images_structure[k]) || 
            int_array_contains(not_allowed, not_allowed_count, k))
                continue;

        float d = get_structure_difference(S, images_structure[k]);
        
        if (d < best_distance)
        {       
            best_distance = d;
            best_image = k;
        }
    }
    return best_image;
}

int match_any_image_above(float Y, float* images_luminance, int count,
                          int* not_allowed, int not_allowed_size)
{
    for (int k=0; k<count; k++)
    {
        int r = (int) (random() * ((double) count / RAND_MAX));
        if (images_luminance[r] >= Y && !int_array_contains(not_allowed, not_allowed_size, k))
            return r;
    }
    return 0;
}

uint8_t* collage_from_multiple_images(
    uint8_t *creator_image, int creator_width, int creator_height, int channels, 
    uint8_t **image_array, int image_width, int image_height, int image_count,
    float *images_luminance, struct image_structure_t* images_structure,
    bool mode_contour) 
{
    if (channels != 3) printf("ERR: unable with %d channels\n", channels);

    int creator_size = creator_width * creator_height * channels;
    int fotos_horiz = creator_width/2,
        fotos_vert = creator_height/2;
    int collage_width = fotos_horiz * image_width,
        collage_height = fotos_vert * image_height;

    size_t collage_size = collage_width * collage_height * channels;
    print_malloc(collage_size, false);
    uint8_t* collage = malloc(collage_size);

    struct image_structure_t white_structure = { 1.0f, 1.0f, 1.0f, 1.0f };

    int image_selection[fotos_vert][fotos_horiz];
    for (int i=0; i<fotos_vert; i++) 
    {
        for (int j=0; j<fotos_horiz; j++) 
        {
            uint8_t* cur_pix = creator_image + (creator_width * i*2 + j*2) * channels;
            uint8_t* right_pix = creator_image + (creator_width * i*2 + (j*2+1)) * channels;
            uint8_t* down_pix = creator_image + (creator_width * (i*2+1) + j) * channels;
            uint8_t* down_right_pix = creator_image + (creator_width * (i*2+1) + (j*2+1)) * channels;

            if (down_right_pix >= creator_image + creator_size)
                return collage;

            struct image_structure_t S = { 
                get_point_luminance(*(cur_pix), *(cur_pix+1), *(cur_pix+2)),
                get_point_luminance(*(right_pix), *(right_pix+1), *(right_pix+2)),
                get_point_luminance(*(down_pix), *(down_pix+1), *(down_pix+2)),
                get_point_luminance(*(down_right_pix), *(down_right_pix+1), *(down_right_pix+2)),
            };

            // TODO: calculate radius according to amount of photos
            int not_allowed_radius = 2,
                not_allowed[not_allowed_radius * not_allowed_radius],
                not_allowed_count = 0;

            for (int k=i-not_allowed_radius; k<=i; k++)
            {
                for (int l=j-not_allowed_radius; l<=j+not_allowed_radius; l++)
                {
                    if (k >= 0 && l >= 0 && !(k == i && l >= j)) 
                        not_allowed[not_allowed_count++] = image_selection[k][l];
                }
            }

            int best_image;
            if (mode_contour && get_structure_difference(S, white_structure) < 0.8f)
            {
                best_image = match_any_image_above(
                    0.2f, images_luminance, image_count,
                    not_allowed, not_allowed_count
                );
            }
            else
            {
                best_image = match_image_by_structure(
                    S, images_structure, image_count, 
                    not_allowed, not_allowed_count
                );
            }

            uint8_t* selected_image = image_array[best_image];
            image_selection[i][j] = best_image;

            bool paste_success = paste_image_at_pos( 
                collage, collage_width, collage_height, channels,
                selected_image, image_width, image_height, channels, 
                j*image_width, i*image_height, 1
            );

            if (!paste_success) 
            {
                return collage;
            }
        }
    }
    
    return collage;
}

uint8_t* add_border(
    uint8_t* image, int width, int height, int channels,
    int border_top, int border_bottom, int border_left, int border_right,
    int border_red, int border_green, int border_blue)
{
    int new_width = width + border_left + border_right,
        new_height = height + border_top + border_bottom;
    print_malloc(new_width * new_height * channels, false);
    uint8_t* image_with_border = malloc(new_width * new_height * channels);

    uint8_t *img_i = image, *img_border_i = image_with_border;
    int image_size = width * height * channels;
    int p=0;

    // top border
    img_border_i = put_pixels(
        img_border_i, border_top * new_width, 
        border_red, border_green, border_blue
    );

    while (img_i < image + image_size)
    {
        // left border
        if (p % width == 0)
        {
            img_border_i = put_pixels(
                img_border_i, border_left, 
                border_red, border_green, border_blue
            );
        }
    
        // copy pixel
        *img_border_i = (uint8_t) *img_i;
        *(img_border_i+1) = (uint8_t) *(img_i+1);
        *(img_border_i+2) = (uint8_t) *(img_i+2);

        // right border
        if (p % width == width-1)
        {
            img_border_i = put_pixels(
                img_border_i, border_right, 
                border_red, border_green, border_blue
            );
        }

        img_border_i += channels;
        img_i += channels;
        p++;
    }

    // bottom border
    img_border_i = put_pixels(
        img_border_i, border_bottom * new_width, 
        border_red, border_green, border_blue
    );

    return image_with_border;
}

uint8_t* get_contour_image(uint8_t* image, int width, int height, int channels)
{
    // TODO
    return NULL;
}