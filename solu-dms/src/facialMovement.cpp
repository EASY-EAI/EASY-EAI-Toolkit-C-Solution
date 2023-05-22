
#include "facialMovement.h"

struct Pointf
{
	float x;
	float y;
};
struct PointS
{
	int x;
	int y;
};

static int sqrt_quick(unsigned long long n)
{
    unsigned int target = 0;
//    unsigned long long before;
    unsigned long long after;

//    before = target;
    for(;;)
    {
        after = target*target;
        if(n <= after){
            return target;
        }
//        before = after;
        
        target++;
    }
}

static int PointToPointDis(PointS p1, PointS p2)
{
    long long dx = p2.x - p1.x;
    long long dy = p2.y - p1.y;
	return sqrt_quick(dx*dx + dy*dy);
}
#if 0
static float PointToLineDis(PointS p, PointS p1, PointS p2)
{
	long long A = p2.y - p1.y;
	long long B = p1.x - p2.x;
	long long C = p2.x * p1.y - p1.x * p2.y;
	float dis = (A*p.x + B*p.y + C) / sqrt_quick(A*A + B*B);
	return dis;
}
#endif

#define EYE_THRESHOLD 15
bool eyesClosing(std::vector<KeyPointType> keyPoints)
{
    int leftEyeHorDis = 0, leftEyeVerDis = 0;// leftEyeSum = 0;
    struct PointS leftEye[9] = {0};
    int rightEyeHorDis = 0, rightEyeVerDis = 0;// rightEyeSum = 0;
    struct PointS rightEye[9] = {0};
    
    if(98 == keyPoints.size()){
        for(int n = 0; n < (int)keyPoints.size(); n++){
            switch(keyPoints[n].id){
                case 61: leftEye[0].x = keyPoints[n].point.x; leftEye[0].y = keyPoints[n].point.y; break;
                case 62: leftEye[1].x = keyPoints[n].point.x; leftEye[1].y = keyPoints[n].point.y; break;
                case 63: leftEye[2].x = keyPoints[n].point.x; leftEye[2].y = keyPoints[n].point.y; break;
                case 65: leftEye[3].x = keyPoints[n].point.x; leftEye[3].y = keyPoints[n].point.y; break;
                case 66: leftEye[4].x = keyPoints[n].point.x; leftEye[4].y = keyPoints[n].point.y; break;
                case 67: leftEye[5].x = keyPoints[n].point.x; leftEye[5].y = keyPoints[n].point.y; break;
                case 60: leftEye[6].x = keyPoints[n].point.x; leftEye[6].y = keyPoints[n].point.y; break;   //眼角
                case 64: leftEye[7].x = keyPoints[n].point.x; leftEye[7].y = keyPoints[n].point.y; break;   //眼角
                case 96: leftEye[8].x = keyPoints[n].point.x; leftEye[8].y = keyPoints[n].point.y; break;   //瞳孔
                
                case 69: rightEye[0].x = keyPoints[n].point.x; rightEye[0].y = keyPoints[n].point.y; break;
                case 70: rightEye[1].x = keyPoints[n].point.x; rightEye[1].y = keyPoints[n].point.y; break;
                case 71: rightEye[2].x = keyPoints[n].point.x; rightEye[2].y = keyPoints[n].point.y; break;
                case 73: rightEye[3].x = keyPoints[n].point.x; rightEye[3].y = keyPoints[n].point.y; break;
                case 74: rightEye[4].x = keyPoints[n].point.x; rightEye[4].y = keyPoints[n].point.y; break;
                case 75: rightEye[5].x = keyPoints[n].point.x; rightEye[5].y = keyPoints[n].point.y; break;
                case 68: rightEye[6].x = keyPoints[n].point.x; rightEye[6].y = keyPoints[n].point.y; break;   //眼角
                case 72: rightEye[7].x = keyPoints[n].point.x; rightEye[7].y = keyPoints[n].point.y; break;   //眼角
                case 97: rightEye[8].x = keyPoints[n].point.x; rightEye[8].y = keyPoints[n].point.y; break;   //瞳孔
            }
        }
        // 左眼
        leftEyeVerDis = PointToPointDis(leftEye[1], leftEye[4]); //眼睑距离
        leftEyeHorDis = PointToPointDis(leftEye[6], leftEye[7]); //眼角水平距离
        // 右眼
        rightEyeVerDis = PointToPointDis(rightEye[1], rightEye[4]); //眼睑距离
        rightEyeHorDis = PointToPointDis(rightEye[6], rightEye[7]); //眼角水平距离

        //printf("[Left]:%d ======== [Right]:%d\n", 100*leftEyeVerDis/leftEyeHorDis, 100*rightEyeVerDis/rightEyeHorDis);
        if(((100*leftEyeVerDis/leftEyeHorDis) <= EYE_THRESHOLD) || ((100*rightEyeVerDis/rightEyeHorDis) <= EYE_THRESHOLD)){
            return true;
        }
    }
    
    return false;
}

