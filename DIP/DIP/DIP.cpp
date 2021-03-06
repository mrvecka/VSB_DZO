// DIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define M_PI 3.14159265358979323846
using namespace cv;

#pragma region GammaCorrection

void GammaCorrection(Mat &image, float gamma) {
	Mat new_image = Mat::zeros(image.size(), image.type());

	printf("Processing...\n");
	for (int y = 0; y < image.rows; y++) {
		for (int x = 0; x < image.cols; x++) {
			for (int c = 0; c < 3; c++) {
				new_image.at<Vec3b>(y, x)[c] = saturate_cast<uchar>(gamma * (image.at<Vec3b>(y, x)[c]));
			}
		}
	}
	printf("Image processed!");

	imshow("Initial", image);
	imshow("Gamma-corrected", new_image);
}

#pragma endregion

#pragma region Convolution

enum CONV_MASK
{
	Box_Blur3x3 = 0,
	Gaussian_Blur3x3=1,
	Gausian_Blur5x5 = 2,
	CannyEdgeMask1 = 3,
	CannyEdgeMask2 = 4
};

Mat Convolution(Mat &image, Mat &mask, const float divided) {
	Mat new_image;
	image.copyTo(new_image);

	float sum = 0;
	int offset = (int)(mask.size().width / 2);

	printf("Processing...\n");
	for (int y = offset; y < image.rows - offset; y++) {
		for (int x = offset; x < image.cols - offset; x++) {
			for (int matrixPixelY = 0; matrixPixelY < mask.rows; matrixPixelY++) {
				for (int matrixPixelX = 0; matrixPixelX < mask.cols; matrixPixelX++) {
					float pixel = image.at<float>(y - offset + matrixPixelY, x - offset + matrixPixelX);
					int m = mask.at<int>(matrixPixelY, matrixPixelX);
					sum += pixel * m;
				}
			}
			new_image.at<float>(y, x) = sum / divided;
			sum = 0;
		}
	}

	printf("Image processed!");
	imshow("Blurred", new_image);
	imshow("Initial", image);
	return new_image;
}


Mat GetConvolutionMask(int type)
{
	Mat mask;
	switch (type)
	{
	case Box_Blur3x3:
		mask = (cv::Mat_<int>(3, 3) << 1, 1, 1, 1, 1, 1, 1, 1, 1);
		break;	
	case Gaussian_Blur3x3:
		mask = (cv::Mat_<int>(3, 3) << 1, 2, 1, 2, 4, 2, 1, 2, 1);
		break;
	case Gausian_Blur5x5:
		mask = (cv::Mat_<int>(5, 5) << 1, 4, 6, 4, 1, 4, 16, 24, 16, 4, 6, 24, 36, 24, 6, 4, 16, 24, 16, 4, 1, 4, 6, 4, 1);
		break;
	case CannyEdgeMask1:
		mask = (cv::Mat_<int>(3, 3) << 1, 2, 1, 0, 0, 0, -1, -2, -1);
		break;
	case CannyEdgeMask2:
		mask = (cv::Mat_<int>(3, 3) << 1, 0, -1, 2, 0, -2, 1, 0, -1);
		break;
	default:
		mask = (cv::Mat_<int>(3, 3) << 1, 1, 1, 1, 1, 1, 1, 1, 1);
		break;
	}

	return mask;

}

#pragma endregion

#pragma region AnisotropicFilter

double computeG(double I, double sigma) {
	return exp(-((I*I) / (sigma*sigma)));
}

