#include "system.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <sys/time.h>
#include "system_opt.h"
#include "camera.h"
#include "disp.h"
#include "person_detect.h"
#include "self_learning.h"
#include "keyEvent.h"


using namespace cv;

typedef struct{
	person_detect_result_group_t result_group;
	int person_number;
}Result_t;

Mat algorithm_image;
pthread_mutex_t img_lock;
pthread_mutex_t shibie_lock;

const int k_value = 3;
struct self_learning_data train_data[50];
int train_data_count = 0;
rknn_context classify_ctx;

int train_flag = 0;



int dataBase_opt_handle(int eType)
{
	static int i = 1;//模型编号
	cv::Mat image_2;
	switch(eType)
	{
		case KEY_PRESS:
					//printf("按下,训练模型\n");
					if(i<=3)//训练3个模型
					{
						if(algorithm_image.empty()) {
						usleep(5);
						}
						pthread_mutex_lock(&img_lock);
						image_2 = algorithm_image.clone();
						pthread_mutex_unlock(&img_lock);
						printf("模型：%d  训练次数：%d\n",i,(train_data_count % 5)+1);
						self_learning_train(classify_ctx, image_2, train_data+train_data_count, i);
						train_data_count++;
						if(train_data_count % 5 == 0)
						{
							i++;
						}
						usleep(1000);
					}else
					{
						printf("模型训练成功，开始测试模型\n");
						train_flag = 1;//训练成功标志
					}
					
					
			break;
		case KEY_LONGPRESS:
			break;
		case KEY_RELEASE:
			break;
		case KEY_CLICK:
			break;
		case KEY_DOUBLECLICK:
			break;
		default:
			break;
	}
	return 0;
}

static Scalar colorArray[10]={
    Scalar(255, 0, 0, 255),
    Scalar(0, 255, 0, 255),
    Scalar(0,0,139,255),
    Scalar(0,100,0,255),
    Scalar(139,139,0,255),
    Scalar(209,206,0,255),
    Scalar(0,127,255,255),
    Scalar(139,61,72,255),
    Scalar(0,255,0,255),
    Scalar(255,0,0,255),
};

int plot_one_box(Mat src, int x1, int x2, int y1, int y2, char *label, char colour)
{
    int tl = round(0.002 * (src.rows + src.cols) / 2) + 1;
    rectangle(src, cv::Point(x1, y1), cv::Point(x2, y2), colorArray[(unsigned char)colour], 3);

    int tf = max(tl -1, 1);

    int base_line = 0;
    cv::Size t_size = getTextSize(label, FONT_HERSHEY_SIMPLEX, (float)tl/3, tf, &base_line);
    int x3 = x1 + t_size.width;
    int y3 = y1 - t_size.height - 3;

    rectangle(src, cv::Point(x1, y1), cv::Point(x3, y3), colorArray[(unsigned char)colour], -1);
    putText(src, label, cv::Point(x1, y1 - 2), FONT_HERSHEY_SIMPLEX, (float)tl/3, cv::Scalar(255, 255, 255, 255), tf, 8);
    return 0;
}

void *detect_thread_entry(void *para)
{
	if(train_flag == 1)
	{
		while(1)
		{
			if(algorithm_image.empty()) {
					usleep(5);
					continue;
			}
			pthread_mutex_lock(&img_lock);
			Mat	image = algorithm_image.clone();
			pthread_mutex_unlock(&img_lock);

			int result;
			result = self_learning_inference(classify_ctx, image, train_data, train_data_count, k_value);

			char label_text[50];
			memset(label_text, 0 , sizeof(label_text));
			sprintf(label_text, "模型编号 %d",result); 
			plot_one_box(image, 0, 10, 0, 10, label_text, 2);
		}
	}
	return NULL;
}

// void *shibie_thread_entry(void *para)
// {
// 	pthread_mutex_lock(&shibie_lock);
// 	while(1)
// 	{
// 		if(algorithm_image.empty()) {
// 				usleep(5);
// 				continue;
// 		}
// 		pthread_mutex_lock(&img_lock);
// 		Mat	image = algorithm_image.clone();
// 		pthread_mutex_unlock(&img_lock);

// 		int result;
// 		result = self_learning_inference(classify_ctx, image, train_data, train_data_count, k_value);

// 		char label_text[50];
// 		memset(label_text, 0 , sizeof(label_text));
// 		sprintf(label_text, "模型编号 %d",result); 
// 		plot_one_box(image, 0, 10, 0, 10, label_text, 2);
// 	}
// 	pthread_mutex_unlock(&shibie_lock);
// }



int main(int argc, char **argv)
{
	int ret = 0;
	char *pbuf = NULL;
	int skip = 0;
	pthread_t mTid;
	
// 自学习模型初始化
	self_learning_init(&classify_ctx, "./classify.model");

	memset(train_data, 0, 50*sizeof(struct self_learning_data));

	Init_KeyEven();
	set_even_handle(dataBase_opt_handle);

	Mat image_1;
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
	// //回调，传入参数，tid
	CreateNormalThread(detect_thread_entry, NULL, &mTid);
	//3.显示初始化
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
		image_1 = algorithm_image.clone();
		pthread_mutex_unlock(&img_lock);
	
        disp_commit(image_1.data, IMAGE_SIZE);
        usleep(20*1000);
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
