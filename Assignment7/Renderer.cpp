//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <thread>

inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

void renderScenePart(const Scene& scene, float imageAspectRatio, float scale, 
                     Vector3f eye_pos, int spp, std::vector<Vector3f>& framebuffer, 
                     uint32_t startRow, uint32_t endRow, std::vector<int>& framebuffer_record) {
    for (uint32_t j = startRow; j < endRow; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            // generate primary ray direction
            float x = (2 * (i + 0.5) / (float)scene.width - 1) * imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

            Vector3f dir = normalize(Vector3f(-x, y, 1));
            for (int k = 0; k < spp; k++) {
                framebuffer[j * scene.width + i] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
            }
            framebuffer_record[j * scene.width + i] = 1;
        }
        UpdateProgress((float) std::accumulate(framebuffer_record.begin(), framebuffer_record.end(), 0) 
                       / (float) (scene.height * scene.width));
    }
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);
    std::vector<int> framebuffer_record(scene.width * scene.height, 0);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    int spp = 16;
    std::cout << "SPP: " << spp << "\n";

    const uint32_t num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_threads);
    uint32_t rows_per_thread = scene.height / num_threads;
    uint32_t remaining_rows = scene.height % num_threads;

    uint32_t start_row = 0;
    for (uint32_t t = 0; t < num_threads; ++t)
    {   
        uint32_t end_row = start_row + rows_per_thread + (t < remaining_rows ? 1 : 0);
        threads[t] = std::thread(renderScenePart, std::ref(scene), imageAspectRatio, scale, 
                                 eye_pos, spp, std::ref(framebuffer), start_row, end_row,
                                 std::ref(framebuffer_record));
        start_row = end_row;
    }

    for (auto& thread : threads) {
        thread.join();
    }
    
    // for (uint32_t j = 0; j < scene.height; ++j) {
    //     for (uint32_t i = 0; i < scene.width; ++i) {
    //         // generate primary ray direction
    //         float x = (2 * (i + 0.5) / (float)scene.width - 1) *
    //                   imageAspectRatio * scale;
    //         float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

    //         Vector3f dir = normalize(Vector3f(-x, y, 1));
    //         for (int k = 0; k < spp; k++){
    //             framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;  
    //         }
    //         m++;
    //     }
    //     UpdateProgress(j / (float)scene.height);
    // }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}