void AnisotropicFilter(Mat &image) {
	cv::Mat result_64fc1;
	cv::Mat result_64fc1_t;

	image.convertTo(result_64fc1, CV_64FC1, 1.0 / 255.0);
	result_64fc1.copyTo(result_64fc1_t);
	cv::imshow("Initial", image);

	double lambda = 0.1;
	double sigma = 0.015;

	printf("Processing...\nLoop steps:");
	for (int i = 0; i < 100; i++) {
		for (int y = 1; y < result_64fc1.rows - 1; y++) {
			for (int x = 1; x < result_64fc1.cols - 1; x++) {
				double I = result_64fc1.at<double>(y, x);

				double iN = result_64fc1.at<double>(y - 1, x);
				double iS = result_64fc1.at<double>(y + 1, x);
				double iE = result_64fc1.at<double>(y, x + 1);
				double iW = result_64fc1.at<double>(y, x - 1);

				double dN = iN - I;
				double dS = iS - I;
				double dE = iE - I;
				double dW = iW - I;

				double cN = computeG(dN, sigma);
				double cS = computeG(dS, sigma);
				double cE = computeG(dE, sigma);
				double cW = computeG(dW, sigma);

				double It = I + lambda * (cN * dN + cS * dS + cE * dE + cW * dW);

				result_64fc1_t.at<double>(y, x) = It;
			}
		}
		printf("\n%d", i);
		result_64fc1 = result_64fc1_t.clone();
		cv::imshow("Anisotropic-filtered", result_64fc1_t);
		cv::waitKey(1);
	}
	printf("\nAnisotropic filter finished!");
}

#pragma endregion

#pragma region Furier transformation and DFT fiters


void phase(Mat &image, Mat &dest) {
	printf("\nPhase func");

	dest = Mat(image.rows, image.cols, CV_64FC1);

	for (int y = 0; y < image.rows; y++) {
		for (int x = 0; x < image.cols; x++) {
			Vec2d pixel = image.at<Vec2d>(y, x);
			dest.at<double>(y, x) = atan(pixel[1] / pixel[0]);
		}
	}
}

void power(Mat &image, Mat &dest) {
	printf("\nPower func");

	dest = Mat(image.rows, image.cols, CV_64FC1);

	for (int y = 0; y < image.rows; y++) {
		for (int x = 0; x < image.cols; x++) {
			Vec2d F = image.at<Vec2d>(y, x);
			double amp = sqrt((F[0] * F[0]) + (F[1] * F[1]));
			dest.at<double>(y, x) = log((amp * amp));
		}
	}
}

void generateSin(Mat &dest, double shift) {
	printf("\nGenerating Sin");
	for (int y = 0; y < dest.rows; y++) {
		for (int x = 0; x < dest.cols; x++) {
			double sinX = sin(x * shift);
			dest.at<double>(y, x) = sinX > 0 ? sinX : 0;
		}
	}
}

void dft(Mat &image, Mat &furier) {
	printf("\nDoing Fourier transform");
	Mat normalized;

	// Helper vars
	int M = image.cols;
	int N = image.cols;
	double sqrtMN_d = 1.0 / sqrt(M * N);
	double PIPI = M_PI * 2;
	double sumR = 0, sumI = 0;
	double M_d = 1.0 / M, N_d = 1.0 / N;

	// Convert and normalize
	image.convertTo(normalized, CV_64FC1, 1.0 / 255.0 * sqrtMN_d);
	furier = Mat(M, N, CV_64FC2);

	for (int k = 0; k < M; k++) {
		for (int l = 0; l < N; l++) {
			sumR = sumI = 0;

			for (int m = 0; m < M; m++) {
				for (int n = 0; n < N; n++) {
					double px = normalized.at<double>(m, n);
					double expX = -PIPI * (m * k * M_d + n * l * N_d);

					sumR += px * cos(expX);
					sumI += px * sin(expX);
				}
			}

			furier.at<Vec2d>(k, l) = Vec2d(sumR, sumI);
		}
	}
}

void idft(Mat &furier, Mat &dest) {
	// Helper vars
	int M = furier.cols;
	int N = furier.cols;
	double sqrtMN_d = 1.0 / sqrt(M * N);
	double PIPI = M_PI * 2;
	double sum = 0;
	double M_d = 1.0 / M, N_d = 1.0 / N;

	// Create dest matrix
	dest = Mat(M, N, CV_64FC1);

	for (int m = 0; m < M; m++) {
		for (int n = 0; n < N; n++) {
			sum = 0;

			for (int k = 0; k < M; k++) {
				for (int l = 0; l < N; l++) {
					double expX = PIPI * ((k * m * M_d) + (l * n * N_d));

					// Compute real and imaginary parts
					double realX = cos(expX) * sqrtMN_d;
					double imagX = sin(expX) * sqrtMN_d;

					// Get furier transform
					Vec2d F = furier.at<Vec2d>(k, l);

					// Sum
					sum += F[0] * realX - F[1] * imagX;
				}
			}

			dest.at<double>(m, n) = sum;
		}
	}

	cv::normalize(dest, dest, 0.0, 1.0, CV_MINMAX);
}

