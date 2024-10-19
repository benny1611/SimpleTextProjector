#include "ScreenStreamer.h"

using namespace std;
using Poco::JSON::Object;
using Poco::JSON::Stringifier;
using Poco::Dynamic::Var;

/* initialize the resources*/
ScreenStreamer::ScreenStreamer(Task* tsk, Event* stop_event, Mutex* mtx, Logger* logger) {
	avdevice_register_all();
	task = tsk;
	stopEvent = stop_event;
	mutex = mtx;
	appLogger = logger;
}

ScreenStreamer::~ScreenStreamer() {}

void ScreenStreamer::stopStreaming() {
	shouldStream = false;
}

std::string ScreenStreamer::peerStateToString(rtc::PeerConnection::State state) {
	switch (state) {
	case rtc::PeerConnection::State::New:
		return "New";
	case rtc::PeerConnection::State::Connecting:
		return "Connecting";
	case rtc::PeerConnection::State::Connected:
		return "Connected";
	case rtc::PeerConnection::State::Disconnected:
		return "Disconnected";
	case rtc::PeerConnection::State::Failed:
		return "Failed";
	case rtc::PeerConnection::State::Closed:
		return "Closed";
	default:
		return "Unknown";
	}
}

std::string ScreenStreamer::gatheringStateToString(rtc::PeerConnection::GatheringState state) {
	switch (state) {
	case rtc::PeerConnection::GatheringState::New:
		return "New";
	case rtc::PeerConnection::GatheringState::InProgress:
		return "In Progress";
	case rtc::PeerConnection::GatheringState::Complete:
		return "Complete";
	default:
		return "Unknown";
	}
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

		appLogger->information("Type: %s, spd: %s", type, sdp);

		rtc::Description answer(sdp, type);

		std::shared_ptr<Receiver> currentReceiver;
		this->getReceiver(client, currentReceiver);

		currentReceiver->conn->setRemoteDescription(answer);
		return 0;
	}
	catch (Exception ex) {
		std::string error = "Something happened when trying to set the answer: " + ex.message();
		appLogger->error(error);
		return -1;
	}
}

