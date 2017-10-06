/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2017, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

#include "vision-precomp.h"   // Precompiled headers

#include <mrpt/vision/CFeatureLines.h>
#include <mrpt/utils/CTicTac.h>

#if MRPT_HAS_OPENCV

#include <opencv2/line_descriptor.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/ximgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui.hpp>
#include <algorithm>

using namespace mrpt::vision;
using namespace mrpt::utils;
using namespace std;
//using namespace cv;
using namespace cv::line_descriptor;

float segment_length_sq(const cv::Vec4f & s1)
{
    return (s1[0]-s1[2])*(s1[0]-s1[2]) + (s1[1]-s1[3])*(s1[1]-s1[3]);
}

bool larger_segment(const cv::Vec4f & s1, const cv::Vec4f & s2)
{
    return (s1[0]-s1[2])*(s1[0]-s1[2]) + (s1[1]-s1[3])*(s1[1]-s1[3]) > (s2[0]-s2[2])*(s2[0]-s2[2]) + (s2[1]-s2[3])*(s2[1]-s2[3]);
}

void CFeatureLines::extractLines(const cv::Mat & image,
                                std::vector<cv::Vec4f>  & segments,
                                const int method,
                                const size_t th_length, const size_t max_lines,
                                const bool display)
{
    if( method < 0 || method > 4 )
        throw std::runtime_error("MRPT ERROR: CFeatureLines::extractLines called with \'method\'' parameter.\n");

    cout << "CFeatureLines::extractLines... method " << method << " th_length " << th_length << " im " << image.depth() << endl;
    CTicTac clock; clock.Tic(); //Clock to measure the runtime

    if( method >= 0 || method < 4 )
        extractLines_cv2(image, segments, th_length, method, false);
    else if(method == 4) // This if condition is set just for readability
        extractLines_CHB(image, segments, th_length, false);

    std::sort (segments.begin(), segments.end(), larger_segment); // Sort segments by their length. This increases the performance for matching them with those segmented from another image
    if( max_lines < segments.size() )
        segments.resize(max_lines);

    for(auto line = begin(segments); line != end(segments); ++line)
    {
        // Force the segments to have a predefined order (e1y <= e2y)
        if((*line)[0] > (*line)[2])
            *line = cv::Vec4f((*line)[2], (*line)[3], (*line)[0], (*line)[1]);
        else if((*line)[0] == (*line)[2] && (*line)[1] > (*line)[3])
            *line = cv::Vec4f((*line)[2], (*line)[3], (*line)[0], (*line)[1]);
    }

    time = 1000*clock.Tac();
    cout << "  CFeatureLines::extractLines.v" << method << " lines " << segments.size() << " took " << 1000*clock.Tac() << " ms \n";

    // Display 2D segments
    if(display)
    {
        cv::Mat image_lines = image.clone();
        if( image_lines.channels() == 1 )
            cv::cvtColor( image_lines, image_lines, cv::COLOR_GRAY2BGR );
        for(auto line = begin(segments); line != end(segments); ++line)
        {
            /* get a random color */
            int R = ( rand() % (int) ( 255 + 1 ) );
            int G = ( rand() % (int) ( 255 + 1 ) );
            int B = ( rand() % (int) ( 255 + 1 ) );
            //R = 0; G = 0; B = 255;
            cv::line( image_lines, cv::Point(cv::Point2f((*line)[0], (*line)[1])), cv::Point2f(cv::Point((*line)[2], (*line)[3])), cv::Scalar(B,G,R), 3 );
            cv::circle(image_lines, cv::Point(cv::Point2f((*line)[0], (*line)[1])), 3, cv::Scalar(B,G,R), 3);
            cv::putText(image_lines, to_string(distance(begin(segments),line)), cv::Point(cv::Point2f(((*line)[0]+(*line)[2])/2,((*line)[1]+(*line)[3])/2)), 0, 0.8, cv::Scalar(B,G,R), 1 );
        }
        cv::imshow( "lines", image_lines );
        cv::waitKey();
    }
}

