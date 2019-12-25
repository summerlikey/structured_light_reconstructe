#include "StereoReconstruct.h"
#include "FileManager.h"
#include <cfloat>

#define PHASE_THRESHOLD  0.01

// 基于极线矫正后行对齐的图像

// 寻找匹配点的思路：在同一行中，先在右图中寻找和左图中的点P1相位绝对差小于阈值的匹配点P2，再在左图中寻找P2的相位绝对差小于阈值的匹配点P3，
// 若P1和P3的距离够近，则（P1+P3）/2、P2就是一对匹配点对。
void find_featurepionts(Mat& leftphase, Mat& rightphase, vector<Point2f>& leftkeypoint, vector<Point2f>& rightkeypoint)
{
	int nr = leftphase.rows;
	int nc = leftphase.cols;
	int x, y, k1, k2;
	float left, right;
	Point2f fleft, fright;
	float *pre_right_data;
	int pre_k;

	for (y = 0; y < nr; y += 1)
	{
		float *left_phase_data = leftphase.ptr<float>(y);
		float *right_phase_data = rightphase.ptr<float>(y);
		float *left_phase_data2;

		for (x = 0; x < nc; x++)
		{
			left = *left_phase_data++;

			if (left > 2 * CV_PI)
			{
				right_phase_data = rightphase.ptr<float>(y);
				k1 = 0;

				while ((abs(left - *right_phase_data++) > PHASE_THRESHOLD) && (k1 < nc)) { k1++; }
				/*
				float minDis = FLT_MAX;
				int idx = 0;
				while (idx < nc)
				{
					float right = *right_phase_data;
					if (abs(left - right) < minDis)
					{
						minDis = abs(left - right);
						k1 = idx;
					}
					idx++;
					right_phase_data++;
				}
				*/

				if (k1 < nc)
				{
					right = *(--right_phase_data);
					left_phase_data2 = leftphase.ptr<float>(y);
					k2 = 0;
					while ((abs(right - *left_phase_data2++) > PHASE_THRESHOLD) && (k2 < nc))
						k2++;

					if ((k2 < nc) && (abs(k2 - x) < 2))
					{
						fleft.x = (x + k2) / 2;
						fleft.y = y;
						fright.x = k1;
						fright.y = y;
						leftkeypoint.push_back(fleft);
						rightkeypoint.push_back(fright);
					}
				}
			}
		}
	}
}

#if 0
// 基于极线矫正后行对齐的图像
// 寻找匹配点的思路：在同一行中，先在右图中寻找和左图中的点P1相位绝对差小于阈值的匹配点P2，再在左图中寻找P2的相位绝对差小于阈值的匹配点P3，
// 若P1和P3的距离够近，则（P1+P3）/2、P2就是一对匹配点对。
void find_featurepionts_single_match(Mat& leftphase, Mat& rightphase, vector<Point2f>& leftkeypoint, vector<Point2f>& rightkeypoint)
{
	int row = leftphase.rows;
	int col = leftphase.cols;
	int k, k_prev, k_next;
	float left;
	Point2f point_left, point_right;

	for (int i = 0; i < row; i++)
	{
		float *left_phase_data = leftphase.ptr<float>(i);
		float *right_phase_data = rightphase.ptr<float>(i);

		for (int j = 0; j < col; j++)
		{
			left = *left_phase_data++;

			if (left > 2 * CV_PI)
			{
				right_phase_data = rightphase.ptr<float>(i);
				k = 0;
				k_prev = 0;
				k_next = 0;

				while ((abs(left - *right_phase_data++) > PHASE_THRESHOLD) && (k < col)) k++;

				if (k < col)
				{
					point_left.x = j;
					point_left.y = i;
					point_right.x = k;
					point_right.y = i;
					leftkeypoint.push_back(point_left);
					rightkeypoint.push_back(point_right);
				}
			}
		}
	}
}
#endif

// 亚像素级相位匹配
void find_featurepionts_single_match(Mat& leftphase, Mat& rightphase, vector<Point2f>& leftkeypoint, vector<Point2f>& rightkeypoint)
{
	int row = leftphase.rows;
	int col = leftphase.cols;
	int k;
	float left;
	Point2f point_left, point_right;
	int area = 10; //在边长40的区域中寻找匹配点

	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			left = leftphase.at<float>(i, j);

			if (left > 2 * CV_PI)
			{
				k = 0;

				while ((abs(left - rightphase.at<float>(i, k)) > PHASE_THRESHOLD) && (k < col)) k++;

				//在以(i,k)为中心的区域内寻找相位最近的4个点
				std::map<float, Point2i> map_points;
				bool area_process = false;
				if (i > area && i<row - area && k>area && k < col - area)
				{
					area_process = true;
					map_points.insert(std::pair<float, Point2i>( abs(left - rightphase.at<float>(i-area, k-area)), Point2i(i-area, k-area) ) );
					map_points.insert(std::pair<float, Point2i>( abs(left - rightphase.at<float>(i-area, k-area+1)), Point2i(i-area, k-area+1) ) );
					map_points.insert(std::pair<float, Point2i>( abs(left - rightphase.at<float>(i-area, k-area+2)), Point2i(i-area, k-area+2) ) );
					map_points.insert(std::pair<float, Point2i>( abs(left - rightphase.at<float>(i-area, k-area+3)), Point2i(i-area, k-area+3) ) );
					//map_points.insert(make_pair( abs(left-rightphase.at<float>(i-area, k-area)), Point2i(i-area, k-area) ) );
					//map_points[1.2] = Point2i(10, 10);

					for (int m = i - area; m < i + area; m++)
					{
						for (int n = k - area; n < k + area; n++)
						{
							std::map<float, Point2i>::reverse_iterator rit = map_points.rbegin();
							if ( abs(left - rightphase.at<float>(m, n)) < rit->first)
							{
								map_points.erase(rit->first);
								map_points.insert(std::pair<float, Point2i>( abs(left - rightphase.at<float>(m, n)), Point2i(m, n) ) );
							}
						}
					}
				}

				/*
				if (true == area_process)
				{
					cout << "==========================\n";
					cout << "value at kkkkkkkkkkkk====> " << abs(left - rightphase.at<float>(i, k)) << "\t[" << i << "," << k  << "]" << endl;
					for (auto& x: map_points) {
						std::cout << x.first << ":\t" << x.second << '\n';
					}
					cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$\n";
				}
				*/

				vector<Point2i> vec_points;
				vec_points.push_back(Point2i(i,k));
				if (true == area_process)
				{
					std::map<float, Point2i>::iterator it = map_points.begin();
					for (auto & x: map_points) {
						if (x.first <= abs(left - rightphase.at<float>(i, k)))
							vec_points.push_back(x.second);
					}
				}

				int cor_x_sum = 0;
				int cor_y_sum = 0;
				for (auto point : vec_points)
				{
					cor_x_sum += point.y;
					cor_y_sum += point.x;
				}

				float cor_x = (float)cor_x_sum / vec_points.size();
				float cor_y = (float)cor_y_sum / vec_points.size();

				if (k < col)
				{
					point_left.x = j;
					point_left.y = i;
					//point_right.x = k;
					//point_right.y = i;
					point_right.x = cor_x;
					point_right.y = cor_y;
					leftkeypoint.push_back(point_left);
					rightkeypoint.push_back(point_right);
				}
			}
		}
	}
}


