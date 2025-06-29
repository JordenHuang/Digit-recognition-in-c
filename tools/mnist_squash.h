/**
 * Store/Load 60000 MNIST training images and labels to/from a single file
*/

#ifndef _MNIST_COMPRESS_H_
#define _MNIST_COMPRESS_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "neuralib.h"
#include "stb_image.h"

#define IMG_SIZE 784 // 28*28
#define TRAIN_IMG_PATH "../mnist_png/training/"
#define TRAIN_IMG_COUNT 60000
#define TEST_IMG_PATH "../mnist_png/testing/"
#define TEST_IMG_COUNT 10000
#define LABEL_COUNT 10

#define FILE_EXTENSION ".bin"

#define LOG(...) fprintf(stderr, __VA_ARGS__)

/* Load mnist images from path */
int load_images(const char *path, Mat image_data_arr, Mat label_arr);
/* Store the mnsit images mat into our format (.bin file), it's basically the sequence of bytes of Mat.items.
 * Param:
 *   as_char: turn float to char when save to file
 */
int store_mnist_mat(const char *filename_with_no_extension, Mat image_data_arr, Mat label_arr, bool as_char);
/* Load the .bin file into Mat
 * Param:
 *   as_char: Set to true if this param is also true in store_mnist_mat().
 */
int load_mnist_mat(const char *filename_with_no_extension, Mat image_data_arr, Mat label_arr, bool as_char);


int load_images(const char *path, Mat image_data_arr, Mat label_arr) {
    int count = 0;
    DIR *d;
    struct dirent *dir;
    char dir_names[][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    size_t dir_name_len = 10;
    char dir_path[80], img_file_path[80];
    int w, h, n;
    unsigned char *img_in_data;

    for (size_t i = 0; i < dir_name_len; ++i) {
        strcpy(dir_path, path);
        strcat(dir_path, dir_names[i]);
        // printf("filepath: %s\n", dir_path);

        d = opendir(dir_path);
        if (d == NULL) {
            LOG("[ERROR] Can not open dir '%s'\n", dir_path);
            return -1;
        }

        dir = readdir(d);
        while (dir != NULL) {
            if (dir->d_type == DT_REG) {
                // printf("%s\n", dir->d_name);

                /* Check if snprintf return negative value, if true, some error happens */
                if (snprintf(img_file_path, 80, "%s/%s", dir_path, dir->d_name) < 0)
                    LOG("Error happened at %d, snprintf\n", __LINE__);

                /* Load image */
                img_in_data = stbi_load(img_file_path, &w, &h, &n, 0);
                // printf("Loaded %s: (w, h, n) = (%d, %d, %d)\n", img_file_path, w, h, n);
                if (n != 1)
                    LOG("[ERROR] Expected grayscale image\n");

                /* Pixel data (Not normalized) */
                for (size_t r = 0; r < (size_t)(w * h); ++r) {
                    NL_MAT_AT(image_data_arr, r, count) = (float)(img_in_data[r]);
                    // if (img_in_data[r] == 0) printf(" ");
                    // else printf("#");
                    // if (r % w == 0) printf("\n");
                }
                // printf("\n");

                /* Label */
                for (size_t digit = 0; digit < 10; ++digit) {
                    NL_MAT_AT(label_arr, digit, count) = ((digit == i) ? 1 : 0);
                }
                count += 1;
                stbi_image_free(img_in_data);
            }
            /* Read next entry */
            dir = readdir(d);

            printf("\rLoad count: %d", count);
        }
        closedir(d);
    }
    return 0;
}

int store_mnist_mat(const char *filename_with_no_extension, Mat image_data_arr, Mat label_arr, bool as_char) {
    char filename[256] = {0};
    strcpy(filename, filename_with_no_extension);
    strcat(filename, FILE_EXTENSION);

    FILE *fptr = fopen(filename, "wb");
    if (!fptr) {
        LOG("[ERROR] Can NOT open file: %s\n", filename);
        return -1;
    }

    if (as_char) {
        unsigned char uchar_value;
        for (size_t r = 0; r < image_data_arr.rows; ++r) {
            for (size_t c = 0; c < image_data_arr.cols; ++c) {
                uchar_value = NL_MAT_AT(image_data_arr, r, c);
                fwrite(&uchar_value, sizeof(unsigned char), 1, fptr);
            }
        }
        for (size_t r = 0; r < label_arr.rows; ++r) {
            for (size_t c = 0; c < label_arr.cols; ++c) {
                uchar_value = NL_MAT_AT(label_arr, r, c);
                fwrite(&uchar_value, sizeof(unsigned char), 1, fptr);
            }
        }
    } else {
        fwrite(image_data_arr.items, sizeof(image_data_arr.items[0]), image_data_arr.rows * image_data_arr.cols, fptr);
        fwrite(label_arr.items, sizeof(label_arr.items[0]), label_arr.rows * label_arr.cols, fptr);
    }

    fclose(fptr);
    return 0;
}

int load_mnist_mat(const char *filename_with_no_extension, Mat image_data_arr, Mat label_arr, bool as_char) {
    char filename[256] = {0};
    strcpy(filename, filename_with_no_extension);
    strcat(filename, FILE_EXTENSION);

    FILE *fptr = fopen(filename, "rb");
    if (!fptr) {
        LOG("[ERROR] Can NOT open file: %s\n", filename);
        return -1;
    }

    if (as_char) {
        unsigned char uchar_value;
        for (size_t r = 0; r < image_data_arr.rows; ++r) {
            for (size_t c = 0; c < image_data_arr.cols; ++c) {
                fread(&uchar_value, sizeof(unsigned char), 1, fptr);
                NL_MAT_AT(image_data_arr, r, c) = (float)uchar_value;
            }
        }
        for (size_t r = 0; r < label_arr.rows; ++r) {
            for (size_t c = 0; c < label_arr.cols; ++c) {
                (void) fread(&uchar_value, sizeof(unsigned char), 1, fptr);
                NL_MAT_AT(label_arr, r, c) = (float)uchar_value;
            }
        }
    } else {
        fread(image_data_arr.items, sizeof(image_data_arr.items[0]), image_data_arr.rows * image_data_arr.cols, fptr);
        fread(label_arr.items, sizeof(label_arr.items[0]), label_arr.rows * label_arr.cols, fptr);
    }

    fclose(fptr);
    return 0;
}

#endif // _MNIST_COMPRESS_H_
