/* +---------------------------------------------------------------------------+
   |                     Mobile Robot Programming Toolkit (MRPT)               |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2017, Individual contributors, see AUTHORS file        |
   | See: http://www.mrpt.org/Authors - All rights reserved.                   |
   | Released under BSD License. See details in http://www.mrpt.org/License    |
   +---------------------------------------------------------------------------+ */

/*  Plane-based Map (PbMap) library
 *  Construction of plane-based maps and localization in it from RGBD Images.
 *  Writen by Eduardo Fernandez-Moral. See docs for <a href="group__mrpt__pbmap__grp.html" >mrpt-pbmap</a>
 */

#include "pbmap-precomp.h"  // Precompiled headers

#if MRPT_HAS_PCL

#include <mrpt/pbmap/Plane.h>
#include <mrpt/pbmap/Miscellaneous.h>
#include <pcl/common/time.h>
#include <pcl/filters/voxel_grid.h>

#include <Eigen/Eigenvalues>

#include <mrpt/system/os.h>

using namespace mrpt::pbmap;
using namespace mrpt::utils;
using namespace std;
using namespace Eigen;

IMPLEMENTS_SERIALIZABLE(Plane, CSerializable, mrpt::pbmap)

Plane::Plane() :
    elongation(1.0),
    bFullExtent(false),
    bFromStructure(false),
    //      contourPtr(new pcl::PointCloud<pcl::PointXYZRGBA>),
    polygonContourPtr(new pcl::PointCloud<pcl::PointXYZRGBA>),
    planePointCloudPtr(new pcl::PointCloud<pcl::PointXYZRGBA>)
{
      //      vector< vector<int> > vec(4, vector<int>(4));
}

//Plane::Plane(const Plane & p) //:
////    polygonContourPtr(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.polygonContourPtr)),
////    planePointCloudPtr(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.planePointCloudPtr)),
////    outerPolygonPtr(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.outerPolygonPtr))
//{
//    cout << "Plane::Plane(const Plane & p) \n";
//    *this = p;
//    polygonContourPtr.reset(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.polygonContourPtr));
//    planePointCloudPtr.reset(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.planePointCloudPtr));
////    outerPolygonPtr.reset(new pcl::PointCloud<pcl::PointXYZRGBA>(*p.outerPolygonPtr));
////    polygonContourPtr = p.polygonContourPtr;
////    planePointCloudPtr = p.planePointCloudPtr;
////    outerPolygonPtr = p.outerPolygonPtr;
//}

/*---------------------------------------------------------------
                        writeToStream
 ---------------------------------------------------------------*/
void  Plane::writeToStream(mrpt::utils::CStream &out, int *out_Version) const
{
    //cout << "Write plane. Version " << *out_Version << endl;
    if (out_Version)
        *out_Version = 0;
    else
    {
        // The data
        //		out << static_cast<uint32_t>(numObservations);//out << uint32_t(numObservations);
        //		out << areaVoxels;
        out << areaHull;
        out << elongation;
        out << curvature;
        out << v3normal(0) << v3normal(1) << v3normal(2);
        out << v3center(0) << v3center(1) << v3center(2);
        out << v3PpalDir(0) << v3PpalDir(1) << v3PpalDir(2);
        out << v3colorNrgb(0) << v3colorNrgb(1) << v3colorNrgb(2);
        ////    out << v3colorNrgbDev(0) << v3colorNrgbDev(1) << v3colorNrgbDev(2);
        out << dominantIntensity;
        out << bDominantColor;

        out << hist_H;
        //cout << "color " << hist_H.size() << endl;
        //    for (size_t i=0; i < 74; i++){//cout << hist_H[i] << " ";
        //      out << hist_H[i];}

        out << inliers;

        out << label;
        out << label_object;
        out << label_context;

        //    out.WriteBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3normal(0),3);
        //    out.WriteBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3center(0),3);
        //    out.WriteBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3PpalDir(0),3);
        //    out.WriteBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3colorNrgb(0),3);
        //    out.WriteBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3colorNrgbDev(0),3);

        //cout << "write neighbors " << neighborPlanes.size() << endl;

        //    out << (uint32_t)neighborPlanes.size();
        //    for (std::map<unsigned,unsigned>::const_iterator it=neighborPlanes.begin(); it != neighborPlanes.end(); it++)
        //      out << static_cast<uint32_t>(it->first) << static_cast<uint32_t>(it->second);

        out << (uint32_t)polygonContourPtr->size();
        for (uint32_t i=0; i < polygonContourPtr->size(); i++)
            out << polygonContourPtr->points[i].x << polygonContourPtr->points[i].y << polygonContourPtr->points[i].z;

        //    out << bFullExtent;
        //    out << bFromStructure;
    }
}

/*---------------------------------------------------------------
                        readFromStream
 ---------------------------------------------------------------*/
void  Plane::readFromStream(mrpt::utils::CStream &in, int version)
{
    switch(version)
    {
    case 0:
        ////	case 1:
    {
        //			// The data
        uint32_t n;
        //			in >> n;
        //			numObservations = (unsigned)n;
        //			in >> areaVoxels;
        in >> areaHull;
        in >> elongation;
        in >> curvature;
        in >> v3normal(0) >> v3normal(1) >> v3normal(2);
        in >> v3center(0) >> v3center(1) >> v3center(2);
        in >> v3PpalDir(0) >> v3PpalDir(1) >> v3PpalDir(2);
        d = -v3normal.dot(v3center);
        in >> v3colorNrgb(0) >> v3colorNrgb(1) >> v3colorNrgb(2);
        ////			in >> v3colorNrgbDev(0) >> v3colorNrgbDev(1) >> v3colorNrgbDev(2);
        in >> dominantIntensity;
        in >> bDominantColor;

        in >> hist_H;
        //      hist_H.resize(74);
        //      for (size_t i=0; i < 74; i++)
        //        in >> hist_H[i];

        in >> inliers;

        in >> label;
        in >> label_object;
        in >> label_context;
        ////cout << "Read Nrgb color \n";// << v3colorNrgb.transpose()
        ////
        ////cout << "Read color histogram\n";
        ////			in.ReadBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3normal[0],sizeof(v3normal[0])*3);
        ////			in.ReadBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3center[0],sizeof(v3center[0])*3);
        ////			in.ReadBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3PpalDir[0],sizeof(v3PpalDir[0])*3);
        ////			in.ReadBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3colorNrgb[0],sizeof(v3colorNrgb[0])*3);
        ////			in.ReadBufferFixEndianness<Eigen::Vector3f::Scalar>(&v3colorNrgbDev[0],sizeof(v3colorNrgbDev[0])*3);
        //
        //			in >> n;
        //			neighborPlanes.clear();
        //      for (uint32_t i=0; i < n; i++)
        //      {
        //        uint32_t neighbor, commonObs;
        //        in >> neighbor >> commonObs;
        //        neighborPlanes[neighbor] = commonObs;
        //      }

        in >> n;
        //        cout << "neighbors " << n << endl;
        polygonContourPtr->resize(n);
        for (unsigned i=0; i < n; i++)
            in >> polygonContourPtr->points[i].x >> polygonContourPtr->points[i].y >> polygonContourPtr->points[i].z;

        //      in >> bFullExtent;
        //      in >> bFromStructure;

    } break;
    default:
        MRPT_THROW_UNKNOWN_SERIALIZATION_VERSION(version)
    };
}