#define MOUTH_THRESHOLD 30
bool yawning(std::vector<KeyPointType> keyPoints)
{
    int mouthHorDis = 0, mouthVerDis = 0;
    struct PointS mouth[20] = {0};
    
    if(98 == keyPoints.size()){
        for(int n = 0; n < (int)keyPoints.size(); n++){
            switch(keyPoints[n].id){                
                case 76: mouth[0].x = keyPoints[n].point.x; mouth[0].y = keyPoints[n].point.y; break;
                case 77: mouth[1].x = keyPoints[n].point.x; mouth[1].y = keyPoints[n].point.y; break;
                case 78: mouth[2].x = keyPoints[n].point.x; mouth[2].y = keyPoints[n].point.y; break;
                case 79: mouth[3].x = keyPoints[n].point.x; mouth[3].y = keyPoints[n].point.y; break;
                case 80: mouth[4].x = keyPoints[n].point.x; mouth[4].y = keyPoints[n].point.y; break;
                case 81: mouth[5].x = keyPoints[n].point.x; mouth[5].y = keyPoints[n].point.y; break;
                case 82: mouth[6].x = keyPoints[n].point.x; mouth[6].y = keyPoints[n].point.y; break;
                case 83: mouth[7].x = keyPoints[n].point.x; mouth[7].y = keyPoints[n].point.y; break;
                case 84: mouth[8].x = keyPoints[n].point.x; mouth[8].y = keyPoints[n].point.y; break;
                case 85: mouth[9].x = keyPoints[n].point.x; mouth[9].y = keyPoints[n].point.y; break;
                case 86: mouth[10].x = keyPoints[n].point.x; mouth[10].y = keyPoints[n].point.y; break;
                case 87: mouth[11].x = keyPoints[n].point.x; mouth[11].y = keyPoints[n].point.y; break;
                case 88: mouth[12].x = keyPoints[n].point.x; mouth[12].y = keyPoints[n].point.y; break;
                case 89: mouth[13].x = keyPoints[n].point.x; mouth[13].y = keyPoints[n].point.y; break;
                case 90: mouth[14].x = keyPoints[n].point.x; mouth[14].y = keyPoints[n].point.y; break;
                case 91: mouth[15].x = keyPoints[n].point.x; mouth[15].y = keyPoints[n].point.y; break;
                case 92: mouth[16].x = keyPoints[n].point.x; mouth[16].y = keyPoints[n].point.y; break;
                case 93: mouth[17].x = keyPoints[n].point.x; mouth[17].y = keyPoints[n].point.y; break;
                case 94: mouth[18].x = keyPoints[n].point.x; mouth[18].y = keyPoints[n].point.y; break;
                case 95: mouth[19].x = keyPoints[n].point.x; mouth[19].y = keyPoints[n].point.y; break;
            }
        }
        // 嘴巴
        mouthVerDis = PointToPointDis(mouth[14], mouth[18]); //嘴角之间距离
        mouthHorDis = PointToPointDis(mouth[0], mouth[6]); //嘴角之间距离

        //printf("[Mouth]===== %d\n", 100*mouthVerDis/mouthHorDis);
        if( (100*mouthVerDis/mouthHorDis) >= MOUTH_THRESHOLD ){
            return true;
        }
    }
    
    return false;
}


