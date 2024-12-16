#include <stdio.h>
#include <dirent.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "std_image/stb_image_write.h"

#include "collage.h"


static const int FILENAME_LENGTH = 100;
static const int SHRINK_FACTOR = 10;
static const int FOTO_LIMIT = 600;
static bool VERBOSE_OUTPUT = false;
static bool DEBUG_OUTPUT = false;


void print_usage()
{
    printf("Usage: collage [options] ..\n");
    printf("\tshrink input-image output-path\tShrink image\n");
    printf("\tfun input-image output-path\tCreate collage from single image and sin function\n");
    printf("\tsingle input-image output-path\tCreate collage from single repeated image\n");
    printf("\tmulti input-image folder-of-images output-path collage-size jpg-quality\tCreate collage from bunch of images\n");
    printf("\t\tcollage-size\t\"A1\", \"A2\", \"A3\", \"A4\" or \"widthxheight\"\n");
    printf("\t\tjpg-quality\tNumber between 0 and 100\n");
    printf("Options:\n\t-h --help\tPrint help/usage\n");
    printf("\t-v --verbose\tEnable verbose output\n");
    printf("\t-d --debug\tFor multi: writes a jpg-image for every stage of process\n");
}

char** get_all_filenames(char* folder, int* file_count)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(folder)) == NULL)
    {
        printf("ERR: Could not open directory \"%s\"\n", folder);
        return NULL;
    }

    *file_count = -2;
    while ((ent = readdir(dir)) != NULL) 
        (*file_count)++;   
    rewinddir(dir);

    print_malloc(*file_count * sizeof(char*), false);
    char** filenames = malloc(*file_count * sizeof(char*));

    int i=0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        print_malloc(FILENAME_LENGTH * sizeof(char), false);
        filenames[i] = malloc(FILENAME_LENGTH * sizeof(char));
        strcpy(filenames[i], ent->d_name);
        i++;
    }

    closedir(dir);
    return filenames;
}

void fun_collage(char* input_image, char* output_image)
{
    printf("TBD\n");
}

void shrink_collage(char* input_image, char* output_image)
{
    int shrink_width = 200, shrink_height = 200;
    int shrink_algorithm = 1;

    int width, height, bpp;
    uint8_t* image = stbi_load(input_image, &width, &height, &bpp, 3);

    if (image == NULL)
    {
        printf("input_image wrong\n");
        return;
    }
    
    if (VERBOSE_OUTPUT) printf("Shrink with algorithm %d\n", shrink_algorithm);

    uint8_t* shrunk_image;
    switch (shrink_algorithm)
    {
        case 0: default:
            shrunk_image = shrink_image_size(
                image, width, height, bpp, shrink_width, shrink_height
            );
            break;
        case 1:
            shrunk_image = shrink_image_factor(
                image, width, height, &shrink_width, &shrink_height, 5
            );
    }


    if (shrunk_image == NULL)
    {
        printf("ERR: fault on resize method\n");
    }
    else 
    {
        if (VERBOSE_OUTPUT) printf("Shrunk to size %dx%d\n", shrink_width, shrink_height);
        stbi_write_jpg(output_image, shrink_width, shrink_height, 3, shrunk_image, 90);
    }

    stbi_image_free(image);
    stbi_image_free(shrunk_image);
}

void single_collage(char* input_image, char* output_image)
{
    int shrink_factor = SHRINK_FACTOR;
    int width, height, bpp;
    uint8_t* image = stbi_load(input_image, &width, &height, &bpp, 3);

    if (image == NULL)
    {
        printf("input_image wrong\n");
        return;
    }
    
    if (VERBOSE_OUTPUT) printf("image: width=%d, height=%d, channels=%d\n", width, height, bpp);

    int shrunk_width, shrunk_height;
    uint8_t* shrunk_image = shrink_image_factor(
        image, width, height, &shrunk_width, &shrunk_height, shrink_factor
    );

    stbi_image_free(image);

    if (VERBOSE_OUTPUT) printf("shrunk image: width=%d, height=%d\n", shrunk_width, shrunk_height);

    int collage_width, collage_height;
    uint8_t* collage = collage_from_single_image(
        shrunk_image, shrunk_width, shrunk_height, 3,
        &collage_width, &collage_height
    );
    
    stbi_write_jpg(output_image, collage_width, collage_height, 3, collage, 60);

    stbi_image_free(shrunk_image);
    stbi_image_free(collage);
}