/*!
 * Force the plane inliers to lay on the plane
 */
void Plane::forcePtsLayOnPlane()
{
    // The plane equation has the form Ax + By + Cz + D = 0, where the vector N=(A,B,C) is the normal and the constant D can be calculated as D = -N*(PlanePoint) = -N*PlaneCenter
    const double D = -(v3normal.dot(v3center));
    for(unsigned i = 0; i < planePointCloudPtr->size(); i++)
    {
        double dist = v3normal[0]*planePointCloudPtr->points[i].x + v3normal[1]*planePointCloudPtr->points[i].y + v3normal[2]*planePointCloudPtr->points[i].z + D;
        planePointCloudPtr->points[i].x -= v3normal[0] * dist;
        planePointCloudPtr->points[i].y -= v3normal[1] * dist;
        planePointCloudPtr->points[i].z -= v3normal[2] * dist;
    }
    // Do the same with the points defining the convex hull
    for(unsigned i = 0; i < polygonContourPtr->size(); i++)
    {
        double dist = v3normal[0]*polygonContourPtr->points[i].x + v3normal[1]*polygonContourPtr->points[i].y + v3normal[2]*polygonContourPtr->points[i].z + D;
        polygonContourPtr->points[i].x -= v3normal[0] * dist;
        polygonContourPtr->points[i].y -= v3normal[1] * dist;
        polygonContourPtr->points[i].z -= v3normal[2] * dist;
    }
}

void Plane::forcePtsLayOnPlane(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr & inliers)
{
    // The plane equation has the form Ax + By + Cz + D = 0, where the vector N=(A,B,C) is the normal and the constant D can be calculated as D = -N*(PlanePoint) = -N*PlaneCenter
    const double D = -(v3normal.dot(v3center));
    for(unsigned i = 0; i < inliers->size(); i++)
    {
        double dist = v3normal[0]*inliers->points[i].x + v3normal[1]*inliers->points[i].y + v3normal[2]*inliers->points[i].z + D;
        inliers->points[i].x -= v3normal[0] * dist;
        inliers->points[i].y -= v3normal[1] * dist;
        inliers->points[i].z -= v3normal[2] * dist;
    }
}

/** \brief Compute the area of a 2D planar polygon patch
  */
float Plane::compute2DPolygonalArea ()
{
    int k0, k1, k2;

    // Find axis with largest normal component and project onto perpendicular plane
    k0 = (fabs (v3normal[0] ) > fabs (v3normal[1])) ? 0  : 1;
    k0 = (fabs (v3normal[k0]) > fabs (v3normal[2])) ? k0 : 2;
    k1 = (k0 + 1) % 3;
    k2 = (k0 + 2) % 3;

    // cos(theta), where theta is the angle between the polygon and the projected plane
    float ct = fabs ( v3normal[k0] );
    float AreaX2 = 0.0;
    float p_i[3], p_j[3];

    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        int j = (i + 1) % polygonContourPtr->size();
        p_j[0] = polygonContourPtr->points[j].x; p_j[1] = polygonContourPtr->points[j].y; p_j[2] = polygonContourPtr->points[j].z;

        AreaX2 += p_i[k1] * p_j[k2] - p_i[k2] * p_j[k1];
    }
    AreaX2 = fabs (AreaX2) / (2 * ct);

    return AreaX2;
}

/** \brief Compute the patch's convex-hull area and mass center
  */
void Plane::computeMassCenterAndArea()
{
    int k0, k1, k2;

    // Find axis with largest normal component and project onto perpendicular plane
    k0 = (fabs (v3normal[0] ) > fabs (v3normal[1])) ? 0  : 1;
    k0 = (fabs (v3normal[k0]) > fabs (v3normal[2])) ? k0 : 2;
    k1 = (k0 + 1) % 3;
    k2 = (k0 + 2) % 3;

    // cos(theta), where theta is the angle between the polygon and the projected plane
    float ct = fabs ( v3normal[k0] );
    float AreaX2 = 0.0;
    Eigen::Vector3f massCenter = Eigen::Vector3f::Zero();
    float p_i[3], p_j[3];

    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        int j = (i + 1) % polygonContourPtr->size();
        p_j[0] = polygonContourPtr->points[j].x; p_j[1] = polygonContourPtr->points[j].y; p_j[2] = polygonContourPtr->points[j].z;
        double cross_segment = p_i[k1] * p_j[k2] - p_i[k2] * p_j[k1];

        AreaX2 += cross_segment;
        massCenter[k1] += (p_i[k1] + p_j[k1]) * cross_segment;
        massCenter[k2] += (p_i[k2] + p_j[k2]) * cross_segment;
    }
    areaHull = fabs (AreaX2) / (2 * ct);

    massCenter[k1] /= (3*AreaX2);
    massCenter[k2] /= (3*AreaX2);
    massCenter[k0] = (v3normal.dot(v3center) - v3normal[k1]*massCenter[k1] - v3normal[k2]*massCenter[k2]) / v3normal[k0];

    v3center = massCenter;

    d = -(v3normal. dot (v3center));
}

void Plane::calcElongationAndPpalDir()
{
    pcl::PCA< PointT > pca;
    pca.setInputCloud(planePointCloudPtr);
    Eigen::VectorXf eigenVal = pca.getEigenValues();
    //  if( eigenVal[0] > 2 * eigenVal[1] )
    {
        elongation = sqrt(eigenVal[0] / eigenVal[1]);
        Eigen::MatrixXf eigenVect = pca.getEigenVectors();
        //    v3PpalDir = makeVector(eigenVect(0,0), eigenVect(1,0), eigenVect(2,0));
        v3PpalDir[0] = eigenVect(0,0);
        v3PpalDir[1] = eigenVect(1,0);
        v3PpalDir[2] = eigenVect(2,0);
    }

    // Compute the geometric center
    Eigen::Vector3f p_i, p_j, p_k, weighted_sum_vertices;
    float total_weights = 0.f; // This value is twice the perimeter
    Eigen::VectorXf weights( polygonContourPtr->size() );
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        int j = (i + 1) % polygonContourPtr->size();
        p_j[0] = polygonContourPtr->points[j].x; p_j[1] = polygonContourPtr->points[j].y; p_j[2] = polygonContourPtr->points[j].z;
        int k = (j + 1) % polygonContourPtr->size();
        p_k[0] = polygonContourPtr->points[k].x; p_k[1] = polygonContourPtr->points[k].y; p_k[2] = polygonContourPtr->points[k].z;

        weights[j] = (p_j-p_i).norm() + (p_j-p_k).norm();
        total_weights += weights[j];
        weighted_sum_vertices += weights[j]*p_j;
    }
    //v3center = weighted_sum_vertices / total_weights;
    Eigen::Vector3f v3center_ = weighted_sum_vertices / total_weights;
    ASSERT_( (v3center_-v3center).isMuchSmallerThan(0.01f) );

    Eigen::Matrix3f M = Eigen::Matrix3f::Zero();
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        //diff.squaredNorm();
        M += weights[i] * (p_i-v3center) * (p_i-v3center).transpose();
    }

}

