#pragma once
#include <iostream>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
}
std::string getErrorMsg(int err);
//视频转码
bool transcode(const char* inputFilePath, const char* outputFilePath, AVCodecID targetCodec);

//剪切视频
bool clipVideo(const std::string& inputFilePath, const std::string& outputFilePath, int64_t startTimeMs, int64_t endTimeMs);
/*
* 合并相同格式的视频
*/
int concatVideos(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath);
