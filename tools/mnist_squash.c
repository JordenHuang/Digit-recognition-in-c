#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#define NERUALIB_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "mnist_squash.h"

int main(void) {
    #define AS_CHAR true
    #define TRAIN_FILENAME "mnist_train_squashed"
    #define TEST_FILENAME "mnist_test_squashed"

    NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);

    Arena arena_x = arena_new(512 * 1024 * 1024);
    Arena arena_y = arena_new(512 * 1024 * 1024);
    Mat x_train_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TRAIN_IMG_COUNT);
    Mat y_train_labels = nl_mat_alloc_with_arena(&arena_y, 10, TRAIN_IMG_COUNT);
    int r;

    printf("[INFO] Loading training images\n");
    r = load_images(TRAIN_IMG_PATH, x_train_images, y_train_labels);
    printf("\n");
    if (r < 0) {
        fprintf(stderr, "[ERROR] Can NOT load images\n");
        return -1;
    } else {
        fprintf(stdout, "[INFO] Images loaded\n");
    }

    /* Saving to file */
    r = store_mnist_mat(TRAIN_FILENAME, x_train_images, y_train_labels, AS_CHAR);
    if (r < 0) {
        fprintf(stderr, "[ERROR] Can NOT store to file: %s"FILE_EXTENSION"\n", TRAIN_FILENAME);
        return -1;
    } else {
        fprintf(stdout, "[INFO] File saved : %s"FILE_EXTENSION"\n", TRAIN_FILENAME);
    }

    arena_reset(&arena_x);
    arena_reset(&arena_y);

    /* Loading from file */
    // r = load_mnist_mat(TRAIN_FILENAME, x_train_images, y_train_labels, AS_CHAR);
    // if (r < 0) {
    //     fprintf(stderr, "[ERROR] Can NOT load from file: %s"FILE_EXTENSION"\n", TRAIN_FILENAME);
    //     return -1;
    // } else {
    //     fprintf(stdout, "[INFO] File loaded : %s"FILE_EXTENSION"\n", TRAIN_FILENAME);
    // }

    // arena_reset(&arena_x);
    // arena_reset(&arena_y);

    /* Verify (labels) */
    // for (size_t c = 0; c < 3; ++c) {
    //     for (size_t r = 0; r < y_train_labels.rows; ++r) {
    //         printf("%f ", NL_MAT_AT(y_train_labels, r, c));
    //     }
    //     printf("\n");
    // }

    /* ============================== */

    Mat x_test_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TEST_IMG_COUNT);
    Mat y_test_labels = nl_mat_alloc_with_arena(&arena_y, 10, TEST_IMG_COUNT);

    printf("[INFO] Loading testing images\n");
    r = load_images(TEST_IMG_PATH, x_test_images, y_test_labels);
    printf("\n");
    if (r < 0) {
        fprintf(stderr, "[ERROR] Can NOT load images\n");
        return -1;
    } else {
        fprintf(stdout, "[INFO] Images loaded\n");
    }

    /* Saving to file */
    r = store_mnist_mat(TEST_FILENAME, x_test_images, y_test_labels, AS_CHAR);
    if (r < 0) {
        fprintf(stderr, "[ERROR] Can NOT store to file: %s"FILE_EXTENSION"\n", TEST_FILENAME);
        return -1;
    } else {
        fprintf(stdout, "[INFO] File saved : %s"FILE_EXTENSION"\n", TEST_FILENAME);
    }

    arena_reset(&arena_x);
    arena_reset(&arena_y);

    /* Loading from file */
    // r = load_mnist_mat(TEST_FILENAME, x_test_images, y_test_labels, AS_CHAR);
    // if (r < 0) {
    //     fprintf(stderr, "[ERROR] Can NOT load from file: %s"FILE_EXTENSION"\n", TEST_FILENAME);
    //     return -1;
    // } else {
    //     fprintf(stdout, "[INFO] File loaded : %s"FILE_EXTENSION"\n", TEST_FILENAME);
    // }

    arena_destroy(arena_x);
    arena_destroy(arena_y);

    printf("\n[INFO] DONE\n");
    return 0;
}
