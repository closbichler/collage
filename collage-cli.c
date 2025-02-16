#include <stdio.h>
#include <dirent.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "std_image/stb_image_write.h"

#include "collage.h"

/* Configuration */

static const int FOTO_LIMIT = 600; // for multi
static const int FILENAME_LENGTH = 100;
static const int DEFAULT_JPG_QUALITY = 70;
static bool VERBOSE_OUTPUT = false; // can be set with -v
static bool DEBUG_OUTPUT = false;   // can be set with -d

/* Miscellaneous methods */

void print_usage()
{
    printf("Usage:\n");
    printf("\tcollage [..] shrink INPUT_IMAGE OUTPUT_PATH\n");
    printf("\tcollage [..] single INPUT_IMAGE OUTPUT_PATH MODE\n");
    printf("\tcollage [..] multi INPUT_IMAGE IMAGE_FOLDER OUTPUT_PATH COLLAGE_SIZE JPG_QUALITY\n");

    printf("\narguments:\n");
    printf("\tINPUT_IMAGE\tpath to image\n");
    printf("\tOUTPUT_PATH\tath of output-image to write\n");
    printf("\tINPUT_FOLDER\tpath of folder, which images are included in the collage\n");
    printf("\tMODE\t0 = based on INPUT_IMAGE, 1 = circle\n");
    printf("\tCOLLAGE_SIZE\t\"widthxheight\" or \"A1\", \"A2\", \"A3\", \"A4\"\n");
    printf("\tJPG_QUALITY\tinteger between 0 and 100\n");

    printf("\noptions:\n");
    printf("\t-h --help\tshow help/usage\n");
    printf("\t-v --verbose\tenable verbose logs\n");
    printf("\t-d --debug\t(for multi) write images for every stage in process\n");
}

char **get_all_filenames(char *folder, int *file_count)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(folder)) == NULL)
    {
        fprintf(stderr, "ERR: Could not open directory \"%s\"\n", folder);
        return NULL;
    }

    *file_count = -2;
    while ((ent = readdir(dir)) != NULL)
        (*file_count)++;
    rewinddir(dir);

    char **filenames = malloc(*file_count * sizeof(char *));

    int i = 0;
    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        filenames[i] = malloc(FILENAME_LENGTH * sizeof(char));
        strcpy(filenames[i], ent->d_name);
        i++;
    }

    closedir(dir);
    return filenames;
}

bool write_image(char *output_path, image_t image, int quality)
{
    stbi_write_jpg(output_path, image.w, image.h, image.ch, image.pix, quality);
    return true;
}

/* Implementation */

void shrink_collage(char *input_image, char *output_image)
{
    int shrink_width = 200, shrink_height = 200;
    int shrink_algorithm = 1;

    image_t image;
    image.pix = stbi_load(input_image, &image.w, &image.h, &image.ch, 3);

    if (image.pix == NULL)
    {
        printf("input_image wrong\n");
        return;
    }

    if (VERBOSE_OUTPUT)
        printf("Shrink with algorithm %d\n", shrink_algorithm);

    image_t shrunk_image;
    switch (shrink_algorithm)
    {
    case 0:
    default:
        shrunk_image = shrink_image_size(image, shrink_width, shrink_height);
        break;
    case 1:
        shrunk_image = shrink_image_factor(image, 5);
    }

    if (shrunk_image.pix == NULL)
    {
        fprintf(stderr, "ERR: fault on resize method\n");
    }
    else
    {
        if (VERBOSE_OUTPUT)
            print_image_dimensions("Shrunk to size", shrunk_image);
        write_image(output_image, shrunk_image, DEFAULT_JPG_QUALITY);
    }

    stbi_image_free(image.pix);
    stbi_image_free(shrunk_image.pix);
}