void CFeatureLines::extractLines_cv2(const cv::Mat & image,
                                     std::vector<cv::Vec4f> & segments,
                                     const float th_length,
                                     const int method,
                                     const bool display )
{
    //cout << "CFeatureLines::extractLines_cv2... method " << method << " th_length " << th_length << endl;
    //CTicTac clock; clock.Tic(); //Clock to measure the runtime
    if(method == 0 || method == 1)
    {
        /* extract lines */
        vector<KeyLine> lines;                                      /* create a structure to store extracted lines */
        cv::Mat mask = cv::Mat::ones( image.size(), CV_8UC1 );      /* create a random binary mask */
        if(method == 0)
        {
            cv::Ptr<LSDDetector> bd = LSDDetector::createLSDDetector(); /* create a pointer to a BinaryDescriptor object with deafult parameters */
            bd->detect( image, lines, 2, 1, mask );                     /* extract lines */
        }
        else if(method == 1)
        {
            cv::Ptr<BinaryDescriptor> bd = BinaryDescriptor::createBinaryDescriptor();
            bd->detect( image, lines, mask );
        }
        //cout << "  CFeatureLines::extractLines_cv2 " << lines.size() << " took " << 1000*clock.Tac() << " ms \n";

        segments.resize( lines.size() );
        size_t j = 0;
        for( auto line = begin(lines); line != end(lines); ++line ){ //cout << " line " << line->lineLength << " " << line->numOfPixels << " " << th_length << endl;
            if( line->octave == 0 && line->lineLength > th_length )
                segments[j++] = cv::Vec4f(line->startPointX, line->startPointY, line->endPointX, line->endPointY); }
        segments.resize(j);
        //cout << "  CFeatureLines::extractLines_cv2 " << segments.size() << " took " << 1000*clock.Tac() << " ms \n";

        /* draw lines extracted from octave 0 */
        if(display)
        {
            cv::Mat image_lines = image.clone();
            if( image_lines.channels() == 1 )
                cv::cvtColor( image_lines, image_lines, cv::COLOR_GRAY2BGR );
            for ( size_t i = 0; i < lines.size(); i++ )
            {
                KeyLine kl = lines[i];
                if( kl.octave == 0)
                {
                    /* get a random color */
                    int R = ( rand() % (int) ( 255 + 1 ) );
                    int G = ( rand() % (int) ( 255 + 1 ) );
                    int B = ( rand() % (int) ( 255 + 1 ) );
                    /* get extremes of line */
                    cv::Point pt1 = cv::Point2f( kl.startPointX, kl.startPointY );
                    cv::Point pt2 = cv::Point2f( kl.endPointX, kl.endPointY );
                    /* draw line */
                    cv::line( image_lines, pt1, pt2, cv::Scalar( B, G, R ), 3 );
                }
            }
            cv::imshow( "extractLines_cv2", image_lines );
            cv::waitKey();
        }
    }
    else if(method == 2 || method == 3) // Detect the lines with FLD
    {
        segments.clear();
        cv::Mat gray;
        if( image.channels() > 1 )
            cv::cvtColor( image, gray, cv::COLOR_BGR2GRAY );
        else
            gray = image;

        if(method == 2)
        {
            cv::Ptr<cv::LineSegmentDetector> lsd = cv::createLineSegmentDetector();
            lsd->detect(gray, segments);

            if(display)
            {
                cv::Mat image_lines = gray.clone();
                lsd->drawSegments(image_lines, segments);
                cv::imshow("extractLines_cv2", image_lines);
                cv::waitKey();
            }
        }
        else
        {
            float distance_threshold = 1.41421356f;
            double canny_th1 = 50.0;
            double canny_th2 = 50.0;
            int canny_aperture_size = 3;
            bool do_merge = true;
            cv::Ptr<cv::ximgproc::FastLineDetector> fld = cv::ximgproc::createFastLineDetector(th_length, distance_threshold, canny_th1, canny_th2, canny_aperture_size, do_merge);
            fld->detect(gray, segments);

            if(display)
            {
                cv::Mat image_lines = gray.clone();
                fld->drawSegments(image_lines, segments);
                cv::imshow("extractLines_cv2", image_lines);
                cv::waitKey();
            }
        }
    }
}

