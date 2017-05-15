#ifndef __PREDICTOR__
#define __PREDICTOR__

#include "yuvimage.h"
#include "pdistrib.h"

#define PREDICTOR_DIR_L			0
#define PREDICTOR_DIR_LU		1
#define PREDICTOR_DIR_U			2
#define PREDICTOR_DIR_RU		3
#define PREDICTOR_DIR_NUM		4

#define PREDICTOR_MED			0
#define PREDICTOR_EQUAL			1
#define PREDICTOR_LINEAR		5
#define PREDICTOR_SIM			9
#define PREDICTOR_NUM			10

struct PredParam {
	PredParam();
	int SpikeRadius;
};

class Predictor {
public:
	Predictor(YUVImage *_img, int _dir, int _plane = 0);
	virtual PDistrib predict(int x, int y);
	PredParam getPredParam();
protected:
	int dir;
	YUVImage *yuvimage;
	int plane;
};

#endif
