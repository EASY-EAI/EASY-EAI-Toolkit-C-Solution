#include "system.h"
#include <opencv2/opencv.hpp>

#include "system_opt.h"
#include "camera.h"
#include "disp.h"
#include "qrdecode.h"

using namespace cv;

typedef struct{
	uint32_t x1;
	uint32_t y1;
	uint32_t x2;
	uint32_t y2;
	char qrString[265];
}Result_t;

Mat algorithm_image;

pthread_mutex_t img_lock;
// 识别线程
void *detect_thread_entry(void *para)
{
	Result_t *pResult = (Result_t *)para;
	
	Mat image;
	Mat image_flip;
	struct qrcode_info info;
	while(1)
	{
        if(algorithm_image.empty()) {
			usleep(5);
            continue;
        }
		
		pthread_mutex_lock(&img_lock);
		image = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);
		
		// 算法分析
		flip(image, image_flip, 1);//左右镜像翻转
		memset(&info, 0, sizeof(struct qrcode_info));
		qr_decode(image_flip, &info);
		
		pResult->x1 = image.cols - info.x2;
		pResult->y1 = info.y1;
		pResult->x2 = image.cols - info.x1;
		pResult->y2 = info.y2;
		
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		printf("  type   :%s\n", info.type);
		printf("  result :%s\n", info.result);
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int ret = 0;
	
	char *pbuf = NULL;
	int skip = 0;
	
	pthread_t mTid;
	Result_t *pResult = NULL;
	
	Mat image;
	// 1.打开摄像头
#define CAMERA_WIDTH	720
#define CAMERA_HEIGHT	1280
#define	IMGRATIO		3
#define	IMAGE_SIZE		(CAMERA_WIDTH*CAMERA_HEIGHT*IMGRATIO)
	ret = rgbcamera_init(CAMERA_WIDTH, CAMERA_HEIGHT, 90);
	if (ret) {
		printf("error: %s, %d\n", __func__, __LINE__);
		goto exit4;
	}
	
	pbuf = NULL;
	pbuf = (char *)malloc(IMAGE_SIZE);
	if (!pbuf) {
		printf("error: %s, %d\n", __func__, __LINE__);
		ret = -1;
		goto exit3;
	}
	
	// 跳过前10帧
	skip = 10;
	while(skip--) {
		ret = rgbcamera_getframe(pbuf);
		if (ret) {
			printf("error: %s, %d\n", __func__, __LINE__);
			goto exit2;
		}
	}
	
	// 2.创建识别线程，以及图像互斥锁
	pthread_mutex_init(&img_lock, NULL);
	pResult = (Result_t *)malloc(sizeof(Result_t));
	if(NULL == pResult){
		goto exit1;
	}
	memset(pResult, 0, sizeof(Result_t));
	if(0 != CreateNormalThread(detect_thread_entry, pResult, &mTid)){
		free(pResult);
	}
	
	// 3.显示初始化
#define SCREEN_WIDTH	720
#define SCREEN_HEIGHT	1280
    ret = disp_init(SCREEN_WIDTH, SCREEN_HEIGHT);
	if (ret) {
		printf("error: %s, %d\n", __func__, __LINE__);
		goto exit1;
	}
	
	// 4.(取流 + 显示)循环
	while(1){
		// 4.1、取流
		pthread_mutex_lock(&img_lock);
		ret = rgbcamera_getframe(pbuf);
		if (ret) {
			printf("error: %s, %d\n", __func__, __LINE__);
			pthread_mutex_unlock(&img_lock);
			continue;
		}
		algorithm_image = Mat(CAMERA_HEIGHT, CAMERA_WIDTH, CV_8UC3, pbuf);
		image = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);

		// 4.2、显示
		// 画框
		rectangle(image, Point(pResult->x1, pResult->y1), Point(pResult->x2, pResult->y2), Scalar(0, 255, 0), 3);
        disp_commit(image.data, IMAGE_SIZE);
		
        usleep(10*1000);
	}

exit1:
	pthread_mutex_destroy(&img_lock);
exit2:
	free(pbuf);
	pbuf = NULL;
exit3:
	rgbcamera_exit();
exit4:
    return ret;
}