void CFeatureLines::extractLines_CHB(const cv::Mat & image,
                                    std::vector<cv::Vec4f> & segments,
                                    size_t th_length,
                                    const bool display )
{
//    cout << "CFeatureLines::extractLines... th_length " << th_length << endl;
//    CTicTac clock; clock.Tic(); //Clock to measure the runtime

    // Canny edge detector
    cv::Mat canny_img;
    int lowth_length = 150;
    //int const max_lowth_length = 100;
    int ratio = 3;
    int kernel_size = 3;
    cv::Canny(image, canny_img, lowth_length, lowth_length*ratio, kernel_size); // 250, 600 // CAUTION: Both th_lengths depend on the input image, they might be a bit hard to set because they depend on the strength of the gradients
    //            cv::namedWindow("Canny detector", cv::WINDOW_AUTOSIZE);
    //            cv::imshow("Canny detector", canny_img);
    //            cv::waitKey(0);
    //            cv::destroyWindow("Canny detector");

    // Get lines through the Hough transform
    std::vector<cv::Vec2f> lines;
    cv::HoughLines(canny_img, lines, 1, CV_PI / 180.0, th_length); // CAUTION: The last parameter depends on the input image, it's the smallest number of pixels to consider a line in the accumulator
    //    double minLineLength=50, maxLineGap=5;
    //    cv::HoughLinesP(canny_img, lines, 1, CV_PI / 180.0, th_length, minLineLength, maxLineGap); // CAUTION: The last parameter depends on the input image, it's the smallest number of pixels to consider a line in the accumulator
    std::cout << lines.size() << " lines detected" << std::endl;

    // Possible dilatation of the canny detector
    // Useful when the lines are thin and not perfectly straight
    cv::Mat filteredCanny;
    // Choose whether filtering the Canny or not, for thin and non-perfect edges
    /*filteredCanny = canny_img.clone();*/
    cv::dilate(canny_img, filteredCanny, cv::Mat());
//    cv::namedWindow("Filtered Canny detector");
//    cv::imshow("Filtered Canny detector", filteredCanny);
//    cv::waitKey(0);
//    cv::destroyWindow("Filtered Canny detector");

    // Extracting segments (pairs of points) from the filtered Canny detector
    // And using the line parameters from lines
    extractLines_CannyHough(filteredCanny, lines, segments, th_length);
    cout << "  CFeatureLines::extractLines_CannyHough " << segments.size() << endl; // << " took " << 1000*clock.Tac() << " ms \n";

    // Display 2D segments
    if(display)
    {
        cv::Mat image_lines;
        image.convertTo(image_lines, CV_8UC1, 1.0 / 2);
        for(auto line = begin(segments); line != end(segments); ++line)
        {
            //cout << "line: " << (*line)[0] << " " << (*line)[1] << " " << (*line)[2] << " " << (*line)[3] << " \n";
            //image.convertTo(image_lines, CV_8UC1, 1.0 / 2);
            cv::line(image_lines, cv::Point((*line)[0], (*line)[1]), cv::Point((*line)[2], (*line)[3]), cv::Scalar(255, 0, 255), 1);
            cv::circle(image_lines, cv::Point((*line)[0], (*line)[1]), 3, cv::Scalar(255, 0, 255), 3);
            cv::putText(image_lines, to_string(distance(begin(segments),line)), cv::Point(((*line)[0]+(*line)[2])/2,((*line)[1]+(*line)[3])/2), 0, 1.2, cv::Scalar(200,0,0), 3 );
            //cv::imshow("lines", image_lines); cv::moveWindow("lines", 20,100+700);
            //cv::waitKey(0);
        }
        cv::imshow("lines", image_lines); cv::moveWindow("lines", 20,100+700);
        cv::waitKey(0);
    }
}

