#include "ScreenStreamer.h"

using namespace std;

/* initialize the resources*/
ScreenStreamer::ScreenStreamer() {
	avdevice_register_all();
}

ScreenStreamer::~ScreenStreamer() {}

void ScreenStreamer::stopStreaming() {
	shouldStream = false;
}

bool ScreenStreamer::isStreaming() {
	return shouldStream;
}

int ScreenStreamer::startSteaming() {

	//#####################
	//## Grab the screen ##
	//#####################

	int ret;
	AVInputFormat* in_InputFormat = (AVInputFormat*)av_find_input_format("gdigrab");
	AVDictionary* in_InputFormatOptions = NULL;
	AVFormatContext* in_InputFormatContext = NULL;
	AVCodec* in_codec;
	AVCodecContext* in_codec_ctx;

	in_InputFormatContext = avformat_alloc_context();

	ret = av_dict_set(&in_InputFormatOptions, "framerate", "30", 0);
	ret = av_dict_set(&in_InputFormatOptions, "offset_x", "0", 0);
	ret = av_dict_set(&in_InputFormatOptions, "offset_y", "0", 0);
	ret = av_dict_set(&in_InputFormatOptions, "video_size", "1920x1080", 0);
	ret = av_dict_set(&in_InputFormatOptions, "probesize", "100M", 0);
	ret = av_dict_set(&in_InputFormatOptions, "draw_mouse", "0", 0);
	if (ret < 0) {
		cout << "error in setting dictionary value" << endl;
		return -1;
	}

	ret = avformat_open_input(&in_InputFormatContext, "desktop", in_InputFormat, &in_InputFormatOptions); // second parameter is the -i parameter of ffmpeg, use title=window_title to specify the window later

	if (ret != 0) {
		cout << "error: could not open gdigrab" << endl;
		return -1;
	}

	ret = avformat_find_stream_info(in_InputFormatContext, &in_InputFormatOptions);
	if (ret < 0) {
		cout << "error in avformat_find_stream_info" << endl;
		return -1;
	}

	in_codec = (AVCodec*)avcodec_find_decoder(in_InputFormatContext->streams[0]->codecpar->codec_id);
	if (in_codec == NULL) {
		fprintf(stderr, "Error occurred getting the in_codec\n");
		return -1;
	}

	in_codec_ctx = avcodec_alloc_context3(in_codec);
	if (in_codec_ctx == NULL) {
		fprintf(stderr, "Error occurred when getting in_codec_ctx\n");
		return -1;
	}

	ret = avcodec_parameters_to_context(in_codec_ctx, in_InputFormatContext->streams[0]->codecpar);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when avcodec_parameters_to_context\n");
		return -1;
	}

	ret = avcodec_open2(in_codec_ctx, in_codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when avcodec_open2\n");
		return -1;
	}
	
	av_dump_format(in_InputFormatContext, 0, "desktop", 0);

	//##########################################
	//## Configure output server / parameters ##
	//##########################################

	AVFormatContext* out_OutputFormatContext = NULL;
	const char* out_ServerURL = "rtp://127.0.0.1:6000";
	AVCodec* out_Codec;
	AVCodecContext* out_CodecContext;
	AVStream* out_Stream;
	AVRational serverFrameRate = { 30, 1 };
	AVDictionary* out_codecOptions = nullptr;
	const AVOutputFormat* out_OutputFormat = av_guess_format("rtp", NULL, NULL);;

	ret = avformat_alloc_output_context2(&out_OutputFormatContext, out_OutputFormat, "rtp", nullptr);
	if (ret < 0) {
		std::cout << "Could not allocate output format context!" << std::endl;
		return -1;
	}

	if (out_OutputFormatContext->oformat->flags & AVFMT_NOFILE) {
		ret = avio_open2(&out_OutputFormatContext->pb, out_ServerURL, AVIO_FLAG_WRITE, nullptr, nullptr);
		if (ret < 0) {
			std::cout << "Could not open output IO context!" << std::endl;
			return -1;
		}
	}

	// set codec parameters
	out_Codec = (AVCodec*)avcodec_find_encoder(AV_CODEC_ID_H264);
	out_Stream = avformat_new_stream(out_OutputFormatContext, out_Codec);
	out_CodecContext = avcodec_alloc_context3(out_Codec);

	out_CodecContext->codec_tag = 0;
	out_CodecContext->codec_id = AV_CODEC_ID_H264;
	out_CodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	out_CodecContext->width = 1920;
	out_CodecContext->height = 1080;
	out_CodecContext->gop_size = 30;
	out_CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	out_CodecContext->framerate = serverFrameRate;
	out_CodecContext->time_base = av_inv_q(serverFrameRate);

	if (out_OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		out_CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	ret = avcodec_parameters_from_context(out_Stream->codecpar, out_CodecContext);
	if (ret < 0) {
		std::cout << "Could not initialize stream codec parameters!" << std::endl;
		return -1;
	}
	
	av_dict_set(&out_codecOptions, "profile", "main", 0);
	av_dict_set(&out_codecOptions, "preset", "ultrafast", 0);
	av_dict_set(&out_codecOptions, "tune", "zerolatency", 0);
	av_dict_set(&out_codecOptions, "pix_fmt", "yuv420p", 0);

	ret = avcodec_open2(out_CodecContext, out_Codec, &out_codecOptions);
	if (ret < 0) {
		std::cout << "Could not open video encoder!" << std::endl;
		return -1;
	}

	out_Stream->codecpar->extradata = out_CodecContext->extradata;
	out_Stream->codecpar->extradata_size = out_CodecContext->extradata_size;

	av_dump_format(out_OutputFormatContext, 0, out_ServerURL, 1);

	if (!(out_OutputFormat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&out_OutputFormatContext->pb, out_ServerURL, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", out_ServerURL);
			return -1;
		}
	}

	ret = avformat_write_header(out_OutputFormatContext, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		return -1;
	}

	//##########################################################
	//## Send frames from input (screen) to the output server ##
	//##########################################################

	SwsContext* swsContext = sws_getContext(in_codec_ctx->width, in_codec_ctx->height, in_codec_ctx->pix_fmt, out_CodecContext->width, out_CodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
	
	AVPacket* pkt = av_packet_alloc();
	if (!pkt) {
		fprintf(stderr, "Could not allocate AVPacket\n");
		return -1;
	}
	
	shouldStream = true;

	while (shouldStream) {
		AVStream* in_stream, * out_stream;

		ret = av_read_frame(in_InputFormatContext, pkt);
		if (ret < 0)
			break;

		in_stream = in_InputFormatContext->streams[pkt->stream_index];

		out_stream = out_OutputFormatContext->streams[pkt->stream_index];

		AVFrame* frame = av_frame_alloc();
		AVFrame* out_frame = av_frame_alloc();

		out_frame->width = out_CodecContext->width;
		out_frame->height = out_CodecContext->height;
		out_frame->format = out_CodecContext->pix_fmt;

		av_frame_get_buffer(out_frame, 0);

		ret = avcodec_send_packet(in_codec_ctx, pkt);
		if (ret < 0) {
			std::cerr << "Error sending packet to codec" << std::endl;
			return -1;
		}

		ret = avcodec_receive_frame(in_codec_ctx, frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		else if (ret < 0) {
			std::cerr << "Error receiving frame from codec" << std::endl;
			return -1;
		}

		AVPacket* outPacket = av_packet_alloc();

		sws_scale(swsContext, frame->data, frame->linesize, 0, in_codec_ctx->height, out_frame->data, out_frame->linesize);

		ret = avcodec_send_frame(out_CodecContext, out_frame);
		if (ret < 0) {
			std::cerr << "Error encoding frame for output" << std::endl;
			return -1;
		}

		ret = avcodec_receive_packet(out_CodecContext, outPacket);
		if (ret < 0) {
			std::cerr << "Error encoding packet for output" << std::endl;
			return -1;
		}

		outPacket->dts = pkt->dts;
		outPacket->pts = pkt->pts;

		ret = av_interleaved_write_frame(out_OutputFormatContext, outPacket);
		/* pkt is now blank (av_interleaved_write_frame() takes ownership of
		 * its contents and resets pkt), so that no unreferencing is necessary.
		 * This would be different if one used av_write_frame(). */
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}

		av_packet_unref(pkt);
		av_packet_unref(outPacket);
		av_frame_free(&frame);
		av_frame_free(&out_frame);
		av_packet_free(&outPacket);
	}
	
	//##########################
	//## free the used memory ##
	//##########################

	sws_freeContext(swsContext);

	av_write_trailer(out_OutputFormatContext);

	av_packet_free(&pkt);

	avformat_close_input(&in_InputFormatContext);

	/* close output */
	if (out_OutputFormatContext && !(out_OutputFormat->flags & AVFMT_NOFILE))
		avio_closep(&out_OutputFormatContext->pb);
	avformat_free_context(out_OutputFormatContext);

	if (ret < 0 && ret != AVERROR_EOF) {
		fprintf(stderr, "Error occurred: %s\n", "idk");
		return -1;
	}

	/* close input*/
	avformat_close_input(&in_InputFormatContext);
	if (!in_InputFormatContext) {
		cout << "\nfile closed sucessfully";
	} else {
		cout << "\nunable to close the file";
		return -1;
	}

	avformat_free_context(in_InputFormatContext);
	if (!in_InputFormatContext) {
		cout << "\navformat free successfully";
	} else {
		cout << "\nunable to free avformat context";
		return -1;
	}

	return 0;
}