void single_collage(char *input_image, char *output_image, int mode)
{
    float shrink_factor = 10;

    image_t image;
    image.pix = stbi_load(input_image, &image.w, &image.h, &image.ch, 3);

    if (image.pix == NULL)
    {
        printf("input_image wrong\n");
        return;
    }

    if (VERBOSE_OUTPUT)
        print_image_dimensions("image", image);

    image_t shrunk_image = shrink_image_factor(image, shrink_factor);

    if (VERBOSE_OUTPUT)
        print_image_dimensions("shrunk image", shrunk_image);

    int image_squared_size = fmin(shrunk_image.w, shrunk_image.h);
    image_t squared_image = shrink_image_size(image, image_squared_size, image_squared_size);

    stbi_image_free(image.pix);

    if (VERBOSE_OUTPUT)
        print_image_dimensions("squared image", squared_image);

    image_t collage = collage_from_single_image(shrunk_image, squared_image, mode);

    if (collage.pix != NULL)
    {
        if (VERBOSE_OUTPUT)
            print_image_dimensions("collage single", collage);
        write_image(output_image, collage, DEFAULT_JPG_QUALITY);
        stbi_image_free(collage.pix);
    }

    stbi_image_free(shrunk_image.pix);
}

void multi_collage(
    char *input_image_path, char *output_image_path, char *image_folder,
    char *collage_size_identifier, int border_size_guidance, int foto_size,
    int jpg_quality, bool mode_contour)
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
        char *pos_of_x = strchr(collage_size_identifier, 'x');

        if (pos_of_x != NULL)
        {
            int width_cand = atoi(collage_size_identifier);
            int height_cand = atoi(pos_of_x + 1);

            if (width_cand == -1 || height_cand == -1)
            {
                fprintf(stderr, "ERR: collage_size wrong %sx%s\n", collage_size_identifier, pos_of_x);
                return;
            }

            collage_width = width_cand;
            collage_height = height_cand;
        }
        else
        {
            fprintf(stderr, "ERR: collage_size wrong \"%s\"\n", collage_size_identifier);
            return;
        }
    }

    int fotos_per_row = floor((float)collage_width / foto_size);
    int foto_width = floor((float)collage_width / fotos_per_row),
        foto_height = foto_width;
    int fotos_horiz = (int)floor((float)(collage_width - 2 * border_size_guidance) / foto_width),
        fotos_vert = (int)floor((float)(collage_height - 2 * border_size_guidance) / foto_height);
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
    if (VERBOSE_OUTPUT)
        printf("Load main image\n");

    image_t creator_image;
    creator_image.pix = stbi_load(input_image_path, &creator_image.w, &creator_image.h, &creator_image.ch, 3);

    if (creator_image.pix == NULL)
    {
        fprintf(stderr, "ERR: input_image wrong\n");
        return;
    }

    if (VERBOSE_OUTPUT)
        print_image_dimensions("Main image loaded", creator_image);

    image_t creator_shrunk = shrink_image_size(creator_image, fotos_horiz * 2, fotos_vert * 2);

    if (VERBOSE_OUTPUT)
        print_image_dimensions("Main image shrunk", creator_shrunk);
    if (DEBUG_OUTPUT)
        write_image("main-image.jpg", creator_shrunk, jpg_quality);
    stbi_image_free(creator_image.pix);

    /*  Load fotos  */

    int foto_count, suitable_foto_count = 0;
    char **filenames = get_all_filenames(image_folder, &foto_count);
    if (filenames == NULL)
    {
        stbi_image_free(creator_shrunk.pix);
        return;
    }

    foto_count = fmin(foto_count, FOTO_LIMIT);

    if (VERBOSE_OUTPUT)
        printf("%d fotos found\n", foto_count);

    uint8_t *all_images[foto_count];
    float images_luminance[foto_count];
    image_shape_t images_structure[foto_count];

    if (!VERBOSE_OUTPUT)
        printf("load images (%d)", foto_count);
    for (int i = 0; i < foto_count; i++)
    {
        if (!VERBOSE_OUTPUT)
            printf(".");
        if (!VERBOSE_OUTPUT)
            fflush(stdout);

        image_t image;
        char path[200];
        sprintf(path, "%s/%s", image_folder, filenames[i]);
        image.pix = stbi_load(path, &image.w, &image.h, &image.ch, 3);

        if (image.pix == NULL ||
            image.w < foto_width || image.h < foto_height || image.ch != 3)
        {
            all_images[i] = NULL;
            images_luminance[i] = 0;
            images_structure[i] = image_shape_default;
            if (VERBOSE_OUTPUT)
                printf("WARN: foto %s not suitable with %dx%d\n", filenames[i], image.w, image.h);
            continue;
        }

        image_t image_cut = shrink_image_size(image, foto_width, foto_height);
        float Y = get_average_luminance(image_cut);
        all_images[i] = image_cut.pix;
        images_luminance[i] = Y;
        image_shape_t S = get_image_shape(image_cut);
        images_structure[i] = S;

        if (VERBOSE_OUTPUT)
            printf("%d image %s with brightness %.2f and struct (%.2f,%.2f,%.2f,%.2f)\n", i, filenames[i], Y, S.y1, S.y2, S.y3, S.y4);
        stbi_image_free(image.pix);
        suitable_foto_count++;
    }

    if (!VERBOSE_OUTPUT)
        printf("\n");

    if (suitable_foto_count == 0)
    {
        fprintf(stderr, "ERR: probably wrong image folder\n");
        free(filenames);
        return;
    }

    /*  Create collage  */

    image_t collage_inner = collage_from_multiple_images(
        creator_shrunk, all_images, foto_width, foto_height, foto_count,
        images_luminance, images_structure, mode_contour);
    stbi_image_free(creator_shrunk.pix);
    for (int i = 0; i < foto_count; i++)
        stbi_image_free(all_images[i]);
    if (VERBOSE_OUTPUT)
        printf("Collage created with %dx%d images\n", fotos_horiz, fotos_vert);

    if (border_size_guidance == 0)
    {
        write_image(output_image_path, collage_inner, jpg_quality);
    }
    else
    {
        /*  Add border  */

        if (DEBUG_OUTPUT)
            write_image(output_image_path, collage_inner, jpg_quality);
        if (VERBOSE_OUTPUT)
            printf("add border: %d %d %d %d px\n", border_top, border_right, border_bottom, border_left);

        image_t collage_with_border = add_border(
            collage_inner, border_top, border_bottom, border_left, border_right,
            255, 255, 255);
        write_image(output_image_path, collage_with_border, jpg_quality);
        stbi_image_free(collage_with_border.pix);
    }

    stbi_image_free(collage_inner.pix);
    free(filenames);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        print_usage();
        return -1;
    }

    clock_t start_time = clock();
    int no_options = 0;

    for (int i = 1; i < 4 && i < argc; i++)
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

    if (strcmp(action, "multi") == 0 && argc >= 7)
    {
        char image_folder[FILENAME_LENGTH], collage_size_id[20], jpg_quality_str[5];
        int jpg_quality;
        strcpy(image_folder, argv[3 + no_options]);
        strcpy(output_image, argv[4 + no_options]);
        strcpy(collage_size_id, argv[5 + no_options]);
        strcpy(jpg_quality_str, argv[6 + no_options]);
        jpg_quality = atoi(jpg_quality_str);

        bool mode_contour = true;
        if (argc > 7 + no_options && strcmp(argv[7 + no_options], "false") == 0)
            mode_contour = false;

        // TODO: calculate size according to collage dimensions
        int foto_size = 118; // default = 118 (good for print 300dpi)

        multi_collage(
            input_image, output_image, image_folder,
            collage_size_id, 0, foto_size,
            jpg_quality, mode_contour);
    }
    else if (strcmp(action, "shrink") == 0)
    {
        strcpy(output_image, argv[3 + no_options]);
        shrink_collage(input_image, output_image);
    }
    else if (strcmp(action, "single") == 0)
    {
        char mode_str[4];
        int mode;
        strcpy(output_image, argv[3 + no_options]);
        strcpy(mode_str, argv[4 + no_options]);
        mode = atoi(mode_str);
        single_collage(input_image, output_image, mode);
    }
    else
    {
        printf("what is this action?? \"%s\"\n\n", action);
        print_usage();
    }

    clock_t end_time = clock();
    double exec_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Time: %.4f s, ", exec_time);
    printf("Memory: %.4f MB\n", TOTAL_MALLOC / 1000000.0);

    return 0;
}