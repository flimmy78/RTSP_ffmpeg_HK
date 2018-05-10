#include "MyEncoder.h"

MyEncoder::MyEncoder()
{
}

void MyEncoder::Ffmpeg_Encoder_Init()
{
	av_register_all();
	avcodec_register_all();
	avformat_network_init();

	m_pRGBFrame = new AVFrame[1];//RGB֡���ݸ�ֵ    
	m_pYUVFrame = new AVFrame[1];//YUV֡���ݸ�ֵ    

	c = NULL;//������ָ����󸳳�ֵ  
}

void MyEncoder::Ffmpeg_Encoder_Setpara(AVCodecID mycodeid)
{
	fmtctx = avformat_alloc_context();
	fmt = av_guess_format(NULL, out_filename, NULL);
	fmtctx->oformat = fmt;
	avformat_alloc_output_context2(&fmtctx, NULL, "rtsp", out_filename);

	pCodecH264 = avcodec_find_encoder(mycodeid);//����h264������  
	if (!pCodecH264)
	{
		fprintf(stderr, "h264 codec not found\n");
		exit(1);
	}

	avio_open(&fmtctx->pb, out_filename, AVIO_FLAG_WRITE);//����

	width = 1920;
	height = 1080;

	video_st = avformat_new_stream(fmtctx, pCodecH264);
	video_st->time_base.num = 1;
	video_st->time_base.den = 25;//֡��

	c = video_st->codec;
	avcodec_get_context_defaults3(c, pCodecH264);

	//c = avcodec_alloc_context3(pCodecH264);//�������ڷ���һ��AVCodecContext������Ĭ��ֵ�����ʧ�ܷ���NULL��������av_free()�����ͷ�  
	c->codec_id = mycodeid;
	c->codec_tag = 0;
	c->codec_type = AVMEDIA_TYPE_VIDEO;
	c->bit_rate = 400000; //���ò�����������������  
	c->width = width;//���ñ�����Ƶ���   
	c->height = height; //���ñ�����Ƶ�߶�  
	c->time_base.den = 25;//����֡��,numΪ���Ӻ�denΪ��ĸ�������1/25���ʾ25֡/s  
	c->time_base.num = 1;
	c->gop_size = 10; //����GOP��С,��ֵ��ʾÿ10֡�����һ��I֡  
	c->max_b_frames = 1;//����B֡�����,��ֵ��ʾ��������B֮֡�䣬����������B֡�����֡��  
	c->pix_fmt = AV_PIX_FMT_YUV420P;//�������ظ�ʽ  

	av_opt_set(c->priv_data, "tune", "zerolatency", 0);//���ñ���������ʱ�����ǰ��ļ�ʮ֡�������ݵ����  
	av_opt_set(c->priv_data, "preset", "slow", 0);
	av_opt_set(c->priv_data, "tune", "zerolatency", 0);
	av_opt_set(c->priv_data, "x264opts", "crf=26:vbv-maxrate=728:vbv-bufsize=3640:keyint=25", 0);

	if (avcodec_open2(video_st->codec, pCodecH264, NULL) < 0)return;//�򿪱�����  
	avformat_write_header(fmtctx, NULL);

	nDataLen = width*height * 3;//����ͼ��rgb����������  

	yuv_buff = new uint8_t[nDataLen / 2];//��ʼ����������Ϊyuvͼ��֡׼����仺��  
	rgb_buff = new uint8_t[nDataLen];//��ʼ����������Ϊrgbͼ��֡׼����仺��  
	outbuf_size = 100000;////��ʼ���������������  
	outbuf = new uint8_t[outbuf_size];

	scxt = sws_getContext(c->width, c->height, AV_PIX_FMT_BGR24, c->width, c->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);//��ʼ����ʽת������  
}

void MyEncoder::Ffmpeg_Encoder_Encode(FILE *file, uint8_t *data)
{
	memcpy(rgb_buff, data, nDataLen);//����ͼ�����ݵ�rgbͼ��֡������׼������  
	avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)rgb_buff, AV_PIX_FMT_RGB24, width, height);//��rgb_buff��䵽m_pRGBFrame  
																								 //av_image_fill_arrays((AVPicture*)m_pRGBFrame, (uint8_t*)rgb_buff, AV_PIX_FMT_RGB24, width, height);
	avpicture_fill((AVPicture*)m_pYUVFrame, (uint8_t*)yuv_buff, AV_PIX_FMT_YUV420P, width, height);//��yuv_buff��䵽m_pYUVFrame  
	sws_scale(scxt, m_pRGBFrame->data, m_pRGBFrame->linesize, 0, c->height, m_pYUVFrame->data, m_pYUVFrame->linesize);// ��RGBת��ΪYUV  
	
	int myoutputlen = 0;
	av_init_packet(&pkt);
	pkt.data = NULL;    // packet data will be allocated by the encoder
	pkt.size = 0;
	pkt.pts = AV_NOPTS_VALUE;
	pkt.dts = AV_NOPTS_VALUE;
	m_pYUVFrame->pts = video_st->codec->frame_number;
	int returnvalue = avcodec_encode_video2(c, &pkt, m_pYUVFrame, &myoutputlen);


	if (returnvalue < 0)
	{
		fprintf(stderr, "Error encoding video frame:\n");
		exit(1);
	}
	if (returnvalue == 0)
	{
		fwrite(pkt.data, 1, pkt.size, file);
	}
	if (myoutputlen)
	{
		if (c->coded_frame->key_frame)
			pkt.flags |= AV_PKT_FLAG_KEY;
		pkt.stream_index = video_st->index;

		//����PTS
		if (pkt.pts != AV_NOPTS_VALUE)
		{
			pkt.pts = av_rescale_q(pkt.pts, video_st->codec->time_base, video_st->time_base);
		}
		if (pkt.dts != AV_NOPTS_VALUE)
		{
			pkt.dts = av_rescale_q(pkt.dts, video_st->codec->time_base, video_st->time_base);
		}
		//д��һ��AVPacket������ļ�, ������һ�������
		av_interleaved_write_frame(fmtctx, &pkt);
	}
	av_free_packet(&pkt);
}

void MyEncoder::Ffmpeg_Encoder_Close()
{
	av_write_trailer(fmtctx);
	delete[]m_pRGBFrame;
	delete[]m_pYUVFrame;
	delete[]rgb_buff;
	delete[]yuv_buff;
	delete[]outbuf;
	sws_freeContext(scxt);
	avcodec_close(c);//�رձ�����  
	av_free(c);
	avio_close(fmtctx->pb);
	avformat_free_context(fmtctx);
}