Mat circleMask(int rows, int cols, int radius, float foreground, float background) {
	Mat result = Mat::ones(rows, cols, CV_32FC1) * background;
	circle(result, Point(rows / 2, cols / 2), radius, foreground, -1);

	return result;
}

template<typename T>
void flipQuadrants(cv::Mat &image) {
	int rows = image.rows / 2;
	int cols = image.cols / 2;

	float leftTop = 0;
	float leftBottom = 0;
	float rightTop = 0;
	float rightBottom = 0;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			T leftTop = image.at<T>(r, c);
			T leftBottom = image.at<T>(r + rows, c);
			T rightTop = image.at<T>(r, c + cols);
			T rightBottom = image.at<T>(r + rows, c + cols);

			image.at<T>(r, c) = rightBottom;
			image.at<T>(r + rows, c) = rightTop;
			image.at<T>(r, c + cols) = leftBottom;
			image.at<T>(r + rows, c + cols) = leftTop;
		}
	}
}


template<typename T>
void dftFilter(Mat &furier, Mat &mask) {
	// Flip quadrants for better computation
	flipQuadrants<Vec2d>(furier);

	for (int y = 0; y < furier.rows; y++) {
		for (int x = 0; x < furier.cols; x++) {
			Vec2d &fPx = furier.at<Vec2d>(y, x);
			T mPx = mask.at<T>(y, x);

			// Reset furier frequencies
			if (mPx == 0) {
				fPx[0] = 0;
				fPx[1] = 0;
			}
		}
	}

	// Flip quadrants back to original values
	flipQuadrants<Vec2d>(furier);
}

void frequencyFilter(Mat &source, Mat &mask, Mat &dest) {
	Mat furier;

	// Calculate DFD
	dft(source, furier);

	// Apply filter
	dftFilter<uchar>(furier, mask);

	// Convert image back to spacial domain using IDFT
	idft(furier, dest);
}

void FourierTransformation(Mat &src_8uc1_lena) {
	Mat furier_64fc2, power_64fc1, phase_64fc1;
	Mat src_64fc1_sin, furier_64fc2_sin, power_64fc1_sin, phase_64fc1_sin;

	// Get img size and generate sin
	int M = src_8uc1_lena.rows;
	int N = src_8uc1_lena.cols;
	src_64fc1_sin = cv::Mat(M, N, CV_64FC1);

	printf("x");
	generateSin(src_64fc1_sin, 0.5f);

	// Generate furier
	dft(src_64fc1_sin, furier_64fc2_sin);
	dft(src_8uc1_lena, furier_64fc2);

	printf("0");
	// Generate Phase filter
	phase(furier_64fc2, phase_64fc1);
	phase(furier_64fc2_sin, phase_64fc1_sin);

	printf("1");
	// Generate Power Filter
	power(furier_64fc2, power_64fc1);
	power(furier_64fc2_sin, power_64fc1_sin);

	printf("2");
	// Show results
	cv::imshow("Furier", src_8uc1_lena);
	cv::imshow("Furier - Power", power_64fc1);
	cv::imshow("Furier - Phase", phase_64fc1);

	// Show results
	cv::imshow("Sin", src_64fc1_sin);
	cv::imshow("Sin - Power", power_64fc1_sin);
	cv::imshow("Sin - Phase", phase_64fc1_sin);

	cv::waitKey(0);
}