void ScreenStreamer::registerReceiver(WebSocket& client, Event* offerEvent) {
	std::shared_ptr<Receiver> r = std::make_shared<Receiver>();
	r->conn = std::make_shared<rtc::PeerConnection>();
	r->conn->onStateChange([this, client](rtc::PeerConnection::State state) {
		this->appLogger->information("State: %s", this->peerStateToString(state));
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
		this->appLogger->information("Gathering State: %s", this->gatheringStateToString(state));
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
		appLogger->error("error in setting dictionary value");
		goto input_error;
	}

	ret = avformat_open_input(&in_InputFormatContext, "title=SimpleTextProjector", in_InputFormat, &in_InputFormatOptions); // second parameter is the -i parameter of ffmpeg, use title=window_title to specify the window later

	if (ret != 0) {
		appLogger->error("error: could not open screen capture");
		goto input_error;
	}

	ret = avformat_find_stream_info(in_InputFormatContext, &in_InputFormatOptions);
	if (ret < 0) {
		appLogger->error("error in avformat_find_stream_info");
		goto input_error;
	}

	in_codec = (AVCodec*)avcodec_find_decoder(in_InputFormatContext->streams[0]->codecpar->codec_id);
	if (in_codec == NULL) {
		appLogger->error("Error occurred getting the in_codec");
		goto input_error;
	}

	in_codec_ctx = avcodec_alloc_context3(in_codec);
	if (in_codec_ctx == NULL) {
		appLogger->error("Error occurred when getting in_codec_ctx");
		goto input_error;
	}

	ret = avcodec_parameters_to_context(in_codec_ctx, in_InputFormatContext->streams[0]->codecpar);
	if (ret < 0) {
		appLogger->error("Error occurred when avcodec_parameters_to_context");
		goto input_error;
	}

	ret = avcodec_open2(in_codec_ctx, in_codec, NULL);
	if (ret < 0) {
		appLogger->error("Error occurred when avcodec_open2");
		goto input_error;
	}

	if (false) {
input_error:
		avformat_close_input(&in_InputFormatContext);
		if (in_InputFormatContext == NULL) {
			appLogger->information("Input format context closed successuflly");
		} else {
			appLogger->error("Input format context wasn't closed properly");
		}

		avformat_free_context(in_InputFormatContext);
		
		avcodec_free_context(&in_codec_ctx);
		if (in_codec_ctx == NULL) {
			appLogger->information("Input codec context was closed successfully");
		} else {
			appLogger->error("Input codec context wasn't closed properly");
		}

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
	AVIOContext* avio_ctx;
	unsigned char* avio_ctx_buffer;

	ret = avformat_alloc_output_context2(&out_OutputFormatContext, nullptr, "rtp", nullptr);
	if (ret < 0) {
		appLogger->error("Could not allocate output format context!");
		goto output_error;
	}

	// Allocate custom AVIO context (for intercepting writes)
	avio_ctx_buffer = (unsigned char*)av_malloc(4096);  // Buffer for AVIO context
	avio_ctx = avio_alloc_context(avio_ctx_buffer, 4096, 1, this, nullptr, &custom_write, nullptr);

	if (!avio_ctx) {
		appLogger->error("Could not allocate AVIO context");
		goto output_error;
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

	appLogger->information("FFmpeg version: %s", std::string(av_version_info()));

	if (!out_Codec) {
		appLogger->error("Could not find VP9 codec.");
		goto output_error;
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
		appLogger->error("Could not initialize stream codec parameters!");
		goto output_error;
	}
	
	av_dict_set(&out_codecOptions, "quality", "realtime", 0);
	av_dict_set(&out_codecOptions, "speed", "6", 0);

	ret = avcodec_open2(out_CodecContext, out_Codec, &out_codecOptions);
	if (ret < 0) {
		appLogger->error("Could not open video encoder!");
		goto output_error;
	}

	av_dict_free(&out_codecOptions);

	out_Stream->codecpar->extradata = out_CodecContext->extradata;
	out_Stream->codecpar->extradata_size = out_CodecContext->extradata_size;

	av_dump_format(out_OutputFormatContext, 0, nullptr, 1);

	ret = avformat_write_header(out_OutputFormatContext, NULL);
	if (ret < 0) {
		appLogger->error("Error occurred when writing header");
		goto output_error;
	}

	if (false) {
output_error:
		// close input
		avformat_close_input(&in_InputFormatContext);
		if (in_InputFormatContext == NULL) {
			appLogger->information("Input format context closed successuflly");
		}
		else {
			appLogger->error("Input format context wasn't closed properly");
		}

		avformat_free_context(in_InputFormatContext);

		avcodec_free_context(&in_codec_ctx);
		if (in_codec_ctx == NULL) {
			appLogger->information("Input codec context was closed successfully");
		}
		else {
			appLogger->error("Input codec context wasn't closed properly");
		}

		// close output
		avformat_free_context(out_OutputFormatContext);
		avio_context_free(&avio_ctx);
		if (avio_ctx == NULL) {
			appLogger->information("AVIO context closed successfully");
		} else {
			appLogger->error("AVIO context wasn't closed properly");
		}

		avcodec_free_context(&out_CodecContext);
		if (out_CodecContext == NULL) {
			appLogger->information("Output context closed sucessfully");
		} else {
			appLogger->error("Output context wasn't closed properly");
		}

		return -1;
	}


	//##########################################################
	//## Send frames from input (screen) to the output server ##
	//##########################################################

	bool errorDuringServing = false;
	AVPacket* pkt;
	SwsContext* swsContext = sws_getContext(in_codec_ctx->width, in_codec_ctx->height, in_codec_ctx->pix_fmt, out_CodecContext->width, out_CodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
	
	if (swsContext != NULL) {
		appLogger->information("SWS context successfully created");
	} else {
		appLogger->error("Could not create SWS context");
		goto end_server;
	}

	pkt = av_packet_alloc();
	if (!pkt) {
		appLogger->error("Could not allocate AVPacket");
		goto end_server;
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
			Thread::sleep(100); // wait 100 ms then check again
			continue;
		}

		AVStream* in_stream, * out_stream;

		ret = av_read_frame(in_InputFormatContext, pkt);
		if (ret < 0) {
			appLogger->error("Could not read frame");
			errorDuringServing = true;
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
			appLogger->error("Error sending packet to codec");
			errorDuringServing = true;
			break;
		}

		ret = avcodec_receive_frame(in_codec_ctx, frame);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			appLogger->error("Error: AVERROR(EAGAIN) or AVERROR_EOF when receiving frames from desktop capture");
			continue;
		}
		else if (ret < 0) {
			appLogger->error("Error receiving frame from codec");
			errorDuringServing = true;
			av_frame_free(&frame);
			break;
		}

		AVPacket* outPacket = av_packet_alloc();

		sws_scale(swsContext, frame->data, frame->linesize, 0, in_codec_ctx->height, out_frame->data, out_frame->linesize);

		ret = avcodec_send_frame(out_CodecContext, out_frame);
		if (ret < 0) {
			appLogger->error("Error encoding frame for output");
			errorDuringServing = true;
			av_frame_free(&frame);
			av_frame_free(&out_frame);
			break;
		}

		ret = avcodec_receive_packet(out_CodecContext, outPacket);
		if (ret < 0) {
			av_packet_unref(pkt);
			av_packet_unref(outPacket);
			av_frame_free(&frame);
			av_frame_free(&out_frame);
			av_packet_free(&outPacket);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				// EAGAIN: Need to feed more frames
				continue;
			} else {
				appLogger->error("Error encoding packet for output");
				errorDuringServing = true;
				break;
			}
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
			appLogger->error("Error muxing packet");
			errorDuringServing = true;
			break;
		}
	}

end_server:

	appLogger->information("Ending server");
	
	//##########################
	//## free the used memory ##
	//##########################

	sws_freeContext(swsContext);
	if (swsContext == NULL) {
		appLogger->information("SWS context closed successfully");
	} else {
		appLogger->information("SWS context wasn't closed properly");
	}

	av_packet_free(&pkt);
	

	// close input
	avformat_close_input(&in_InputFormatContext);
	if (in_InputFormatContext == NULL) {
		appLogger->information("Input format context closed successuflly");
	}
	else {
		appLogger->error("Input format context wasn't closed properly");
	}

	avformat_free_context(in_InputFormatContext);

	avcodec_free_context(&in_codec_ctx);
	if (in_codec_ctx == NULL) {
		appLogger->information("Input codec context was closed successfully");
	}
	else {
		appLogger->error("Input codec context wasn't closed properly");
	}

	// close output
	avformat_free_context(out_OutputFormatContext);
	avio_context_free(&avio_ctx);
	if (avio_ctx == NULL) {
		appLogger->information("AVIO context closed successfully");
	}
	else {
		appLogger->error("AVIO context wasn't closed properly");
	}

	avcodec_free_context(&out_CodecContext);
	if (out_CodecContext == NULL) {
		appLogger->information("Output context closed sucessfully");
	}
	else {
		appLogger->error("Output context wasn't closed properly");
	}

	stopEvent->set();
	if (errorDuringServing) {
		return 0;
	} else {
		return -1;
	}
}
