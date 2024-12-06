#include <stdio.h>
#include <dirent.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std_image/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "std_image/stb_image_write.h"

#include "collage.h"


static const int FILENAME_LENGTH = 100;
static bool DEBUG_OUTPUT = false;
static int SHRINK_FACTOR = 10;


void print_usage()
{
    printf("Usage: collage [options] action input-image output-image [folder-of-images]\n");
    printf("Actions:\n");
    printf("\tfun\tCreate collage from single image and sin function\n");
    printf("\tsingle\tCreate collage from single repeated image\n");
    printf("\tmulti\tCreate collage from bunch of images (you have to provide folder-of-images)\n");
    printf("Options:\n\t-h --help\tPrint help/usage\n");
    printf("\t-v --verbose\tEnable verbose output\n");
}

char** get_all_filenames(char* folder, int* file_count)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(folder)) == NULL)
    {
        printf("Could not open directory");
        return NULL;
    }

    *file_count = -2;
    while ((ent = readdir(dir)) != NULL) 
        (*file_count)++;   
    rewinddir(dir);

    char** filenames = malloc(*file_count * sizeof(char*));

    int i=0;
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

void fun_collage()
{
    printf("TBD\n");
}

void single_collage(char* input_image, char* output_image)
{
    int shrink_factor = SHRINK_FACTOR;

    int width, height, bpp;
    uint8_t* rgb_image = stbi_load(input_image, &width, &height, &bpp, 3);

    if (rgb_image == NULL)
    {
        printf("input_image wrong\n");
        return;
    }
    
    if (DEBUG) printf("image: width=%d, height=%d, channels=%d\n", width, height, bpp);

    int shrunk_width, shrunk_height;
    uint8_t* shrunk_image = shrink_image_factor(
        rgb_image, width, height, &shrunk_width, &shrunk_height, shrink_factor
    );

    stbi_image_free(rgb_image);

    if (DEBUG) printf("shrunk image: width=%d, height=%d\n", shrunk_width, shrunk_height);

    int collage_width, collage_height;
    uint8_t *collage = collage_from_image(
        shrunk_image, shrunk_width, shrunk_height, 3,
        &collage_width, &collage_height
    );
    
    stbi_write_jpg(output_image, collage_width, collage_height, 3, collage, 60);
    
    stbi_image_free(shrunk_image);
    stbi_image_free(collage);
}

void multi_collage(char* input_image, char* output_image, char* image_folder)
{
    int file_count;
    char** filenames = get_all_filenames(image_folder, &file_count);

    if (DEBUG_OUTPUT)
        printf("%d fotos found\n", file_count);

    int desired_width = 80, desired_height = 60;

    uint8_t *all_images[file_count];
    float images_luminance[file_count];

    for (int i=0; i<file_count; i++)
    {
        int width, height, bpp;
        char path[50];
        sprintf(path, "%s%s", image_folder, filenames[i]);
        uint8_t* image = stbi_load(path, &width, &height, &bpp, 3);

        uint8_t* image_cut = shrink_image_size(image, width, height, desired_width, desired_height);        
        float Y = get_average_luminance(image_cut, desired_width, desired_height, bpp); 

        all_images[i] = image_cut;
        images_luminance[i] = Y;

        printf("%d brightness %f of image %s\n", i, Y, filenames[i]);

        free(image);
    }

    int creator_width, creator_height, creator_bpp;
    uint8_t* creator_image = stbi_load(input_image, &creator_width, &creator_height, &creator_bpp, 3);
    uint8_t* creator_shrunk = shrink_image_size(
        creator_image, creator_width, creator_height,
        80, 60
    );

    int collage_width = desired_width * desired_width,
        collage_height = desired_height * desired_height;

    uint8_t* collage = collage_from_images(
        creator_shrunk, 80, 60, 3,
        all_images, desired_width, desired_height, file_count,
        images_luminance
    );

    stbi_write_jpg(output_image, collage_width, collage_height, 3, collage, 80);

    for (int i=0; i<file_count; i++)
        stbi_image_free(all_images[i]);
    stbi_image_free(collage);
    stbi_image_free(creator_image);
    stbi_image_free(creator_shrunk);
    free(filenames);
}

int main(int argc, char* argv[])
{
    if (argc < 4 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_usage();
        return 0;
    }

    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--verbose") == 0)
    {
        DEBUG_OUTPUT = true;
        set_debug(DEBUG_OUTPUT);
    }

    char action[20];
    char input_image[FILENAME_LENGTH];
    char output_image[FILENAME_LENGTH];
    if (argc == 4)
    {
        strcpy(action, argv[1]);
        strcpy(input_image, argv[2]);
        strcpy(output_image, argv[3]);
    }
    else
    {
        strcpy(action, argv[2]);
        strcpy(input_image, argv[3]);
        strcpy(output_image, argv[4]);
    }

    if (strcmp(action, "multi") == 0 && argc >= 5)
    {
        char image_folder[100];
        strcpy(image_folder, argv[argc-1]);
        multi_collage(input_image, output_image, image_folder);
    }
    else if (strcmp(action, "fun") == 0)
    {
        fun_collage(input_image, output_image);
    }
    else if (strcmp(action, "single") == 0)
    {
        single_collage(input_image, output_image);
    }
    else
    {
        print_usage();
    }

    return 0;
}