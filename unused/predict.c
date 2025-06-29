#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define NERUALIB_IMPLEMENTATION
#include "neuralib.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define IMG_SIZE 784 // 28*28

#define LOG(...) fprintf(stderr, __VA_ARGS__)

void to_grayscale(unsigned char *dst, const unsigned char *src, int src_channel)
{
    int w, h, idx;
    // RGB to grayscale
    // 0.299×R+0.587×G+0.114×B
    for (h = 0; h < 28; ++h) {
        for (w = 0; w < 28 * src_channel; w += src_channel) {
            idx = h * 28 * src_channel + w;
            dst[h * 28 + w / src_channel] = (unsigned char)(
                (float)(src[idx])     * 0.299 +
                (float)(src[idx + 1]) * 0.587 +
                (float)(src[idx + 2]) * 0.114
            );
            // printf("%d ", w % src_channel);
            // printf("%3u ", dst[h * 28 + w / src_channel]);
        }
        // printf("\n");
    }
}

void predict_all()
{
    // const char model_name[] = "digitRecog.model";
    // const char model_name[] = "0514_32.model";
    const char model_name[] = "0514_16.model";
    int w, h, n;
    unsigned char *img_data;
    char image_path[20];
    int count;

    printf("===== Set some parameters\n");
    NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);
    nl_rand_init(false, 0);
    NL_PRINT_COST_EVERY_N_EPOCHS(1);

    printf("===== Allocate space\n");
    Arena arena_input = arena_new(512 * 1024 * 1024);
    Arena arena_output = arena_new(512 * 1024 * 1024);
    Arena arena_model = arena_new(512 * 1024 * 1024);
    Mat input_image = nl_mat_alloc_with_arena(&arena_input, IMG_SIZE, 1);
    Mat prediction = nl_mat_alloc_with_arena(&arena_output, 10, 1);

    printf("===== Define model layer components\n");
    NeuralNet model;
    nl_model_load_with_arena(&arena_model, model_name, &model);
    printf("[INFO] Model loaded\n");

    // printf("===== Model summary\n");
    // nl_model_summary(model, stdout);

    for (count = 0; count < 10; ++count) {
        sprintf(image_path, "pngs/%d.png", count);
        img_data = stbi_load(image_path, &w, &h, &n, 1);
        printf("Loaded %s: (w, h, n) = (%d, %d, %d)\n", image_path, w, h, n);

        // Get pixel data
        for (size_t r = 0; r < (size_t)(IMG_SIZE); ++r) {
            NL_MAT_AT(input_image, r, 0) = (float)img_data[r] / 255.f;
            if (r % 28 == 0) printf("\n");
            // printf("%3.0f ", (float)img_data[r]);
            if (img_data[r] == 0) printf(".");
            else printf("#");
        }
        printf("\n");

        stbi_image_free(img_data);

        printf("===== Predict\n");
        nl_model_predict(model, input_image, prediction);
        float maxProb = -1.f;
        int predicted_number = -1;
        for (size_t r = 0; r < prediction.rows; ++r) {
            printf("%f ", NL_MAT_AT(prediction, r, 0));
            if (NL_MAT_AT(prediction, r, 0) > maxProb) {
                maxProb = NL_MAT_AT(prediction, r, 0);
                predicted_number = (int)r;
            }
        }
        printf("\n");
        printf("Predicted: %d\n", predicted_number);

    }

    arena_destroy(arena_input);
    arena_destroy(arena_output);
    arena_destroy(arena_model);
    return;
}

int main(void)
{
    predict_all();
    return 0;

    // const char model_name[] = "digitRecog.model";
    // const char model_name[] = "0514_32.model";
    const char model_name[] = "0514_16.model";
    int w, h, n;
    unsigned char *img_read_data, *img_data;
    // const char image_path[] = "pngs/m9_1.png";
    const char image_path[] = "pngs/digit_image.png";
    // const char image_path[] = "pngs/3.png";

    img_read_data = stbi_load(image_path, &w, &h, &n, 0);
    printf("Loaded %s: (w, h, n) = (%d, %d, %d)\n", image_path, w, h, n);

    // n = 1;
    if (n != 1) {
        img_data = malloc(IMG_SIZE);
        printf("[INFO] Convert to grayscale image\n");
        to_grayscale(img_data, img_read_data, n);
    }
    else img_data = img_read_data;

    printf("===== Set some parameters\n");
    NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);
    nl_rand_init(false, 0);
    NL_PRINT_COST_EVERY_N_EPOCHS(1);

    printf("===== Allocate space\n");
    Arena arena_input = arena_new(512 * 1024 * 1024);
    Arena arena_output = arena_new(512 * 1024 * 1024);
    Arena arena_model = arena_new(512 * 1024 * 1024);
    Mat input_image = nl_mat_alloc_with_arena(&arena_input, IMG_SIZE, 1);
    Mat prediction = nl_mat_alloc_with_arena(&arena_output, 10, 1);

    // Get pixel data
    for (size_t r = 0; r < (size_t)(IMG_SIZE); ++r) {
        NL_MAT_AT(input_image, r, 0) = (float)img_data[r] / 255.f;
        if (r % 28 == 0) printf("\n");
        // printf("%3.0f ", (float)img_data[r]);
        if (img_data[r] == 0) printf(".");
        else printf("#");
    }
    printf("\n");

    stbi_image_free(img_read_data);
    if (n != 1) free(img_data);

    printf("===== Load model\n");
    NeuralNet model;
    nl_model_load_with_arena(&arena_model, model_name, &model);
    printf("[INFO] Model loaded\n");

    // printf("===== Model summary\n");
    // nl_model_summary(model, stdout);

    printf("===== Predict\n");
    nl_model_predict(model, input_image, prediction);
    float maxProb = -1.f;
    int predicted_number = -1;
    for (size_t r = 0; r < prediction.rows; ++r) {
        printf("%f ", NL_MAT_AT(prediction, r, 0));
        if (NL_MAT_AT(prediction, r, 0) > maxProb) {
            maxProb = NL_MAT_AT(prediction, r, 0);
            predicted_number = (int)r;
        }
    }
    printf("\n");
    printf("Predicted: %d\n", predicted_number);

    arena_destroy(arena_input);
    arena_destroy(arena_output);
    arena_destroy(arena_model);
    return 0;
}