// Compute the plane parameters from a set of noisy points (described in our RAS paper)
void Plane::computeInvariantParams (pcl::PointCloud<pcl::PointXYZRGBA>::Ptr & inliers )
{
//    std::cout << "Plane::computeInvariantParams... inliers " << inliers->size() << std::endl;
//    double start_time = pcl::getTime();

    // Recompute the plane parameters {n,d,c,Sigma}
    float stdDevFactor = 0.01f;
    float total_weights = 0.f;
    Eigen::Vector3f p_i;
    Eigen::VectorXf weights( inliers->size() );
    Eigen::Vector3f v3center_ = Eigen::Vector3f::Zero();
    // Compute the geometric center
    for (unsigned int i = 0; i < inliers->size(); i++)
    {
        p_i[0] = inliers->points[i].x; p_i[1] = inliers->points[i].y; p_i[2] = inliers->points[i].z;
        float sigma_p = stdDevFactor * p_i.squaredNorm();
        weights[i] = 1.f / (sigma_p*sigma_p);
        total_weights += weights[i];
        v3center_ += weights[i] * p_i;
    }
    //    v3center_ = v3center_ / total_weights;
    //    std::cout << " v3center_ " << v3center_.transpose() << std::endl;
    //    Eigen::Vector3f diff = v3center_-v3center;
    v3center = v3center_ / total_weights;
//    std::cout << "v3center " << v3center.transpose() << std::endl;

    // Compute the inliers's covariance matrix M
    Eigen::Matrix3f M = Eigen::Matrix3f::Zero();
    for (unsigned int i = 0; i < inliers->size(); i++)
    {
        p_i[0] = inliers->points[i].x; p_i[1] = inliers->points[i].y; p_i[2] = inliers->points[i].z;
        v3center_ += weights[i] * p_i;
        M += weights[i] * (p_i-v3center) * (p_i-v3center).transpose();
    }

    JacobiSVD<MatrixXf> svd(M, ComputeThinU | ComputeThinV);
    v3normal = svd.matrixU().col(2);
    if(v3normal .dot (v3center) > 0)
        v3normal = -v3normal;
    d = -v3normal .dot( v3center );

    //    std::cout << "svd eigenvectors \n" << svd.matrixU() << std::endl;
    //    std::cout << "svd eigenvectors \n" << svd.matrixV() << std::endl;
    //    std::cout << "svd eigenvalues \n" << svd.singularValues() << std::endl;
    //    std::cout << "v3normal " << v3normal.transpose() << std::endl;
    //    std::cout << "d " << d << std::endl;
    //mrpt::system::pause();

    information(3,3) = total_weights;
    information.block(0,3,3,1) = total_weights * v3center_;
    information.block(3,0,1,3) = information.block(0,3,3,1).transpose();
    float nMn = v3normal.transpose()*M*v3normal;
    information.block(0,0,3,3) = M - total_weights*v3center_*v3center_.transpose() - nMn*Eigen::Matrix3f::Identity();
    //information.block(0,0,3,3) = M - total_weights*v3center_*v3center_.transpose() - (Eigen::Matrix3f::Identity()*(v3normal.transpose()*M*v3normal));

    svd = JacobiSVD<MatrixXf>(information);//(M, ComputeThinU | ComputeThinV);
    info_3rd_eigenval = sqrt( svd.singularValues()[2] );
    //info_3rd_eigenval = ( svd.singularValues()[2] );
//    std::cout << "information \n" << information << std::endl;
//    std::cout << "info_3rd_eigenval " << info_3rd_eigenval << std::endl;

    //    double start_end = pcl::getTime();
    //    cout << "computeParams profiling " << (start_end - start_time)*1e6 << " us\n";
}

// Compute the plane parameters from a set of noisy points (described in our RAS paper)
void Plane::computeParamsConvexHull()
{
//    std::cout << "Plane::computeParamsConvexHull... " << std::endl;
//    double start_time = pcl::getTime();

    // Shift the points to fulfill the plane equation
    //forcePtsLayOnPlane(); // This is already done right after computing the convex hull

    Eigen::Vector3f p_i, p_j, p_k, weighted_sum_vertices;
    float total_weights = 0.f; // This value is twice the perimeter
    Eigen::VectorXf weights( polygonContourPtr->size() );
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        int j = (i + 1) % polygonContourPtr->size();
        p_j[0] = polygonContourPtr->points[j].x; p_j[1] = polygonContourPtr->points[j].y; p_j[2] = polygonContourPtr->points[j].z;
        int k = (j + 1) % polygonContourPtr->size();
        p_k[0] = polygonContourPtr->points[k].x; p_k[1] = polygonContourPtr->points[k].y; p_k[2] = polygonContourPtr->points[k].z;

        weights[j] = (p_j-p_i).norm() + (p_j-p_k).norm();
        total_weights += weights[j];
        weighted_sum_vertices += weights[j]*p_j;
    }
    v3center = weighted_sum_vertices / total_weights;
    //std::cout << "v3center " << v3center.transpose() << std::endl;

    Eigen::Matrix3f M = Eigen::Matrix3f::Zero();
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        M += weights[i] * (p_i-v3center) * (p_i-v3center).transpose();
    }

    JacobiSVD<MatrixXf> svd(M, ComputeThinU | ComputeThinV);
    if( svd.singularValues()[0] > 2 * svd.singularValues()[1] )
    {
        v3PpalDir = svd.matrixU().col(0);
        elongation = sqrt( svd.singularValues()[0] / svd.singularValues()[1] );
    }
    //std::cout << " elongation " << elongation << " v3PpalDir " << v3PpalDir.transpose() << std::endl;

    //    double start_end = pcl::getTime();
    //    cout << "computeParams profiling " << (start_end - start_time)*1e6 << " us\n";
}

// The point cloud of the plane must be filled before using this function
void Plane::getPlaneNrgb()
{
    //cout << "Plane::getPlaneNrgb() " << planePointCloudPtr->size() << endl;
    ASSERT_( planePointCloudPtr->size() > 0);

    r.resize(planePointCloudPtr->size());
    g.resize(planePointCloudPtr->size());
    b.resize(planePointCloudPtr->size());
    intensity.resize(planePointCloudPtr->size());

    size_t countPix=0;
    for(size_t i=0; i < planePointCloudPtr->size(); i++)
    {
        float sumRGB = (float)planePointCloudPtr->points[i].r + planePointCloudPtr->points[i].g + planePointCloudPtr->points[i].b;
        intensity[i] = sumRGB;
        if(sumRGB != 0)
        {
            r[countPix] = planePointCloudPtr->points[i].r / sumRGB;
            g[countPix] = planePointCloudPtr->points[i].g / sumRGB;
            b[countPix] = planePointCloudPtr->points[i].b / sumRGB;
            ++countPix;
        }
    }
    r.resize(countPix);
    g.resize(countPix);
    b.resize(countPix);
    intensity.resize(countPix);
}

