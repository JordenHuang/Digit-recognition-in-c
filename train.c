#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>

#define NERUALIB_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "mnist_squash.h"

#define MNIST_MAT_TRAIN_FILENAME "tools/mnist_train_squashed"
#define MNIST_MAT_TEST_FILENAME  "tools/mnist_test_squashed"

int main(void) {
    printf("[INFO] Set some parameters\n");
    NL_SET_ARENA_CAPACITY(512 * 1024 * 1024);
    nl_rand_init(false, 0);
    NL_PRINT_COST_EVERY_N_EPOCHS(5);

    printf("[INFO] Allocate space\n");
    Arena arena_x = arena_new(512 * 1024 * 1024);
    Arena arena_y = arena_new(4 * 1024 * 1024);
    Arena arena_train = arena_new(4 * 1024 * 1024);
    Mat x_train_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TRAIN_IMG_COUNT);
    Mat y_train_labels = nl_mat_alloc_with_arena(&arena_y, 10, TRAIN_IMG_COUNT);
    int r;

// {
#if 1
    printf("[INFO] Prepare training data\n");
    r = load_mnist_mat(MNIST_MAT_TRAIN_FILENAME, x_train_images, y_train_labels, true);
    if (r < 0)
        exit(1);

    printf("[INFO] Normalize training data\n");
    nl_mat_mult_c(x_train_images, x_train_images, 1/255.f);

    // #define RELU_AND_SOFTMAX_AND_CROSS_ENTROPY
    #define SIGMOID_AND_MSE

    printf("[INFO] Define model layer components\n");
    NeuralNet model;
#if defined(RELU_AND_SOFTMAX_AND_CROSS_ENTROPY)
    size_t layers[] = {IMG_SIZE, 16, 16, 10};
    // Activation_type acts[] = {RELU, RELU, SOFTMAX};
    // size_t layers[] = {IMG_SIZE, 128, 64, 10};
    Activation_type acts[] = {RELU, RELU, SOFTMAX};
#elif defined(SIGMOID_AND_MSE)
    // size_t layers[] = {IMG_SIZE, 16, 16, 10};
    size_t layers[] = {IMG_SIZE, 64, 32, 10};
    Activation_type acts[] = {SIGMOID, SIGMOID, SIGMOID};
#endif

    printf("[INFO] Define model layers\n");
#if defined(RELU_AND_SOFTMAX_AND_CROSS_ENTROPY)
    nl_define_layers_with_arena(&arena_train, &model, NL_ARRAY_LEN(layers), layers, acts, CROSS_ENTROPY);
#elif defined(SIGMOID_AND_MSE)
    nl_define_layers_with_arena(&arena_train, &model, NL_ARRAY_LEN(layers), layers, acts, MSE);
#endif

    nl_model_summary(model, stdout);

    // /* View the image */
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

    printf("[INFO] Train\n");
#if defined(RELU_AND_SOFTMAX_AND_CROSS_ENTROPY)
    float lr = 1e-2; // 7e-2
#elif defined(SIGMOID_AND_MSE)
    float lr = 5e-2; // 7e-1;
#endif
    size_t epochs = 80;
    size_t batch_size = 10;
    nl_model_train(model, x_train_images, y_train_labels, lr, epochs, batch_size, true);

    // Save model
    const char model_path[] = "digitRecog.model";
    nl_model_save(model_path, model);
    printf("Model [%s] saved\n", model_path);

#else  // Just load the model
    NeuralNet model;
    const char model_path[] = "digitRecog.model";
    nl_model_load_with_arena(&arena_train, model_path, &model);

#endif
// }

    /* Verify the model */
    printf("[INFO] Verify\n");
    {
        arena_reset(&arena_x);
        arena_reset(&arena_y);

        Mat x_test_images = nl_mat_alloc_with_arena(&arena_x, IMG_SIZE, TEST_IMG_COUNT);
        Mat y_test_labels = nl_mat_alloc_with_arena(&arena_y, 10, TEST_IMG_COUNT);
        Mat predictions = nl_mat_alloc_with_arena(&arena_train, 10, TEST_IMG_COUNT);

        Mat input_image = nl_mat_alloc_with_arena(&arena_train, IMG_SIZE, 1);
        Mat prediction = nl_mat_alloc_with_arena(&arena_train, 10, 1);

        printf("[INFO] Prepare testing data\n");
        load_mnist_mat(MNIST_MAT_TEST_FILENAME, x_test_images, y_test_labels, true);

        printf("[INFO] Normalize testing data\n");
        nl_mat_mult_c(x_test_images, x_test_images, 1/255.f);

        printf("[INFO] Predict\n");
        for (size_t c = 0; c < TEST_IMG_COUNT; ++c) {
            nl_mat_get_col(input_image, x_test_images, c);
            nl_model_predict(model, input_image, prediction);

            nl_mat_set_col(predictions, prediction, c);
        }

        printf("[INFO] Calculate score\n");
        float score = nl_model_accuracy_score(y_test_labels, predictions);

        printf("[INFO] Score: %.3f%%\n", score * 100);
    }

    arena_destroy(arena_x);
    arena_destroy(arena_y);
    arena_destroy(arena_train);

    return 0;
}
