#ifndef YOLO_LIB_H
#define YOLO_LIB_H

#include <vector>
#include <string>
#include <algorithm>
#include "opencv2/opencv.hpp"
#include <onnxruntime_cxx_api.h>
#include "dml_provider_factory.h"
#include <numeric>

using namespace std;

struct ImageInfo {
    cv::Size raw_size;
};

struct ModelImageSize {
    int width;
    int height;
};

struct YoloResults {
    int class_id;
    float confidence;
    cv::Rect_<float> box;

    bool operator==(const YoloResults& other) const {
        return class_id == other.class_id
            && confidence == other.confidence
            && box == other.box;
    }
};

void GetInputDetails(const Ort::Session& session,
    vector<string>& inputNames,
    vector<int64_t>& inputShape,
    int& inputHeight,
    int& inputWidth);

void GetOutputDetails(const Ort::Session& session, vector<string>& outputNames);

vector<float> PrepareInput(const cv::Mat& image, int inputWidth, int inputHeight);

vector<Ort::Value> Inference(Ort::Session& session,
    const vector<string>& outputNames,
    const string& inputName,
    const vector<float>& inputTensor,
    const vector<int64_t>& inputShape);

void clip_boxes(cv::Rect& box, const cv::Size& shape);
void clip_boxes(cv::Rect_<float>& box, const cv::Size& shape);
void clip_boxes(vector<cv::Rect>& boxes, const cv::Size& shape);
void clip_boxes(vector<cv::Rect_<float>>& boxes, const cv::Size& shape);

float compute_iou(const cv::Rect_<float>& box, const cv::Rect_<float>& other_box);
vector<int> nms(const vector<cv::Rect_<float>>& boxes, const vector<float>& scores, float iou_threshold);
cv::Rect_<float> intersect_rects(const cv::Rect_<float>& rect1, const cv::Rect_<float>& rect2);

cv::Rect_<float> scale_boxes(const cv::Size& img1_shape, cv::Rect_<float>& box, const cv::Size& img0_shape, pair<float, cv::Point2f> ratio_pad, bool padding);

void postprocess_detects(cv::Mat& output0, ImageInfo image_info, vector<YoloResults>& output, int class_names_num, float conf_threshold, float iou_threshold, ModelImageSize modelImgSize);

#endif
