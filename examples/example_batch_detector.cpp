#include <chrono>
#include <memory>

#include "trace.hpp"
#include <gflags/gflags.h>
#include <iostream>

#include "pose_detector.h"
#include "utils.hpp"

// Model flags
DEFINE_string(model_file, "../../data/models/hao28-600000-256x384.uff",
              "Path to uff model.");
DEFINE_int32(input_height, 256, "Height of input image.");
DEFINE_int32(input_width, 384, "Width of input image.");

// profiling flags
DEFINE_int32(repeat, 1, "Number of repeats.");
DEFINE_int32(batch_size, 8, "Batch size.");
DEFINE_int32(gauss_kernel_size, 17, "Gauss kernel size for smooth operation.");
DEFINE_bool(use_f16, false, "Use float16.");
DEFINE_bool(flip_rgb, true, "Flip RGB.");

// input flags
DEFINE_string(image_files,
              "../../data/media/COCO_val2014_000000000192.jpg,"
              "../../data/media/COCO_val2014_000000000459.jpg,"
              "../../data/media/COCO_val2014_000000000415.jpg,"
              "../../data/media/COCO_val2014_000000000564.jpg,"
              "../../data/media/COCO_val2014_000000000294.jpg,"
              "../../data/media/COCO_val2014_000000000623.jpg,"
              "../../data/media/COCO_val2014_000000000357.jpg,"
              "../../data/media/COCO_val2014_000000000488.jpg,"
              "../../data/media/COCO_val2014_000000000589.jpg,"
              "../../data/media/COCO_val2014_000000000474.jpg,"
              "../../data/media/COCO_val2014_000000000338.jpg,"
              "../../data/media/COCO_val2014_000000000569.jpg,"
              "../../data/media/COCO_val2014_000000000544.jpg,"
              "../../data/media/COCO_val2014_000000000428.jpg,"
              "../../data/media/COCO_val2014_000000000536.jpg,"
              "../../data/media/COCO_val2014_000000000395.jpg",
              "Comma separated list of paths to image.");

int main(int argc, char *argv[])
{
    TRACE_SCOPE(__func__);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // TODO: derive from model
    const int f_height = FLAGS_input_height / 8;
    const int f_width = FLAGS_input_width / 8;
    auto files = repeat(split(FLAGS_image_files, ','), FLAGS_repeat);

    std::unique_ptr<pose_detector> pd(create_pose_detector(
        FLAGS_model_file, FLAGS_input_height, FLAGS_input_width, f_height,
        f_width, FLAGS_batch_size, FLAGS_use_f16, FLAGS_gauss_kernel_size,
        FLAGS_flip_rgb));

    std::cout << "Arguments parsed, running models..." << std::endl;

    {
        using clock_t = std::chrono::system_clock;
        using duration_t = std::chrono::duration<double>;
        const auto t0 = clock_t::now();

        pd->inference(files);

        const int n = files.size();
        const duration_t d = clock_t::now() - t0;
        double mean = d.count() / n;
        printf("// inferenced %d images of %d x %d, took %.2fs, mean: %.2fms, "
               "FPS: %f, batch size: %d, use f16: %d, gauss kernel size: %d\n",
               n, FLAGS_input_height, FLAGS_input_width, d.count(), mean * 1000,
               1 / mean, FLAGS_batch_size, FLAGS_use_f16,
               FLAGS_gauss_kernel_size);
    }

    return 0;
}
