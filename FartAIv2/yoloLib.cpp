#include "yoloLib.h"


void GetInputDetails(const Ort::Session& session,
    vector<string>& inputNames,
    vector<int64_t>& inputShape,
    int& inputHeight,
    int& inputWidth) {
    Ort::AllocatorWithDefaultOptions allocator;

    // Get the number of model inputs
    size_t numInputs = session.GetInputCount();
    inputNames.clear();

    for (size_t i = 0; i < numInputs; ++i) {
        // Getting name input
        auto inputName = session.GetInputNameAllocated(i, allocator);
        inputNames.push_back(inputName.get());

        // Getting form for first input
        if (i == 0) {
            auto tensorInfo = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
            inputShape = tensorInfo.GetShape();

            // Assume that the form of the input [N,C,H,W]
            if (inputShape.size() >= 4) {
                inputHeight = static_cast<int>(inputShape[2]);
                inputWidth = static_cast<int>(inputShape[3]);
            }
        }
    }
}

void GetOutputDetails(const Ort::Session& session, vector<string>& outputNames) {
    Ort::AllocatorWithDefaultOptions allocator;

    // Get the number of model outputs
    size_t numOutputs = session.GetOutputCount();
    outputNames.clear();

    for (size_t i = 0; i < numOutputs; ++i) {
        // Get the output name
        auto outputName = session.GetOutputNameAllocated(i, allocator);
        outputNames.push_back(outputName.get());
    }
}

vector<float> PrepareInput(const cv::Mat& image, int inputWidth, int inputHeight) {
    // Converting color from BGR to RGB
    cv::Mat inputRgb;
    cv::cvtColor(image, inputRgb, cv::COLOR_BGR2RGB);

    // Image resizing
    cv::Mat resizedImage;
    cv::resize(inputRgb, resizedImage, cv::Size(inputWidth, inputHeight));

    // Scaling pixel values
    resizedImage.convertTo(resizedImage, CV_32F, 1.0 / 255);

    // Rearrangement of image axes
    vector<cv::Mat> inputChannels(3);
    cv::split(resizedImage, inputChannels);

    // Creating an image vector for image pixels
    vector<float> inputTensor;
    for (auto& channel : inputChannels) {
        vector<float> data;
        channel.reshape(1, 1).copyTo(data);
        inputTensor.insert(inputTensor.end(), data.begin(), data.end());
    }

    return inputTensor;
}

vector<Ort::Value> Inference(Ort::Session& session,
    const vector<string>& outputNames,
    const string& inputName,
    const vector<float>& inputTensor,
    const vector<int64_t>& inputShape) {
    // Creating the ONNX input tensor
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensorOrt = Ort::Value::CreateTensor<float>(memoryInfo,
        const_cast<float*>(inputTensor.data()),
        inputTensor.size(),
        inputShape.data(),
        inputShape.size());

    // Set the names of input and output nodes
    vector<const char*> inputNames = { inputName.c_str() };
    vector<const char*> outputNamesCStr;
    for (const auto& name : outputNames) {
        outputNamesCStr.push_back(name.c_str());
    }

    // Output execution
    auto outputTensors = session.Run(Ort::RunOptions{ nullptr },
        inputNames.data(),
        &inputTensorOrt,
        1,
        outputNamesCStr.data(),
        outputNames.size());

    return outputTensors;
}

void clip_boxes(cv::Rect& box, const cv::Size& shape) {
    box.x = max(0, min(box.x, shape.width));
    box.y = max(0, min(box.y, shape.height));
    box.width = max(0, min(box.width, shape.width - box.x));
    box.height = max(0, min(box.height, shape.height - box.y));
}

void clip_boxes(cv::Rect_<float>& box, const cv::Size& shape) {
    box.x = max(0.0f, min(box.x, static_cast<float>(shape.width)));
    box.y = max(0.0f, min(box.y, static_cast<float>(shape.height)));
    box.width = max(0.0f, min(box.width, static_cast<float>(shape.width - box.x)));
    box.height = max(0.0f, min(box.height, static_cast<float>(shape.height - box.y)));
}

void clip_boxes(vector<cv::Rect>& boxes, const cv::Size& shape) {
    for (cv::Rect& box : boxes) {
        clip_boxes(box, shape);
    }
}

void clip_boxes(vector<cv::Rect_<float>>& boxes, const cv::Size& shape) {
    for (cv::Rect_<float>& box : boxes) {
        clip_boxes(box, shape);
    }
}