void Plane::getPlaneC1C2C3()
{
    c1.resize(planePointCloudPtr->size());
    c2.resize(planePointCloudPtr->size());
    c3.resize(planePointCloudPtr->size());

    for(unsigned i=0; i < planePointCloudPtr->size(); i++)
    {
        c1[i] = atan2((double) planePointCloudPtr->points[i].r, (double) max(planePointCloudPtr->points[i].g,planePointCloudPtr->points[i].b));
        c2[i] = atan2((double) planePointCloudPtr->points[i].g, (double) max(planePointCloudPtr->points[i].r,planePointCloudPtr->points[i].b));
        c3[i] = atan2((double) planePointCloudPtr->points[i].b, (double) max(planePointCloudPtr->points[i].r,planePointCloudPtr->points[i].g));
    }
}

void Plane::calcPlaneHistH()
{
    cout << "Plane::calcPlaneHistH()\n";
    float fR, fG, fB;
    float fH, fS, fV;
    const float FLOAT_TO_BYTE = 255.0f;
    const float BYTE_TO_FLOAT = 1.0f / FLOAT_TO_BYTE;

    vector<int> H(74,0);

    for(unsigned i=0; i < planePointCloudPtr->size(); i++)
    {
        // Convert from 8-bit integers to floats.
        fR = planePointCloudPtr->points[i].r * BYTE_TO_FLOAT;
        fG = planePointCloudPtr->points[i].g * BYTE_TO_FLOAT;
        fB = planePointCloudPtr->points[i].b * BYTE_TO_FLOAT;
        // Convert from RGB to HSV, using float ranges 0.0 to 1.0.
        float fDelta;
        float fMin, fMax;
        int iMax;
        // Get the min and max, but use integer comparisons for slight speedup.
        if (planePointCloudPtr->points[i].b < planePointCloudPtr->points[i].g) {
            if (planePointCloudPtr->points[i].b < planePointCloudPtr->points[i].r) {
                fMin = fB;
                if (planePointCloudPtr->points[i].r > planePointCloudPtr->points[i].g) {
                    iMax = planePointCloudPtr->points[i].r;
                    fMax = fR;
                }
                else {
                    iMax = planePointCloudPtr->points[i].g;
                    fMax = fG;
                }
            }
            else {
                fMin = fR;
                fMax = fG;
                iMax = planePointCloudPtr->points[i].g;
            }
        }
        else {
            if (planePointCloudPtr->points[i].g < planePointCloudPtr->points[i].r) {
                fMin = fG;
                if (planePointCloudPtr->points[i].b > planePointCloudPtr->points[i].r) {
                    fMax = fB;
                    iMax = planePointCloudPtr->points[i].b;
                }
                else {
                    fMax = fR;
                    iMax = planePointCloudPtr->points[i].r;
                }
            }
            else {
                fMin = fR;
                fMax = fB;
                iMax = planePointCloudPtr->points[i].b;
            }
        }
        fDelta = fMax - fMin;
        fV = fMax;				// Value (Brightness).
        if(fV < 0.01)
        {
            //      fH = 72; // Histogram has 72 bins for Hue values, and 2 bins more for black and white
            H[72]++;
            continue;
        }
        else
            //    if (iMax != 0)
        {			// Make sure its not pure black.
            fS = fDelta / fMax;		// Saturation.
            if(fS < 0.2)
            {
                //        fH = 73; // Histogram has 72 bins for Hue values, and 2 bins more for black and white
                H[73]++;
                continue;
            }

            //      float ANGLE_TO_UNIT = 1.0f / (6.0f * fDelta);	// Make the Hues between 0.0 to 1.0 instead of 6.0
            float ANGLE_TO_UNIT = 12.0f / fDelta;	// 12 bins
            if (iMax == planePointCloudPtr->points[i].r) {		// between yellow and magenta.
                fH = (fG - fB) * ANGLE_TO_UNIT;
            }
            else if (iMax == planePointCloudPtr->points[i].g) {		// between cyan and yellow.
                fH = (2.0f/6.0f) + ( fB - fR ) * ANGLE_TO_UNIT;
            }
            else {				// between magenta and cyan.
                fH = (4.0f/6.0f) + ( fR - fG ) * ANGLE_TO_UNIT;
            }
            // Wrap outlier Hues around the circle.
            if (fH < 0.0f)
                fH += 72.0f;
            if (fH >= 72.0f)
                fH -= 72.0f;
        }
        //  cout << "H " << fH << endl;
        H[int(fH)]++;
    }
    //cout << "FInish histogram calculation\n";
    // Normalize histogram
    float numPixels = 0;
    for(unsigned i=0; i < H.size(); i++)
        numPixels += H[i];
    hist_H.resize(74);
    for(unsigned i=0; i < H.size(); i++)
        hist_H[i] = H[i]/numPixels;

    cout << "Got H histogram" << hist_H.size() << "\n";
}

float concentrationThreshold = 0.5;

void Plane::calcMainColor2()
{
    ASSERT_( planePointCloudPtr->size() > 0);

    //  double getColorStart, getColorEnd;
    //  getColorStart = pcl::getTime();
    std::vector<Eigen::Vector4f> color(planePointCloudPtr->size());

    size_t countPix=0;
    size_t stepColor = std::max(planePointCloudPtr->size() / 2000, static_cast<size_t>(1)); // Limit the main color calculation to 2000 pixels
    for(size_t i=0; i < planePointCloudPtr->size(); i+=stepColor)
    {
        float sumRGB = (float)planePointCloudPtr->points[i].r + planePointCloudPtr->points[i].g + planePointCloudPtr->points[i].b;
        if(sumRGB != 0)
        {
            color[countPix][0] = planePointCloudPtr->points[i].r / sumRGB;
            color[countPix][1] = planePointCloudPtr->points[i].g / sumRGB;
            color[countPix][2] = planePointCloudPtr->points[i].b / sumRGB;
            color[countPix][3] = sumRGB;
            //      intensity[countPix] = sumRGB;
            //cout << "color " << color[countPix][0] << " " << color[countPix][1] << " " << color[countPix][2] << " " << color[countPix][3] << endl;
            ++countPix;
        }
    }
    color.resize(countPix);
    intensity.resize(countPix);

    float concentration, histStdDev;
    //  Eigen::Vector4f dominantColor;
    Eigen::Vector4f dominantColor = getMultiDimMeanShift_color(color, concentration, histStdDev);
    v3colorNrgb = dominantColor.head(3);
    dominantIntensity = dominantColor(3);

    if(concentration > concentrationThreshold)
        bDominantColor = true;
    else
        bDominantColor = false;
    //getColorEnd = pcl::getTime ();
    //cout << "Nrgb2 in " << (getColorEnd - getColorStart)*1000000 << " us\n";
    //cout << "colorNrgb " << v3colorNrgb[0] << " " << v3colorNrgb[1] << " " << v3colorNrgb[2] << " " << v3colorNrgb[3] << endl;

    //getColorStart = pcl::getTime ();
    //
    //  // c1c2c3
    //  getPlaneC1C2C3();
    //  v3colorC1C2C3[0] = getHistogramMeanShift(c1, (float)1.5708, v3colorNrgbDev[0]);
    //  v3colorC1C2C3[1] = getHistogramMeanShift(c2, (float)1.5708, v3colorNrgbDev[1]);
    //  v3colorC1C2C3[2] = getHistogramMeanShift(c3, (float)1.5708, v3colorNrgbDev[2]);
    //getColorEnd = pcl::getTime ();
    //cout << "c1c2c3 in " << (getColorEnd - getColorStart)*1000000 << " us\n";
    //
    //getColorStart = pcl::getTime ();
    //
    //  // H histogram
    //  calcPlaneHistH();
    //getColorEnd = pcl::getTime ();
    //cout << "HistH in " << (getColorEnd - getColorStart)*1000000 << " us\n";
}