void DftFilters() {
	Mat src_8uc1_lena_noise, lena_noise_64fc1_cleared;
	Mat src_8uc1_lena_noise_2, lena_noise_2_64fc1_cleared;
	Mat src_8uc1_lena_bars, lena_bars_64fc1_cleared;
	Mat filter_bars, filter_lowPass, filter_highPass;

	// Load images
	src_8uc1_lena_noise = cv::imread("images/lena64_noise.png", CV_LOAD_IMAGE_GRAYSCALE);
	src_8uc1_lena_noise_2 = cv::imread("images/lena64_noise.png", CV_LOAD_IMAGE_GRAYSCALE);
	src_8uc1_lena_bars = cv::imread("images/lena64_bars.png", CV_LOAD_IMAGE_GRAYSCALE);

	// Get source rows and cols
	int rows = src_8uc1_lena_noise.rows;
	int cols = src_8uc1_lena_noise.cols;

	// Create Filters
	filter_lowPass = cv::Mat::zeros(64, 64, CV_8UC1);
	filter_bars = cv::Mat::ones(64, 64, CV_8UC1) * 255;
	filter_highPass = cv::Mat::ones(64, 64, CV_8UC1) * 255;

	cv::circle(filter_lowPass, cv::Point(rows / 2, cols / 2), 20, 255, -1);
	cv::circle(filter_highPass, cv::Point(rows / 2, cols / 2), 8, 0, -1);

	// Bars filter
//    cv::circle(filter_bars, cv::Point(rows / 2, cols / 2), 28, 255, -1);

	for (int i = 0; i < 64; i++) {
		// We skip center of image
		if (i > 30 && i < 34) continue;
		filter_bars.at<uchar>(32, i) = 0;
	}

	// Apply filtering
	frequencyFilter(src_8uc1_lena_noise, filter_lowPass, lena_noise_64fc1_cleared);
	frequencyFilter(src_8uc1_lena_noise_2, filter_highPass, lena_noise_2_64fc1_cleared);
	frequencyFilter(src_8uc1_lena_bars, filter_bars, lena_bars_64fc1_cleared);

	// Resize so changes are more visible
	cv::resize(lena_noise_64fc1_cleared, lena_noise_64fc1_cleared, cv::Size(), 3, 3);
	cv::resize(lena_noise_2_64fc1_cleared, lena_noise_2_64fc1_cleared, cv::Size(), 3, 3);
	cv::resize(lena_bars_64fc1_cleared, lena_bars_64fc1_cleared, cv::Size(), 3, 3);

	// Resize filters
	cv::resize(filter_lowPass, filter_lowPass, cv::Size(), 3, 3);
	cv::resize(filter_highPass, filter_highPass, cv::Size(), 3, 3);
	cv::resize(filter_bars, filter_bars, cv::Size(), 3, 3);

	// Show results
	cv::imshow("Low Pass", lena_noise_64fc1_cleared);
	cv::imshow("High Pass", lena_noise_2_64fc1_cleared);
	cv::imshow("Bars", lena_bars_64fc1_cleared);

	// Show Filter results
	cv::imshow("Low Pass filter", filter_lowPass);
	cv::imshow("High Pass filter", filter_highPass);
	cv::imshow("Bars filter", filter_bars);

	cv::waitKey(0);
}

#pragma endregion

#pragma region GeometricDistortion

#define SQR(x) ( ( x ) * ( x ) )
#define DEG2RAD(x) ( ( x ) * M_PI / 180.0 )
#define MY_MIN(X, Y) ( ( X ) < ( Y ) ? ( X ) : ( Y ) )
#define MY_MAX(X, Y) ( ( X ) > ( Y ) ? ( X ) : ( Y ) )
int K1 = 1, K2 = 3;
static Mat img, img_geom;

Vec3b bilinearInterpolation(Mat &src, Point2d point) {
	// Get adjacent pixel coordinates
	double x1 = floor(point.x);
	double y1 = floor(point.y);
	double x2 = ceil(point.x);
	double y2 = ceil(point.y);

	// Get adjacent pixel values
	Vec3b f_00 = src.at<Vec3b>(y1, x1);
	Vec3b f_01 = src.at<Vec3b>(y1, x2);
	Vec3b f_10 = src.at<Vec3b>(y2, x1);
	Vec3b f_11 = src.at<Vec3b>(y2, x2);

	// Move x and y into <0,1> square
	double x = point.x - x1;
	double y = point.y - y1;

	// Interpolate
	return (f_00 * (1 - x) * (1 - y)) + (f_01 * x * (1 - y)) + (f_10 * (1 - x) * y) + (f_11 * x * y);
}

