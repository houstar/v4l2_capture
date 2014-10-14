extern "C"  
{  
#include <stdio.h>  
#include <string.h>  
#include <fcntl.h>  
#include <unistd.h>  
#include <stdint.h>  
#include <stdlib.h>  
#include <errno.h>  
#include <getopt.h>  
#include <sys/ioctl.h>  
#include <sys/mman.h>  
#include <sys/select.h>  
#include <sys/time.h>  
#include <linux/fb.h>  
#include <linux/videodev.h>  

}  



#include "video_cap.h"  


int main( int argc, char* argv[] )  
{  

	int width = 1024;  
	int height = 768;  
	VideoDevice *pVideoDev = new VideoDevice(width,height);  
	// CDisplayFB  * m_pDisplay = new  CDisplayFB;  
	// CObjdetect *  m_pObjdetect = new CObjdetect;  


	/*****************************open capture*************************************/  
	int rs;  
	int index;  
	unsigned char  *pp=(unsigned char *)malloc(width*height*sizeof(char)*4);  
	unsigned char * p;  
	unsigned int len;  

	pp = (unsigned char *)malloc(width * height* 2 * sizeof(char));  
	rs = pVideoDev->openCamera("/dev/video5");  
	if (rs < 0) {  
		printf("Error in opening camera device \n");  
		return rs;  
	}  
	/*****************************open  capture end ********************************/
	//while(1){
		index = pVideoDev->get_frame((void **)&p,&len); 
		if(index){
			printf("get_frame succeed\n");
		}
	//}
	return 0;
}
