
#include <stdio.h>
#include <conio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32	//Windows
extern "C" {
#include "libavformat/avformat.h"
};
#else			//Linux...
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif

#define errReport(info, val) do{ \
	fprintf(stderr, "ERR:: %s %s(line=%d) code=%d\n", __FUNCTION__, info,  __LINE__, val);\
	getch(); exit(0);\
}while (0);

typedef struct {
	AVFormatContext *ifmt_ctx;
	int stream_index = 0;
	AVStream *in_stream, *out_stream;
}data_t;

AVOutputFormat *ofmt = NULL;
AVFormatContext *ifmt_ctx_v = NULL, *ifmt_ctx_a = NULL, *ofmt_ctx = NULL;
AVPacket pkt;
int videoindex_v = -1, videoindex_out = -1;
int audioindex_a = -1, audioindex_out = -1;
int frame_index = 0;
int64_t cur_pts_v = 0, cur_pts_a = 0;
#if 0
const char *in_filename_v = "input.h264";
const char *in_filename_a = "input.aac";
#if 1
const char *out_filename = "output.mkv";
#else
const char *out_filename = "output.ts";
#endif
#else
const char *in_filename_v = "input.h264";
const char *in_filename_a = "input.mp3";
#if 0
const char *out_filename = "output.mp4";
#elif 1
const char *out_filename = "output.flv";
#else
const char *out_filename = "output.avi";
#endif
#endif

int create_out_stream(AVFormatContext **fmt_ctx, AVMediaType type, int *idx, int *idx_o) {
	for (int i = 0; i < (*fmt_ctx)->nb_streams; i++) {
		if ((*fmt_ctx)->streams[i]->codec->codec_type == type) {
			AVStream *in_stream = (*fmt_ctx)->streams[i];
			AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
			*idx = i;
			if (!out_stream) return -1;

			*idx_o = out_stream->index;
			if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0)
				return -2;

			out_stream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
				out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			break;
		}
	}
	return 0;
}
int init_muxer() {
	int i;
	av_register_all();
	if (avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0) < 0)
		return -1;

	if (avformat_find_stream_info(ifmt_ctx_v, 0) < 0)
		return -2;

	if (avformat_open_input(&ifmt_ctx_a, in_filename_a, 0, 0) < 0)
		return -3;

	if (avformat_find_stream_info(ifmt_ctx_a, 0) < 0)
		return -4;

	printf("======================Input Information=====================\n");
	av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	av_dump_format(ifmt_ctx_a, 0, in_filename_a, 0);
	printf("============================================================\n");
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) return -5;

	ofmt = ofmt_ctx->oformat;
	if (create_out_stream(&ifmt_ctx_v, AVMEDIA_TYPE_VIDEO, &videoindex_v, &videoindex_out) < 0)
		return -6;
	if (create_out_stream(&ifmt_ctx_a, AVMEDIA_TYPE_AUDIO, &audioindex_a, &audioindex_out)<0)
		return -7;

	printf("=====================Output Information=====================\n");
	av_dump_format(ofmt_ctx, 0, out_filename, 1);
	printf("============================================================\n");
	if (!(ofmt->flags & AVFMT_NOFILE))
		if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0)
			return -8;

	if (avformat_write_header(ofmt_ctx, NULL) < 0)
		return -9;
	return 0;
}

int headle_data(data_t *d, AVFormatContext *fmt_ctx, int idx, int idx_o, int64_t *pts) {
	d->ifmt_ctx = fmt_ctx;
	d->stream_index = idx_o;
	if (av_read_frame(d->ifmt_ctx, &pkt) >= 0) {
		do {
			d->in_stream = d->ifmt_ctx->streams[pkt.stream_index];
			d->out_stream = ofmt_ctx->streams[d->stream_index];
			if (pkt.stream_index == idx) {
				//FIX£ºNo PTS (Example: Raw H.264)
				//Simple Write PTS
				if (pkt.pts == AV_NOPTS_VALUE) {
					//Write PTS
					AVRational time_base1 = d->in_stream->time_base;
					//Duration between 2 frames (us)
					int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(d->in_stream->r_frame_rate);
					//Parameters
					pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
					pkt.dts = pkt.pts;
					pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
					frame_index++;
				}
				*pts = pkt.pts;
				break;
			}
		} while (av_read_frame(d->ifmt_ctx, &pkt) >= 0);
	}
	else {
		return 1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	int ret, i;
	if ((ret = init_muxer()) < 0)
		errReport("init_muxer", ret);

	while (1) {
		data_t dat;
		//Get an AVPacket
		if (av_compare_ts(cur_pts_v, ifmt_ctx_v->streams[videoindex_v]->time_base,
			cur_pts_a, ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0) {
			if (headle_data(&dat, ifmt_ctx_v, videoindex_v, videoindex_out, &cur_pts_v) > 0)
				break;
		}
		else {
			if (headle_data(&dat, ifmt_ctx_a, audioindex_a, audioindex_out, &cur_pts_a) > 0)
				break;
		}
		//Convert PTS/DTS
		pkt.pts = av_rescale_q_rnd(pkt.pts, dat.in_stream->time_base, dat.out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, dat.in_stream->time_base, dat.out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, dat.in_stream->time_base, dat.out_stream->time_base);
		pkt.pos = -1;
		pkt.stream_index = dat.stream_index;

		printf("#:%5d:%lld\t", pkt.size, pkt.pts);
		if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
			printf("Error muxing packet\n");
			break;
		}
		av_free_packet(&pkt);
	}
	av_write_trailer(ofmt_ctx);

	avformat_close_input(&ifmt_ctx_v);
	avformat_close_input(&ifmt_ctx_a);
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF)
		printf("Error occurred.\n");
	else
		printf("successed.\n");
	getch();
	return 0;
}