void geom_dist(Mat &src, Mat &dst, bool bili, double K1 = 1.0, double K2 = 1.0) {
	// Center of source image and R
	int cu = src.cols / 2;
	int cv = src.rows / 2;
	double R = sqrt(SQR(cu) + SQR(cv));

	for (int y_n = 0; y_n < src.rows; y_n++) {
		for (int x_n = 0; x_n < src.cols; x_n++) {
			double x_ = (x_n - cu) / R;
			double y_ = (y_n - cv) / R;
			double r_2 = SQR(x_) + SQR(y_);
			double theta = 1 + K1 * r_2 + K2 * SQR(r_2);

			double x_d = (x_n - cu) * (1 / theta) + cu;
			double y_d = (y_n - cv) * (1 / theta) + cv;

			// Interpolate
			dst.at<Vec3b>(y_n, x_n) = bili ? bilinearInterpolation(src, Point2d(x_d, y_d)) : src.at<Vec3b>(y_d, x_d);
		}
	}
}

void on_change(int id, void*) {
	geom_dist(img, img_geom, true, K1 / 100.0, K2 / 100.0);
	if (img_geom.data) {
		imshow("Geom Dist", img_geom);
	}
}

void GeometricDistortion() {
	img = imread("images/distorted_window.jpg", CV_LOAD_IMAGE_COLOR);

	if (!img.data) {
		printf("Unable to load image!\n");
		exit(-1);
	}

	std::string windowNameOriginal = "Original Image";
	namedWindow(windowNameOriginal);
	imshow(windowNameOriginal, img);

	img.copyTo(img_geom);
	geom_dist(img, img_geom, true, K1 / 100.0, K2 / 100.0);

	std::string windowNameDist = "Geom Dist";
	namedWindow(windowNameDist);
	imshow(windowNameDist, img_geom);

	createTrackbar("K1", windowNameDist, &K1, 1000, on_change);
	createTrackbar("K2", windowNameDist, &K2, 1000, on_change);

	waitKey(0);
}

#pragma endregion

#pragma region Histogram

#define L 256

int cdfMin(Mat &cdf) {
	int min = 1;

	for (int i = 0; i < cdf.rows; i++) {
		float newMin = cdf.at<float>(i);
		if (newMin < min) min = (int)newMin;
	}

	return min;
}

Mat calcCdf(Mat &hist) {
	// Calculate image integral
	Mat cdfInt;
	integral(hist, cdfInt, CV_32F);

	// Remove first column and row, since integral adds W + 1 and H + 1
	return cdfInt(Rect(1, 1, 1, hist.rows - 1));
}

Mat calcHistogram(Mat &src) {
	Mat hist = Mat::zeros(L, 1, CV_32FC1);

	for (int y = 0; y < src.rows; y++) {
		for (int x = 0; x < src.cols; x++) {
			uchar &srcPx = src.at<uchar>(y, x);
			float &histPx = hist.at<float>(srcPx);
			histPx++;
		}
	}

	return hist;
}

Mat drawHistogram(Mat &hist) {
	Mat histNorm;
	normalize(hist, histNorm, 0.0f, 255.0f, CV_MINMAX);
	Mat dst = Mat::zeros(L, L, CV_8UC1);

	for (int x = 0; x < dst.cols-1; x++) {
		int vertical = saturate_cast<int>(histNorm.at<float>(x));
		for (int y = L - 1; y > (L - vertical - 1); y--) {
			dst.at<uchar>(y, x) = 255;
		}
	}

	return dst;
}

