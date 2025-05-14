#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#include "mnist_compress.c"

#if 0 //{
#define NERUALIB_IMPLEMENTATION
#include "neuralib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMG_SIZE 784 // 28*28
#define TRAIN_IMG_PATH "mnist_png/training/"
#define TRAIN_IMG_COUNT 60000
#define TEST_IMG_PATH "mnist_png/testing/"
#define TEST_IMG_COUNT 10000

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

#endif //}

int main(void)
{
    // printf("===== Prepare training data\n");
    // int w, h, n;
    // unsigned char *img_in_data = stbi_load(img_in_path, &w, &h, &n, 0);

    printf("===== Set some parameters\n");
    NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);
    nl_rand_init(false, 0);
    NL_PRINT_COST_EVERY_N_EPOCHS(10);

    printf("===== Allocate space\n");
    Arena arena_x = arena_new(512 * 1024 * 1024);
    Arena arena_y = arena_new(512 * 1024 * 1024);
    Arena arena_train = arena_new(512 * 1024 * 1024);
    // FIXME: y_train_labels must declare before x_train_images, but I don't know why
    Mat x_train_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TRAIN_IMG_COUNT);
    Mat y_train_labels = nl_mat_alloc_with_arena(&arena_y, 10, TRAIN_IMG_COUNT);

    printf("===== Prepare training data\n");
    // load_images(TRAIN_IMG_PATH, x_train_images, y_train_labels);
    load_compressed_mnist("mnist_compressed.bin", x_train_images, y_train_labels);

    printf("===== Define model layer components\n");
    NeuralNet model;
    size_t layers[] = {IMG_SIZE, 16, 16, 10};
    Activation_type acts[] = {RELU, RELU, SOFTMAX};
    // size_t layers[] = {IMG_SIZE, 32, 32, 10};
    // Activation_type acts[] = {RELU, RELU, SOFTMAX};

    printf("===== Define model layers\n");
    nl_define_layers_with_arena(&arena_train, &model, NL_ARRAY_LEN(layers), layers, acts, CROSS_ENTROPY);

    nl_model_summary(model, stdout);

    // // View the image
    // int offset = 2;
    // for(int c = 0; c < 10; ++c) {
    //     for (int i = 0; i < 784; ++i) {
    //         if (i % 28 == 0) printf("\n");
    //         if (x_train_images.items[(img_label_start[c] + offset) + x_train_images.cols * i] == 0) printf(".");
    //         else printf("#");
    //     }
    //     for (int i = 0; i < 10; ++i) {
    //         printf("%.0f ", y_train_labels.items[(img_label_start[c] + offset) + y_train_labels.cols * i]);
    //     }
    //     printf("\n\n");
    // }

#if 1
    printf("===== Shuffle training data\n");
    Mat arr[] = {x_train_images, y_train_labels};
    for (size_t i = 0; i < 50; ++i) nl_mat_shuffle_array(arr, 2);

// for (size_t i = 0; i < TRAIN_IMG_COUNT; ++i) {
//     size_t idx;
//     float m = 0.f;
//     for (size_t j = 0; j < 10; ++j) {
//         if (NL_MAT_AT(y_train_labels, j, i) > m) {
//             m = NL_MAT_AT(y_train_labels, j, i);
//             idx = j;
//         }
//     }
//     printf("%zu ", idx);
// }
    printf("===== Train\n");
    float lr = 7e-3;
    size_t epochs = 100;
    size_t batch_size = 10;
    nl_model_train(model, x_train_images, y_train_labels, lr, epochs, batch_size, false);
 // nl_model_train(NeuralNet model, Mat xs, Mat ys, float lr, size_t epochs, size_t batch_size, bool shuffle)

    // Save model
    const char model_path[] = "digitRecog.model";
    nl_model_save(model_path, model);
    printf("Model [%s] saved\n", model_path);
#endif

    arena_destroy(arena_x);
    arena_destroy(arena_y);
    arena_destroy(arena_train);


    return 0;
}
