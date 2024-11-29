#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// CONFIG
static const char FOLDER[] = "./fotos/";
static const char FILENAME[] = "gitti.png";
static const int SHRINK_FACTOR = 5;

static const int DEBUG = 1;


uint8_t* shrink_image(uint8_t* image, int width, int height, int* shrunk_width, int* shrunk_height) 
{
    int factor = SHRINK_FACTOR;

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

void paste_image_at_pos(uint8_t *image_to, int width1, int height1, int channels1, 
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

uint8_t *collage_from_function(uint8_t *image, int width, int height, int channels, int mode)
{
    // float circle = fabs(sin(i*M_PI/f) * sin(j*M_PI/f));
    return NULL;
}


uint8_t *collage_from_image(uint8_t *image, int width, int height, int channels,
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

int main() 
{
    int width, height, bpp;
    char path[50];
    sprintf(path, "%s%s", FOLDER, FILENAME);
    uint8_t* rgb_image = stbi_load(path, &width, &height, &bpp, 3);

    if (rgb_image == NULL)
    {
        printf("image wrong\n");
        return 1;
    }
    
    if (DEBUG) printf("image: width=%d, height=%d, channels=%d\n", width, height, bpp);

    int shrunk_width, shrunk_height;
    uint8_t* shrunk_image = shrink_image(
        rgb_image, width, height, &shrunk_width, &shrunk_height
    );

    if (DEBUG) printf("shrunk image: width=%d, height=%d\n", shrunk_width, shrunk_height);

    int collage_width, collage_height;
    uint8_t *collage = collage_from_image(
        shrunk_image, shrunk_width, shrunk_height, 3,
        &collage_width, &collage_height
    );

    char path_collage[50];
    char path_shrunk[50];
    sprintf(path_collage, "%scollage_%s", FOLDER, FILENAME);
    sprintf(path_shrunk, "%sshrunk_%s", FOLDER, FILENAME);
    stbi_write_jpg(path_collage, collage_width, collage_height, 3, collage, 60);
    stbi_write_jpg(path_shrunk, shrunk_width, shrunk_height, 3, shrunk_image, 60);
    stbi_image_free(rgb_image);
    stbi_image_free(shrunk_image);
    stbi_image_free(collage);

    return 0;
}