void Histogram() {
	Mat src_8uc1_img, src_32fc1_hist, src_32fc1_cdf;
	src_8uc1_img = imread("images/uneq.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	Mat dst_32fc1_normed = Mat::zeros(src_8uc1_img.rows, src_8uc1_img.cols, CV_32FC1);

	// Get rows and cols
	int rows = src_8uc1_img.rows;
	int cols = src_8uc1_img.cols;
	int WxH = rows * cols;

	// Calc histogram and img integral (cdf)
	src_32fc1_hist = calcHistogram(src_8uc1_img);
	src_32fc1_cdf = calcCdf(src_32fc1_hist);

	// Get cdf min
	float min = cdfMin(src_32fc1_cdf);

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			int srcPx = saturate_cast<int>(src_8uc1_img.at<uchar>(y, x));
			float cdf = src_32fc1_cdf.at<float>(srcPx);

			dst_32fc1_normed.at<float>(y, x) = roundf(((cdf - min) / (WxH - min)) * (L - 1));
		}
	}

	// Normalize result
	normalize(dst_32fc1_normed, dst_32fc1_normed, 0.0f, 1.0f, CV_MINMAX);

	// Show results
	imshow("Source", src_8uc1_img);
	imshow("Result", dst_32fc1_normed);
	imshow("Source Histogram", drawHistogram(src_32fc1_hist));
	imshow("Result Histogram", drawHistogram(src_32fc1_cdf));

	waitKey(0);
}

#pragma endregion

#pragma region Perspective Transformation

void fillMatrix(double in[8][9], Mat &leftT, Mat &rightT) {
	leftT = Mat(8, 8, CV_64F);
	rightT = Mat(8, 1, CV_64F);

	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			leftT.at<double>(y, x) = in[y][x];
		}

		rightT.at<double>(y) = -in[y][8];
	}
}

void fillP3x3(Mat &in, Mat &out) {
	out = Mat(3, 3, CV_64F);

	for (int i = 0; i < 9; i++) {
		if (i == 0) {
			out.at<double>(i) = 1;
			continue;
		}

		out.at<double>(i) = in.at<double>(i - 1);
	}
}

inline Mat fillXYT(Mat &in, int x, int y) {
	if (in.rows == 0) {
		in = Mat(3, 1, CV_64F);
	}

	in.at<double>(0) = x;
	in.at<double>(1) = y;
	in.at<double>(2) = 1;

	return in;
}

void PerspectiveTransformation() {
	Mat src_8uc3_flag = imread("images/flag.png", CV_LOAD_IMAGE_COLOR);
	Mat src_8uc3_vsb = imread("images/vsb.jpg", CV_LOAD_IMAGE_COLOR);
	Mat dst_8uc3_result = src_8uc3_vsb.clone();

	// Points
	double transformData[8][9] = {
			{107, 1, 0,   0,   0, 0,          0,          0,    69},
			{0,   0, 69,  107, 1, 0,          0,          0,    0},
			{76,  1, 0,   0,   0, -323 * 227, -323 * 76,  -323, 227},
			{0,   0, 227, 76,  1, 0,          0,          0,    0},
			{122, 1, 0,   0,   0, -323 * 228, -323 * 122, -323, 228},
			{0,   0, 228, 122, 1, -215 * 228, -215 * 122, -215, 0},
			{134, 1, 0,   0,   0, 0,          0,          0,    66},
			{0,   0, 66,  134, 1, -215 * 66,  -215 * 134, -215, 0},
	};

	// Fill matrix
	Mat leftT, rightT;
	fillMatrix(transformData, leftT, rightT);

	// Solve equation
	Mat vecBuilding, vecFlag;
	Mat resultP, P3x3;
	solve(leftT, rightT, resultP);
	fillP3x3(resultP, P3x3);

	for (int y = 0; y < src_8uc3_vsb.rows; y++) {
		for (int x = 0; x < src_8uc3_vsb.cols; x++) {
			// Get xy1 matrix
			fillXYT(vecBuilding, x, y);

			// Solve P matrix * XY matrix
			vecFlag = P3x3 * vecBuilding;
			vecFlag /= vecFlag.at<double>(2); // divide by "z"

			// Get transformed new x and y
			int px = saturate_cast<int>(vecFlag.at<double>(0));
			int py = saturate_cast<int>(vecFlag.at<double>(1));

			if (px > 0 && py > 0 && px < src_8uc3_flag.cols && py < src_8uc3_flag.rows) {
				src_8uc3_vsb.at<Vec3b>(y, x) = src_8uc3_flag.at<Vec3b>(py, px);
			}
		}
	}

	imshow("Flag", src_8uc3_flag);
	imshow("VSB", src_8uc3_vsb);
	waitKey(0);
}

