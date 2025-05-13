#include "processing.h"
#include "DesktopDuplicationCapture.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
using namespace std;
using namespace std::chrono;
// Helper function to format time
wstring formatMilliseconds(double ms) {
    wstringstream ss;
    ss.precision(2);             // Set precision to 2 decimal places
    ss << fixed << ms << L"ms";  // Output the number with "ms"
    return ss.str();
}
void processing() {
    // ONNX Runtime setup
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetExecutionMode(ORT_SEQUENTIAL);
    sessionOptions.DisableMemPattern();
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    Ort::AllocatorWithDefaultOptions allocator;
    // Get DirectML API
    const OrtApi& ortApi = Ort::GetApi();
    const OrtDmlApi* ortDmlApi = nullptr;
    ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, reinterpret_cast<const void**>(&ortDmlApi));
    if (ortDmlApi) {
        ortDmlApi->SessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
        {
            lock_guard<mutex> lock(dynamicTextMutex);
            dynamicText = L"Status: DML API loaded";
        }
        cout << "DirectML loaded" << endl;
    }
    else {
        {
            lock_guard<mutex> lock(dynamicTextMutex);
            dynamicText = L"Status: DML API failed";
        }
        cout << "Failed to get DirectML API" << endl;
    }
    // Get model path
    filesystem::path modelPath = filesystem::current_path() / "model.onnx";
    // Load model
    Ort::Session session(env, modelPath.wstring().c_str(), sessionOptions);
    {
        lock_guard<mutex> lock(dynamicTextMutex);
        dynamicText = L"Status: Model loaded";
    }
    cout << "Model loaded" << endl;
    // Get model details
    vector<string> inputNames;
    vector<int64_t> inputShape;
    int inputHeight = 0, inputWidth = 0;
    GetInputDetails(session, inputNames, inputShape, inputHeight, inputWidth);
    ModelImageSize modelImgSize;
    modelImgSize.width = inputWidth;
    modelImgSize.height = inputHeight;
    vector<string> outputNames;
    GetOutputDetails(session, outputNames);
    // Variables for accumulating delays
    double sumCaptureDuration = 0.0;
    double sumInferenceDuration = 0.0;
    int iterationCounter = 0;
    // Main loop
    while (true) {
        // Start timing frame capture
        auto captureStart = high_resolution_clock::now();
        int aimLocal;
        {
            lock_guard<mutex> lock(aimMutex);
            aimLocal = config.aim;
        }
        // Check if aim is turned on
        if (!aimLocal) {
            Sleep(10);
            continue;
        }
        cv::Mat image;
        {
            lock_guard<mutex> lock(global_image_mutex); // Lock mutex
            image = global_image; // Copy results to globalResults
        }
        if (image.empty()) {
            continue; // Skip iteration if the image is empty
        }
        //cv::imshow("Game Image", image);
        //cv::waitKey(1);
        // Calculate capture duration
        auto captureEnd = high_resolution_clock::now();
        double captureDuration = duration_cast<duration<double, milliseconds::period>>(captureEnd - captureStart).count();
        // Prepare input data for inference
        auto inferenceStart = high_resolution_clock::now();
        ImageInfo img_info;
        img_info.raw_size = image.size();
        vector<float> inputTensor = PrepareInput(image, inputWidth, inputHeight);
        // Start inference
        auto outputTensors = Inference(session, outputNames, inputNames[0], inputTensor, inputShape);
        // Postprocessing parameters
        int class_names_num = 2;        // Number of classes
        float conf_threshold = 0.50f;   // Confidence threshold
        float iou_threshold = 0.50f;    // NMS threshold
        vector<YoloResults> results;
        cv::Mat output0;
        if (!outputTensors.empty()) {
            auto shape0 = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
            float* all_data0 = outputTensors[0].GetTensorMutableData<float>();
            output0 = cv::Mat(
                cv::Size((int)shape0[2], (int)shape0[1]),
                CV_32F,
                all_data0
            ).t();
        }
        // Start postprocessing
        postprocess_detects(output0, img_info, results, class_names_num, conf_threshold, iou_threshold, modelImgSize);
        // Calculate inference duration (including postprocessing)
        auto inferenceEnd = high_resolution_clock::now();
        double inferenceDuration = duration_cast<duration<double, milliseconds::period>>(inferenceEnd - inferenceStart).count();
        // Accumulate delays
        sumCaptureDuration += captureDuration;
        sumInferenceDuration += inferenceDuration;
        iterationCounter++;
        // Update dynamicText every 20 iterations
        if (iterationCounter >= 20) {
            double avgCaptureDuration = sumCaptureDuration / 20.0;
            double avgInferenceDuration = sumInferenceDuration / 20.0;
            double avgTotalDuration = avgCaptureDuration + avgInferenceDuration;
            {
                lock_guard<mutex> lock(dynamicTextMutex);
                dynamicText = L"Capture: " + formatMilliseconds(avgCaptureDuration) + L", Inference: " + formatMilliseconds(avgInferenceDuration) + L", Total: " + formatMilliseconds(avgTotalDuration);
            }
            // Reset accumulators
            sumCaptureDuration = 0.0;
            sumInferenceDuration = 0.0;
            iterationCounter = 0;
        }
        // Synchronize access to globalResults
        {
            lock_guard<mutex> lock(globalResultsMutex); // Lock mutex
            globalResults = results; // Copy results to globalResults
        }
    }
}