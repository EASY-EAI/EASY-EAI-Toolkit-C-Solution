#ifndef __FACIAL_MOVEMENT_H__
#define __FACIAL_MOVEMENT_H__

#include <stdint.h>
#include "face_landmark98.h"


bool eyesClosing(std::vector<KeyPointType> keyPoints);
bool yawning(std::vector<KeyPointType> keyPoints);


#endif