#pragma endregion

#pragma region Retinex

#define N 360

using namespace cv;

Mat genInput(int rows, int cols) {
	Mat input = Mat::zeros(rows, cols, CV_64FC1);

	// Fill input matrix
	circle(input, Point(cols / 2, rows / 2), 60, 1.0, 1);
	rectangle(input, Rect((cols / 2), (rows / 2), 40, 40), 1.0, -1);

	return input;
}

void genProjections(Mat &input, Mat tpls[]) {
	// Get center point
	Mat inputRotated, inputBackup;
	Point2f center(input.rows / 2.0f, input.cols / 2.0f);

	for (int i = 0; i < N; i++) {
		// Get rotation matrix
		inputBackup = input.clone();
		Mat rotationMat = getRotationMatrix2D(center, (360 / N) * i, 1.0f);
		warpAffine(inputBackup, inputRotated, rotationMat, input.size());
		tpls[i] = Mat::zeros(input.rows, input.cols, CV_64FC1);

		// Create slices
		for (int y = 0; y < inputRotated.rows; y++) {
			for (int x = 0; x < inputRotated.cols; x++) {
				tpls[i].at<double>(y, 0) += inputRotated.at<double>(y, x);
			}
		}

		// Copy slices to other cols
		for (int k = 1; k < tpls[i].cols; k++) {
			tpls[i].col(0).copyTo(tpls[i].col(k));
		}
	};
}

void project(Mat tpls[], Mat &out) {
	// Get center point
	Rect cropRect(100, 100, 400, 400);
	Mat rotated, tmTpl, tmp;
	Point2f center(tpls[0].rows / 2.0f, tpls[0].cols / 2.0f);
	out = Mat::zeros(tpls[0].rows, tpls[0].cols, CV_64FC1);

	for (int i = 0; i < N; i++) {
		// Get rotation matrix
		Mat rotationMat = getRotationMatrix2D(center, -(360 / N) * i, 1.0f);
		warpAffine(tpls[i], rotated, rotationMat, tpls[i].size());

		for (int y = 0; y < out.rows; y++) {
			for (int x = 0; x < out.cols; x++) {
				out.at<double>(y, x) += rotated.at<double>(y, x);
			}
		}

		// Show projections
		normalize(out, tmp, 0.0, 1.0, CV_MINMAX);
		normalize(rotated, tmTpl, 0.0, 1.0, CV_MINMAX);

		// Crop image
		tmp = tmp(cropRect);
		tmTpl = tmTpl(cropRect);

		// Show projections
		imshow("Projected", tmp);
		imshow("Projected Tpl", tmTpl);
		waitKey(1);
	}

	// Normalize output
	out = out(cropRect);
	normalize(out, out, 0.0, 1.0, CV_MINMAX);
}

void Retinex() {
	Mat out;
	Mat input = genInput(600, 600);
	Mat tpls[N];

	// Show input
	imshow("Input", input(Rect(100, 100, 400, 400)));
	//waitKey(0);

	// Generate projections
	genProjections(input, tpls);
	project(tpls, out);

	// Show results
	imshow("Projected", out);

	waitKey(0);
}

#pragma endregion


#pragma region CannyEdgeDetection

Mat SumImages(Mat img1, Mat img2)
{
	Mat res;
	img1.copyTo(res);
	for (int i = 0; i < img1.rows; i++)
	{
		for (int j = 0; j < img1.cols; j++)
		{
			res.at<float>(i, j) = sqrt(pow(img1.at<float>(i, j), 2) + pow(img2.at<float>(i, j), 2));
		}
		
	}

	return res;
}