void multi_collage(
    char* input_image_path, char* output_image_path, char* image_folder,
    char* collage_size_identifier, int border_size_guidance, int foto_size,
    int jpg_quality, bool debug_and_write)
{
    int collage_width, collage_height;

    if (strcmp(collage_size_identifier, "A1") == 0)
    {
        collage_width = A1_WIDTH_300PPI;
        collage_height = A1_HEIGHT_300PPI;
    }
    else if (strcmp(collage_size_identifier, "A2") == 0)
    {
        collage_width = A2_WIDTH_300PPI;
        collage_height = A2_HEIGHT_300PPI;
    }
    else if (strcmp(collage_size_identifier, "A3") == 0)
    {
        collage_width = A3_WIDTH_300PPI;
        collage_height = A3_HEIGHT_300PPI;
    }
    else if (strcmp(collage_size_identifier, "A4") == 0)
    {
        collage_width = A4_WIDTH_300PPI;
        collage_height = A4_HEIGHT_300PPI;
    }
    else
    {
        char* pos_of_x = strchr(collage_size_identifier, 'x');

        if (pos_of_x != NULL)
        {
            int width_cand = atoi(collage_size_identifier);
            int height_cand = atoi(pos_of_x + 1);

            if (width_cand == -1 || height_cand == -1)
            {
                printf("ERR: collage_size wrong %sx%s\n", collage_size_identifier, pos_of_x);
                return;
            }

            collage_width = width_cand;
            collage_height = height_cand;
        }
        else
        {
            printf("ERR: collage_size wrong \"%s\"\n", collage_size_identifier);
            return;
        }        
    }

    int fotos_per_row = floor((float) collage_width / foto_size);
    int foto_width = floor((float) collage_width / fotos_per_row),
        foto_height = foto_width;
    int fotos_horiz = (int) floor((float)(collage_width - 2*border_size_guidance) / foto_width),
        fotos_vert = (int) floor((float)(collage_height - 2*border_size_guidance) / foto_height);
    int collage_inner_width = foto_width * fotos_horiz,
        collage_inner_height = foto_height * fotos_vert;
    int border_left = floor((collage_width - collage_inner_width) / 2.0f),
        border_right = ceil((collage_width - collage_inner_width) / 2.0f),
        border_top = floor((collage_height - collage_inner_height) / 2.0f),
        border_bottom = ceil((collage_height - collage_inner_height) / 2.0f);

    if (VERBOSE_OUTPUT)
    {
        printf("collage fotos: %dx%d\n", fotos_horiz, fotos_vert);
        printf("foto dimensions: %dx%d px\n", foto_width, foto_height);
        printf("collage inner dimensions: %dx%d px\n", collage_inner_width, collage_inner_height);
        printf("border (t,r,b,l): %d, %d, %d, %d px\n", 
            border_top, border_right, border_bottom, border_left);
    }


    /*  Load creator image and shrink  */
    if (VERBOSE_OUTPUT) printf("Load main image\n");

    int creator_width, creator_height, creator_bpp;
    uint8_t* creator_image = stbi_load(input_image_path, &creator_width, &creator_height, &creator_bpp, 3);

    if (creator_image == NULL)
    {
        printf("ERR: input_image wrong\n");
        return;
    }
    
    uint8_t* creator_shrunk = shrink_image_size(
        creator_image, creator_width, creator_height, creator_bpp,
        fotos_horiz, fotos_vert
    );

    if (VERBOSE_OUTPUT) printf("Main image %s with dim %dx%d px\n", input_image_path, creator_width, creator_height);
    if (debug_and_write) stbi_write_jpg("main-image.jpg", fotos_horiz, fotos_vert, 3, creator_shrunk, jpg_quality);
    stbi_image_free(creator_image);


    /*  Load fotos  */

    int foto_count, suitable_foto_count = 0;
    char** filenames = get_all_filenames(image_folder, &foto_count);
    if (filenames == NULL)
    {
        stbi_image_free(creator_shrunk);
        return;
    }

    foto_count = fmin(foto_count, FOTO_LIMIT);

    if (VERBOSE_OUTPUT) printf("%d fotos found\n", foto_count);

    uint8_t *all_images[foto_count];
    float images_luminance[foto_count];
    struct image_structure_t images_structure[foto_count];

    if (!VERBOSE_OUTPUT) printf("load images (%d)", foto_count);
    for (int i=0; i<foto_count; i++)
    {
        if (!VERBOSE_OUTPUT) printf(".");

        int width, height, bpp;
        char path[200];
        sprintf(path, "%s/%s", image_folder, filenames[i]);
        uint8_t* image = stbi_load(path, &width, &height, &bpp, 3);

        if (image == NULL ||
            width < foto_width || height < foto_height || bpp != 3)
        {
            all_images[i] = NULL;
            images_luminance[i] = 0;
            images_structure[i] = get_null_structure();
            if (VERBOSE_OUTPUT) printf("WARN: foto %s not suitable with %dx%d\n", filenames[i], width, height);
            continue;
        }

        uint8_t* image_cut = shrink_image_size(
            image, width, height, bpp, foto_width, foto_height
        );        
        float Y = get_average_luminance(image_cut, foto_width, foto_height, bpp); 
        all_images[i] = image_cut;
        images_luminance[i] = Y;
        struct image_structure_t s_image = get_image_structure(
            image_cut, foto_width, foto_height, bpp
        );
        images_structure[i] = s_image;

        if (VERBOSE_OUTPUT) printf("%d brightness %f of image %s\n", i, Y, filenames[i]);
        stbi_image_free(image);
        suitable_foto_count++;
    }

    if (!VERBOSE_OUTPUT) printf("\n");

    if (suitable_foto_count == 0) 
    {
        printf("ERR: probably wrong image folder\n");
        free(filenames);
        return;
    } 


    /*  Create collage  */

    uint8_t* collage_inner = collage_from_multiple_images(
        creator_shrunk, fotos_horiz, fotos_vert, 3,
        all_images, foto_width, foto_height, foto_count,
        images_luminance, images_structure
    );
    stbi_image_free(creator_shrunk);
    for (int i=0; i<foto_count; i++) stbi_image_free(all_images[i]);
    if (VERBOSE_OUTPUT) printf("Collage created with %dx%d images\n", fotos_horiz, fotos_vert);

    if (border_size_guidance == 0)
    {
        stbi_write_jpg(output_image_path, collage_inner_width, collage_inner_height, 3, collage_inner, jpg_quality);
    }
    else
    {
        /*  Add border  */

        if (debug_and_write) stbi_write_jpg(output_image_path, collage_inner_width, collage_inner_height, 3, collage_inner, jpg_quality);
        if (VERBOSE_OUTPUT) printf("add border: %d %d %d %d px\n", border_top, border_right, border_bottom, border_left);
        
        uint8_t* collage_with_border = add_border(
            collage_inner, collage_inner_width, collage_inner_height, 3, 
            border_top, border_bottom, border_left, border_right, 
            255, 255, 255
        );
        stbi_write_jpg(output_image_path, collage_width, collage_height, 3, collage_with_border, jpg_quality);
        stbi_image_free(collage_with_border);
    }

    stbi_image_free(collage_inner);
    free(filenames);
}

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        print_usage();
        return -1;
    }

    clock_t start_time = clock();
    int no_options = 0;

    for (int i=1; i<4 && i<argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            VERBOSE_OUTPUT = true;
            set_debug(VERBOSE_OUTPUT);
            no_options++;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0)
        {
            DEBUG_OUTPUT = true;
            no_options++;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_usage();
            return 0;
        }
    }

    char action[20];
    char input_image[FILENAME_LENGTH];
    char output_image[FILENAME_LENGTH];

    strcpy(action, argv[1 + no_options]);
    strcpy(input_image, argv[2 + no_options]);

    if (strcmp(action, "multi") == 0 && argc >= 5)
    {
        char image_folder[100], collage_size_id[50], jpg_quality_str[50];
        int jpg_quality; 
        strcpy(image_folder, argv[3 + no_options]);
        strcpy(output_image, argv[4 + no_options]);
        strcpy(collage_size_id, argv[5 + no_options]);
        strcpy(jpg_quality_str, argv[6 + no_options]);
        jpg_quality = atoi(jpg_quality_str);

        multi_collage(
            input_image, output_image, image_folder,
            collage_size_id, 0, 118,
            jpg_quality, false
        );
    }
    else if (strcmp(action, "shrink") == 0)
    {
        strcpy(output_image, argv[3 + no_options]);
        shrink_collage(input_image, output_image);
    }
    else if (strcmp(action, "fun") == 0)
    {
        strcpy(output_image, argv[3 + no_options]);
        fun_collage(input_image, output_image);
    }
    else if (strcmp(action, "single") == 0)
    {
        strcpy(output_image, argv[3 + no_options]);
        single_collage(input_image, output_image);
    }
    else
    {
        printf("what is this action?? \"%s\"\n\n", action);
        print_usage();
    }

    clock_t end_time = clock();
    double exec_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Time: %.4f s, ", exec_time);
    printf("Memory: %.4f MB\n", TOTAL_MALLOC / 1000000.0f);

    return 0;
}