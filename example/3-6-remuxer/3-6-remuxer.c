#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

int main(int argc, char *argv[])
{
    AVPacket pkt;
    AVFormatContext *input_fmtctx = NULL;
    AVFormatContext *output_fmtctx = NULL;
    AVCodecContext *enc_ctx = NULL;
    AVCodecContext *dec_ctx = NULL;
    AVCodec *encoder = NULL;
    int ret = 0;
    int i = 0;

    av_register_all();

    if (avformat_open_input(&input_fmtctx, "./a.avi", NULL, NULL) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open the file %s\n", "./a.mp4");
        return -ENOENT;
    }

    if (avformat_find_stream_info(input_fmtctx,0) < 0) {
        av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ Failed to retrieve input stream information\n");
        return -EINVAL;
    }

    av_dump_format(input_fmtctx, NULL, "./a.avi", 0);

    if (avformat_alloc_output_context2(&output_fmtctx, NULL, NULL, "./fuck.mp4") < 0) {
        av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ Cannot open the file %s\n", "./fuck.mp4");
        return -ENOENT;
    }

    for (i = 0; i < input_fmtctx->nb_streams; i++) {
        AVStream *out_stream = NULL;
        AVStream *in_stream = NULL;
        in_stream = input_fmtctx->streams;
        out_stream = avformat_new_stream(output_fmtctx, NULL);
        if (out_stream < 0) {
            av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ Alloc new Stream error\n");
            return -EINVAL;
        }

        avcodec_copy_context(output_fmtctx->streams->codec, input_fmtctx->streams->codec);
        out_stream->codec->codec_tag = 0;
        if (output_fmtctx->oformat->flags & AVFMT_GLOBALHEADER) {
            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        }
    }
    av_dump_format(output_fmtctx, NULL, "./fuck.mp4", 1);
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    if (avio_open(&output_fmtctx->pb, "./fuck.mp4", AVIO_FLAG_WRITE) < 0) {
        av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ cannot open the output file '%s'\n", "./fuck.mp4");
        return -ENOENT;
    }

    if ((ret = avformat_write_header(output_fmtctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ Cannot write the header for the file '%s' ret = %d\n", "fuck.mp4", ret);
        return -ENOENT;
    }


    while (1) {
        AVStream *in_stream = NULL;
        AVStream *out_stream = NULL;

        ret = av_read_frame(input_fmtctx, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ read frame error %d\n", ret);
            break;
        }

        in_stream = input_fmtctx->streams[pkt.stream_index];
        out_stream = output_fmtctx->streams[pkt.stream_index];
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        ret = av_write_frame(output_fmtctx, &pkt);

        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "bbs.chinaffmpeg.org ËïÎò¿Õ Muxing Error\n");
            break;
        }
        av_free_packet(&pkt);
    }

    av_write_trailer(output_fmtctx);
    avformat_close_input(&input_fmtctx);
    avio_close(output_fmtctx->pb);
    avformat_free_context(output_fmtctx);

        return 0;

}