void CFeatureLines::extractLines_CannyHough( const cv::Mat & canny_image,
                                             const std::vector<cv::Vec2f> lines,
                                             std::vector<cv::Vec4f> & segments,
                                             size_t th_length )
{
    // Some variables to change the coordinate system from polar to cartesian
    double rho, theta;
    double cosTheta, sinTheta;
    double m, c, cMax;

    // Variables to define the two extreme points on the line, clipped on the window
    // The constraint is x <= xF
    int x, y, xF, yF;

    segments.clear();

    // For each line
    for (size_t n(0); n < lines.size(); ++n) {

        // OpenCV implements the Hough accumulator with theta from 0 to PI, thus rho could be negative
        // We want to always have rho >= 0
        if (lines[n][0] < 0) {
            rho = -lines[n][0];
            theta = lines[n][1] - CV_PI;
        }
        else {
            rho = lines[n][0];
            theta = lines[n][1];
        }

        if (rho == 0 && theta == 0) { // That case appeared at least once, so a test is needed
            continue;
        }

        // Since the step for theta in cv::HoughLines should not be too small,
        // theta should exactly be 0 (and the line vertical) when this condition is true
        if (fabs(theta) < 0.00001)
        {
            x = xF = static_cast<int>(rho + 0.5);
            y = 0;
            yF = canny_image.rows - 1;
        }
        else {
            // Get the (m, c) slope and y-intercept from (rho, theta), so that y = mx + c <=> (x, y) is on the line
            cosTheta = cos(theta);
            sinTheta = sin(theta);
            m = -cosTheta / sinTheta; // We are certain that sinTheta != 0
            c = rho * (sinTheta - m * cosTheta);

            // Get the two extreme points (x, y) and (xF, xF) for the line inside the window, using (m, c)
            if (c >= 0) {
                if (c < canny_image.rows) {
                    // (x, y) is at the left of the window
                    x = 0;
                    y = static_cast<int>(c);
                }
                else {
                    // (x, y) is at the bottom of the window
                    y = canny_image.rows - 1;
                    x = static_cast<int>((y - c) / m);
                }
            }
            else {
                // (x, y) is at the top of the window
                x = static_cast<int>(-c / m);
                y = 0;
            }
            // Define the intersection with the right side of the window
            cMax = m * (canny_image.cols - 1) + c;
            if (cMax >= 0) {
                if (cMax < canny_image.rows) {
                    // (xF, yF) is at the right of the window
                    xF = canny_image.cols - 1;
                    yF = static_cast<int>(cMax);
                }
                else {
                    // (xF, yF) is at the bottom of the window
                    yF = canny_image.rows - 1;
                    xF = static_cast<int>((yF - c) / m);
                }
            }
            else {
                // (xF, yF) is at the top of the window
                xF = static_cast<int>(-c / m);
                yF = 0;
            }

        }

        // Going through the line using the Bresenham algorithm
        // dx1, dx2, dy1 and dy2 are increments that allow to be successful for each of the 8 octants (possible directions while iterating)
        bool onSegment = false;
        int memory;
        int memoryX = 0, memoryY = 0;
        int xPrev = 0, yPrev = 0;
        size_t nbPixels = 0;

        int w = xF - x;
        int h = yF - y;
        int dx1, dy1, dx2, dy2 = 0;

        int longest, shortest;
        int numerator;

        if (w < 0)
        {
            longest = -w;
            dx1 = -1;
            dx2 = -1;
        }
        else
        {
            longest = w;
            dx1 = 1;
            dx2 = 1;
        }

        if (h < 0) {
            shortest = -h;
            dy1 = -1;
        }
        else {
            shortest = h;
            dy1 = 1;
        }

        // We want to know whether the direction is more horizontal or vertical, and set the increments accordingly
        if (longest <= shortest)
        {
            memory = longest;
            longest = shortest;
            shortest = memory;
            dx2 = 0;
            if (h < 0) {
                dy2 = -1;
            }
            else {
                dy2 = 1;
            }
        }

        numerator = longest / 2;

        //cout << n << " numerator " << numerator << endl;
        for (int i(0); i <= longest; ++i)
        {
            // For each pixel, we don't want to use a classic "plot", but to look into canny_image for a black or white pixel
            if (onSegment) {
                if (canny_image.at<char>(y, x) == 0 || i == longest)
                {
                    // We are leaving a segment
                    onSegment = false;
                    if (nbPixels >= th_length) {
                        segments.push_back(cv::Vec4f(memoryX, memoryY, xPrev, yPrev));
                        //cout << "new segment " << segments.back() << endl;
                    }
                }
                else
                {
                    // We are still on a segment
                    ++nbPixels;
                }
            }
            else if (canny_image.at<char>(y, x) != 0)
            {
                // We are entering a segment, and keep this first position in (memoryX, memoryY)
                onSegment = true;
                nbPixels = 0;
                memoryX = x;
                memoryY = y;
            }

            // xPrev and yPrev are used when leaving a segment, to keep in memory the last pixel on it
            xPrev = x;
            yPrev = y;

            // Next pixel using the condition of the Bresenham algorithm
            numerator += shortest;
            if (numerator >= longest)
            {
                numerator -= longest;
                x += dx1;
                y += dy1;
            }
            else {
                x += dx2;
                y += dy2;
            }
        }

    }

}


