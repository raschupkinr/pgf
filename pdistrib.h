#ifndef __PDISTRIB__
#define __PDISTRIB__

#include <vector>
#include <map>
#include "logging.h"

using namespace std;

class PDistrib {
public:
	PDistrib();
	PDistrib(map<long, double> _pdistrib);
	void normalize();
	void sort(bool inverse = false);
	void reset();
	void set(long v, double p);
	double get(long v);
	int getSize() const;
	const pair<long, double> &operator[](std::size_t i) const;
	const map<long, double> *getPDistrib() const	{	return &pdistrib;	}
	const PDistrib operator+(const PDistrib &other) const;	// must be normalized
	float getMedProb() const;
	void addSpikeUniform(long V, unsigned long radius, double P);
	void addSpikeLinear(long V, unsigned long radius, double P);
	void addSpikeEllipse(long V, unsigned long radius, double P);

	static PDistrib getNullPD()	{	return NullPD;	}
	static PDistrib getUniformPD(int MaxVal);

	string print() const;
	static void initUniformPD(int MaxVal) {
		UMaxVal = MaxVal;
		for (int i=0; i<=MaxVal; i++)
			UniformPD.set(i, 1.0/MaxVal);
		UniformPD.sort();
		UniformPD.normalize();
	}
private:
	map<long, double> pdistrib;
	vector<pair<long, double> > psorted;
	static pair<long, double> nullp;
	static PDistrib NullPD;
	static PDistrib UniformPD;
	static int UMaxVal;
};

typedef struct _PDistribWeight {
	PDistrib pd;
	float weight;
} PDistribWeight;
#endif
