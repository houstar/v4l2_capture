#include "video_cap.h"

//#define INITMAP


VideoDevice::VideoDevice()
{
   // this->dev_name = dev_name;
    this->fd = -1;
    this->buffers = NULL;
    this->n_buffers = 0;
    this->index = -1;
}
VideoDevice::VideoDevice(int width, int height )
{
   // this->dev_name = dev_name;
    this->fd = -1;
    this->buffers = NULL;
    this->n_buffers = 0;
    this->index = -1;
    this->Vwidth = width;
    this->Vheight = height;
}

int VideoDevice::openCamera(char * dev_name)
{
  
    
    rs =open_device(dev_name);
    if(-1==rs)
    {
        printf("cannt open video device\n");
        close_device();
         return -1;
    }
    
    rs = init_device();
    if(-1==rs)
    {
       printf("cannt init video device\n");
       close_device();
        return -1;
       
    }
    rs = start_capturing();
    if(-1==rs)
    {
       printf("cannt  start capturing \n");
       close_device();
        return -1;
    }
    
    if(-1==rs)
    {
       printf("cannt  stop  capturing \n");
       stop_capturing();
      return -1;
    }
  return 0;
}
int VideoDevice::open_device(char * dev_name)
{
    fd = open(dev_name, O_RDWR/*|O_NONBLOCK*/, 0);
    // fd = open(dev_name.toStdString().c_str(), O_RDWR|O_NONBLOCK, 0);

    if(-1 == fd)
    {
        //emit display_error(tr("open: %1").arg(QString(strerror(errno))));
		printf("open: %u\n",errno);
        return -1;
    }
		printf("open: sucess\n",errno);
    return 0;
}

int VideoDevice::close_device()
{
    if(-1 == close(fd))
    {
       // emit display_error(tr("close: %1").arg(QString(strerror(errno))));
		printf("close: %u\n",errno);
        return -1;
    }
    return 0;
}

int VideoDevice::init_device()
{
    v4l2_capability cap;
    v4l2_cropcap cropcap;
    v4l2_crop crop;
    v4l2_format fmt;

    if(-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if(EINVAL == errno)
        {
           // emit display_error(tr("%1 is no V4l2 device").arg(dev_name));
		     printf("init:  no V4l2 device\n");
        }
        else
        {
            //emit display_error(tr("VIDIOC_QUERYCAP: %1").arg(QString(strerror(errno))));
            printf("VIDIOC_QUERYCAP: %u\n",errno);
        }
        return -1;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
       // emit display_error(tr("%1 is no video capture device").arg(dev_name));
		     printf("init: no video capture device\n");
        return -1;
    }

    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        //emit display_error(tr("%1 does not support streaming i/o").arg(dev_name));
		     printf("init:  does not support streaming i/o \n");
        return -1;
    }
	frame_info();
	printf("device properity\n");

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = Vwidth;
    fmt.fmt.pix.height = Vheight;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.bytesperline = fmt.fmt.pix.width;
	buffer_size = fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline *
		fmt.fmt.pix.height * 3 /2;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
#if 1
    if(-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        //emit display_error(tr("VIDIOC_S_FMT").arg(QString(strerror(errno))));
        printf("VIDIOC_S_FMT %u\n",errno);
        return -1;
    }
#endif
	printf("init_mmap\n");
#if defined(INITMAP)
    if(-1 == init_mmap())
    {
        return -1;
    }
#else
	
    if(-1 == init_userptr())
    {
        return -1;
    }
#endif

    return 0;
}
int VideoDevice::frame_info()
{
    struct v4l2_format fmt;
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd,VIDIOC_G_FMT,&fmt);
    printf("info:Current data format information:\n\twidth:%d\n\theight:%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.index=0;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
    {
        if(fmtdesc.pixelformat & fmt.fmt.pix.pixelformat)
        {
            printf("\tformat:%s\n",fmtdesc.description);
            break;
        }
        fmtdesc.index++;
    } 
    return 1;
}

