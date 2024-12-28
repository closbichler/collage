#include "collage.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static bool DEBUG = false;
static bool DEBUG_MALLOC = false;
unsigned long TOTAL_MALLOC = 0;

/* Helper methods */

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

bool int_array_contains(int *array, int size, int item)
{
    for (int i = 0; i < size; i++)
    {
        if (array[i] == item)
            return true;
    }
    return false;
}

/* Pixel manipulation */

void copy_pixel(uint8_t *image_to, uint8_t *image_from, int channels)
{
    for (int i = 0; i < channels; i++)
    {
        *(image_to + i) = (uint8_t) * (image_from + i);
    }
}

uint8_t *put_pixels(uint8_t *image, int amount, int r, int g, int b)
{
    for (int i = 0; i < amount; i++)
    {
        *image = r;
        *(image + 1) = g;
        *(image + 2) = b;
        image += 3;
    }
    return image;
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

float get_average_luminance(image_t image)
{
    float Y_sum = 0;

    int image_size = image.h * image.w * image.ch;
    uint8_t *img_i = image.pix;
    while (img_i < image.pix + image_size)
    {
        Y_sum += get_point_luminance(*img_i, *(img_i + 1), *(img_i + 2));
        img_i += image.ch;
    }

    return Y_sum / (image.w * image.h);
}

image_shape_t get_image_shape(image_t image)
{
    image_shape_t Y;
    int image_size = image.w * image.h * image.ch;
    int quarter_area = image.w * image.h / 4;
    Y.y1 = 0;
    Y.y2 = 0;
    Y.y3 = 0;
    Y.y4 = 0;

    int p = 0;
    uint8_t *img_i = image.pix;
    while (p < quarter_area && img_i < image.pix + image_size)
    {
        Y.y1 += get_point_brightness(*img_i, *(img_i + 1), *(img_i + 2));
        img_i += image.ch;
        p++;

        if (p % (image.w / 2) == 0 && p != 0)
            img_i += image.ch * (image.w / 2);
    }

    p = 0;
    img_i = image.pix + (image.ch * image.w / 2);
    while (p < quarter_area && img_i < image.pix + image_size)
    {
        Y.y2 += get_point_luminance(*img_i, *(img_i + 1), *(img_i + 2));
        img_i += image.ch;
        p++;

        if (p % (image.w / 2) == 0 && p != 0)
            img_i += image.ch * (image.w / 2);
    }

    p = 0;
    img_i = image.pix + (image.ch * image.w * image.h / 2);
    while (p < quarter_area && img_i < image.pix + image_size)
    {
        Y.y3 += get_point_luminance(*img_i, *(img_i + 1), *(img_i + 2));
        img_i += image.ch;
        p++;

        if (p % (image.w / 2) == 0 && p != 0)
            img_i += image.ch * (image.w / 2);
    }

    p = 0;
    img_i = image.pix + (image.ch * image.w * image.h / 2) + (image.ch * image.w / 2);
    while (p < quarter_area && img_i < image.pix + image_size)
    {
        Y.y4 += get_point_luminance(*img_i, *(img_i + 1), *(img_i + 2));
        img_i += image.ch;
        p++;

        if (p % (image.w / 2) == 0 && p != 0)
            img_i += image.ch * (image.w / 2);
    }

    Y.y1 /= quarter_area;
    Y.y2 /= quarter_area;
    Y.y3 /= quarter_area;
    Y.y4 /= quarter_area;

    return Y;
}

float get_shape_difference(image_shape_t s1, image_shape_t s2)
{
    return (fabs(s1.y1 - s2.y1) + fabs(s1.y2 - s2.y2) +
            fabs(s1.y3 - s2.y3) + fabs(s1.y4 - s2.y4));
}

bool is_default_shape(image_shape_t s)
{
    return (s.y1 == image_shape_default.y1 &&
            s.y2 == image_shape_default.y2 &&
            s.y3 == image_shape_default.y3 &&
            s.y4 == image_shape_default.y4);
}

/* Image analysis */

bool check_image_dimensions(image_t image)
{
    return (image.w > 0 && image.h > 0 && image.ch > 0 &&
            image.w <= MAX_DIMENSION && image.h <= MAX_DIMENSION && image.ch <= MAX_CHANNELS &&
            image.ch == ALLOWED_CHANNELS);
}

void print_image_dimensions(char *name, image_t image)
{
    printf("%s: %dx%dx%d\n", name, image.w, image.h, image.ch);
}

size_t get_image_size(image_t image)
{
    return image.w * image.h * image.ch;
}

bool write_image(char *output_path, image_t image, int quality)
{
    stbi_write_jpg(output_path, image.w, image.h, image.ch, image.pix, quality);
    return true;
}

int match_image_by_luminance(float Y, float *images_luminance, int count,
                             int not_allowed_1, int not_allowed_2)
{
    int best_image = 0;
    float best_distance = 1;
    for (int k = 0; k < count; k++)
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

int match_image_by_shape(
    image_shape_t S, image_shape_t *images_structure, int count,
    int *not_allowed, int not_allowed_count)
{
    int best_image = 0;
    float best_distance = 1000;
    for (int k = 0; k < count; k++)
    {
        if (is_default_shape(images_structure[k]) ||
            int_array_contains(not_allowed, not_allowed_count, k))
            continue;

        float d = get_shape_difference(S, images_structure[k]);

        if (d < best_distance)
        {
            best_distance = d;
            best_image = k;
        }
    }
    return best_image;
}

int match_any_image_above(float Y, float *images_luminance, int count,
                          int *not_allowed, int not_allowed_size)
{
    for (int k = 0; k < count; k++)
    {
        int r = (int)(random() * ((double)count / RAND_MAX));
        if (images_luminance[r] >= Y && !int_array_contains(not_allowed, not_allowed_size, k))
            return r;
    }
    return 0;
}

/* Image manipulation */

image_t shrink_image_factor(image_t image, int factor)
{
    // divide scale and floor (int division = "truncate to zero")
    image_t shrunk = {NULL, image.w / factor, image.h / factor, image.ch};
    int image_size = get_image_size(image);
    int shrunk_size = get_image_size(shrunk);
    int diff_width = image.w - shrunk.w * factor;

    print_malloc(shrunk_size, false);
    shrunk.pix = malloc(shrunk_size);

    uint8_t *img_i = image.pix, *shrunk_img_i = shrunk.pix;
    int p = 0;
    while (img_i < image.pix + image_size &&
           shrunk_img_i < shrunk.pix + shrunk_size)
    {
        // copy pixel
        *shrunk_img_i = (uint8_t)*img_i;
        *(shrunk_img_i + 1) = (uint8_t) * (img_i + 1);
        *(shrunk_img_i + 2) = (uint8_t) * (img_i + 2);

        // jump over factor-lines or factor-pixels
        if (p % image.w >= image.w - factor)
            p += image.w * (factor - 1) + factor + diff_width;
        else
            p += factor;

        // go to next pixel
        img_i = image.pix + image.ch * p;
        shrunk_img_i += image.ch;
    }

    return shrunk;
}

image_t shrink_image_size(image_t image, int shrunk_width, int shrunk_height)
{
    if (shrunk_width > image.w || shrunk_height > image.h)
    {
        fprintf(stderr, "ERR: Shrunk has to be smaller\n");
        return image_default;
    }

    double width_factor = image.w / (double)shrunk_width,
           height_factor = image.h / (double)shrunk_height;
    double diff_width = 0,
           diff_height = 0;
    double shrink_factor;

    if (width_factor <= height_factor)
    {
        shrink_factor = width_factor;
        diff_height = image.h / shrink_factor - shrunk_height;
    }
    else
    {
        shrink_factor = height_factor;
        diff_width = image.w / shrink_factor - shrunk_width;
    }

    image_t shrunk = {NULL, shrunk_width, shrunk_height, image.ch};
    size_t shrunk_size = get_image_size(shrunk);
    print_malloc(shrunk_size, false);
    shrunk.pix = malloc(shrunk_size);

    for (int y = 0; y < shrunk_height; y++)
    {
        for (int x = 0; x < shrunk_width; x++)
        {
            double x_src = (x * shrink_factor + diff_width),
                   y_src = (y * shrink_factor + diff_height);
            int x1 = (int)x_src, x2 = x1 + 1;
            int y1 = (int)y_src, y2 = y1 + 1;

            double x_weight = x_src - x1;
            double y_weight = y_src - y1;

            uint8_t *p1 = image.pix + (y1 * image.w + x1) * image.ch;
            uint8_t *p2 = image.pix + (y1 * image.w + x2) * image.ch;
            uint8_t *p3 = image.pix + (y2 * image.w + x1) * image.ch;
            uint8_t *p4 = image.pix + (y2 * image.w + x2) * image.ch;

            for (int c = 0; c < image.ch; ++c)
            {
                double value = (1 - x_weight) * (1 - y_weight) * p1[c] +
                               x_weight * (1 - y_weight) * p2[c] +
                               (1 - x_weight) * y_weight * p3[c] +
                               x_weight * y_weight * p4[c];
                shrunk.pix[(y * shrunk_width + x) * image.ch + c] = (uint8_t)value;
            }
        }
    }

    return shrunk;
}

bool paste_image_at_pos(image_t image_to, image_t image_from, int x, int y, float tone)
{
    size_t image_to_size = get_image_size(image_to);
    size_t image_from_size = get_image_size(image_from);

    if (image_to_size < image_from_size)
    {
        if (DEBUG)
            fprintf(stderr, "ERR: Not posible to paste onto smaller image\n");
        return false;
    }
    else if ((x + image_from.w > image_to.w) || (y + image_from.h > image_to.h))
    {
        if (DEBUG)
            fprintf(stderr, "ERR: Paste coordinates out of bounds %d,%d\n", x, y);
        return false;
    }

    uint8_t *img_to_i = image_to.pix,
            *img_from_i = image_from.pix;

    // go to x, y on image_to
    img_to_i += image_to.ch * (y * image_to.w + x);

    int p = 1;
    while (img_from_i < image_from.pix + image_from_size &&
           img_to_i < image_to.pix + image_to_size)
    {
        // copy pixel
        *img_to_i = tone * (uint8_t)*img_from_i;
        *(img_to_i + 1) = tone * (uint8_t) * (img_from_i + 1);
        *(img_to_i + 2) = tone * (uint8_t) * (img_from_i + 2);

        // jump over every other line
        if (p % image_from.w == 0)
        {
            img_to_i += image_to.ch * (image_to.w - image_from.w);
        }

        // go to next pixel
        img_to_i += image_to.ch;
        img_from_i += image_from.ch;
        p++;
    }

    return true;
}

image_t collage_from_single_image(image_t base, image_t paste, int mode)
{
    if (!check_image_dimensions(base) ||
        !check_image_dimensions(paste) ||
        !(mode == 0 || mode == 1))
    {
        fprintf(stderr, "ERR: invalid input image dimensions or wrong function call\n");
        return image_default;
    }

    image_t collage;
    collage.w = (base.w * paste.w);
    collage.h = (base.h * paste.h);
    collage.ch = paste.ch;

    size_t collage_size = get_image_size(collage);
    print_malloc(collage_size, false);
    collage.pix = malloc(collage_size);
    if (collage.pix == NULL)
    {
        print_malloc_error(collage_size);
        return image_default;
    }

    for (int x = 0; x < base.w; x++)
    {
        for (int y = 0; y < base.h; y++)
        {
            uint8_t *pix = base.pix + (y * base.w + x) * base.ch;
            float Y;
            switch (mode)
            {
            default:
            case 0:
                Y = get_point_luminance(*pix, *(pix + 1), *(pix + 2));
                break;
            case 1:
                Y = fabs(sinf(x * M_PI / base.w) * sinf(y * M_PI / base.h));
                break;
            }

            bool success = paste_image_at_pos(collage, paste, x * paste.w, y * paste.h, Y);
            if (!success)
                break;
        }
    }

    return collage;
}

image_t collage_from_multiple_images(
    image_t creator,
    uint8_t **image_array, int image_width, int image_height, int image_count,
    float *images_luminance, image_shape_t *images_structure,
    bool mode_contour)
{
    if (!check_image_dimensions(creator))
    {
        fprintf(stderr, "ERR: wrong creator image dimensions\n");
        print_image_dimensions("Dim: ", creator);
        return image_default;
    }

    size_t creator_size = get_image_size(creator);
    int fotos_horiz = creator.w / 2,
        fotos_vert = creator.h / 2;

    image_t collage;
    collage.w = fotos_horiz * image_width;
    collage.h = fotos_vert * image_height;
    collage.ch = creator.ch;

    size_t collage_size = get_image_size(collage);
    print_malloc(collage_size, false);
    collage.pix = malloc(collage_size);

    image_shape_t white_structure = {1.0f, 1.0f, 1.0f, 1.0f};

    int image_selection[fotos_vert][fotos_horiz];
    for (int i = 0; i < fotos_vert; i++)
    {
        for (int j = 0; j < fotos_horiz; j++)
        {
            uint8_t *cur_pix = creator.pix + (creator.w * i * 2 + j * 2) * creator.ch;
            uint8_t *right_pix = creator.pix + (creator.w * i * 2 + (j * 2 + 1)) * creator.ch;
            uint8_t *down_pix = creator.pix + (creator.w * (i * 2 + 1) + j) * creator.ch;
            uint8_t *down_right_pix = creator.pix + (creator.w * (i * 2 + 1) + (j * 2 + 1)) * creator.ch;

            if (down_right_pix >= creator.pix + creator_size)
                return collage;

            image_shape_t S = {
                get_point_luminance(*(cur_pix), *(cur_pix + 1), *(cur_pix + 2)),
                get_point_luminance(*(right_pix), *(right_pix + 1), *(right_pix + 2)),
                get_point_luminance(*(down_pix), *(down_pix + 1), *(down_pix + 2)),
                get_point_luminance(*(down_right_pix), *(down_right_pix + 1), *(down_right_pix + 2)),
            };

            // TODO: calculate radius according to amount of photos
            int not_allowed_radius = 2,
                not_allowed[not_allowed_radius * not_allowed_radius],
                not_allowed_count = 0;

            for (int k = i - not_allowed_radius; k <= i; k++)
            {
                for (int l = j - not_allowed_radius; l <= j + not_allowed_radius; l++)
                {
                    if (k >= 0 && l >= 0 && !(k == i && l >= j))
                        not_allowed[not_allowed_count++] = image_selection[k][l];
                }
            }

            int best_image;
            if (mode_contour && get_shape_difference(S, white_structure) < 0.8f)
            {
                best_image = match_any_image_above(
                    0.2f, images_luminance, image_count,
                    not_allowed, not_allowed_count);
            }
            else
            {
                best_image = match_image_by_shape(
                    S, images_structure, image_count,
                    not_allowed, not_allowed_count);
            }

            uint8_t *selected_image = image_array[best_image];
            image_selection[i][j] = best_image;

            image_t selected_image_s;
            selected_image_s.pix = selected_image;
            selected_image_s.w = image_width;
            selected_image_s.h = image_height;
            selected_image_s.ch = collage.ch;
            paste_image_at_pos(collage, selected_image_s,
                               j * image_width, i * image_height, 1);
        }
    }

    return collage;
}

image_t get_contour_image(image_t image)
{
    // TODO
    return image_default;
}

image_t add_border(
    image_t image,
    int border_top, int border_bottom, int border_left, int border_right,
    int border_red, int border_green, int border_blue)
{
    image_t new_image =
        {
            NULL,
            image.w + border_left + border_right,
            image.h + border_top + border_bottom,
            image.ch};

    size_t new_image_size = get_image_size(new_image);
    print_malloc(new_image_size, false);
    new_image.pix = malloc(new_image_size);

    uint8_t *img_i = image.pix, *img_border_i = new_image.pix;
    int image_size = get_image_size(image);
    int p = 0;

    // top border
    img_border_i = put_pixels(
        img_border_i, border_top * new_image.w,
        border_red, border_green, border_blue);

    while (img_i < image.pix + image_size)
    {
        // left border
        if (p % image.w == 0)
        {
            img_border_i = put_pixels(
                img_border_i, border_left,
                border_red, border_green, border_blue);
        }

        // copy pixel
        *img_border_i = (uint8_t)*img_i;
        *(img_border_i + 1) = (uint8_t) * (img_i + 1);
        *(img_border_i + 2) = (uint8_t) * (img_i + 2);

        // right border
        if (p % image.w == image.w - 1)
        {
            img_border_i = put_pixels(
                img_border_i, border_right,
                border_red, border_green, border_blue);
        }

        img_border_i += new_image.ch;
        img_i += image.ch;
        p++;
    }

    // bottom border
    img_border_i = put_pixels(
        img_border_i, border_bottom * new_image.w,
        border_red, border_green, border_blue);

    return new_image;
}