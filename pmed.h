#include "predictor.h"

class PMed : public Predictor
{
public:
	PMed(YUVImage *_img, int _dir, int _plane = -1): Predictor(_img, _dir, _plane)	{}
	PDistrib predict(int x, int y);
private:
};