int VideoDevice::init_userptr()
{
	v4l2_requestbuffers req;
	CLEAR(req);
	req.count = NUM_OF_BUF;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if( -1 == ioctl(fd,VIDIOC_REQBUFS,&req)){
		printf("couldn't allocate buffers\n");
		return -1;
	}

    buffers = (buffer*)calloc(req.count,sizeof(*buffers));

	if(!buffers){
		printf("Out of Memory!\n");
		return -1;
	}
	for(n_buffers =0; n_buffers < req.count;++n_buffers){
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = malloc(buffer_size);
		if(!buffers[n_buffers].start){
			printf("Out of Memory!\n");
			return -1;
		}
	}
	return 1;
}
int VideoDevice::init_mmap()
{
    v4l2_requestbuffers req;
    CLEAR(req);
    //NUM_OF_BUF
    //req.count = 4;
    req.count = NUM_OF_BUF;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(-1 == ioctl(fd, VIDIOC_REQBUFS, &req))
    {
        if(EINVAL == errno)
        {
            //emit display_error(tr("%1 does not support memory mapping").arg(dev_name));
			printf("does not support memory mapping\n");
            return -1;
        }
        else
        {
           // emit display_error(tr("VIDIOC_REQBUFS %1").arg(QString(strerror(errno))));
			printf("VIDIOC_REQBUFS %u\n",errno);
            return -1;
        }
    }

    if(req.count < 2)
    {
        //emit display_error(tr("Insufficient buffer memory on %1").arg(dev_name));
        return -1;
    }

    buffers = (buffer*)calloc(req.count, sizeof(*buffers));

    if(!buffers)
    {
        //emit display_error(tr("out of memory"));
        return -1;
    }

    for(n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if(-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            //emit display_error(tr("VIDIOC_QUERYBUF: %1").arg(QString(strerror(errno))));
            return -1;
        }

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
                mmap(NULL, // start anywhere
                     buf.length,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd, buf.m.offset);

        if(MAP_FAILED == buffers[n_buffers].start)
        {
            //emit display_error(tr("mmap %1").arg(QString(strerror(errno))));
            printf("mmap %u\n",errno);
            return -1;
        }
    }
    return 0;

}


int VideoDevice::start_capturing()
{
	printf("start_capturing\n");
    unsigned int i;
    for(i = 0; i < n_buffers; ++i)
    {
        v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = i;
		//userptr
#if defined(INITMAP)
        buf.memory =V4L2_MEMORY_MMAP;
#else
        buf.memory =V4L2_MEMORY_USERPTR;
		buf.m.userptr =(unsigned long)buffers[i].start;
		buf.length = buffers[i].length;
#endif
        //        fprintf(stderr, "n_buffers: %d\n", i);

        if(-1 == ioctl(fd, VIDIOC_QBUF, &buf))
        {
            //emit display_error(tr("VIDIOC_QBUF: %1").arg(QString(strerror(errno))));
            printf("VIDIOC_QBUF: %u",errno);
            return -1;
        }
    }

    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == ioctl(fd, VIDIOC_STREAMON, &type))
    {
        //emit display_error(tr("VIDIOC_STREAMON: %1").arg(QString(strerror(errno))));
        printf("VIDIOC_STREAMON: %u",errno);
        return -1;
    }
    printf("started capture");
    return 0;
}


int VideoDevice::stop_capturing()
{
    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        //emit display_error(tr("VIDIOC_STREAMOFF: %1").arg(QString(strerror(errno))));
        return -1;
    }
    return 0;
}

int VideoDevice::uninit_device()
{
    unsigned int i;
    for(i = 0; i < n_buffers; ++i)
    {
        if(-1 == munmap(buffers[i].start, buffers[i].length))
        {
           // emit display_error(tr("munmap: %1").arg(QString(strerror(errno))));
            return -1;
        }

    }
    free(buffers);
    return 0;
}

