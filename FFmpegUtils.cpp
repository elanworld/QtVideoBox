#include <iostream>
#include <string>
#include <cstring>
#include "FFmpegUtils.h"


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

std::string getErrorMsg(int err) {
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(err, errbuf, AV_ERROR_MAX_STRING_SIZE);
	return std::string(errbuf, strlen(errbuf));
}

bool transcode(const char* inputFilePath, const char* outputFilePath, AVCodecID targetCodec) {
	// 打开输入文件
	AVFormatContext* inputFormatCtx = nullptr;
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	std::string errMsg;
	int ret = avformat_open_input(&inputFormatCtx, inputFilePath, nullptr, nullptr);
	if (ret < 0) {
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "无法打开输入文件: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 获取输入文件的流信息
	ret = avformat_find_stream_info(inputFormatCtx, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "无法获取输入文件的流信息: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 找到视频流
	AVCodec* videoCodec = nullptr;
	int videoStreamIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, const_cast<const AVCodec**>(&videoCodec), 0);
	if (videoStreamIndex < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "无法找到视频流: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 创建输出文件
	AVFormatContext* outputFormatCtx = nullptr;
	ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilePath);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "无法创建输出文件: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 找到目标编码器
	AVCodec* targetCodecPtr = const_cast<AVCodec*>(avcodec_find_decoder(targetCodec));
	if (!targetCodecPtr) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法找到目标编码器: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 创建新的输出流
	AVStream* outputVideoStream = avformat_new_stream(outputFormatCtx, nullptr);
	if (!outputVideoStream) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法创建输出流: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 设置输出流的编码器参数
	AVCodecContext* outputCodecCtx = avcodec_alloc_context3(targetCodecPtr);
	if (!outputCodecCtx) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法分配输出编码器上下文: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	outputCodecCtx->codec_id = targetCodecPtr->id;
	outputCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	outputCodecCtx->width = inputFormatCtx->streams[videoStreamIndex]->codecpar->width;
	outputCodecCtx->height = inputFormatCtx->streams[videoStreamIndex]->codecpar->height;
	outputCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// 打开输出编码器
	ret = avcodec_open2(outputCodecCtx, targetCodecPtr, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		avcodec_free_context(&outputCodecCtx);
		errMsg = "无法打开输出编码器: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 将输出编码器参数拷贝到输出流
	ret = avcodec_parameters_from_context(outputVideoStream->codecpar, outputCodecCtx);
	if (ret < 0) {
		avcodec_free_context(&outputCodecCtx);
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法拷贝编码器参数到输出流: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	// 打开输出文件
	if (avio_open(&outputFormatCtx->pb, outputFilePath, AVIO_FLAG_WRITE) < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法打开输出文件: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	// 写入输出文件的文件头
	ret = avformat_write_header(outputFormatCtx, nullptr);
	if (ret < 0) {
		avcodec_free_context(&outputCodecCtx);
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "无法写入输出文件的文件头: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// 逐帧读取输入文件的视频帧，并将其转码为目标编码并写入输出文件
	AVPacket packet;
	while (av_read_frame(inputFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStreamIndex) {
			// 发送视频帧进行编码
			ret = avcodec_send_packet(outputCodecCtx, &packet);
			if (ret < 0) {
				av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
				avcodec_free_context(&outputCodecCtx);
				avformat_close_input(&inputFormatCtx);
				avformat_free_context(outputFormatCtx);
				errMsg = "无法发送视频包进行编码: ";
				av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
				errMsg += errbuf;
				throw std::runtime_error(errMsg);
			}

			while (ret >= 0) {
				ret = avcodec_receive_packet(outputCodecCtx, &packet);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				if (ret < 0) {
					avcodec_free_context(&outputCodecCtx);
					avformat_close_input(&inputFormatCtx);
					avformat_free_context(outputFormatCtx);
					errMsg = "无法从编码器接收编码后的数据: ";
					av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
					errMsg += errbuf;
					throw std::runtime_error(errMsg);
				}

				// 写入编码后的视频包到输出文件
				packet.stream_index = outputVideoStream->index;
				av_interleaved_write_frame(outputFormatCtx, &packet);
				av_packet_unref(&packet);
			}
		}
		av_packet_unref(&packet);
	}

	// 写入输出文件的文件尾
	av_write_trailer(outputFormatCtx);

	// 清理资源
	avcodec_free_context(&outputCodecCtx);
	avformat_close_input(&inputFormatCtx);
	avformat_free_context(outputFormatCtx);
	return true;
}



bool clipVideo(const std::string& inputFilePath, const std::string& outputFilePath, int64_t startTimeMs, int64_t endTimeMs) {
	AVFormatContext* inputFormatCtx = nullptr;
	AVFormatContext* outputFormatCtx = nullptr;
	AVPacket packet;

	int ret = avformat_open_input(&inputFormatCtx, inputFilePath.c_str(), nullptr, nullptr);
	if (ret < 0) {
		return false;
	}

	ret = avformat_find_stream_info(inputFormatCtx, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		return false;
	}

	ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilePath.c_str());
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		return false;
	}

	for (unsigned int i = 0; i < inputFormatCtx->nb_streams; i++) {
		AVStream* inStream = inputFormatCtx->streams[i];
		AVCodec* codec = const_cast<AVCodec*>(avcodec_find_decoder(inStream->codecpar->codec_id));
		if (!codec) {
			continue;
		}
		AVStream* outStream = avformat_new_stream(outputFormatCtx, codec);
		if (!outStream) {
			avformat_close_input(&inputFormatCtx);
			avformat_free_context(outputFormatCtx);
			return false;
		}
		ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
		outStream->time_base = inStream->time_base;
		if (ret < 0) {
			avformat_close_input(&inputFormatCtx);
			avformat_free_context(outputFormatCtx);
			return false;
		}
	}

	ret = avio_open(&outputFormatCtx->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		return false;
	}

	ret = avformat_write_header(outputFormatCtx, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		return false;
	}

	av_seek_frame(inputFormatCtx, -1, startTimeMs * AV_TIME_BASE / 1000, AVSEEK_FLAG_ANY);

	while (av_read_frame(inputFormatCtx, &packet) >= 0) {
		if (endTimeMs && endTimeMs > 0)
		{
			if (av_q2d(inputFormatCtx->streams[packet.stream_index]->time_base) * packet.pts > endTimeMs / 1000.0) {
				av_packet_unref(&packet);
				break;
			}
		}
		ret = av_interleaved_write_frame(outputFormatCtx, &packet);
		if (ret < 0) {
			av_packet_unref(&packet);
			continue;
		}
		av_packet_unref(&packet);
	}

	av_write_trailer(outputFormatCtx);
	avformat_close_input(&inputFormatCtx);
	avformat_free_context(outputFormatCtx);

	return true;
}


/*
* 合并相同格式的视频 todo
*/
int concatVideos(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath) {
	AVFormatContext* inputFormatCtx = nullptr;
	AVFormatContext* outputFormatCtx = nullptr;
	AVPacket packet;
	int64_t totalDuration = 0; // 总时长，用于计算新的时间戳偏移量
	int64_t timestampOffset = 0; // 时间戳偏移量

	// 创建输出文件
	int ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilePath.c_str());
	if (ret < 0) {
		return ret;
	}

	for (int index = 0; index < inputFilePaths.size(); index++) {

		// 打开输入文件
		ret = avformat_open_input(&inputFormatCtx, inputFilePaths.at(index).c_str(), nullptr, nullptr);
		if (ret < 0) {
			goto exit_err;
		}

		// 获取输入文件的流信息
		ret = avformat_find_stream_info(inputFormatCtx, nullptr);
		if (ret < 0) {
			goto exit_err;
		}
		/*if (index == 0)
		{
		}*/
		// 计算总时长
		totalDuration += inputFormatCtx->duration;
		if (index == inputFilePaths.size() - 1) {
			// 复制流到输出文件
			for (unsigned int i = 0; i < inputFormatCtx->nb_streams; i++) {
				AVStream* inStream = inputFormatCtx->streams[i];
				AVCodec* codec = const_cast<AVCodec*>(avcodec_find_decoder(inStream->codecpar->codec_id));
				if (!codec) {
					continue;
				}
				AVStream* outStream = avformat_new_stream(outputFormatCtx, codec);
				if (!outStream) {
					ret = -100;
					goto exit_err;
				}
				ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
				if (ret < 0) {
					goto exit_err;
				}
				// 使用时间基
				outStream->time_base = inStream->time_base;
				outStream->start_time = 0;
				outStream->duration = totalDuration;
			}
			outputFormatCtx->duration = 0;
			outputFormatCtx->duration = totalDuration;
			outputFormatCtx->bit_rate = inputFormatCtx->bit_rate;
		}

		avformat_close_input(&inputFormatCtx);
	}

	// 打开输出文件
	ret = avio_open(&outputFormatCtx->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		goto exit_err;
	}

	// 写入输出文件的文件头
	ret = avformat_write_header(outputFormatCtx, nullptr);
	if (ret < 0) {
		goto exit_err;
	}

	// 写入所有输入文件的帧到输出文件，并调整时间戳
	for (const auto& inputFilePath : inputFilePaths) {
		AVFormatContext* inputFormatCtx = nullptr;

		// 打开输入文件
		ret = avformat_open_input(&inputFormatCtx, inputFilePath.c_str(), nullptr, nullptr);
		if (ret < 0) {
			goto exit_err;
		}

		// 获取输入文件的流信息
		ret = avformat_find_stream_info(inputFormatCtx, nullptr);
		if (ret < 0) {
			goto exit_err;
		}
		// 从每个输入文件读取帧并写入输出文件，并调整时间戳
		while (av_read_frame(inputFormatCtx, &packet) >= 0) {
			// 调整时间戳
			packet.pts += timestampOffset;
			packet.dts += timestampOffset;

			ret = av_interleaved_write_frame(outputFormatCtx, &packet);

			if (ret < 0) {
				av_packet_unref(&packet); // 跳过无效的包
				continue; // 继续处理下一个包
			}
			av_packet_unref(&packet);
		}

		// 更新时间戳偏移量
		int64_t durationPts = av_rescale_q(inputFormatCtx->duration * 1000, AV_TIME_BASE_Q, inputFormatCtx->streams[0]->time_base);
		timestampOffset += durationPts;
		avformat_close_input(&inputFormatCtx);
	}

	// 写入输出文件的文件尾
	av_write_trailer(outputFormatCtx);

	avformat_free_context(outputFormatCtx);

	return 1;
exit_err:
	{
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		return ret; // 忽略无法获取流信息的输入文件
	}

}