void Plane::calcMainColor()
{
    // Normalized rgb
    //double getColorStart ,getColorEnd;
    //getColorStart = pcl::getTime ();

    getPlaneNrgb();
    //cout << "r size " << r.size() << " " << v3colorNrgbDev(0) << endl;
    v3colorNrgb(0) = getHistogramMeanShift(r, 1.0, v3colorNrgbDev(0));
    v3colorNrgb(1) = getHistogramMeanShift(g, 1.0, v3colorNrgbDev(1));
    v3colorNrgb(2) = getHistogramMeanShift(b, 1.0, v3colorNrgbDev(2));
    ////  dominantIntensity = getHistogramMeanShift(intensity, 767.0, v3colorNrgbDev(2));
    //ASSERT_(v3colorNrgb(0) != 0 && v3colorNrgb(1) != 0 && v3colorNrgb(2) != 0);
    //
    dominantIntensity = 0;
    int countFringe05 = 0;
    for(unsigned i=0; i < r.size(); i++)
        if(fabs(r[i] - v3colorNrgb(0)) < 0.05 && fabs(g[i] - v3colorNrgb(1)) < 0.05 && fabs(b[i] - v3colorNrgb(2)) < 0.05)
        {
            dominantIntensity += intensity[i];
            ++countFringe05;
        }
    ASSERT_(countFringe05 > 0);
    dominantIntensity /= countFringe05;
    float concentration05 = static_cast<float>(countFringe05) / r.size();

    //getColorEnd = pcl::getTime ();
    //cout << "Nrgb in " << (getColorEnd - getColorStart)*1000000 << " us\n";
    //cout << "Concentration " << concentration1 << " " << concentration2 << " " << concentration3 << endl;
    // We are storing the concentration vale in v3colorNrgbDev to heck if there exists a dominant color
    //  if(concentration1 > concentrationThreshold && concentration2 > concentrationThreshold && concentration3 > concentrationThreshold)
    if(concentration05 > 0.5)
        bDominantColor = true;
    else
        bDominantColor = false;

    //getColorStart = pcl::getTime ();

    //  // c1c2c3
    //  getPlaneC1C2C3();
    //  v3colorC1C2C3[0] = getHistogramMeanShift(c1, (float)1.5708, v3colorNrgbDev[0]);
    //  v3colorC1C2C3[1] = getHistogramMeanShift(c2, (float)1.5708, v3colorNrgbDev[1]);
    //  v3colorC1C2C3[2] = getHistogramMeanShift(c3, (float)1.5708, v3colorNrgbDev[2]);
    ////getColorEnd = pcl::getTime ();
    ////cout << "c1c2c3 in " << (getColorEnd - getColorStart)*1000000 << " us\n";
    //
    ////getColorStart = pcl::getTime ();
    // H histogram
    calcPlaneHistH();
    ////cout << "Update color fin" << endl;
    //getColorEnd = pcl::getTime ();
    //cout << "HistH in " << (getColorEnd - getColorStart)*1000000 << " us\n";
}

/**!
 * mPointHull serves to calculate the convex hull of a set of points in 2D, which are defined by its position (x,y)
 * and an identity id
*/
struct mPointHull {
    float x, y;
    size_t id;

    bool operator <(const mPointHull &p) const {
        return x < p.x || (x == p.x && y < p.y);
    }
};

float cross(const mPointHull &O, const mPointHull &A, const mPointHull &B)
{
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

///**!
// * Calculate the plane's convex hull with the monotone chain algorithm.
//*/
//void Plane::calcConvexHull(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr &pointCloud)
//{
//  // Find axis with largest normal component and project onto perpendicular plane
//  int k0;//, k1, k2;
//  k0 = (fabs (v3normal(0) ) > fabs(v3normal[1])) ? 0  : 1;
//  k0 = (fabs (v3normal(k0) ) > fabs(v3normal(2))) ? k0 : 2;
//
//  std::vector<mPointHull> P;//(pointCloud->size() );
//  P.resize(pointCloud->size() );
//  if(k0 == 0)
//    for(size_t i=0; i < pointCloud->size(); i++)
//    {
//      P[i].x = pointCloud->points[i].y;
//      P[i].y = pointCloud->points[i].z;
//      P[i].id = i;
//    }
//  else if(k0 == 1)
//    for(size_t i=0; i < pointCloud->size(); i++)
//    {
//      P[i].x = pointCloud->points[i].x;
//      P[i].y = pointCloud->points[i].z;
//      P[i].id = i;
//    }
//  else // (k0 == 2)
//    for(size_t i=0; i < pointCloud->size(); i++)
//    {
//      P[i].x = pointCloud->points[i].x;
//      P[i].y = pointCloud->points[i].y;
//      P[i].id = i;
//    }
//
//  int n = P.size(), k = 0;
//  std::vector<mPointHull> H(2*n);
//
//  // Sort points lexicographically
//  std::sort(P.begin(), P.end());
//
//  // Build lower hull
//  for (int i = 0; i < n; i++)
//  {
//    while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0)
//      k--;
//    H[k++] = P[i];
//    if(k > 0)
//        ASSERT_(H[k-1].id != H[k-2].id);
//  }
//
//  // Build upper hull
//  for (int i = n-2, t = k+1; i >= 0; i--)
//  {
//    while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0)
//      k--;
//    H[k++] = P[i];
//  }
//
//  // Fill convexHull vector
//  size_t c_hull_size = k-1; // Neglect the last_point = first_point
//  H.resize(k);
//  polygonContourPtr->resize(c_hull_size);
//
//  for(size_t i=0; i < c_hull_size; i++)
//  {
//    polygonContourPtr->points[i] = pointCloud->points[ H[i].id ];
//  }
//
//}

void Plane::calcConvexHull(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr &pointCloud, std::vector<int> &indices)
{
    // Find axis with largest normal component and project onto perpendicular plane
    int k0;//, k1, k2;
    k0 = (fabs (v3normal(0) ) > fabs(v3normal[1])) ? 0  : 1;
    k0 = (fabs (v3normal(k0) ) > fabs(v3normal(2))) ? k0 : 2;

    std::vector<mPointHull> P;//(pointCloud->size() );
    P.resize(pointCloud->size() );
    if(k0 == 0)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].y;
            P[i].y = pointCloud->points[i].z;
            P[i].id = i;
        }
    else if(k0 == 1)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].x;
            P[i].y = pointCloud->points[i].z;
            P[i].id = i;
        }
    else // (k0 == 2)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].x;
            P[i].y = pointCloud->points[i].y;
            P[i].id = i;
        }

    int n = P.size(), k = 0;
    std::vector<mPointHull> H(2*n);

    // Sort points lexicographically
    std::sort(P.begin(), P.end());

    // Build lower hull
    for (int i = 0; i < n; i++)
    {
        while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
        if(k > 0)
            ASSERT_(H[k-1].id != H[k-2].id);
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--)
    {
        while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
    }

    // Fill convexHull vector (polygonContourPtr)
    size_t c_hull_size = k-1; // Neglect the last_point = first_point
    H.resize(k);
    polygonContourPtr->resize(c_hull_size);
    indices.resize(c_hull_size);

    for(size_t i=0; i < c_hull_size; i++)
    {
        polygonContourPtr->points[i] = pointCloud->points[ H[i].id ];
        indices[i] = H[i].id;
    }

    // Shift the points to fulfill the plane equation
    forcePtsLayOnPlane(polygonContourPtr);
}