cv::Rect_<float> scale_boxes(const cv::Size& img1_shape, cv::Rect_<float>& box, const cv::Size& img0_shape,
    pair<float, cv::Point2f> ratio_pad = make_pair(-1.0f, cv::Point2f(-1.0f, -1.0f)), bool padding = true) {

    float gain, pad_x, pad_y;

    if (ratio_pad.first < 0.0f) {
        gain = min(static_cast<float>(img1_shape.height) / static_cast<float>(img0_shape.height),
            static_cast<float>(img1_shape.width) / static_cast<float>(img0_shape.width));
        pad_x = roundf((img1_shape.width - img0_shape.width * gain) / 2.0f - 0.1f);
        pad_y = roundf((img1_shape.height - img0_shape.height * gain) / 2.0f - 0.1f);
    }
    else {
        gain = ratio_pad.first;
        pad_x = ratio_pad.second.x;
        pad_y = ratio_pad.second.y;
    }

    //cv::Rect scaledCoords(box);
    cv::Rect_<float> scaledCoords(box);

    if (padding) {
        scaledCoords.x -= pad_x;
        scaledCoords.y -= pad_y;
    }

    scaledCoords.x /= gain;
    scaledCoords.y /= gain;
    scaledCoords.width /= gain;
    scaledCoords.height /= gain;

    // Clip the box to the bounds of the image
    clip_boxes(scaledCoords, img0_shape);

    return scaledCoords;
}


float compute_iou(const cv::Rect_<float>& box, const cv::Rect_<float>& other_box) {
    float xmin = max(box.x, other_box.x);
    float ymin = max(box.y, other_box.y);
    float xmax = min(box.x + box.width, other_box.x + other_box.width);
    float ymax = min(box.y + box.height, other_box.y + other_box.height);

    float intersection_area = max(0.0f, xmax - xmin) * max(0.0f, ymax - ymin);

    float box_area = box.width * box.height;
    float other_box_area = other_box.width * other_box.height;
    float union_area = box_area + other_box_area - intersection_area;

    return intersection_area / union_area;
}

vector<int> nms(const vector<cv::Rect_<float>>& boxes, const vector<float>& scores, float iou_threshold) {
    vector<int> indices(boxes.size());
    iota(indices.begin(), indices.end(), 0);

    sort(indices.begin(), indices.end(), [&scores](int i, int j) {
        return scores[i] > scores[j];
        });

    vector<int> keep_boxes;
    while (!indices.empty()) {
        int box_id = indices.front();
        keep_boxes.push_back(box_id);

        vector<int> new_indices;
        for (size_t i = 1; i < indices.size(); ++i) {
            int other_box_id = indices[i];
            if (compute_iou(boxes[box_id], boxes[other_box_id]) < iou_threshold) {
                new_indices.push_back(other_box_id);
            }
        }
        indices.swap(new_indices);
    }

    return keep_boxes;
}

cv::Rect_<float> intersect_rects(const cv::Rect_<float>& rect1, const cv::Rect_<float>& rect2) {
    float x = max(rect1.x, rect2.x);
    float y = max(rect1.y, rect2.y);
    float right = min(rect1.x + rect1.width, rect2.x + rect2.width);
    float bottom = min(rect1.y + rect1.height, rect2.y + rect2.height);

    if (right < x || bottom < y) {
        return cv::Rect_<float>(0, 0, 0, 0); // Return an empty rectangle if there is no intersection
    }

    return cv::Rect_<float>(x, y, right - x, bottom - y);
}


void postprocess_detects(cv::Mat& output0, ImageInfo image_info, vector<YoloResults>& output,
    int class_names_num, float conf_threshold, float iou_threshold, ModelImageSize modelImgSize) {
    output.clear();
    vector<int> class_ids;
    vector<float> confidences;
    vector<cv::Rect_<float>> boxes; // Use cv::Rect_ to store boxes
    vector<vector<float>> masks;
    int data_width = class_names_num + 4;
    int rows = output0.rows;
    float* pdata = (float*)output0.data;

    for (int r = 0; r < rows; ++r) {
        cv::Mat scores(1, class_names_num, CV_32FC1, pdata + 4);
        cv::Point class_id;
        double max_conf;
        minMaxLoc(scores, nullptr, &max_conf, nullptr, &class_id);

        if (max_conf > conf_threshold) {
            masks.emplace_back(pdata + 4 + class_names_num, pdata + data_width);
            class_ids.push_back(class_id.x);
            confidences.push_back((float)max_conf);

            float out_w = pdata[2];
            float out_h = pdata[3];
            float out_left = MAX((pdata[0] - 0.5 * out_w + 0.5), 0);
            float out_top = MAX((pdata[1] - 0.5 * out_h + 0.5), 0);

            cv::Rect_<float> bbox = cv::Rect_<float>(out_left, out_top, (out_w + 0.5), (out_h + 0.5));
            boxes.push_back(bbox);
        }
        pdata += data_width; // next pred
    }

    vector<int> keep_indices = nms(boxes, confidences, iou_threshold);

    for (int idx : keep_indices) {
        cv::Rect_<float> final_box = scale_boxes(cv::Size(modelImgSize.width, modelImgSize.height), boxes[idx], image_info.raw_size);
        final_box = intersect_rects(final_box, cv::Rect_<float>(0, 0, image_info.raw_size.width, image_info.raw_size.height));
        YoloResults result = { class_ids[idx], confidences[idx], final_box };
        output.push_back(result);
    }
}
