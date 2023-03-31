#include "system.h"

#include "system_opt.h"
#include "font_engine.h"
#include "camera.h"
#include "disp.h"
#include "touchscreen.h"

#include "self_learning.h"

#if (CV_VERSION_MAJOR >= 4)
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/highgui/highgui_c.h>
#endif

using namespace cv;

Mat algorithm_image;
pthread_mutex_t img_lock;
pthread_mutex_t shibie_lock;

const int k_value = 3;
struct self_learning_data train_data[50];
int train_data_count = 0;
int train_data_cnt = 0;
int train_id_cnt = 0;
rknn_context classify_ctx;

int train_flag = 0;
static int mode_id = 1;//模型编号


int dataBase_opt_handle(uint32_t eType, int x , int y )
{
    uint32_t m_event = eType;
	
	cv::Mat image_2;
    if(0 != (m_event & TS_PRESS)){
    }else if (0 != (m_event & TS_RELEASE)) {
    }else if (0 != (m_event & TS_CLICK)) {
    }else if (0 != (m_event & TS_DOUBLECLICK)) {
        printf("****双击屏幕(x:%d, y:%d)****\n",x ,y);  
		//printf("按下,训练模型\n");
		if(mode_id <=3)//训练3个模型
		{
			if(algorithm_image.empty()) {
			usleep(5);
			}
			pthread_mutex_lock(&img_lock);
			image_2 = algorithm_image.clone();
			pthread_mutex_unlock(&img_lock);
			printf("模型：%d  训练次数：%d\n",mode_id ,(train_data_count % 5)+1);
			train_data_cnt = (train_data_count % 5)+1;
			train_id_cnt = mode_id;
			self_learning_train(classify_ctx, image_2, train_data+train_data_count, mode_id);
			train_data_count++;
			if(train_data_count % 5 == 0)
			{
				mode_id++;
			}
			usleep(1000);
		}else
		{
			printf("模型训练成功，开始测试模型:%d\n",train_flag);
			train_flag = 1;//训练成功标志
		}
    }else if (0 != (m_event & TS_LONGPRESS)) {
    }else if (0 != (m_event & TS_DRAG)) {
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
	int * ret = (int *)para;
	int result = 0;
	while(1)
	{
		if(train_flag == 1)
		{
			if(algorithm_image.empty()) {
					usleep(5);
					continue;
			}
			pthread_mutex_lock(&img_lock);
			Mat	image = algorithm_image.clone();
			pthread_mutex_unlock(&img_lock);

			*ret = self_learning_inference(classify_ctx, image, train_data, train_data_count, k_value);
		}
		usleep(200*1000);
	}
	return NULL;
}


void  drawDashRect(CvArr* img,int linelength,int dashlength, CvRect* blob, CvScalar color,int thickness)
{
	int w=cvRound(blob->width);//width
	int h=cvRound(blob->height);//height
	int tl_x=cvRound(blob->x);//top left x
	int tl_y=cvRound(blob->y);//top  left y
    int totallength=dashlength+linelength;
	int nCountX=w/totallength;//
	int nCountY=h/totallength;//
	CvPoint start,end;//start and end point of each dash

	//draw the horizontal lines
	start.y=tl_y;
	start.x=tl_x;
	end.x=tl_x;
	end.y=tl_y;
	for (int i=0;i<nCountX;i++)
	{
		end.x=tl_x+(i+1)*totallength-dashlength;//draw top dash line
		end.y=tl_y;
		start.x=tl_x+i*totallength;
		start.y=tl_y;
		cvLine(img,start,end,color,thickness);   
	}
	for (int i=0;i<nCountX;i++)
	{  
		start.x=tl_x+i*totallength;
		start.y=tl_y+h;
		end.x=tl_x+(i+1)*totallength-dashlength;//draw bottom dash line
		end.y=tl_y+h;
		cvLine(img,start,end,color,thickness);     
	}
 
 
	for (int i=0;i<nCountY;i++)
	{  
		start.x=tl_x;
		start.y=tl_y+i*totallength;
		end.y=tl_y+(i+1)*totallength-dashlength;//draw left dash line
		end.x=tl_x;
		cvLine(img,start,end,color,thickness);     
	}
 
 
	for (int i=0;i<nCountY;i++)
	{  
		start.x=tl_x+w;
		start.y=tl_y+i*totallength;
		end.y=tl_y+(i+1)*totallength-dashlength;//draw right dash line
		end.x=tl_x+w;
		cvLine(img,start,end,color,thickness);     
	}
}







int main(int argc, char **argv)
{
	int ret = 0;
	char *pbuf = NULL;
	int skip = 0;
	pthread_t mTid;
	int result = 0;
	// 初始化字体透明度和颜色
	FontColor color = {200, 115, 43, 245};    // {A, R, G, B};
	
	// 创建全局字体
	global_font_create("./simhei.ttf", CODE_UTF8);
	global_font_set_fontSize(40);
// 自学习模型初始化
	self_learning_init(&classify_ctx, "./classify.model");

	memset(train_data, 0, 50*sizeof(struct self_learning_data));

    Init_TsEven(NULL,0);//使用环境变量，非阻塞
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

	
	CreateNormalThread(detect_thread_entry, &result, &mTid);

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
			// 写入文字
		char label_text[50];
		char label_text2[50];
		if(1 == train_flag){/*识别模式*/
			memset(label_text, 0 , sizeof(label_text));
			memset(label_text2, 0 , sizeof(label_text2));
			sprintf(label_text, "训练结束，开始识别"); 
			sprintf(label_text2, "模型ID：%d",result); 
		}else{/*训练模式*/
			memset(label_text, 0 , sizeof(label_text));
			memset(label_text2, 0 , sizeof(label_text2));
			sprintf(label_text, "模型训练中，双击屏幕开始训练"); 
			sprintf(label_text2, "模型ID：%d ， 训练次数：%d",train_id_cnt ,train_data_cnt); 
		}
		putText(image_1.data, image_1.cols, image_1.rows, label_text, 30, 30, color);
		putText(image_1.data, image_1.cols, image_1.rows, label_text2, 30, 1000, color);
#if (CV_VERSION_MAJOR >= 4)
		IplImage tmp = cvIplImage(image_1);
		CvArr* arr = (CvArr*)&tmp;
		CvRect rect1 = cvRect(20,300,680,680);
		drawDashRect(arr,1,2,&rect1,cvScalar(253,255,85),2);
#else
		IplImage tmp = IplImage(image_1);
		CvArr* arr = (CvArr*)&tmp;
		CvRect rect1 = cvRect(20,300,680,680);
		drawDashRect(arr,1,2,&rect1,CV_RGB(253,255,85),2);
#endif

        disp_commit(image_1.data, IMAGE_SIZE);
        usleep(20*1000);
	}

exit1:
	pthread_mutex_destroy(&img_lock);
exit2:
	free(pbuf);
	pbuf = NULL;
exit3:
	global_font_destory();
	rgbcamera_exit();
exit4:
    return ret;
}