int VideoDevice::get_frame(void **frame_buf, size_t* len)
{
    v4l2_buffer queue_buf;
    CLEAR(queue_buf);

    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if defined(INITMAP)
    queue_buf.memory = V4L2_MEMORY_MMAP;
#else
    queue_buf.memory = V4L2_MEMORY_USERPTR;
#endif
    if(-1 == ioctl(fd, VIDIOC_DQBUF, &queue_buf))
    {
        switch(errno)
        {
        case EAGAIN:
            //            perror("dqbuf");
            return -1;
        case EIO:
            return -1 ;
        default:
           // emit display_error(tr("VIDIOC_DQBUF: %1").arg(QString(strerror(errno))));
            return -1;
        }
    }

    *frame_buf = buffers[queue_buf.index].start;
    *len = buffers[queue_buf.index].length;
    index = queue_buf.index;

    return 1;

}

int VideoDevice::unget_frame()
{
    if(index != -1)
    {
        v4l2_buffer queue_buf;
        CLEAR(queue_buf);

        queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if defined(INITMAP)
        queue_buf.memory = V4L2_MEMORY_MMAP;
#else
    queue_buf.memory = V4L2_MEMORY_USERPTR;
#endif
    if(-1 == ioctl(fd, VIDIOC_DQBUF, &queue_buf))
    {
        switch(errno)
        {
        case EAGAIN:
            //            perror("dqbuf");
            return -1;
        case EIO:
            return -1 ;
        default:
           // emit display_error(tr("VIDIOC_DQBUF: %1").arg(QString(strerror(errno))));
            return -1;
        }
    }
	}
    return 1;
}
/*
int VideoDevice::unget_frame()
{
    if(index != -1)
    {
        v4l2_buffer queue_buf;
        CLEAR(queue_buf);

        queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if defined(INITMAP)
        queue_buf.memory = V4L2_MEMORY_MMAP;
#else
    queue_buf.memory = V4L2_MEMORY_USERPTR;
#endif
        queue_buf.index = index;

        if(-1 == ioctl(fd, VIDIOC_QBUF, &queue_buf))
        {
            //emit display_error(tr("VIDIOC_QBUF: %1").arg(QString(strerror(errno))));
            return -1;
        }
        return 0;
        queue_buf.index = index;

        if(-1 == ioctl(fd, VIDIOC_QBUF, &queue_buf))
        {
            //emit display_error(tr("VIDIOC_QBUF: %1").arg(QString(strerror(errno))));
            return -1;
        }
        return 0;
    }
    return -1;
}
*/





    inline void VideoDevice::yuv_to_rgb24_N(unsigned char y, unsigned char u, unsigned char v,  unsigned char *rgb) {

    register int r, g, b;


    r = (1192 * (y - 16) + 1634 * (v - 128)) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u - 128)) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128)) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    *rgb = r;
    rgb++;
    *rgb = g;
    rgb++;
    *rgb = b;



#if 0
    rgb16 = (int) (((r >> 3) << 11) | ((g >> 2) << 5) | ((b >> 3) << 0));

    *rgb = (unsigned char) (rgb16 & 0xFF);
    rgb++;
    *rgb = (unsigned char) ((rgb16 & 0xFF00) >> 8);
