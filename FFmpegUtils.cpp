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
	// �������ļ�
	AVFormatContext* inputFormatCtx = nullptr;
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	std::string errMsg;
	int ret = avformat_open_input(&inputFormatCtx, inputFilePath, nullptr, nullptr);
	if (ret < 0) {
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "�޷��������ļ�: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// ��ȡ�����ļ�������Ϣ
	ret = avformat_find_stream_info(inputFormatCtx, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "�޷���ȡ�����ļ�������Ϣ: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// �ҵ���Ƶ��
	AVCodec* videoCodec = nullptr;
	int videoStreamIndex = av_find_best_stream(inputFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, const_cast<const AVCodec**>(&videoCodec), 0);
	if (videoStreamIndex < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "�޷��ҵ���Ƶ��: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// ��������ļ�
	AVFormatContext* outputFormatCtx = nullptr;
	ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilePath);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg = "�޷���������ļ�: ";
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// �ҵ�Ŀ�������
	AVCodec* targetCodecPtr = const_cast<AVCodec*>(avcodec_find_decoder(targetCodec));
	if (!targetCodecPtr) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷��ҵ�Ŀ�������: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// �����µ������
	AVStream* outputVideoStream = avformat_new_stream(outputFormatCtx, nullptr);
	if (!outputVideoStream) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷����������: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// ����������ı���������
	AVCodecContext* outputCodecCtx = avcodec_alloc_context3(targetCodecPtr);
	if (!outputCodecCtx) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷��������������������: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	outputCodecCtx->codec_id = targetCodecPtr->id;
	outputCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	outputCodecCtx->width = inputFormatCtx->streams[videoStreamIndex]->codecpar->width;
	outputCodecCtx->height = inputFormatCtx->streams[videoStreamIndex]->codecpar->height;
	outputCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// �����������
	ret = avcodec_open2(outputCodecCtx, targetCodecPtr, nullptr);
	if (ret < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		avcodec_free_context(&outputCodecCtx);
		errMsg = "�޷������������: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// ��������������������������
	ret = avcodec_parameters_from_context(outputVideoStream->codecpar, outputCodecCtx);
	if (ret < 0) {
		avcodec_free_context(&outputCodecCtx);
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷����������������������: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	// ������ļ�
	if (avio_open(&outputFormatCtx->pb, outputFilePath, AVIO_FLAG_WRITE) < 0) {
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷�������ļ�: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}
	// д������ļ����ļ�ͷ
	ret = avformat_write_header(outputFormatCtx, nullptr);
	if (ret < 0) {
		avcodec_free_context(&outputCodecCtx);
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		errMsg = "�޷�д������ļ����ļ�ͷ: ";
		av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
		errMsg += errbuf;
		throw std::runtime_error(errMsg);
	}

	// ��֡��ȡ�����ļ�����Ƶ֡��������ת��ΪĿ����벢д������ļ�
	AVPacket packet;
	while (av_read_frame(inputFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStreamIndex) {
			// ������Ƶ֡���б���
			ret = avcodec_send_packet(outputCodecCtx, &packet);
			if (ret < 0) {
				av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
				avcodec_free_context(&outputCodecCtx);
				avformat_close_input(&inputFormatCtx);
				avformat_free_context(outputFormatCtx);
				errMsg = "�޷�������Ƶ�����б���: ";
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
					errMsg = "�޷��ӱ��������ձ���������: ";
					av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
					errMsg += errbuf;
					throw std::runtime_error(errMsg);
				}

				// д���������Ƶ��������ļ�
				packet.stream_index = outputVideoStream->index;
				av_interleaved_write_frame(outputFormatCtx, &packet);
				av_packet_unref(&packet);
			}
		}
		av_packet_unref(&packet);
	}

	// д������ļ����ļ�β
	av_write_trailer(outputFormatCtx);

	// ������Դ
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
* �ϲ���ͬ��ʽ����Ƶ todo
*/
int concatVideos(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath) {
	AVFormatContext* inputFormatCtx = nullptr;
	AVFormatContext* outputFormatCtx = nullptr;
	AVPacket packet;
	int64_t totalDuration = 0; // ��ʱ�������ڼ����µ�ʱ���ƫ����
	int64_t timestampOffset = 0; // ʱ���ƫ����

	// ��������ļ�
	int ret = avformat_alloc_output_context2(&outputFormatCtx, nullptr, nullptr, outputFilePath.c_str());
	if (ret < 0) {
		return ret;
	}

	for (int index = 0; index < inputFilePaths.size(); index++) {

		// �������ļ�
		ret = avformat_open_input(&inputFormatCtx, inputFilePaths.at(index).c_str(), nullptr, nullptr);
		if (ret < 0) {
			goto exit_err;
		}

		// ��ȡ�����ļ�������Ϣ
		ret = avformat_find_stream_info(inputFormatCtx, nullptr);
		if (ret < 0) {
			goto exit_err;
		}
		/*if (index == 0)
		{
		}*/
		// ������ʱ��
		totalDuration += inputFormatCtx->duration;
		if (index == inputFilePaths.size() - 1) {
			// ������������ļ�
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
				// ʹ��ʱ���
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

	// ������ļ�
	ret = avio_open(&outputFormatCtx->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE);
	if (ret < 0) {
		goto exit_err;
	}

	// д������ļ����ļ�ͷ
	ret = avformat_write_header(outputFormatCtx, nullptr);
	if (ret < 0) {
		goto exit_err;
	}

	// д�����������ļ���֡������ļ���������ʱ���
	for (const auto& inputFilePath : inputFilePaths) {
		AVFormatContext* inputFormatCtx = nullptr;

		// �������ļ�
		ret = avformat_open_input(&inputFormatCtx, inputFilePath.c_str(), nullptr, nullptr);
		if (ret < 0) {
			goto exit_err;
		}

		// ��ȡ�����ļ�������Ϣ
		ret = avformat_find_stream_info(inputFormatCtx, nullptr);
		if (ret < 0) {
			goto exit_err;
		}
		// ��ÿ�������ļ���ȡ֡��д������ļ���������ʱ���
		while (av_read_frame(inputFormatCtx, &packet) >= 0) {
			// ����ʱ���
			packet.pts += timestampOffset;
			packet.dts += timestampOffset;

			ret = av_interleaved_write_frame(outputFormatCtx, &packet);

			if (ret < 0) {
				av_packet_unref(&packet); // ������Ч�İ�
				continue; // ����������һ����
			}
			av_packet_unref(&packet);
		}

		// ����ʱ���ƫ����
		int64_t durationPts = av_rescale_q(inputFormatCtx->duration * 1000, AV_TIME_BASE_Q, inputFormatCtx->streams[0]->time_base);
		timestampOffset += durationPts;
		avformat_close_input(&inputFormatCtx);
	}

	// д������ļ����ļ�β
	av_write_trailer(outputFormatCtx);

	avformat_free_context(outputFormatCtx);

	return 1;
exit_err:
	{
		avformat_close_input(&inputFormatCtx);
		avformat_free_context(outputFormatCtx);
		return ret; // �����޷���ȡ����Ϣ�������ļ�
	}

}