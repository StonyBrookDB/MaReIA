#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <openslide.h>
//#include "openslide-features.h"
///#include <stdint.h>

#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>

#define MAXTHREADNUM 5
#define MAXIMAGENUM 500

//gcc -I/usr/openslide -L/usr/local/lib -lopenslide -o tileimage tileImage.c

static char tiledir[256] = "/home/db2inst1/tiles";
static char imagedir[256] = "/home/db2inst1/images";

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t image_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t thread_mtx = PTHREAD_MUTEX_INITIALIZER;
static char images[MAXIMAGENUM][100];
static int threadNum = 0;
static int imagenum = 0;

void *thread_func(void *arg)
{
	while(1)
	{
		char imagePath[256];
		char outDirPath[256];
		char imageName[256];
		int curimage = 0;
		pthread_mutex_lock(&image_mtx);
		if(imagenum<0)
		{
			pthread_mutex_unlock(&image_mtx);
			break;
		}
		else 
		{
			curimage = imagenum--;
			pthread_mutex_unlock(&image_mtx);
			strcpy(imageName,images[curimage]);
			sprintf(imagePath,"%s/%s",imagedir,imageName);
			sprintf(outDirPath,"%s/%s",tiledir,imageName);
		} 

		//printf("%s,%s\n",imagePath,outDirPath);

		FILE *tilefp;	// ppm file pointer

		long lSize = 0;
		long len;

		int64_t w = 4096, h=4096;
		int64_t width, height;

		if ((fopen (imagePath, "rb")) == NULL)
		{
			return 0;
		}


		if(openslide_can_open(imagePath) == 0){
			return 0;
		}
		openslide_t *osr = openslide_open(imagePath);
		if(osr == NULL) {
			return 0;
		}
		openslide_get_layer_dimensions(osr, 0, &width, &height);

		int xdimension = 0;
		int ydimension = 0;
		xdimension = width/w;
		ydimension = height/h;
		if(width%w!=0)xdimension++;
		if(height%h!=0)ydimension++;
		printf("%d %d %d %d %d %d\n",width,height,w,h,xdimension,ydimension);
		int i,j;
		int64_t SX, SY, RW, RH;
		mkdir(outDirPath,0777);
		for(i=0;i<xdimension;i++)
			for(j=0;j<ydimension;j++)
			{
				SX=i*w;
				SY=j*h;
				char tilename[256];
				sprintf(tilename,"%s/%s_%d_%d.pnm",outDirPath,imageName,SX,SY);

				SX = SX-316;
				SY = SY-316;
				if (SX<0)
				{
					RW = w+316;
					SX = 0;
				}  
				else 
					RW = w+2*316;
				if(SY<0)
				{
					RH = h+316;
					SY = 0;
				}
				else
				{
					RH = h+2*316;
				}

				if(RW+SX>width)
					RW = width-SX;
				if(RH+SY>height)
					RH = height - SY;

				uint32_t *buf = malloc(RW * RH * 4);
				openslide_read_region(osr, buf, SX, SY, 0, RW, RH);

				tilefp = fopen(tilename, "w+");

				if (tilefp == NULL){
					free(buf);
					return 0;
				}

				fseek(tilefp, 0, SEEK_SET);
				fprintf(tilefp, "P6\n%" PRId64 " %" PRId64 "\n255\n", RW, RH);
				int64_t o;
				for (o = 0; o< RW * RH; o++) {
					uint32_t val = buf[o];
					putc((val >> 16) & 0xFF, tilefp);
					putc((val >> 8) & 0xFF, tilefp);
					putc((val >> 0) & 0xFF, tilefp);
				}
				fclose(tilefp);

			}
	}
	pthread_mutex_lock(&thread_mtx);
	threadNum--;
	pthread_mutex_unlock(&thread_mtx);
	printf("finished job!\n");
	return NULL;
}


int main(int argc, char **args) {

	printf("got here 2");
	//PTHREAD_SCOPE_SYSTEM
	pthread_attr_t attr = {0};
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
	int i=0;
	threadNum = 3;
	if(argc>1)
	{
		strcpy(imagedir,args[1]);
		strcpy(tiledir,args[2]);
		threadNum = atoi(args[3]);
	}
	pthread_t tid[threadNum];
	struct dirent * ent = NULL;
	DIR *dir = opendir(imagedir);

	printf("got here");

	while((ent = readdir(dir))!=NULL)
	{
		if(ent->d_type==8)
		{
			strcpy(images[imagenum++],ent->d_name);
			printf("%s  %d\n",ent->d_name,ent->d_type);
		}

	}
	imagenum--;
	for(i=0;i<threadNum;i++)
	{
		pthread_create(&tid[i],&attr,thread_func,NULL);
	}
	while(1)
	{
		sleep(1);
		pthread_mutex_lock(&thread_mtx);
		if(threadNum==0)break;
		pthread_mutex_unlock(&thread_mtx);
	}
	return EXIT_SUCCESS;
}

