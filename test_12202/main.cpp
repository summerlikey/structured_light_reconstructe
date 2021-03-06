#include <iostream>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <time.h>
#include <string>
#include "CameraCalib.h"
#include "PhaseProcess.h"
#include "PointcloudProcess.h"
#include "StereoReconstruct.h"
#include "FileManager.h"

using namespace cv;
using namespace std;

#define PATTERN_TEST

#if 1
int main(int argc, char **argv)
{
	/*******************************Stereo calibration*****************************************/
#if 0
	cout << "\n======================================================" << endl;
	const string Calibimagelistfn = "../input/stereo_calib_images.xml";

	cout << ">>>1 Stereo Calibration" << endl;

	//clock_t start=0, end=0;
	//start = clock();  //开始计时     

	//根据标定图像进行相机内外参的计算
	//StereoCalibration(Calibimagelistfn, storintrinsicsyml, storextrinsicsyml);

	//根据Matlab标定得到的内参数据填充内参文件，进行外参的计算（相机位置校正）
	//StereoCalibrationByParams(storintrinsicsyml, storextrinsicsyml);

	//根据标定角点数据进行相机标定
	StereoCalibrationByCorner( storintrinsicsyml, storextrinsicsyml );

	//end = clock(); //计时结束

	//double elapsed_secs = double(end - start) / CLOCKS_PER_SEC;
	//printf("Done in %.2lf seconds.\n", elapsed_secs);

#endif

#if 0
	cout << "\n======================================================" << endl;
	// images Rectified
	cout << ">>>2 Image Rectify" << endl;
	const string Phaseimageslistfn = "../input/phase_images.xml";
	const string Rectifiedimageslistfn = "../input/Rect_images.xml";

	ImgRectified(storintrinsicsyml, storextrinsicsyml, Phaseimageslistfn, Rectifiedimageslistfn);
#endif

	/*******************************Calculate unwrapped phase*****************************************/
#if 1
	// Calculate unwrapped phase
	
	cout << "\n======================================================" << endl;
	cout << ">>>3 Calculate phase" << endl;

	const char* wrapped_phaseleft_txt = "../result/wrapped_phase_left.txt";
	const char* wrapped_phaseright_txt = "../result/wrapped_phase_right.txt";
	const char* unwrapped_phaseleft_txt = "../result/unwrapped_phase_left.txt";
	const char* unwrapped_phaseright_txt = "../result/unwrapped_phase_right.txt";

#ifndef PATTERN_TEST
	const char* input_images_left = "../input/Rect_images_left.xml";
	const char* input_images_right = "../input/Rect_images_right.xml";
#else
	const char* input_images_left = "../input/Pattern_images_left.xml";
	const char* input_images_right = "../input/Pattern_images_right.xml";
#endif

	//Calculate left phase
	cout << "\n[1] Calculate left phase" << endl;

	Mat wrapped_phase_left = CalWrappedPhase(input_images_left).clone();

	if (wrapped_phaseleft_txt)
	{
		cout << "storing the wrapped_phaseleft_txt" << endl;
		savePhase(wrapped_phaseleft_txt, wrapped_phase_left);
	}

	Mat unwrapped_phase_left(Size(wrapped_phase_left.cols, wrapped_phase_left.rows), CV_32FC1, Scalar(0.0));
	cout << "Phase unwrapping..." << endl;

	UnwrappedPhaseGraycode(wrapped_phase_left, unwrapped_phase_left, input_images_left);

	if (unwrapped_phaseleft_txt)
	{
		cout << "storing the unwrapped_phaseleft_txt" << endl;
		savePhase(unwrapped_phaseleft_txt, unwrapped_phase_left);
	}

#if 0
	const char * test_txt = "../result/test.txt";
	FILE* fp = fopen(test_txt, "wt");
	int rs[] = { 30, 130, 230, 330, 430, 530, 630, 730 };
	for (size_t i = 0; i < 8; i++)
	{
		float *pixel_phase_data = unwrapped_phase_left.ptr<float>(rs[i]);
		for (int x = 0; x < unwrapped_phase_left.cols; x++)
		{
			float point = *pixel_phase_data++;
			fprintf(fp, "%f,", point);
		}
		fprintf(fp, "\n");
	}
#endif

	cout << "storing the unwrapped_phase_image_left" << endl;
	Mat tmp_left = unwrapped_phase_left.clone();
	cv::normalize(unwrapped_phase_left, tmp_left, 0, 255, NORM_MINMAX);
	//cv::medianBlur(tmp_left, tmp_left, 5); //中值滤波
	//GaussianBlur(tmp_left, tmp_left, Size(11, 11), 0.1);
	imwrite(unwrapped_phase_image_left, tmp_left);

	/*************************Calculate right phase***********************************/
	cout << "\n[2] Calculate right phase" << endl;
	Mat wrapped_phase_right = CalWrappedPhase(input_images_right).clone();
	Mat unwrapped_phase_right(Size(wrapped_phase_right.cols, wrapped_phase_right.rows), CV_32FC1, Scalar(0.0));  // warning SIZE(cols,rows)!!!

	if (wrapped_phaseright_txt)
	{
		cout << "storing the wrapped_phaseright_txt" << endl;
		savePhase(wrapped_phaseright_txt, wrapped_phase_right);
	}

	cout << "Phase unwrapping..." << endl;

	UnwrappedPhaseGraycode(wrapped_phase_right, unwrapped_phase_right, input_images_right);

	if (unwrapped_phaseright_txt)
	{
		cout << "storing the unwrapped_phaseright_txt" << endl;
		savePhase(unwrapped_phaseright_txt, unwrapped_phase_right);
	}

	cout << "storing the unwrapped_phase_image_right" << endl;
	Mat tmp_right;
	cv::normalize(unwrapped_phase_right, tmp_right, 0, 255, NORM_MINMAX);
	imwrite(unwrapped_phase_image_right, tmp_right);

	cout << endl << "Calculate phase successful!" << endl;
#endif

	/*****************************Stereo matching and 3D reconstruction************************************/
#if 1
	vector<Point2f> leftfeaturepoints, rightfeaturepoints;
	cout << "\n======================================================" << endl;
	cout << ">>>4 Stereo match and 3D reconstruct" << endl;
	cout << "\n[1] Calculate feature points" << endl;

	//find_featurepionts(unwrapped_phase_left, unwrapped_phase_right, leftfeaturepoints, rightfeaturepoints);
	find_featurepionts_single_match(unwrapped_phase_left, unwrapped_phase_right, leftfeaturepoints, rightfeaturepoints);

	cout << "the number of feature: " << leftfeaturepoints.size() << endl;

	FileStorage fs(storextrinsicsyml, FileStorage::READ);
	if (!fs.isOpened())
	{
		printf("Failed to open file extrinsics_filename.\n");
		return 0;
	}

	Mat P1, P2;
	fs["P1"] >> P1;
	fs["P2"] >> P2;

	//cout << "\n==> P1:\n" << P1 << endl;
	//cout << "\n==> P2:\n" << P2 << endl;
	////////////////////////////////////////////////////////////////////////////////////////
	Mat R_T_1 = (Mat_<double>(3, 4) <<
		-0.003449992052740, 0.908392369471684, 0.418104533149851, 3.688988301573581,
		0.992268580264290, 0.054980888811595, -0.111266196509893, -4.927452164451585,
		-0.124061122738460, 0.414488124016969, -0.901558890408035, 329.276493470459510);

	Mat R_T_2 = (Mat_<double>(3, 4) <<
		-0.005778730523496, 0.970132888506089, 0.242505226567117, 3.780742082249347,
		0.992520961272705, 0.035135856240512, -0.116908567010947, -4.998608845649666,
		-0.121937474583672, 0.240015937481406, -0.963080267707255, 328.926407599367390);

	Mat cameraMatrix_1 = (Mat_<double>(3, 3) <<
							5004.084968538499200,   -0.000186077310987, 796.177176385571330,
							 0,					  5004.288845428079200, 645.098858869668220,
							 0,						 0,					  1);

	Mat cameraMatrix_2 = (Mat_<double>(3, 3) <<
							4991.369386877208900,    0.000227164222608, 786.153820356970750,
							 0,					  4991.811878028854200, 648.483429215111640,
							 0,						 0,					  1);

	Mat PP1 = cameraMatrix_1 * R_T_1;
	Mat PP2 = cameraMatrix_2 * R_T_2;
	////////////////////////////////////////////////////////////////////////////////////////

	Mat points(4, leftfeaturepoints.size(), CV_64F);

	cout << "\n[2] Calculate points3D" << endl;
	cv::triangulatePoints(P1, P2, leftfeaturepoints, rightfeaturepoints, points);
	//cv::triangulatePoints(PP1, PP2, leftfeaturepoints, rightfeaturepoints, points);

    cout << "\n[3] Save points3D" <<endl;
	const char* pnts3d_txt = "../result/points.txt";
    savepnts3D( pnts3d_txt, points);
    savepointcloud(points);

	cout << "Stereo match and 3D reconstruct successful!\n";
#endif    

	/*****************************Surface reconstruction************************************/
	cout << "\n======================================================" << endl;
	cout << ">>>5 Point cloud filte and Surface reconstruction" <<endl;
	filterpointcloud();
	//poissonreconstruction();
    
	cout << "\n======================================================" << endl;
	cout << endl << ">>>";
	cout << "All Done" <<endl;
    
    return 0;
}
#endif