#endif
}

     void VideoDevice::YUVToRGB24_N(unsigned char *buf, unsigned char *rgb, int width, int height) {


    int  y = 0;
    int blocks;

    blocks = (width * height) * 2;
    int j = 0;

    for (y = 0; y < blocks; y += 4) {
        unsigned char Y1, Y2, U1, V1;

        Y1 = buf[y + 0];
        U1 = buf[y + 1];
        Y2 = buf[y + 2];
        V1 = buf[y + 3];


        yuv_to_rgb24_N(Y1, U1, V1, &rgb[j]);
        yuv_to_rgb24_N(Y2, U1, V1, &rgb[j + 3]);
        j +=6;
    }

}

     void VideoDevice::YUVToRGB24_8_N(unsigned char *buf, unsigned char *rgb, int width, int height) {


    int  y = 0;
    int blocks;

    blocks = (width * height) * 2;
    int j = 0;

    for (y = 0; y < blocks; y += 8) {
        unsigned char Y1, Y2, U1, V1;
        unsigned char Y1_1, Y2_1, U1_1, V1_1;

        Y1 = buf[y + 0];
        U1 = buf[y + 1];
        Y2 = buf[y + 2];
        V1 = buf[y + 3];

        // V1 = buf[y + 1];
        //U1 = buf[y + 3];

        Y1_1 = buf[y + 4];
        U1_1 = buf[y + 5];
        Y2_1 = buf[y + 6];
        V1_1 = buf[y + 7];

        // V1_1 = buf[y + 7];
        // U1_1 = buf[y + 5];




        yuv_to_rgb24_N(Y1, U1, V1, &rgb[j]);
        yuv_to_rgb24_N(Y2, U1, V1, &rgb[j + 3]);

        yuv_to_rgb24_N(Y1_1, U1_1, V1_1, &rgb[j + 6]);
        yuv_to_rgb24_N(Y2_1, U1_1, V1_1, &rgb[j + 9]);
        j +=12;
    }

}


     void VideoDevice::rgb24_to_rgb565_N(unsigned char  *rgb24, unsigned char *rgb16, int width, int height)
{
    int i = 0, j = 0;
    for (i = 0; i < width*height*3; i += 3)
    {
        rgb16[j] = rgb24[i] >> 3; // B
        rgb16[j] |= ((rgb24[i+1] & 0x1C) << 3); // G
        rgb16[j+1] = rgb24[i+2] & 0xF8; // R
        rgb16[j+1] |= (rgb24[i+1] >> 5); // G
        j += 2;
    }
}


     void VideoDevice::rgb24_to_rgb565_6_N(unsigned char  *rgb24, unsigned char *rgb16, int width, int height)
{
    int i = 0, j = 0;
    for (i = 0; i < width*height*3; i += 6)
    {
        rgb16[j] = rgb24[i] >> 3; // B
        rgb16[j] |= ((rgb24[i+1] & 0x1C) << 3); // G
        rgb16[j+1] = rgb24[i+2] & 0xF8; // R
        rgb16[j+1] |= (rgb24[i+1] >> 5); // G

        rgb16[j+2] = rgb24[i+3] >> 3; // B
        rgb16[j+2] |= ((rgb24[i+4] & 0x1C) << 3); // G
        rgb16[j+3] = rgb24[i+5] & 0xF8; // R
        rgb16[j+3] |= (rgb24[i+4] >> 5); // G

        j += 4;
    }
}

     int VideoDevice::convert_yuv_to_rgb_pixel_N(int y, int u, int v)
{
    unsigned int pixel32 = 0;
    unsigned char *pixel = (unsigned char *)&pixel32;
    int r, g, b;
    r = y + (1.370705 * (v-128));
    g = y - (0.698001 * (v-128)) - (0.337633 * (u-128));
    b = y + (1.732446 * (u-128));
    if(r > 255) r = 255;
    if(g > 255) g = 255;
    if(b > 255) b = 255;
    if(r < 0) r = 0;
    if(g < 0) g = 0;
    if(b < 0) b = 0;
    pixel[0] = r * 220 / 256;
    pixel[1] = g * 220 / 256;
    pixel[2] = b * 220 / 256;
    return pixel32;
}
/*yuv格式转换为rgb格式*/
     int VideoDevice::convert_yuv_to_rgb_buffer_N(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height)
{
    unsigned int in, out = 0;
    unsigned int pixel_16;
    unsigned char pixel_24[3];
    unsigned int pixel32;
    int y0, u, y1, v;
    for(in = 0; in < width * height * 2; in += 4) {
        pixel_16 =
                yuv[in + 3] << 24 |
                yuv[in + 2] << 16 |
                yuv[in + 1] <<  8 |
                yuv[in + 0];
        y0 = (pixel_16 & 0x000000ff);
        u  = (pixel_16 & 0x0000ff00) >>  8;
        y1 = (pixel_16 & 0x00ff0000) >> 16;
        v  = (pixel_16 & 0xff000000) >> 24;
        pixel32 = convert_yuv_to_rgb_pixel_N(y0, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        rgb[out++] = pixel_24[0];
        rgb[out++] = pixel_24[1];
        rgb[out++] = pixel_24[2];
        pixel32 = convert_yuv_to_rgb_pixel_N(y1, u, v);
        pixel_24[0] = (pixel32 & 0x000000ff);
        pixel_24[1] = (pixel32 & 0x0000ff00) >> 8;
        pixel_24[2] = (pixel32 & 0x00ff0000) >> 16;
        rgb[out++] = pixel_24[0];
        rgb[out++] = pixel_24[1];
        rgb[out++] = pixel_24[2];
    }
    return 0;
}


     int VideoDevice::yuv_to_rgb16_N(unsigned char y,unsigned char u,unsigned char v,unsigned char * rgb)
{

    register int r,g,b;
    int rgb16;

    r = (1192 * (y - 16) + 1634 * (v - 128) ) >> 10;
    g = (1192 * (y - 16) - 833 * (v - 128) - 400 * (u -128) ) >> 10;
    b = (1192 * (y - 16) + 2066 * (u - 128) ) >> 10;

    r = r > 255 ? 255 : r < 0 ? 0 : r;
    g = g > 255 ? 255 : g < 0 ? 0 : g;
    b = b > 255 ? 255 : b < 0 ? 0 : b;

    rgb16 = (int)(((r >> 3)<<11) | ((g >> 2) << 5)| ((b >> 3) << 0));

    *rgb = (unsigned char)(rgb16 & 0xFF);
    rgb++;
    *rgb = (unsigned char)((rgb16 & 0xFF00) >> 8);
    return 0;

}
    int VideoDevice::convert_m_N(unsigned char * buf,unsigned char * rgb,int width,int height)
{
    int y=0;
    int blocks;

    blocks = (width * height) * 2;

    for (y = 0; y < blocks; y+=4) {
        unsigned char Y1, Y2, U, V;

        ///////////////////////////////////////
        Y1 = buf[y + 0];
        U = buf[y + 1];
        Y2 = buf[y + 2];
        V = buf[y + 3];
        //////////////////////////////////////

        yuv_to_rgb16_N(Y1, U, V, &rgb[y]);
        yuv_to_rgb16_N(Y2, U, V, &rgb[y + 2]);
        //yuv_to_rgb16(Y1, 0x80, 0x80, &rgb[y]);
        //yuv_to_rgb16(Y2, 0x80, 0x80, &rgb[y + 2]);
    }

    return 0;
}

/*yuv格式转换为rgb格式*/


    inline void  VideoDevice::yuv2rgb565_N(const int y, const int u, const int v, unsigned short &rgb565)
{
    int r, g, b;

    r = y + redAdjust[v];
    g = y + greenAdjust1[u] + greenAdjust2[v];
    b = y + blueAdjust[u];

#define CLAMP(x) if (x < 0) x = 0 ; else if (x > 255) x = 255
    CLAMP(r);
    CLAMP(g);
    CLAMP(b);
#undef CLAMP
    rgb565 = (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

    void  VideoDevice::ConvertYUYVtoRGB565_N(const void *yuyv_data, void *rgb565_data, const unsigned int w, const unsigned int h)
{

    const unsigned char *src = (unsigned char *)yuyv_data;
    unsigned short *dst = (unsigned short *)rgb565_data;

    for (unsigned int i = 0, j = 0; i < w * h * 2; i += 4, j += 2) {
        int y, u, v;
        unsigned short rgb565;

        y = src[i + 0];
        u = src[i + 1];
        v = src[i + 3];
        yuv2rgb565_N(y, u, v, rgb565);
        dst[j + 0] = rgb565;


        y = src[i + 2];
        yuv2rgb565_N(y, u, v, rgb565);
        dst[j + 1] = rgb565;

    }
}



