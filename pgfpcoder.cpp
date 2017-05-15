/*
 * DFCencoder.cpp
 *
 *  Created on: 19.12.2014
 *      Author: roman
 */

#include <math.h>
#include <stdlib.h>
#include "pgfpcoder.h"
#include "image.h"
#include "logging.h"
#include "hcoder.h"
#include "pmed.h"
#include "pequal.h"
#include "plinear.h"
#include "psim.h"
#include "c3x.h"
using namespace std;

extern void signal_stats(int signal);

int getHighBit(int v)
{
	for (int i=31;i>0;i--)
		if (v & 1<<i)
			return i;
	return 0;
}


void PGFPCoder::remove_edges()
{
	edge_image = new YUVImage(cur_image);
	for (int k=0; k<yuvimage->getPlaneNum(); k++) {
		Image *img = cur_image->getPlane(k);
		Image *edge_img = edge_image->getPlane(k);
		edges = img->find_edges();
		for (int i=0; i<edges.size(); i++) {
			edge e = edges[i];
			int v = img->get(e.x, e.y);
			edge_img->set(e.x, e.y, v);
			img->set(e.x, e.y, EDGE);
		}
	}
}

int PGFPCoder::encode(int trim)
{
	PDistrib::initUniformPD(yuvimage->getPlane(0)->getMaxValue());
	for (int k=0; k<PREDICTOR_NUM; k++) {
		pds[k] = new PDistribWeight *[yuvimage->getPlaneNum()];
		for (int i=0; i<yuvimage->getPlaneNum(); i++)
			pds[k][i] = new PDistribWeight[yuvimage->getWidth(i)*yuvimage->getHeight(i)];
	}

	for (int i=0; i<yuvimage->getPlaneNum(); i++) {
		int ret = encode_plane(i, trim);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int PGFPCoder::encode_plane(int pnum, int trim)
{
	dlog(LOG_NORMAL, "\nencode_plane %d\n", pnum);
	Image *img = yuvimage->getPlane(pnum);
	PMed pMed = PMed(yuvimage, PREDICTOR_DIR_NUM, pnum);
	PEqual pEqual[PREDICTOR_DIR_NUM] = {	PEqual(yuvimage, PREDICTOR_DIR_L, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_LU, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_U, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_RU, pnum)};
	PLinear pLinear[PREDICTOR_DIR_NUM] = {	PLinear(yuvimage, PREDICTOR_DIR_L, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_LU, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_U, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_RU, pnum)};
	PSim pSim = PSim(yuvimage, pnum);
//	hcoder hc;
	for (int y=0; y<img->getHeight(); y++) {
		float bits_row = 0;
		for (int x=0; x<img->getWidth(); x++) {
			dlog(LOG_ALL, "%d e[%d,%d]=%d\n", pnum, y, x, img->get(x, y));
			PDistrib pd = PDistrib::getUniformPD(img->getMaxValue());
			{
				PDistrib pdMed = pMed.predict(x, y);
//				pds[PREDICTOR_MED][pnum][y*img->getWidth()+x].pd = pdMed;
				pd = pd + pdMed;
				dlog(LOG_ALL,"pdm %s\n", pdMed.print().c_str());
			}
//if (0)
			for (int dir=0; dir<PREDICTOR_DIR_NUM; dir++) {
				PDistrib pdEqual = pEqual[dir].predict(x, y);
//				pds[PREDICTOR_EQUAL+dir][pnum][y*img->getWidth()+x].pd = pdEqual;
				pd = pd + pdEqual;
				dlog(LOG_ALL,"pde %s\n", pdEqual.print().c_str());
			}

			if (0)
			for (int dir=0; dir<PREDICTOR_DIR_NUM; dir++) {
				PDistrib pdLinear = pLinear[dir].predict(x, y);
//				pds[PREDICTOR_LINEAR+dir][pnum][y*img->getWidth()+x].pd = pdLinear;
				pd = pd + pdLinear;
				dlog(LOG_ALL,"pdl %s\n", pdLinear.print().c_str());
			}

//			if (0)
			{
				PDistrib pdSim = pSim.predict(x, y);
//				pds[PREDICTOR_MED][pnum][y*img->getWidth()+x].pd = pdMed;
				pd = pd + pdSim;
				dlog(LOG_ALL,"pds %s\n", pdSim.print().c_str());
			}
			img->setPD(x, y, pd);

			/*
			pd.sort();
//			continue;
			pd.normalize();
			dlog(LOG_ALL,"pd %s\n", pd.print().c_str());
			if (hc.setPDistrib(pd, true) < 0) {
				hc.dump(LOG_FATAL);
				dlog(LOG_FATAL, "Encode error(hc.setPDistrib).\n");
//				dlog(LOG_FATAL,"pds %s\n", pdSim.print().c_str());
				dlog(LOG_FATAL,"[%d, %d]: pd %s", pd.print().c_str(), y, x);
				return -1;
			}
//hc.dump();
			bitarr code = hc.encode(img->get(x, y));
			codes->append(code);
			bits_row += code.size();

			dlog(LOG_ALL,"coded[%d,%d] %d_%s|%d|\n", y, x, img->get(x,y), code.print().c_str(),codes->size());
*/
/*
			for (int i=0; i<=PREDICTOR_NUM; i++) {
				PDistrib pd = pds[i][pnum][y*img->getWidth()+x].pd;
				if (pd.get(img->get(x, y)) < pd.getMedProb())
					pds[i][pnum][y*img->getWidth()+x].weight = 1;
				else
					pds[i][pnum][y*img->getWidth()+x].weight = 0;
			}
*/
		}
		dlog(LOG_NORMAL, "r%d ", y);
	}
	encode_plane_PDs(pnum);
	return 0;
}

int PGFPCoder::encode_plane_PDs(int pnum)
{
	Image *img = yuvimage->getPlane(pnum);
	dlog(LOG_NORMAL, "\nencode_plane_PDs %d\n", pnum);
	hcoder hc;
	for (int y=0; y<img->getHeight(); y++) {
		float bits_row = 0;
		for (int x=0; x<img->getWidth(); x++) {
			dlog(LOG_ALL, "%d w[%d,%d]\n", pnum, y, x);
			PDistrib pd = img->getPD(x, y);
			pd.sort();
		//			continue;
			pd.normalize();
			dlog(LOG_ALL,"pd %s\n", pd.print().c_str());
			if (hc.setPDistrib(pd, true) < 0) {
				hc.dump(LOG_FATAL);
				dlog(LOG_FATAL, "Encode error(hc.setPDistrib).\n");
		//				dlog(LOG_FATAL,"pds %s\n", pdSim.print().c_str());
				dlog(LOG_FATAL,"[%d, %d]: pd %s", pd.print().c_str(), y, x);
				return -1;
			}
		//hc.dump();
			bitarr code = hc.encode(img->get(x, y));
			codes->append(code);
			bits_row += code.size();

			dlog(LOG_ALL,"coded[%d,%d] %d_%s|%d|\n", y, x, img->get(x,y), code.print().c_str(),codes->size());
			//		dlog(LOG_NORMAL, "r%d{%.2f} ", y, bits_row/img->getWidth());
		}
		dlog(LOG_NORMAL, "R%d{%.2f} ", y, bits_row/img->getWidth());
	}
}

int PGFPCoder::decode(FILE *fout)
{
	PDistrib::initUniformPD(yuvimage->getPlane(0)->getMaxValue());
	decode_pos = 0;
	for (int i=0; i<yuvimage->getPlaneNum(); i++) {
		int ret = decode_plane(i, fout);
		if (ret < 0)
			return ret;
	}
	return 0;
}

int PGFPCoder::decode_plane(int pnum, FILE *fout)
{
	Image *img = yuvimage->getPlane(pnum);
	PMed pMed = PMed(yuvimage, PREDICTOR_DIR_NUM, pnum);
	PEqual pEqual[PREDICTOR_DIR_NUM] = {	PEqual(yuvimage, PREDICTOR_DIR_L, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_LU, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_U, pnum),
											PEqual(yuvimage, PREDICTOR_DIR_RU, pnum)};
	PLinear pLinear[PREDICTOR_DIR_NUM] = {	PLinear(yuvimage, PREDICTOR_DIR_L, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_LU, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_U, pnum),
											PLinear(yuvimage, PREDICTOR_DIR_RU, pnum)};
	PSim pSim = PSim(yuvimage, pnum);
	hcoder hc;
	for (int y=0; y<img->getHeight(); y++) {
		for (int x=0; x<img->getWidth(); x++) {
			dlog(LOG_ALL, "%d d[%d,%d]\n", pnum, y, x);
			PDistrib pd = PDistrib::getUniformPD(img->getMaxValue());

			{
				PDistrib pdMed = pMed.predict(x, y);
//				pds[PREDICTOR_MED][pnum][y*img->getWidth()+x].pd = pdMed;
				pd = pd + pdMed;
				dlog(LOG_ALL,"pdm %s\n", pdMed.print().c_str());
			}
//3			if (0)
			for (int dir=0; dir<PREDICTOR_DIR_NUM; dir++) {
				PDistrib pdEqual = pEqual[dir].predict(x, y);
//				pds[PREDICTOR_EQUAL+dir][pnum][y*img->getWidth()+x].pd = pdEqual;
				pd = pd + pdEqual;
				dlog(LOG_ALL,"pde %s\n", pdEqual.print().c_str());
			}

//			if (0)
			for (int dir=0; dir<PREDICTOR_DIR_NUM; dir++) {
				PDistrib pdLinear = pLinear[dir].predict(x, y);
//				pds[PREDICTOR_LINEAR+dir][pnum][y*img->getWidth()+x].pd = pdLinear;
				pd = pd + pdLinear;
				dlog(LOG_ALL,"pdl %s\n", pdLinear.print().c_str());
			}

			if (0)
			{
				PDistrib pdSim = pSim.predict(x, y);
//				pds[PREDICTOR_MED][pnum][y*img->getWidth()+x].pd = pdMed;
				pd = pd + pdSim;
				dlog(LOG_ALL,"pds %s\n", pdSim.print().c_str());
			}
			img->setPD(x, y, pd);

			pd.sort();
			pd.normalize();
			dlog(LOG_ALL,"pd %s\n", pd.print().c_str());
			if (hc.setPDistrib(pd, true) < 0) {
				hc.dump(LOG_FATAL);
				dlog(LOG_FATAL, "Decode error(hc.setPDistrib).\n");
//				dlog(LOG_FATAL,"pds %s\n", pdSim.print().c_str());
				dlog(LOG_FATAL,"[%d, %d]: pd %s", y , x, pd.print().c_str());
				return -1;
			}
			long v;
	//dlog(LOG_ERROR,"%s\n",pd.print().c_str());
			int len = hc.decode(codes, decode_pos, &v);
			if (len == 0) {
				dlog(LOG_FATAL, "Decode error(hc.decode).\n");
				hc.dump(LOG_FATAL);
				dlog(LOG_FATAL, "[%d, %d] decoding: ", y, x);
				for (int i=0; i<16;i++)
					dlog(LOG_FATAL, "%d", codes->get(decode_pos+i));
				dlog(LOG_FATAL, "\n");
				return -1;
			}
			dlog(LOG_ALL,"decoded[%d,%d] %d=%x{", y, x, v, v);
			for (int i=0;i<len;i++)
				dlog(LOG_ALL,"%d", codes->get(decode_pos+i));
			dlog(LOG_ALL,"}|%d|\n",decode_pos+len);
//dlog(LOG_ERROR,"%x ", v);
			decode_pos += len;

			img->set(x, y, v);
		}
		dlog(LOG_NORMAL, "r%d ", y);
		yuvimage->writeRow(fout, pnum, y);
	}
	return 0;
}

void PGFPCoder::test_edge()
{
	std::vector<edge> edges;
	cur_image = new YUVImage(yuvimage);
	cur_image->copy(yuvimage);
	Image *img = cur_image->getPlane(PLANE_LUM);
	dlog_block(LOG_NORMAL, img, 0, 0, 8, 8);
	remove_edges();
	dlog_block(LOG_NORMAL, img, 0, 0, 8, 8);
}

PGFPCoder::PGFPCoder()
: yuvimage(0),
  cur_image(0),
  edge_image(0),
  pred_image(0)
//  coef(0)
{
	codes = new bitarr();
}

PGFPCoder::~PGFPCoder()
{
/*	if (coef && yuvimage) {
		delete coef;
	}*/
	if (pred_image)
		delete pred_image;
	if (edge_image)
		delete edge_image;
	if (cur_image)
		delete cur_image;
	if (yuvimage)
		delete yuvimage;
}