void CFeatureLines::computeContrastOfSegments(const cv::Mat & image, std::vector<cv::Vec4f> & segments, std::vector<int> & v_contrast, const size_t band_dist, const bool display)
{
    cout << "CFeatureLines::computeContrastOfSegments... display " << display << endl;
    cv::Mat image_lines;
    if(display)
    {
        image_lines = image.clone();
        if( image_lines.channels() == 1 )
            cv::cvtColor( image_lines, image_lines, cv::COLOR_GRAY2BGR );
    }

    v_contrast.resize( segments.size() );
    size_t i(0);
    for(auto line = begin(segments); line != end(segments); ++line, ++i)
    {
//        if(display)
//        {
//            image_lines = image.clone();
//            if( image_lines.channels() == 1 )
//                cv::cvtColor( image_lines, image_lines, cv::COLOR_GRAY2BGR );
//        }

        int contrast(0);
        float deltaX = (*line)[2] - (*line)[0];
        float deltaY = (*line)[3] - (*line)[1];
        if(deltaX >= fabs(deltaY))
        {
            float m = deltaY / deltaX;
            int x = round((*line)[0]);
            int xF = round((*line)[2]);
            int y = round((*line)[1]);
            float error = (*line)[1] - y + m*(x-(*line)[0]);
            if(error > 0.5f)
            {
                ++y;
                error -= 1;
            }
            else if(error < 0.5f)
            {
                --y;
                error += 1;
            }
            if(m >= 0)
            {
                //cout << "line type 1\n";
                for(; x <= xF; ++x)
                {
                    contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
                    //cout << int(image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x)) << " contrast diff " << int(image.at<uchar>(y+band_dist,x)) << "-" << int(image.at<uchar>(y-band_dist,x)) << " accum " << contrast << endl;
                    if(display)
                    {
                        //image_lines.at<cv::Vec3b>(y+band_dist,x) = cv::Vec3b(0,255,0);
                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    }
                    error += m;
                    if(error > 0.5f)
                    {
                        ++y;
                        error -= 1;
                    }
                }
            }
            else
            {
                //cout << "line type 2\n";
                for(; x <= xF; ++x)
                {
                    contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
                    //cout << int(image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x)) << " contrast diff " << int(image.at<uchar>(y+band_dist,x)) << "-" << int(image.at<uchar>(y-band_dist,x)) << " accum " << contrast << endl;
                    if(display)
                    {
                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    }
                    error += m;
                    if(error < 0.5f)
                    {
                        --y;
                        error += 1;
                    }
                }
            }
            contrast /= round(deltaX);
            //cout << i << " contrast " << contrast << " deltaX " << round(deltaX) << endl;
        }
        else
        {
            float m = deltaX / deltaY;
            int x = round((*line)[0]);
            int y = round((*line)[1]);
            int yF = round((*line)[3]);
            float error = (*line)[0] - x + m*(y-(*line)[1]);
            if(error > 0.5f)
            {
                ++x;
                error -= 1;
            }
            else if(error < 0.5f)
            {
                --x;
                error += 1;
            }
            if(m >= 0)
            {
                //cout << "line type 3\n";
                for(; y <= yF; ++y)
                {
                    contrast += image.at<uchar>(y,x-band_dist) - image.at<uchar>(y,x+band_dist);
                    if(display)
                    {
                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    }
                    error += m;
                    if(error > 0.5f)
                    {
                        ++x;
                        error -= 1;
                    }
                }
                contrast /= round(deltaY);
            }
            else
            {
                //cout << "line type 4\n";
                for(; y >= yF; --y)
                {
                    //cout << "contrast diff " << int(image.at<uchar>(y,x+band_dist)) << "-" << int(image.at<uchar>(y,x-band_dist)) << endl;
                    //cout << "contrast " << contrast << endl;
                    contrast += image.at<uchar>(y,x+band_dist) - image.at<uchar>(y,x-band_dist);
                    if(display)
                    {
//                    image_lines.at<cv::Vec3b>(y,x+band_dist) = cv::Vec3b(0,255,0);
                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    }
                    error += m;
                    if(error < 0.5f)
                    {
                        ++x;
                        error += 1;
                    }
                }
                contrast /= -round(deltaY);
            }
        }
        v_contrast[i] = contrast;

        if(display)
        {
            Eigen::Vector2f seg((*line)[2]-(*line)[0], (*line)[3]-(*line)[1]);
            int l = seg.norm();
            seg.normalize();
            Eigen::Vector2f n((*line)[3]-(*line)[1], (*line)[0]-(*line)[2]);
            n.normalize();
            cv::Point e1((*line)[0],(*line)[1]);
            cv::Point e2((*line)[2],(*line)[3]);
            int d = 6 + rand() % 10;
            int rand_line = 0.8*(rand() % l);
            cv::Point seg1(rand_line*seg[0], rand_line*seg[1]);
            cv::Point pt_line = e1 + seg1;
            cv::Point n1(d*n[0], d*n[1]);
            cv::Point pt_txt = pt_line + n1;
            cv::line( image_lines, pt_line, pt_txt, cv::Scalar(0,255,0), 1 );
            cv::putText(image_lines, to_string(int(contrast)), pt_txt, 0, 0.6, cv::Scalar(200,0,0), 1 );
//            cv::imshow("line_contrast", image_lines);
//            cv::waitKey();
        }
    }
    if(display)
    {
        cv::imshow("line_contrast", image_lines);
        cv::waitKey();
    }
}