void CannyEdgeDetection()
{
	Mat lenaColor = imread("images/lena.png", CV_LOAD_IMAGE_GRAYSCALE);
	Mat lenaGray2;
	lenaColor.convertTo(lenaGray2, CV_32FC1, 1.0 / 255.0);

	Mat GyMask = GetConvolutionMask(CannyEdgeMask1);
	Mat GxMask = GetConvolutionMask(CannyEdgeMask2);
	Mat conv1 = Convolution(lenaGray2, GyMask, 9);
	Mat conv2 = Convolution(lenaGray2, GxMask, 9);

	Mat res = SumImages(conv1, conv2);
	imshow("canny detection", res);

}

#pragma endregion



int main()
{
	cv::Mat src_8uc3_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_COLOR); // load color image from file system to Mat variable, this will be loaded using 8 bits (uchar)

	if (src_8uc3_img.empty()) {
		printf("Unable to read input file (%s, %d).", __FILE__, __LINE__);
	}

	//cv::imshow( "LENA", src_8uc3_img );

	cv::Mat gray_8uc1_img; // declare variable to hold grayscale version of img variable, gray levels wil be represented using 8 bits (uchar)
	cv::Mat gray_32fc1_img; // declare variable to hold grayscale version of img variable, gray levels wil be represented using 32 bits (float)

	cv::cvtColor(src_8uc3_img, gray_8uc1_img, CV_BGR2GRAY); // convert input color image to grayscale one, CV_BGR2GRAY specifies direction of conversion
	gray_8uc1_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0); // convert grayscale image from 8 bits to 32 bits, resulting values will be in the interval 0.0 - 1.0

	int x = 10, y = 15; // pixel coordinates

	uchar p1 = gray_8uc1_img.at<uchar>(y, x); // read grayscale value of a pixel, image represented using 8 bits
	float p2 = gray_32fc1_img.at<float>(y, x); // read grayscale value of a pixel, image represented using 32 bits
	cv::Vec3b p3 = src_8uc3_img.at<cv::Vec3b>(y, x); // read color value of a pixel, image represented using 8 bits per color channel

	gray_8uc1_img.at<uchar>(y, x) = 0; // set pixel value to 0 (black)

	// draw a rectangle
	cv::rectangle(gray_8uc1_img, cv::Point(65, 84), cv::Point(75, 94),
		cv::Scalar(50), CV_FILLED);

	// declare variable to hold gradient image with dimensions: width= 256 pixels, height= 50 pixels.
	// Gray levels wil be represented using 8 bits (uchar)
	cv::Mat gradient_8uc1_img(50, 256, CV_8UC1);

	// For every pixel in image, assign a brightness value according to the x coordinate.
	// This wil create a horizontal gradient.
	for (int y = 0; y < gradient_8uc1_img.rows; y++) {
		for (int x = 0; x < gradient_8uc1_img.cols; x++) {
			gradient_8uc1_img.at<uchar>(y, x) = x;
		}
	}
	Mat lenaColor = imread("images/lena.png", CV_LOAD_IMAGE_GRAYSCALE);
	Mat lenaColor1 = imread("images/lena.png", CV_LOAD_IMAGE_COLOR);
	Mat lenaGray;
	Mat lenaGray2;
	//cv::cvtColor(lenaColor, lenaGray, CV_BGR2GRAY);
	lenaColor.convertTo(lenaGray2, CV_32FC1, 1.0 / 255.0);
	Mat lenaGrayscale = imread("images/lena64.png", CV_LOAD_IMAGE_GRAYSCALE);
	Mat valveGrayscale = imread("images/valve.png", CV_LOAD_IMAGE_GRAYSCALE);

	Mat mask = GetConvolutionMask(CannyEdgeMask1);
	//GammaCorrection(lenaColor1 ,1.9);
	//Convolution(lenaGray2, mask, 1);
	//AnisotropicFilter(valveGrayscale);
	//FourierTransformation(lenaGrayscale);
	//GeometricDistortion();
	//DftFilters();
	//Histogram();
	//PerspectiveTransformation();
	//Retinex();

	//CannyEdgeDetection();

	cv::waitKey(0); // wait until keypressed

	return 0;
}
