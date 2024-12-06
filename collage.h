#include <stdbool.h>

static bool DEBUG = false;

static float LUMINANCE_RED = 0.2126f;
static float LUMINANCE_GREEN = 0.7152f;
static float LUMINANCE_BLUE = 0.0722f;
static float LUMINANCE_POWER_CURVE = 2.2f;

void set_debug(bool debug)
{
    DEBUG = debug;
}

float get_point_luminance(int r, int g, int b)
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

    if (DEBUG) printf("malloc: %d\n", *shrunk_width * *shrunk_height * 3);
    uint8_t* shrunk_image = malloc(*shrunk_width * *shrunk_height * 3);

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
    uint8_t* image, int width, int height, 
    int shrunk_width, int shrunk_height) 
{
    if (shrunk_width > width || shrunk_height > height)
    {
        printf("ERR: Shrunk has to be smaller\n");
        return (uint8_t*) NULL;
    }

    uint channels = 3;
    uint line_size = width * channels;
    uint image_size = line_size * height;
    
    int factor_width = width / shrunk_width,
        factor_height = height / shrunk_height;

    uint shrunk_image_size = shrunk_width * shrunk_height * 3;
    int diff_width = width - shrunk_width * factor_width;

    if (DEBUG) printf("malloc: %d\n", shrunk_width * shrunk_height * 3);
    uint8_t* shrunk_image = malloc(shrunk_width * shrunk_height * 3);

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
        if (p % width >= width - factor_width)
            p += width * (factor_height - 1) + factor_width + diff_width;
        else 
            p += factor_width;

        // go to next pixel
        img_i = image + channels * p;
        shrunk_img_i += channels;
    }

    return shrunk_image;
}

void paste_image_at_pos(
    uint8_t *image_to, int width1, int height1, int channels1, 
    uint8_t *image_from, int width2, int height2, int channels2, 
    int x, int y, float tone) 
{
    int image_from_size = channels2*width2*height2;
    int image_to_size = channels1*width1*height1;

    if (image_to_size < image_from_size)
    {
        printf("not posible to paste onto smaller image");
        return;
    }

    uint8_t* img_to_i = image_to, *img_from_i = image_from;

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
}

uint8_t* collage_from_function(
    uint8_t *image, int width, int height, int channels, int mode)
{
    // float circle = fabs(sin(i*M_PI/f) * sin(j*M_PI/f));
    // TODO
    return NULL;
}


uint8_t* collage_from_image(
    uint8_t *image, int width, int height, int channels,
    int *collage_width, int *collage_height) 
{
    int resize = 1;

    uint image_size = width*height*channels;
    *collage_width = width * width;
    *collage_height = height * height;

    float aspect_ratio = (float)width / (float) height;
    float width_factor = 1, height_factor = 1;

    if (resize)
    {
        if (aspect_ratio >= 1) 
            width_factor = 1/aspect_ratio;
        else
            height_factor = aspect_ratio;
    }

    if (DEBUG) printf("ratio: %f, width_factor=%f, height_factor=%f\n", aspect_ratio, width_factor, height_factor);

    *collage_width = (int) ((*collage_width) * width_factor);
    *collage_height = (int) ((*collage_height) * height_factor);

    if (DEBUG) printf("collage: width=%d, height=%d\n", *collage_width, *collage_height);
    
    int collage_size = (*collage_width) * (*collage_height) * channels;

    if (DEBUG) printf("malloc: %d\n", collage_size);
    uint8_t* collage = malloc(collage_size);

    uint8_t* img_i = image;
    if (resize)
    {
        img_i += (int) (channels * (width * (1 - width_factor) + height * (1 - height_factor) * width) / 2);
    }

    for (int i=0; i<height*height_factor; i++) 
    {
        for (int j=0; j<width*width_factor; j++) 
        {
            if (img_i > image + image_size) 
                break;

            int brightness = 0;
            brightness += *img_i;
            brightness += *(img_i+1);
            brightness += *(img_i+2);

            float factor = (float) brightness / (256*3);

            paste_image_at_pos( 
                collage, *collage_width, *collage_height, channels,
                image, width, height, channels, 
                j*width, i*height, factor
            );

            img_i += channels;
        }

        // manchmal brauchts dieses +1, manchmal nicht :(
        if (resize)
            img_i += (int)(height_factor * channels * (width * (1 - width_factor)) + 1);
    }
    
    return collage;
}

uint8_t* collage_from_images(
    uint8_t *creator_image, int creator_width, int creator_height, int channels,
    uint8_t **image_array, int image_width, int image_height, int image_count,
    float *images_luminance) 
{
    // TODO
    int resize = 0;

    int creator_size = creator_width * creator_height * channels;
    int collage_width = creator_width * image_width;
    int collage_height = creator_height * image_height;

    float aspect_ratio = (float)creator_width / (float) creator_height;
    float width_factor = 1, height_factor = 1;

    if (resize)
    {
        if (aspect_ratio >= 1) 
            width_factor = 1/aspect_ratio;
        else
            height_factor = aspect_ratio;
    }

    if (DEBUG) printf("ratio: %f, width_factor=%f, height_factor=%f\n", aspect_ratio, width_factor, height_factor);

    collage_width = (int) (collage_width * width_factor);
    collage_height = (int) (collage_height * height_factor);

    if (DEBUG) printf("collage: width=%d, height=%d\n", collage_width, collage_height);
    
    int collage_size = collage_width * collage_height * channels;

    if (DEBUG) printf("malloc: %d\n", collage_size);
    uint8_t* collage = malloc(collage_size);

    uint8_t* img_i = creator_image;
    if (resize)
    {
        img_i += (int) (channels * (creator_width * (1 - width_factor) + creator_height * (1 - height_factor) * creator_width) / 2);
    }
    
    int image_selection[creator_width][creator_height];
    for (int i=0; i<creator_height*height_factor; i++) 
    {
        for (int j=0; j<creator_width*width_factor; j++) 
        {
            if (img_i > creator_image + creator_size) 
                break;

            float Y = get_point_luminance(*img_i, *(img_i+1), *(img_i+2));

            uint8_t* selected_image;
            float best_distance = 1;
            int best_image = 0;
            for (int k=0; k<image_count; k++)
            {
                float d = fabs(Y - images_luminance[k]);
                if (d < best_distance)
                {
                    if ((i > 0 && k == image_selection[i-1][j]) ||
                        (j > 0 && k == image_selection[i][j-1]))
                        continue;
                        
                    best_distance = d;
                    best_image = k;
                }
            }
            selected_image = image_array[best_image];
            image_selection[i][j] = best_image;

            paste_image_at_pos( 
                collage, collage_width, collage_height, channels,
                selected_image, image_width, image_height, channels, 
                j*image_width, i*image_height, 1
            );

            img_i += channels;
        }

        // manchmal brauchts dieses +1, manchmal nicht :(
        if (resize)
            img_i += (int)(height_factor * channels * (creator_width * (1 - width_factor)) + 1);
    }
    
    return collage;
}