void CFeatureLines::computeContrastOfSegments(const cv::Mat & image, std::vector<cv::Vec6f> & segments, const size_t band_dist)
{
    size_t i(0);
    for(auto line = begin(segments); line != end(segments); ++line, ++i)
    {
        float contrast(0.f);
        float deltaX = (*line)[2] - (*line)[0];
        float deltaY = (*line)[3] - (*line)[1];
        if(deltaX >= fabs(deltaY))
        {
            float m = deltaY / deltaX;
            int x = round((*line)[0]);
            int xF = round((*line)[2]);
            int y = round((*line)[1]);
            float error = (*line)[1] - y + m*(x-(*line)[0]);
            if(error > 0.5f)
            {
                ++y;
                error -= 1;
            }
            else if(error < 0.5f)
            {
                --y;
                error += 1;
            }
            if(m >= 0)
            {
                cout << "line type 1\n";
                for(; x <= xF; ++x)
                {
                    contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
                    //cout << "contrast diff " << int(image.at<uchar>(y+band_dist,x)) << "-" << int(image.at<uchar>(y-band_dist,x)) << endl;
//                    image_lines.at<cv::Vec3b>(y+band_dist,x) = cv::Vec3b(0,255,0);
//                    image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    error += m;
                    if(error > 0.5f)
                    {
                        ++y;
                        error -= 1;
                    }
                }
            }
            else
            {
                cout << "line type 2\n";
                for(; x <= xF; ++x)
                {
                    contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
//                    image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    error += m;
                    if(error < 0.5f)
                    {
                        --y;
                        error += 1;
                    }
                }
            }
            contrast /= round(deltaX);
        }
        else
        {
            float m = deltaX / deltaY;
            int x = round((*line)[0]);
            int y = round((*line)[1]);
            int yF = round((*line)[3]);
            float error = (*line)[0] - x + m*(y-(*line)[1]);
            if(error > 0.5f)
            {
                ++x;
                error -= 1;
            }
            else if(error < 0.5f)
            {
                --x;
                error += 1;
            }
            if(m >= 0)
            {
                cout << "line type 3\n";
                for(; y <= yF; ++y)
                {
                    contrast += image.at<uchar>(y,x-band_dist) - image.at<uchar>(y,x+band_dist);
//                    image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    error += m;
                    if(error > 0.5f)
                    {
                        ++x;
                        error -= 1;
                    }
                }
                contrast /= round(deltaY);
            }
            else
            {
                cout << "line type 4\n";
                for(; y >= yF; --y)
                {
                    //cout << "contrast diff " << int(image.at<uchar>(y,x+band_dist)) << "-" << int(image.at<uchar>(y,x-band_dist)) << endl;
                    //cout << "contrast " << contrast << endl;
                    contrast += image.at<uchar>(y,x+band_dist) - image.at<uchar>(y,x-band_dist);
//                    image_lines.at<cv::Vec3b>(y,x+band_dist) = cv::Vec3b(0,255,0);
//                    image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
                    error += m;
                    if(error < 0.5f)
                    {
                        ++x;
                        error += 1;
                    }
                }
                contrast /= -round(deltaY);
            }
        }
        segments[i][4] = contrast;
    }
}

