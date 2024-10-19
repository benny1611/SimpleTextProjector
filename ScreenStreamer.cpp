#include "ScreenStreamer.h"

using namespace std;
using Poco::JSON::Object;
using Poco::JSON::Stringifier;
using Poco::Dynamic::Var;

/* initialize the resources*/
ScreenStreamer::ScreenStreamer(Task* tsk, Event* stop_event, Mutex* mtx) {
	avdevice_register_all();
	task = tsk;
	stopEvent = stop_event;
	mutex = mtx;
}

ScreenStreamer::~ScreenStreamer() {}

void ScreenStreamer::stopStreaming() {
	shouldStream = false;
}

bool ScreenStreamer::isStreaming() {
	bool result;
	mutex->lock();
	result = shouldStream;
	mutex->unlock();
	return result;
}

int custom_write(void* opaque, const uint8_t* buf, int buf_size) {

	ScreenStreamer* instance = static_cast<ScreenStreamer*>(opaque);

	return instance->handle_write((uint8_t*)buf, buf_size);
}

int ScreenStreamer::handle_write(uint8_t* buf, int buf_size) {

	auto rtp = reinterpret_cast<rtc::RtpHeader*>(buf);
	rtp->setSsrc(ssrc);

	set<std::shared_ptr<Receiver>>::iterator itr;

	for (itr = receivers.begin(); itr != receivers.end(); itr++) {
		std::shared_ptr<Receiver> tmp = *itr;
		Receiver rr = *tmp;
		if (rr.isConnected) {
			std::shared_ptr<rtc::Track> tmpTrack = rr.track;
			tmpTrack->send(reinterpret_cast<const std::byte*>(buf), buf_size);
		}
	}

	return buf_size;  // Returning buf_size tells FFmpeg we handled the packet
}

int ScreenStreamer::setAnswer(WebSocket& client, Object::Ptr answerJSON) {

	try {
		std::string sdp = answerJSON->getValue<std::string>("sdp");
		std::string type = answerJSON->getValue<std::string>("type");

		std::cout << "Type: " << type << std::endl << "sdp: " << sdp << std::endl;

		rtc::Description answer(sdp, type);

		std::shared_ptr<Receiver> currentReceiver;
		this->getReceiver(client, currentReceiver);

		currentReceiver->conn->setRemoteDescription(answer);
		return 0;
	}
	catch (Exception ex) {
		std::string error = "Something happened when trying to set the answer: " + ex.message();
		std::cout << error << std::endl;
		return -1;
	}
}

void ScreenStreamer::registerReceiver(WebSocket& client, Event* offerEvent) {
	std::shared_ptr<Receiver> r = std::make_shared<Receiver>();
	r->conn = std::make_shared<rtc::PeerConnection>();
	r->conn->onStateChange([this, client](rtc::PeerConnection::State state) {
		std::cout << "State: " << state << std::endl;
		std::shared_ptr<Receiver> currentReceiver;
		this->getReceiver(client, currentReceiver);
		if (state == rtc::PeerConnection::State::Connected) {
			currentReceiver->isConnected = true;
		}
		if (state == rtc::PeerConnection::State::Disconnected || state == rtc::PeerConnection::State::Closed) {
			this->receivers.erase(currentReceiver);
		}
	});

	r->conn->onGatheringStateChange([r, this, client, offerEvent](rtc::PeerConnection::GatheringState state) {
		std::cout << "Gathering State: " << state << std::endl;
		if (state == rtc::PeerConnection::GatheringState::Complete) {
			auto description = r->conn->localDescription();
			Object* jsonMessage = new Object();
			jsonMessage->set("type", description->typeString());
			jsonMessage->set("sdp", std::string(description.value()));

			std::ostringstream buffer;
			Stringifier::stringify(*jsonMessage, buffer);

			std::shared_ptr<Receiver> currentReceiver;
			this->getReceiver(client, currentReceiver);

			if (currentReceiver != NULL) {
				currentReceiver.get()->offer = buffer.str();
			}

			offerEvent->set();
		}
	});

	rtc::Description::Video media("video", rtc::Description::Direction::SendOnly);
	//media.addH264Codec(96);
	media.addVP9Codec(96);
	media.setBitrate(3000);
	media.addSSRC(ssrc, "video-send");

	r->track = r->conn->addTrack(media);

	r->track->onOpen([r]() {
		r->track->requestKeyframe(); // So the receiver can start playing immediately
	});

	r->track->onClosed([this, r]() {
		this->mutex->lock();
		this->receivers.erase(r);
		this->mutex->unlock();
	});

	r->track->onMessage([](rtc::binary var) {}, nullptr);

	r->conn->setLocalDescription();

	r->client = &client;

	receivers.insert(r);
}

std::string ScreenStreamer::getOffer(WebSocket& client) {
	std::shared_ptr<Receiver> currentReceiver;
	this->getReceiver(client, currentReceiver);
	if (currentReceiver != NULL) {
		return currentReceiver.get()->offer;
	}
	else {
		return "\"error\"";
	}
}