void Plane::calcConvexHullandParams(pcl::PointCloud<pcl::PointXYZRGBA>::Ptr &pointCloud, std::vector<int> & indices)
{
    //std::cout << "Plane::calcConvexHullandParams... cloud " << pointCloud->size() << " indices " << indices.size() << std::endl;

    // Find axis with largest normal component and project onto perpendicular plane
    int k0, k1, k2;
    k0 = (fabs (v3normal(0) ) > fabs(v3normal[1])) ? 0  : 1;
    k0 = (fabs (v3normal(k0) ) > fabs(v3normal(2))) ? k0 : 2;
    k1 = (k0 + 1) % 3;
    k2 = (k0 + 2) % 3;

    std::vector<mPointHull> P;//(pointCloud->size() );
    P.resize(pointCloud->size() );
    if(k0 == 0)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].y;
            P[i].y = pointCloud->points[i].z;
            P[i].id = i;
        }
    else if(k0 == 1)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].z;
            P[i].y = pointCloud->points[i].x;
            P[i].id = i;
        }
    else // (k0 == 2)
        for(size_t i=0; i < pointCloud->size(); i++)
        {
            P[i].x = pointCloud->points[i].x;
            P[i].y = pointCloud->points[i].y;
            P[i].id = i;
        }

    int n = P.size(), k = 0;
    std::vector<mPointHull> H(2*n);

    // Sort points lexicographically
    std::sort(P.begin(), P.end());

    // Build lower hull
    for (int i = 0; i < n; i++)
    {
        while (k >= 2 && cross(H[k-2], H[k-1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
        if(k > 0)
            ASSERT_(H[k-1].id != H[k-2].id);
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--)
    {
        while (k >= t && cross(H[k-2], H[k-1], P[i]) <= 0)
            k--;
        H[k++] = P[i];
    }

    // Fill convexHull vector (polygonContourPtr)
    size_t c_hull_size = k-1; // No repetition -> Neglect the last_point = first_point
    H.resize(k);
    indices.resize(c_hull_size);
    //std::cout << "indices " << c_hull_size << std::endl;
    for(size_t i=0; i < c_hull_size; i++)
    {
//        std::cout << i << " index " << c_hull_size << std::endl;
//        std::cout << "  H[i].id " << H[i].id << std::endl;
//        std::cout << "  H[i].id " << pointCloud->points[H[i].id].x << std::endl;
//        polygonContourPtr->points[i] = pointCloud->points[ H[i].id ];
        indices[i] = H[i].id;
    }
    polygonContourPtr.reset(new pcl::PointCloud<pcl::PointXYZRGBA>(*pointCloud, indices));
    //    std::cout << "polygonContourPtr " << polygonContourPtr->size() << std::endl;

    // Shift the points to fulfill the plane equation
//    std::cout << "forcePtsLayOnPlane " << std::endl;
    forcePtsLayOnPlane(polygonContourPtr);

    // Calc area and mass center
    float ct = fabs ( v3normal[k0] );
    float AreaX2 = 0.0;
    Eigen::Vector3f massCenter = Eigen::Vector3f::Zero();
    for(size_t i=0, j=1; i < c_hull_size; i++, j++)
    {
        float cross_segment = H[i].x * H[j].y - H[i].y * H[j].x;

        AreaX2 += cross_segment;
        massCenter[k1] += (H[i].x + H[j].x) * cross_segment;
        massCenter[k2] += (H[i].y + H[j].y) * cross_segment;
    }
    areaHull = fabs(AreaX2) / (2 * ct);
//    std::cout << "areaHull " << areaHull << std::endl;
//    std::cout << "massCenter " << massCenter.transpose() << std::endl;

    //  std::cout << " PREV v3center " << v3center.transpose() << std::endl;
    //  float error = v3center.transpose()*v3normal+d;
    //  std::cout << " fit error " << error << std::endl;

    v3center[k1] /= (3*AreaX2);
    v3center[k2] /= (3*AreaX2);
    v3center[k0] = (-d - v3normal[k1]*massCenter[k1] - v3normal[k2]*massCenter[k2]) / v3normal[k0];
    //std::cout << "v3center " << v3center.transpose() << std::endl;

    //d = -v3normal .dot( v3center );

    //  // Compute elongation and ppal direction
    //  Eigen::Matrix2f covariances = Eigen::Matrix2f::Zero();
    //  Eigen::Vector2f p1, p2;
    //  p2[0] = H[0].x-massCenter[k1]; p2[1] = H[0].y-massCenter[k2];
    ////  float perimeter = 0.0;
    //  for(size_t i=1; i < c_hull_size; i++)
    //  {
    //    p1 = p2;
    //    p2[0] = H[i].x-massCenter[k1]; p2[1] = H[i].y-massCenter[k2];
    //    float lenght_side = (p1 - p2).norm();
    ////    perimeter += lenght_side;
    //    covariances += lenght_side * (p1*p1.transpose() + p2*p2.transpose());
    //  }
    ////  covariances /= perimeter;

    //  // Compute eigen vectors and values
    //  Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> evd (covariances);
    //  // Organize eigenvectors and eigenvalues in ascendent order
    //  if( evd.eigenvalues()[0] > 2 * evd.eigenvalues()[1] )
    //  {
    //    elongation = sqrt(evd.eigenvalues()[0] / evd.eigenvalues()[1]);
    //    v3PpalDir[k1] = evd.eigenvectors()(0,0);
    //    v3PpalDir[k2] = evd.eigenvectors()(1,0);
    //    v3PpalDir[k0] = (-d - v3normal[k1]*v3PpalDir[k1] - v3normal[k2]*v3PpalDir[k2]) / v3normal[k0];
    //    v3PpalDir /= v3PpalDir.norm();
    //  }
    //  std::cout << " elongation " << elongation << std::endl;
    //  std::cout << " v3center " << v3center.transpose() << std::endl;
    //  std::cout << " v3PpalDir " << v3PpalDir.transpose() << std::endl;
    //  error = v3center.transpose()*v3normal+d;
    //  std::cout << " fit error " << error << std::endl;

    Eigen::Vector3f p_i, p_j, p_k, weighted_sum_vertices;
    float total_weights = 0.f; // This value is twice the perimeter
    Eigen::VectorXf weights( polygonContourPtr->size() );
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        int j = (i + 1) % polygonContourPtr->size();
        p_j[0] = polygonContourPtr->points[j].x; p_j[1] = polygonContourPtr->points[j].y; p_j[2] = polygonContourPtr->points[j].z;
        int k = (j + 1) % polygonContourPtr->size();
        p_k[0] = polygonContourPtr->points[k].x; p_k[1] = polygonContourPtr->points[k].y; p_k[2] = polygonContourPtr->points[k].z;

        weights[j] = (p_j-p_i).norm() + (p_j-p_k).norm();
        total_weights += weights[j];
        weighted_sum_vertices += weights[j]*p_j;
    }
    v3center = weighted_sum_vertices / total_weights;

    Eigen::Matrix3f M = Eigen::Matrix3f::Zero();
    for (unsigned int i = 0; i < polygonContourPtr->size(); i++)
    {
        p_i[0] = polygonContourPtr->points[i].x; p_i[1] = polygonContourPtr->points[i].y; p_i[2] = polygonContourPtr->points[i].z;
        M += weights[i] * (p_i-v3center) * (p_i-v3center).transpose();
    }

    JacobiSVD<MatrixXf> svd(M, ComputeThinU | ComputeThinV);
    if( svd.singularValues()[0] > 2 * svd.singularValues()[1] )
    {
        v3PpalDir = svd.matrixU().col(0);
        elongation = sqrt( svd.singularValues()[0] / svd.singularValues()[1] );
    }
    //std::cout << " elongation " << elongation << " v3PpalDir " << v3PpalDir.transpose() << std::endl;

    //  error = v3center.transpose()*v3normal+d;
    //  std::cout << " fit error " << error << std::endl;
}

/*!Returns true when the closest distance between the patches "this" and the input "plane_nearby" is under distThreshold. This function is approximated*/
bool Plane::isPlaneNearby(Plane &plane_nearby, const float distThreshold)
{
    float distThres2 = distThreshold * distThreshold;

    // First we check distances between centroids and vertex to accelerate this check
    if( (v3center - plane_nearby.v3center).squaredNorm() < distThres2 )
        return true;

    for(unsigned i=1; i < polygonContourPtr->size(); i++)
        if( (polygonContourPtr->points[i].getVector3fMap() - plane_nearby.v3center).squaredNorm() < distThres2 )
            return true;

    for(unsigned j=1; j < plane_nearby.polygonContourPtr->size(); j++)
        if( (v3center - plane_nearby.polygonContourPtr->points[j].getVector3fMap() ).squaredNorm() < distThres2 )
            return true;

    for(unsigned i=1; i < polygonContourPtr->size(); i++)
        for(unsigned j=1; j < plane_nearby.polygonContourPtr->size(); j++)
            if( (diffPoints(polygonContourPtr->points[i], plane_nearby.polygonContourPtr->points[j]) ).squaredNorm() < distThres2 )
                return true;

    // a) Between an edge and a vertex
    // b) Between two edges (imagine two polygons on perpendicular planes)
    // a) & b)
    for(unsigned i=1; i < polygonContourPtr->size(); i++)
        for(unsigned j=1; j < plane_nearby.polygonContourPtr->size(); j++)
            if(dist3D_Segment_to_Segment2(Segment(polygonContourPtr->points[i],polygonContourPtr->points[i-1]), Segment(plane_nearby.polygonContourPtr->points[j],plane_nearby.polygonContourPtr->points[j-1])) < distThres2)
                return true;

    return false;
}

/*! Returns true if the two input planes represent the same physical surface for some given angle and distance thresholds.
* If the planes are the same they are merged in this and the function returns true. Otherwise it returns false.*/
bool Plane::isSamePlane(Plane &plane_nearby, const float &cosAngleThreshold, const float &parallelDistThres, const float &proxThreshold)
{
    // Check that both planes have similar orientation
    if( v3normal .dot (plane_nearby.v3normal) < cosAngleThreshold )
        return false;

    // Check the normal distance of the planes centers using their average normal
    float dist_normal = v3normal .dot (plane_nearby.v3center - v3center);
    if(fabs(dist_normal) > parallelDistThres ) // Then merge the planes
        return false;

    // Check that the distance between the planes centers is not too big
    if( !isPlaneNearby(plane_nearby, proxThreshold) )
        return false;

    return true;
}

/*!Check if the the input plane is the same than this plane for some given angle and distance thresholds.
 * If the planes are the same they are merged in this and the function returns true. Otherwise it returns false.*/
bool Plane::isSamePlane(Eigen::Matrix4f &Rt, Plane &plane_, const float &cosAngleThreshold, const float &parallelDistThres, const float &proxThreshold)
{
    Plane plane = plane_;
    plane.v3normal = Rt.block(0,0,3,3) * plane_.v3normal;
    plane.v3center = Rt.block(0,0,3,3) * plane_.v3center + Rt.block(0,3,3,1);
    pcl::transformPointCloud(*plane_.polygonContourPtr,*plane.polygonContourPtr,Rt);

    if(fabs(d - plane.d) > parallelDistThres )
        return false;

    // Check that both planes have similar orientation
    if( v3normal.dot(plane.v3normal) < cosAngleThreshold )
        return false;

    //  // Check the normal distance of the planes centers using their average normal
    //  float dist_normal = v3normal.dot(plane.v3center - v3center);
    ////  if(fabs(dist_normal) > parallelDistThres ) // Avoid matching different parallel planes
    ////    return false;
    //  float thres_max_dist = max(parallelDistThres, parallelDistThres*2*norm(plane.v3center - v3center));
    //  if(fabs(dist_normal) > thres_max_dist ) // Avoid matching different parallel planes
    //    return false;

    // Once we know that the planes are almost coincident (parallelism and position)
    // we check that the distance between the planes is not too big
    return isPlaneNearby(plane, proxThreshold);
}

bool Plane::hasSimilarDominantColor(Plane &plane, const float colorThreshold)
{
    if(bDominantColor && plane.bDominantColor &&
            (fabs(v3colorNrgb[0] - plane.v3colorNrgb[0]) > colorThreshold ||
             fabs(v3colorNrgb[1] - plane.v3colorNrgb[1]) > colorThreshold )
            )
        return false;
    else
        return true;
}


/*! Merge the input "same_plane_patch" into "this".
*  Recalculate center, normal vector, area, inlier points (filtered), convex hull, etc.
*/
void Plane::mergePlane(Plane &same_plane_patch)
{
    // Update normal and center
    //ASSERT_(areaVoxels > 0 && same_plane_patch.areaVoxels > 0)
    //  v3normal = (areaVoxels*v3normal + same_plane_patch.areaVoxels*same_plane_patch.v3normal);
    ASSERT_(inliers.size() > 0 && same_plane_patch.inliers.size() > 0);
    v3normal = (inliers.size()*v3normal + same_plane_patch.inliers.size()*same_plane_patch.v3normal);
    v3normal = v3normal / v3normal.norm();

    // Update point inliers
    //  *polygonContourPtr += *same_plane_patch.polygonContourPtr; // Merge polygon points
    *planePointCloudPtr += *same_plane_patch.planePointCloudPtr; // Add the points of the new detection and perform a voxel grid

    // Filter the points of the patch with a voxel-grid. This points are used only for visualization
    static pcl::VoxelGrid<pcl::PointXYZRGBA> merge_grid;
    merge_grid.setLeafSize(0.05,0.05,0.05);
    pcl::PointCloud<pcl::PointXYZRGBA> mergeCloud;
    merge_grid.setInputCloud (planePointCloudPtr);
    merge_grid.filter (mergeCloud);
    planePointCloudPtr->clear();
    *planePointCloudPtr = mergeCloud;
    //  calcMainColor();
    //  if(configPbMap.use_color)
    //    calcMainColor();

    inliers.insert(inliers.end(), same_plane_patch.inliers.begin(), same_plane_patch.inliers.end());

    *same_plane_patch.polygonContourPtr += *polygonContourPtr;
    calcConvexHull(same_plane_patch.polygonContourPtr);
    computeMassCenterAndArea();

    d = -v3normal .dot( v3center );

    // Shift the points to fulfill the plane equation
    //forcePtsLayOnPlane(); // This is already done right after computing the convex hull

    // Update area
    //  double area_recalc = planePointCloudPtr->size() * 0.0025;
    //  mpPlaneInferInfo->isFullExtent(same_plane_patch, area_recalc);
    areaVoxels= planePointCloudPtr->size() * 0.0025;

}

// Adaptation for RGBD360
void Plane::mergePlane2(Plane &same_plane_patch)
{
    // Update normal and center
    ASSERT_(inliers.size() > 0 && same_plane_patch.inliers.size() > 0);
    v3normal = (inliers.size()*v3normal + same_plane_patch.inliers.size()*same_plane_patch.v3normal);
    v3normal = v3normal / v3normal.norm();

    // Update point inliers
    *planePointCloudPtr += *same_plane_patch.planePointCloudPtr; // Add the points of the new detection and perform a voxel grid

    inliers.insert(inliers.end(), same_plane_patch.inliers.begin(), same_plane_patch.inliers.end());

    *same_plane_patch.polygonContourPtr += *polygonContourPtr;
    calcConvexHull(same_plane_patch.polygonContourPtr);
    //  calcConvexHullandParams(same_plane_patch.polygonContourPtr);
    //std::cout << d << " area " << areaHull << " center " << v3center.transpose() << " elongation " << elongation << " v3PpalDir " << v3PpalDir.transpose() << std::endl;
    computeMassCenterAndArea();

    calcElongationAndPpalDir();

    d = -v3normal .dot( v3center );
    //std::cout << d << "area " << areaHull << " center " << v3center.transpose() << " elongation " << elongation << " v3PpalDir " << v3PpalDir.transpose() << std::endl;

    calcMainColor2(); // Calculate dominant color

    // Update color histogram
    for(size_t i=0; i < hist_H.size(); i++)
    {
        hist_H[i] += same_plane_patch.hist_H[i];
        hist_H[i] /= 2;
    }

}

void Plane::mergePlane_convexHull(Plane &same_plane_patch)
{
    // Update normal and d
    Vector4f nd; nd.block(0,0,3,1) = v3normal; nd(3) = d;
    Vector4f nd_merge; nd_merge.block(0,0,3,1) = same_plane_patch.v3normal; nd_merge(3) = same_plane_patch.d;
    Matrix4f total_information = information + same_plane_patch.information;
    Matrix4f total_covariance = total_information.inverse();
//    JacobiSVD<Matrix4f> pseudo_inverse(total_information);
//    pseudo_inverse.pinv(total_covariance);
    Vector4f nd_fused = total_covariance*(information*nd + same_plane_patch.information*nd_merge);

    std::cout << "PREV v3normal " << v3normal.transpose() << std::endl;
    std::cout << "d " << d << std::endl;
    std::cout << "information \n" << information << std::endl;

    v3normal = nd_fused.block(0,0,3,1);
    v3normal = v3normal / v3normal.norm();
    d = nd_fused(3);
    information = total_information;
    JacobiSVD<MatrixXf> svd(information);//(M, ComputeThinU | ComputeThinV);
    info_3rd_eigenval = sqrt( svd.singularValues()[2] );
    //info_3rd_eigenval = ( svd.singularValues()[2] );

    std::cout << "v3normal " << v3normal.transpose() << std::endl;
    std::cout << "d " << d << std::endl;
    std::cout << "information \n" << information << std::endl;

    // Update point inliers
    *planePointCloudPtr += *same_plane_patch.planePointCloudPtr; // Add the points of the new detection and perform a voxel grid

    // Filter the points of the patch with a voxel-grid. This points are used only for visualization
    static pcl::VoxelGrid<pcl::PointXYZRGBA> merge_grid;
    merge_grid.setLeafSize(0.05,0.05,0.05);
    pcl::PointCloud<pcl::PointXYZRGBA> mergeCloud;
    merge_grid.setInputCloud (planePointCloudPtr);
    merge_grid.filter (mergeCloud);
    planePointCloudPtr->clear();
    *planePointCloudPtr = mergeCloud;
    //  calcMainColor();
    //  if(configPbMap.use_color)
    //    calcMainColor();

    inliers.insert(inliers.end(), same_plane_patch.inliers.begin(), same_plane_patch.inliers.end());

    // Update convex hull
    *same_plane_patch.polygonContourPtr += *polygonContourPtr;
    calcConvexHull(same_plane_patch.polygonContourPtr);
    computeMassCenterAndArea();
    calcElongationAndPpalDir();

    // Shift the points to fulfill the plane equation
    //forcePtsLayOnPlane(); // This is already done right after computing the convex hull

    // Update area
    //  double area_recalc = planePointCloudPtr->size() * 0.0025;
    //  mpPlaneInferInfo->isFullExtent(same_plane_patch, area_recalc);
    areaVoxels= planePointCloudPtr->size() * 0.0025;
}

void Plane::transform(Eigen::Matrix4f &Rt)
{
    const Eigen::Matrix3f rotation = Rt.block(0,0,3,3);
    const Eigen::Vector3f translation = Rt.block(0,3,3,1);

    // Transform normal and ppal direction
    v3normal = rotation * v3normal;
    v3PpalDir = rotation * v3PpalDir;

    // Transform centroid
    v3center = rotation * v3center + translation;

    d = -(v3normal. dot (v3center));

    // Rotate covariance
    const Eigen::Matrix3f cov_nn = information.block(0,0,3,3);
    const Eigen::Vector3f cov_nd = information.block(0,3,3,1);
    const float cov_dd = information(3,3);
    information.block(0,0,3,3) = rotation * cov_nn * rotation.transpose();
    information.block(0,3,3,1) = rotation * (cov_nn*translation + cov_nd);
    information.block(3,0,1,3) = information.block(0,3,3,1).transpose();
    information(3,3) = translation.transpose()*cov_nn*translation + 2*(translation .dot (cov_nd) ) + cov_dd;

    JacobiSVD<MatrixXf> svd(information);//(M, ComputeThinU | ComputeThinV);
    info_3rd_eigenval = sqrt( svd.singularValues()[2] );
    //info_3rd_eigenval = svd.singularValues()[2];

    // Transform convex hull points
    pcl::transformPointCloud(*polygonContourPtr, *polygonContourPtr, Rt);

    pcl::transformPointCloud(*planePointCloudPtr, *planePointCloudPtr, Rt);
}


#endif