/*
typedef pair<string, int> PAIR;

struct CmpByValue {
	bool operator()(const PAIR& lhs, const PAIR& rhs) {
		return lhs.second < rhs.second;
	}
};

int main() {
	map<string, int> name_score_map;
	name_score_map["LiMin"] = 90;
	name_score_map["ZiLinMi"] = 79;
	name_score_map["BoB"] = 92;
	name_score_map.insert(make_pair("Bing", 99));
	name_score_map.insert(make_pair("Albert", 86));
	//把map中元素转存到vector中   
	vector<PAIR> name_score_vec(name_score_map.begin(), name_score_map.end());
	sort(name_score_vec.begin(), name_score_vec.end(), CmpByValue());
	for (int i = 0; i != name_score_vec.size(); ++i) {
		cout << name_score_vec[i].first << endl;
	}
	return 0;
}
*/

/*
typedef pair<string, int> PAIR;

ostream& operator<<(ostream& out, const PAIR& p) {
	return out << p.first << "\t" << p.second;
}

int main() {
	map<string, int> name_score_map;
	name_score_map["LiMin"] = 90;
	name_score_map["ZiLin"] = 79;
	name_score_map["BoB"] = 92;
	name_score_map.insert(make_pair("Bing", 99));
	name_score_map.insert(make_pair("Albert", 86));

	for (auto& x: name_score_map) {
		std::cout << x.first << ":\t" << x.second << '\n';
	}

#if 0
	for (map<string, int>::iterator iter = name_score_map.begin(); iter != name_score_map.end(); ++iter)
	{
		cout << *iter << endl;
	}
#endif
	return 0;
}
*/