void ScreenStreamer::getReceiver(const WebSocket& client, std::shared_ptr<Receiver>& recv) {
	set<std::shared_ptr<Receiver>>::iterator itr;

	for (itr = receivers.begin(); itr != receivers.end(); itr++) {
		std::shared_ptr<Receiver> tmp = *itr;
		Receiver rr = *tmp;
		if (*(rr.client) == client) {
			recv = *itr;
			return;
		}
	}

	recv = NULL;
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

	ret = avformat_open_input(&in_InputFormatContext, "title=SimpleTextProjector", in_InputFormat, &in_InputFormatOptions); // second parameter is the -i parameter of ffmpeg, use title=window_title to specify the window later

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
	//## Configure output format && codecs    ##
	//##########################################

	AVFormatContext* out_OutputFormatContext = NULL;
	AVCodec* out_Codec;
	AVCodecContext* out_CodecContext;
	AVStream* out_Stream;
	AVRational serverFrameRate = { 30, 1 };
	AVDictionary* out_codecOptions = nullptr;
	const AVOutputFormat* out_OutputFormat = av_guess_format("rtp", NULL, NULL);

	ret = avformat_alloc_output_context2(&out_OutputFormatContext, nullptr, "rtp", nullptr);
	if (ret < 0) {
		std::cout << "Could not allocate output format context!" << std::endl;
		return -1;
	}

	// Allocate custom AVIO context (for intercepting writes)
	unsigned char* avio_ctx_buffer = (unsigned char*)av_malloc(4096);  // Buffer for AVIO context
	AVIOContext* avio_ctx = avio_alloc_context(avio_ctx_buffer, 4096, 1, this, nullptr, &custom_write, nullptr);

	if (!avio_ctx) {
		std::cerr << "Could not allocate AVIO context" << std::endl;
		return 1;
	}

	// Replace the default AVIO context with our custom one
	out_OutputFormatContext->pb = avio_ctx;
	out_OutputFormatContext->pb->max_packet_size = 1200;
	out_OutputFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;
	out_OutputFormatContext->oformat = out_OutputFormat;

	// set codec parameters
	out_Codec = (AVCodec*)avcodec_find_encoder(AV_CODEC_ID_VP9);
	out_Stream = avformat_new_stream(out_OutputFormatContext, out_Codec);
	out_CodecContext = avcodec_alloc_context3(out_Codec);

	printf("FFmpeg version: %s\n", av_version_info());

	if (!out_Codec) {
		fprintf(stderr, "Could not find VP9 codec.\n");
		return -1;
	}

	out_CodecContext->codec_id = AV_CODEC_ID_VP9;
	out_CodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	out_CodecContext->width = 1920;
	out_CodecContext->height = 1080;
	out_CodecContext->bit_rate = 3000;
	out_CodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
	out_CodecContext->framerate = serverFrameRate;
	out_CodecContext->time_base = av_inv_q(serverFrameRate);
	out_CodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
	out_OutputFormatContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

	if (out_OutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		out_CodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	ret = avcodec_parameters_from_context(out_Stream->codecpar, out_CodecContext);
	if (ret < 0) {
		std::cout << "Could not initialize stream codec parameters!" << std::endl;
		return -1;
	}
	
	av_dict_set(&out_codecOptions, "quality", "realtime", 0);
	av_dict_set(&out_codecOptions, "speed", "6", 0);

	ret = avcodec_open2(out_CodecContext, out_Codec, &out_codecOptions);
	if (ret < 0) {
		std::cout << "Could not open video encoder!" << std::endl;
		return -1;
	}

	av_dict_free(&out_codecOptions);

	out_Stream->codecpar->extradata = out_CodecContext->extradata;
	out_Stream->codecpar->extradata_size = out_CodecContext->extradata_size;

	av_dump_format(out_OutputFormatContext, 0, nullptr, 1);

	ret = avformat_write_header(out_OutputFormatContext, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when writing header\n");
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
	
	mutex->lock();
	shouldStream = true;
	mutex->unlock();

	while (!task->isCancelled()) {
		mutex->lock();

		if (!shouldStream) {
			mutex->unlock();
			break;
		}
		else {
			mutex->unlock();
		}

		if (receivers.size() <= 0) {
			mutex->unlock();
			Thread::sleep(100); // wait 100 ms then check again
			continue;
		}

		AVStream* in_stream, * out_stream;

		ret = av_read_frame(in_InputFormatContext, pkt);
		if (ret < 0) {
			mutex->unlock();
			break;
		}

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
			mutex->unlock();
			std::cerr << "Error sending packet to codec" << std::endl;
			return -1;
		}

		ret = avcodec_receive_frame(in_codec_ctx, frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			mutex->unlock();
			break;
		}
		else if (ret < 0) {
			std::cerr << "Error receiving frame from codec" << std::endl;
			mutex->unlock();
			return -1;
		}

		AVPacket* outPacket = av_packet_alloc();

		sws_scale(swsContext, frame->data, frame->linesize, 0, in_codec_ctx->height, out_frame->data, out_frame->linesize);

		ret = avcodec_send_frame(out_CodecContext, out_frame);
		if (ret < 0) {
			std::cerr << "Error encoding frame for output" << std::endl;
			mutex->unlock();
			return -1;
		}

		ret = avcodec_receive_packet(out_CodecContext, outPacket);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				// EAGAIN: Need to feed more frames
				mutex->unlock();
				av_packet_unref(pkt);
				av_packet_unref(outPacket);
				av_frame_free(&frame);
				av_frame_free(&out_frame);
				av_packet_free(&outPacket);
				continue;
			}
			std::cerr << "Error encoding packet for output" << std::endl;
			mutex->unlock();
			return -1;
		}

		outPacket->dts = pkt->dts;
		outPacket->pts = pkt->pts;

		ret = av_write_frame(out_OutputFormatContext, outPacket);

		av_packet_unref(pkt);
		av_packet_unref(outPacket);
		av_frame_free(&frame);
		av_frame_free(&out_frame);
		av_packet_free(&outPacket);

		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			mutex->unlock();
			break;
		}
		//mutex->unlock();
	}

	std::cout << "Ending server" << std::endl;
	
	//##########################
	//## free the used memory ##
	//##########################

	sws_freeContext(swsContext);

	av_write_trailer(out_OutputFormatContext);

	av_packet_free(&pkt);

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

	stopEvent->set();

	return 0;
}
