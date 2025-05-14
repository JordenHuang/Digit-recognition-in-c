/**
 * Compress 60000 MNIST training images and labels into a single file
*/

#ifndef _MNIST_COMPRESS_C_
#define _MNIST_COMPRESS_C_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#define NERUALIB_IMPLEMENTATION
#include "neuralib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMG_SIZE 784 // 28*28
#define TRAIN_IMG_PATH "mnist_png/training/"
#define TRAIN_IMG_COUNT 60000
#define TEST_IMG_PATH "mnist_png/testing/"
#define TEST_IMG_COUNT 10000
#define LABEL_COUNT 10

#define LOG(...) fprintf(stderr, __VA_ARGS__)

size_t img_label_start[10];


void load_images(const char *path, Mat train_data_arr, Mat label_arr)
{
    int total_count = TRAIN_IMG_COUNT;
    int count = 0;
    DIR *d;
    struct dirent *dir;
    char dir_names[][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    size_t dir_name_len = 10;
    char dir_path[80], img_file_path[80];
    int w, h, n;
    unsigned char *img_in_data;

    for (size_t i = 0; i < dir_name_len; ++i) {
        img_label_start[i] = count;
        strcpy(dir_path, path);
        strcat(dir_path, dir_names[i]);
        // printf("filepath: %s\n", dir_path);

        d = opendir(dir_path);
        if (d == NULL) {
            LOG("[ERROR] Can not open dir '%s'\n", dir_path);
            return;
        }

        dir = readdir(d);
        while (dir != NULL) {
            if (dir->d_type == DT_REG) {
                // printf("%s\n", dir->d_name);
                // Check if snprintf return negative value, if true, some error happens
                if (snprintf(img_file_path, 80, "%s/%s", dir_path, dir->d_name) < 0) LOG("Error happened at %d, snprintf\n", __LINE__);
                img_in_data = stbi_load(img_file_path, &w, &h, &n, 0);
                // printf("Loaded %s: (w, h, n) = (%d, %d, %d)\n", img_file_path, w, h, n);
                if (n != 1) LOG("[ERROR] Expected grayscale image\n");
                // Pixel data
                for (size_t r = 0; r < (size_t)(w * h); ++r) {
                    NL_MAT_AT(train_data_arr, r, count) = (float)(img_in_data[r]) / 255.f;
                    // if (img_in_data[r] == 0) printf(" ");
                    // else printf("#");
                    // if (r % w == 0) printf("\n");
                }
                // printf("\n");
                // Label
                for (size_t digit = 0; digit < 10; ++digit) {
                    NL_MAT_AT(label_arr, digit, count) = ((digit == i) ? 1 : 0);
                }
                count += 1;
                stbi_image_free(img_in_data);
            }
            // Read next entry
            dir = readdir(d);

            printf("\rLoad count: %d", count);
        }
        closedir(d);
    }
}

void compress_mnist(const char *filename, Mat train_data_arr, Mat label_arr)
{
    FILE *fptr = fopen(filename, "wb");
    if (!fptr) {
        LOG("[ERROR] Can NOT open file: %s\n", filename);
        return;
    }

    // for (size_t r = 0; r < IMG_SIZE; ++r) {
    //     for (size_t c = 0; c < TRAIN_IMG_COUNT; ++c) {
            // fwrite(NL_MAT_AT(train_data_arr, r, c), sizeof(train_data_arr.items[0]), TRAIN_IMG_COUNT, fptr);
    //     }
    // }
    fwrite(train_data_arr.items, sizeof(train_data_arr.items[0]), IMG_SIZE * TRAIN_IMG_COUNT, fptr);
    fwrite(label_arr.items, sizeof(label_arr.items[0]), LABEL_COUNT * TRAIN_IMG_COUNT, fptr);

    fclose(fptr);
}

void load_compressed_mnist(const char *filename, Mat train_data_arr, Mat label_arr)
{
    FILE *fptr = fopen(filename, "rb");
    if (!fptr) {
        LOG("[ERROR] Can NOT open file: %s\n", filename);
        return;
    }

    fread(train_data_arr.items, sizeof(train_data_arr.items[0]), IMG_SIZE * TRAIN_IMG_COUNT, fptr);
    fread(label_arr.items, sizeof(label_arr.items[0]), LABEL_COUNT * TRAIN_IMG_COUNT, fptr);

    fclose(fptr);
}

// int main(void)
// {
//     NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);
//
//     Arena arena_x = arena_new(512 * 1024 * 1024);
//     Arena arena_y = arena_new(512 * 1024 * 1024);
//     Mat x_train_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TRAIN_IMG_COUNT);
//     Mat y_train_labels = nl_mat_alloc_with_arena(&arena_y, 10, TRAIN_IMG_COUNT);
//
//     load_images(TRAIN_IMG_PATH, x_train_images, y_train_labels);
//
//     const char filename[] = "mnist_compressed.bin";
//     compress_mnist(filename, x_train_images, y_train_labels);
//
//     arena_reset(&arena_x);
//     arena_reset(&arena_y);
//
//     load_compressed_mnist(filename, x_train_images, y_train_labels);
//
//     for (size_t c = 0; c < 3; ++c) {
//         for (size_t r = 0; r < y_train_labels.rows; ++r) {
//             printf("%f ", NL_MAT_AT(y_train_labels, r, c));
//         }
//         printf("\n");
//     }
//
//     arena_destroy(arena_x);
//     arena_destroy(arena_y);
//     return 0;
// }

#endif // _MNIST_COMPRESS_C_
