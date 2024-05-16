#pragma once
#include <iostream>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
}
std::string getErrorMsg(int err);
//��Ƶת��
bool transcode(const char* inputFilePath, const char* outputFilePath, AVCodecID targetCodec);

//������Ƶ
bool clipVideo(const std::string& inputFilePath, const std::string& outputFilePath, int64_t startTimeMs, int64_t endTimeMs);
/*
* �ϲ���ͬ��ʽ����Ƶ
*/
int concatVideos(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath);