void CFeatureLines::extractLinesDesc(const cv::Mat & image,
                                    std::vector<cv::Vec4f> & segments,
                                    std::vector<cv::Vec6f> & segmentsDesc,
                                    const int method,
                                    const size_t th_length, const size_t max_lines,
                                    const bool display)
{
    cout << "CFeatureLines::extractLinesDesc... method " << method << " th_length " << th_length << " im " << image.depth() << endl;
    CTicTac clock; clock.Tic(); //Clock to measure the runtime

    extractLines(image, segments, method, th_length, max_lines);
    segmentsDesc.resize( segments.size() );
    for(size_t i=0; i < segments.size(); ++i)
        segmentsDesc[i] = cv::Vec6f(segments[i][0],segments[i][1],segments[i][2],segments[i][3], 0.f, 0.f);
    computeContrastOfSegments(image, segmentsDesc);

    time = 1000*clock.Tac();
    cout << "  CFeatureLines::extractLinesDesc.v" << method << " lines " << segments.size() << " took " << 1000*clock.Tac() << " ms \n";

    // Display 2D segments
    if(display)
    {
        cv::Mat image_lines = image.clone();
        if( image_lines.channels() == 1 )
            cv::cvtColor( image_lines, image_lines, cv::COLOR_GRAY2BGR );
        for(auto line = begin(segments); line != end(segments); ++line)
        {
            /* get a random color */
            int R = ( rand() % (int) ( 255 + 1 ) );
            int G = ( rand() % (int) ( 255 + 1 ) );
            int B = ( rand() % (int) ( 255 + 1 ) );
            R = 0; G = 0; B = 255;
            cv::line( image_lines, cv::Point(cv::Point2f((*line)[0], (*line)[1])), cv::Point2f(cv::Point((*line)[2], (*line)[3])), cv::Scalar(B,G,R), 3 );
            cv::circle(image_lines, cv::Point(cv::Point2f((*line)[0], (*line)[1])), 3, cv::Scalar(B,G,R), 3);
//            cv::putText(image_lines, to_string(distance(begin(segments),line)), cv::Point(cv::Point2f(((*line)[0]+(*line)[2])/2,((*line)[1]+(*line)[3])/2)), 0, 1.2, cv::Scalar(B,G,R), 3 );

//            float contrast(0.f);
//            float deltaX = (*line)[2] - (*line)[0];
//            float deltaY = (*line)[3] - (*line)[1];
//            if(deltaX >= fabs(deltaY))
//            {
//                float m = deltaY / deltaX;
//                int x = round((*line)[0]);
//                int xF = round((*line)[2]);
//                int y = round((*line)[1]);
//                float error = (*line)[1] - y + m*(x-(*line)[0]);
//                if(error > 0.5f)
//                {
//                    ++y;
//                    error -= 1;
//                }
//                else if(error < 0.5f)
//                {
//                    --y;
//                    error += 1;
//                }
//                if(m >= 0)
//                {
//                    cout << "line type 1\n";
//                    for(; x <= xF; ++x)
//                    {
//                        contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
//                        //cout << "contrast diff " << int(image.at<uchar>(y+band_dist,x)) << "-" << int(image.at<uchar>(y-band_dist,x)) << endl;
//                        image_lines.at<cv::Vec3b>(y+band_dist,x) = cv::Vec3b(0,255,0);
//                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
//                        error += m;
//                        if(error > 0.5f)
//                        {
//                            ++y;
//                            error -= 1;
//                        }
//                    }
//                }
//                else
//                {
//                    cout << "line type 2\n";
//                    for(; x <= xF; ++x)
//                    {
//                        contrast += image.at<uchar>(y+band_dist,x) - image.at<uchar>(y-band_dist,x);
//                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
//                        error += m;
//                        if(error < 0.5f)
//                        {
//                            --y;
//                            error += 1;
//                        }
//                    }
//                }
//                contrast /= round(deltaX);
//            }
//            else
//            {
//                float m = deltaX / deltaY;
//                int x = round((*line)[0]);
//                int y = round((*line)[1]);
//                int yF = round((*line)[3]);
//                float error = (*line)[0] - x + m*(y-(*line)[1]);
//                if(error > 0.5f)
//                {
//                    ++x;
//                    error -= 1;
//                }
//                else if(error < 0.5f)
//                {
//                    --x;
//                    error += 1;
//                }
//                if(m >= 0)
//                {
//                    cout << "line type 3\n";
//                    for(; y <= yF; ++y)
//                    {
//                        contrast += image.at<uchar>(y,x-band_dist) - image.at<uchar>(y,x+band_dist);
//                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
//                        error += m;
//                        if(error > 0.5f)
//                        {
//                            ++x;
//                            error -= 1;
//                        }
//                    }
//                    contrast /= round(deltaY);
//                }
//                else
//                {
//                    cout << "line type 4\n";
//                    for(; y >= yF; --y)
//                    {
//                        //cout << "contrast diff " << int(image.at<uchar>(y,x+band_dist)) << "-" << int(image.at<uchar>(y,x-band_dist)) << endl;
//                        //cout << "contrast " << contrast << endl;
//                        contrast += image.at<uchar>(y,x+band_dist) - image.at<uchar>(y,x-band_dist);
//                        image_lines.at<cv::Vec3b>(y,x+band_dist) = cv::Vec3b(0,255,0);
//                        image_lines.at<cv::Vec3b>(y,x) = cv::Vec3b(0,0,255);
//                        error += m;
//                        if(error < 0.5f)
//                        {
//                            ++x;
//                            error += 1;
//                        }
//                    }
//                    contrast /= -round(deltaY);
//                }
//            }
//            cout << " contrast " << contrast << endl;
//            cv::imshow( "lines", image_lines );
//            cv::waitKey();
        }
        cv::imshow( "lines", image_lines );
        cv::waitKey();
    }
}

#endif //MRPT_END
