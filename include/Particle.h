/*******************************************************************************
 * Author: Hongkun Yu, Mingxu Hu, Kunpeng Wang
 * Dependecy:
 * Test:
 * Execution:
 * Description:
 * ****************************************************************************/

#ifndef PARTICLE_H
#define PARTICLE_H

#include <iostream>
#include <numeric>
#include <cmath>

#include <gsl/gsl_math.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_cdf.h>

#include "Config.h"
#include "Macro.h"
#include "Typedef.h"
#include "Logging.h"
#include "Precision.h"

#include "Coordinate5D.h"
#include "Random.h"
#include "Euler.h"
#include "Functions.h"
#include "Symmetry.h"
#include "DirectionalStat.h"

#define FOR_EACH_C(par) for (int iC = 0; iC < par.nC(); iC++)
#define FOR_EACH_R(par) for (int iR = 0; iR < par.nR(); iR++)
#define FOR_EACH_T(par) for (int iT = 0; iT < par.nT(); iT++)
#define FOR_EACH_D(par) for (int iD = 0; iD < par.nD(); iD++)

#define FOR_EACH_PAR(par) \
    FOR_EACH_C(par) \
        FOR_EACH_R(par) \
            FOR_EACH_T(par) \
                FOR_EACH_D(par)

#define PEAK_FACTOR_MAX 0.5
#define PEAK_FACTOR_MIN 1e-3

#define PEAK_FACTOR_C (1 - 1e-2)

#define PEAK_FACTOR_BASE 2

#define INIT_OUTSIDE_CONFIDENCE_AREA 0.5

#define RHO_MAX (1 - 1e-1)
#define RHO_MIN (-1 + 1e-1)

#define PERTURB_K_MAX 1

enum ParticleType
{
    PAR_C,
    PAR_R,
    PAR_T,
    PAR_D
};

class Particle
{
    private:

        /**
         * MODE_2D: the reference is a 2D image, and the perturbation in
         * rotation is in 2D
         *
         * MODE_3D: the reference is a 3D volume, and the perturbation in
         * rotation is in 2D
         */
        int _mode;

        /**
         * number of classes in this particle filter
         */
        int _nC;

        /**
         * number of particles in this particle filter
         */
        int _nR;

        int _nT;

        int _nD;

        /**
         * the standard deviation of translation, assuming the translation
         * follows a 2D Gaussian distribution
         */
        double _transS;

        /**
         * the re-center threshold of translation
         * For example, assuming _transQ = 0.01, if a translation lies beyond
         * the confidence area of 99%, this translation will be re-centre to the
         * original point.
         */
        double _transQ;

        double _peakFactorC;

        double _peakFactorR;

        double _peakFactorT;
        
        double _peakFactorD;

        /**
         * a dvector storing the class of each particle
         */
        uvec _c;

        /**
         * MODE_2D: a table storing the rotation information as the first and
         * second elements stand for a unit dvector in circle and the other two
         * elements are zero
         *
         * MODE_3D: a table storing the rotation information with each row
         * storing a quaternion
         */
        dmat4 _r;

        /**
         * a table storing the translation information with each row storing a
         * 2-dvector with x and y respectively
         */
        dmat2 _t;

        /**
         * a dvector storing the defocus factor of each particle
         */
        dvec _d;

        /**
         * a dvector storing the weight of each particle
         */
        dvec _wC;

        dvec _wR;

        dvec _wT;

        dvec _wD;

        dvec _uC;

        dvec _uR;

        dvec _uT;

        dvec _uD;
        
        /**
         * a pointer points to a Symmetry object which indicating the symmetry
         */
        const Symmetry* _sym;

        /**
         * concentration parameter of Angular Central Gaussian distribution of
         * rotation
         */
        /**
         * concnetration paramter of von Mises distribution of rotation (kappa)
         */
        double _k1;

        double _k2;

        double _k3;

        /**
         * sigma0 of 2D Gaussian distribution of translation
         */
        double _s0;

        /**
         * sigma1 of 2D Gaussian distribution of translation
         */
        double _s1;

        /**
         * rho of 2D Gaussian distribution of translation
         */
        double _rho;

        /**
         * sigma of 1D Gaussian distribution of defocus factor
         */
        double _s;

        double _score;

        /**
         * the previous most likely class
         */
        size_t _topCPrev;

        /**
         * the most likely class
         */
        size_t _topC;

        /**
         * MODE_2D: the first element stands for the previous most likely
         * rotation
         *
         * MODE_3D: quaternion of the previous most likely rotation
         */
        dvec4 _topRPrev;

        /**
         * MODE_2D: the first element stands for the most likely rotation
         *
         * MODE_3D: quaternion of the most likely rotation
         */
        dvec4 _topR;

        /**
         * the previous most likely translation
         */
        dvec2 _topTPrev;

        /**
         * the most likely translation
         * it will be refreshed by resampling
         */
        dvec2 _topT;

        /**
         * the previous most likely defocus factor
         */
        double _topDPrev;

        /**
         * the most likely defocus factor
         */
        double _topD;

        /**
         * default initialiser
         */
        void defaultInit()
        {
            _mode = MODE_3D;

            _nC = 1;

            _sym = NULL;

            _k1 = 1;
            _k2 = 1;
            _k3 = 1;

            _s0 = DBL_MAX;
            _s1 = DBL_MAX;

            _rho = 0;

            _s = 0;

            _topCPrev = 0;
            _topC = 0;

            _topRPrev = dvec4(1, 0, 0, 0);
            _topR = dvec4(1, 0, 0, 0);

            _topTPrev = dvec2(0, 0);
            _topT = dvec2(0, 0);

            _topDPrev = 1;
            _topD = 1;
        }

    public:

        /**
         * default constructor of Particle
         */
        Particle();

        /**
         * constructor of Particle
         *
         * @param m      number of classes in this particle filter
         * @param n      number of particles in this particle filter
         * @param transS standard deviation of translation
         * @param transQ the re-center threshold of translation
         * @param sym    symmetry of resampling space
         */
        Particle(const int mode,
                 const int nC,
                 const int nR,
                 const int nT,
                 const int nD,
                 const double transS,
                 const double transQ = 0.01,
                 const Symmetry* sym = NULL);

        /**
         * deconstructor of Particle
         */
        ~Particle();

        /**
         * This function initialise Particle.
         *
         * @param transS stndard deviation of translation
         * @param transQ the re-center threshold of translation
         * @param sym    symmetry of resampling space
         */
        void init(const int mode,
                  const double transS,
                  const double transQ = 0.01,
                  const Symmetry* sym = NULL);

        /**
         * This function initialises Particle.
         *
         * @param n      number of particles in this particle filter
         * @param transS stndard deviation of translation
         * @param transQ the re-center threshold of translation
         * @param sym    symmetry of resampling space
         */
        void init(const int mode,
                  const int nC,
                  const int nR,
                  const int nT,
                  const int nD,
                  const double transS,
                  const double transQ = 0.01,
                  const Symmetry* sym = NULL);

        /**
         * This function resets the particles in this particle to a uniform
         * distribution.
         */
        void reset();

        /**
         * This function resets the particles in this particle filter to a uniform
         * distribution in rotation and 2D Gaussian distribution in translation
         * with a given number of sampling points.
         *
         * @param n number of particles in this particle filter
         */
        /***
        void reset(const int m,
                   const int n);
        ***/

        /**
         * This function resets the particles in this particle filter to a
         * uiform distribution in rotation with nR sampling points, and 2D
         * Gaussian distribution in translation with nT sampling points. The
         * total number of particles in this particle will be k x nR x nT. The
         * sampling points for the iR-th rotation and the iT-th translation of
         * the k-th clas will be at (k * nR * nT + iR * nT + iT) index of the
         * particles in this particle filter.
         *
         * @param m  the number of classes in this particle filter
         * @param nR the number of rotation in this particle filter
         * @param nT the number of translation in this particle filter
         */
        void reset(const int nC,
                   const int nR,
                   const int nT,
                   const int nD);

        /**
         * initialise defocus factor
         *
         * @param sD the standard deviation of defocus factor
         */
        void initD(const int nD,
                   const double sD = 0.05);

        int mode() const;

        void setMode(const int mode);

        /**
         * This function returns the number of classes in this particle fitler.
         */
        int nC() const;

        void setNC(const int nC);

        /**
         * This function returns the number of particles in this particle
         * filter.
         */
        int nR() const;

        /**
         * This function sets the number of particles in this particle filter.
         *
         * @param n the number of particles in this particle fitler
         */
        void setNR(const int nR);

        int nT() const;

        void setNT(const int nT);

        int nD() const;

        void setND(const int nD);

        /**
         * This function returns the standard deviation of translation, assuming
         * the translation follows a 2D Gaussian distribution.
         */
        double transS() const;

        /**
         * This function sets the standard deviation of translation, assuming
         * the translation follows a 2D Gaussian distribution.
         *
         * @param transS the standard deviation of translation
         */
        void setTransS(const double transS);

        /**
         * This function returns the re-center threshold of translation.
         */
        double transQ() const;

        /**
         * This function sets the re-center theshold of translation.
         *
         * @param the re-center threshold of translation
         */
        void setTransQ(const double transQ);

        uvec c() const;

        void setC(const uvec& c);

        /**
         * This function returns the table storing the rotation information
         * with each row storing a quaternion.
         */
        dmat4 r() const;

        /**
         * This function sets the table storing the rotation information with
         * each row storing a quaternion.
         *
         * @param r the table storing the rotation information with each row
         * storing a quaternion
         */
        void setR(const dmat4& r);

        /**
         * This function returns the table storing the translation information
         * with each row storing a 2-dvector with x and y respectively.
         */
        dmat2 t() const;

        /**
         * This function sets the table storing the translation information
         * with each row storing a 2-dvector with x and y respectively.
         *
         * @param t the table storing the translation information with each row
         * storing a 2-dvector with x and y respectively
         */
        void setT(const dmat2& t);

        dvec d() const;

        void setD(const dvec& d);

        dvec wC() const;

        void setWC(const dvec& wC);

        /**
         * This function returns the dvector storing the weight of each particle.
         */
        dvec wR() const;

        /**
         * This function sets the dvector storing the weight of each particle.
         *
         * @param w the dvector storing the weight of each particle
         */
        void setWR(const dvec& wR);

        dvec wT() const;

        void setWT(const dvec& wT);

        dvec wD() const;

        void setWD(const dvec& wD);

        dvec uC() const;

        void setUC(const dvec& uC);

        dvec uR() const;

        void setUR(const dvec& uR);

        dvec uT() const;

        void setUT(const dvec& uT);

        dvec uD() const;

        void setUD(const dvec& uD);

        dvec2 topT() const;

        void setTopT(const dvec2& topT);

        dvec2 topTPrev() const;

        void setTopTPrev(const dvec2& topTPrev);

        /**
         * This function returns the symmetry.
         */
        const Symmetry* symmetry() const;

        /**
         * This function sets the symmetry.
         *
         * @param sym a pointer points to the Symmetry object
         */
        void setSymmetry(const Symmetry* sym);

        /**
         * This function generates the particles by loading the class,
         * quaternion of translation, standard devaation of rotation,
         * translation dvector, standard deviation of translation X,
         * standard deviation of translation Y, defocus factor and
         * standard deviation of defocus factor.
         *
         * @param m     the number of classes
         * @param n     the number of particles
         * @param cls   the class
         * @param quat  the quaternion of rotation
         * @param stdR  the standard deviation of rotation
         * @param tran  the translation dvector
         * @param stdTX the standard deviation of translation X
         * @param stdTY the standard deviation of translation Y
         * @param d     the defocus factor
         * @param stdD  the standard deviation of defocus factor
         */
        void load(const int nR,
                  const int nT,
                  const int nD,
                  const dvec4& q,
                  const double k1,
                  const double k2,
                  const double k3,
                  const dvec2& t,
                  const double s0,
                  const double s1,
                  const double d,
                  const double s,
                  const double score);

        /**
         * This function returns the concentration parameters, including
         * rotation and translation.
         *
         * @param k0  the concentration parameter of the rotation
         * @param k1  the concentration parameter of the rotation
         * @param s0  sigma0 of 2D Gaussian distribution of the translation
         * @param s1  sigma1 of 2D Gaussian distribution of the translation
         * @param rho rho of 2D Gaussian distribution of the translation
         */
        void vari(double& k1,
                  double& k2,
                  double& k3,
                  double& s0,
                  double& s1,
                  double& s) const;

        /**
         * This function returns the concentration parameters, including
         * rotation and translation.
         *
         * @param rVari the concentration parameter of the rotation
         * @param s0    sigma0 of 2D Gaussian distribution of the translation
         * @param s1    sigma1 of 2D Gaussian distribution of the translation
         */
        void vari(double& rVari,
                  double& s0,
                  double& s1,
                  double& s) const;

        double variR() const;

        double variT() const;

        double variD() const;

        double compressR() const;

        double compressT() const;

        double compressD() const;

        double score() const;

        double wC(const int i) const;

        void setWC(const double wC,
                   const int i);

        void mulWC(const double wC,
                   const int i);

        /**
         * This function returns the weight of the i-th particle in this
         * particle filter.
         *
         * @param i the index of particle
         */
        double wR(const int i) const;

        /**
         * This function sets the weight of the i-th particle in this particle
         * filter.
         *
         * @param w the weight of particle
         * @param i the index of particle
         */
        void setWR(const double wR,
                   const int i);

        /**
         * This function multiply the weight of the i-th particle in this
         * particle with a factor.
         *
         * @param w the factor
         * @param i the index of particle
         */
        void mulWR(const double wR,
                   const int i);

        double wT(const int i) const;

        void setWT(const double wT,
                   const int i);

        void mulWT(const double wT,
                   const int i);

        double wD(const int i) const;

        void setWD(const double wD,
                   const int i);

        void mulWD(const double wD,
                   const int i);

        double uC(const int i) const;

        void setUC(const double uC,
                   const int i);

        double uR(const int i) const;

        void setUR(const double uR,
                   const int i);

        double uT(const int i) const;

        void setUT(const double uT,
                   const int i);

        double uD(const int i) const;

        void setUD(const double uD,
                   const int i);
        /**
         * This function normalizes the dvector of the weights.
         */
        void normW();

        /**
         * This function returns the 5D coordinates of the i-th particle.
         *
         * @param dst the 5D coordinate
         * @param i   the index of particle
         */
        /***
        void coord(Coordinate5D& dst,
                   const int i) const;
                   ***/

        /**
         * This function returns the class of the i-th particle.
         *
         * @param dst the class
         * @param i   the index of particle
         */
        void c(size_t& dst,
               const int i) const;

        /**
         * This function sets the class of the i-th particle.
         *
         * @param src the class
         * @param i   the index of particle
         */
        void setC(const size_t src,
                  const int i);

        /**
         * This function returns the 2D rotation matrix of the i-th particle.
         *
         * @param dst the 2D rotation matrix
         * @param i   the index of particle
         */
        void rot(dmat22& dst,
                 const int i) const;

        /**
         * This function returns the 3D rotation matrix of the i-th particle.
         *
         * @param dst the 3D rotation matrix
         * @param i   the index of particle
         */
        void rot(dmat33& dst,
                 const int i) const;

        /**
         * This function returns the translation dvector of the i-th particle.
         *
         * @param dst the translation dvector
         * @param i   the index of particle
         */
        void t(dvec2& dst,
               const int i) const;

        /**
         * This function sets the translation dvector of the i-th particle.
         *
         * @param src the translation dvector
         * @param i   the index of particle
         */
        void setT(const dvec2& src,
                  const int i);

        /**
         * This function returns the quaternion of the i-th particle.
         *
         * @param dst the quaternion
         * @param i   the index of particle
         */
        void quaternion(dvec4& dst,
                        const int i) const;

        /**
         * This function sets the quaternion of the i-th particle.
         *
         * @param src the quaternion
         * @param i the index of particle
         */
        void setQuaternion(const dvec4& src,
                           const int i);

        void d(double& d,
               const int i) const;

        void setD(const double d,
                  const int i);

        /***
        double k0() const;

        void setK0(const double k0);
        ***/

        double k1() const;

        void setK1(const double k1);

        double k2() const;

        void setK2(const double k2);

        double k3() const;

        void setK3(const double k3);

        double s0() const;

        void setS0(const double s0);

        double s1() const;

        void setS1(const double s1);

        double rho() const;

        void setRho(const double rho);

        double s() const;

        void setS(const double s);

        //void calClassDistr();
        
        void calRank1st(const ParticleType pt);

        /**
         * This function calculates the concentration paramters, including
         * rotation and translation.
         */
        void calVari(const ParticleType pt);

        void calScore();

        void perturb(const double pf,
                     const ParticleType pt);

        /**
         * This function performs a perturbation on the particles in this
         * particle filter.
         *
         * @param pf perturbation factor, which stands for the portion of
         *           confidence area of perturbation of the confidence area
         *           of the sampling points
         */
        /***
        void perturb(const double pfR,
                     const double pfT,
                     const double pfD);
        ***/

        void resample(const int n,
                      const ParticleType pt);

        /***
        void resample(const int nR,
                      const int nT,
                      const int nD);
        ***/

        // void resample();

        /**
         * This function resamples the particles in this particle filter with
         * adding a portion of global sampling points.
         *
         * @param alpha the portion of global sampling points in the resampled
         *              particles
         */
        /***
        void resample(const double alpha = 0);
        ***/

        /**
         * This function resamples the particles in this particle filter to a
         * given number of particles with adding a portion of global sampling
         * points.
         *
         * @param n     the number of sampling points of the resampled particle
         *              filter
         * @param alpha the portion of global sampling points in the resampled
         *              particles
         */
        /***
        void resample(const int n,
                      const double alpha = 0);
        ***/

        /**
         * This function returns the neff value of this particle filter, which
         * indicates the degengency of it.
         */
        /***
        double neff() const;

        void segment(const double thres);

        void flatten(const double thres);

        void sort();
        ***/

        /**
         * This function sorts all particles by their weight in a descending
         * order. It only keeps top N particles.
         *
         * @param n the number of particles to keep
         */
        //void sort(const int n);

        void sort(const int n,
                  const ParticleType pt);

        void sort(const int nC,
                  const int nR,
                  const int nT,
                  const int nD);

        void sort();

        /**
         * This function returns the index of sorting of the particles' weight
         * in a descending order.
         */
        uvec iSort(const ParticleType pt) const;

        void setPeakFactor(const ParticleType pt);
        
        void resetPeakFactor();

        void keepHalfHeightPeak(const ParticleType pt);

        bool diffTopC();

        /**
         * This function returns the difference between the most likely
         * rotations between two iterations. This function also resets the most likely
         * rotatation.
         */
        double diffTopR();

        /**
         * This function returns the difference between the most likely
         * translations between two iterations. This function also resets the
         * most likely translation.
         */
        double diffTopT();

        /**
         * This function returns the difference between the most likely
         * defocus factor between two iterations. This function also resets the
         * most likely defocus factor.
         */
        double diffTopD();

        /**
         * This function gives the most likely class.
         *
         * @param cls the most likely class
         */
        void rank1st(size_t& cls) const;
        
        /**
         * This function gives the most likely quaternion of rotation.
         *
         * @param quat the most likely quaternion of rotation
         */
        void rank1st(dvec4& quat) const;

        /**
         * This function gives the most likely rotation matrix in 2D.
         *
         * @param rot the most likely rotation matrix in 2D
         */
        void rank1st(dmat22& rot) const;

        /**
         * This function gives the most likely rotation matrix in 3D.
         *
         * @param rot the most likely rotation matrix in 3D
         */
        void rank1st(dmat33& rot) const;

        /**
         * This function gives the most likely translation vector.
         *
         * @param tran the most likely translation vector
         */
        void rank1st(dvec2& tran) const;

        /**
         * This function gives the most likely defocus factor.
         *
         * @param d the most likely defocus factor
         */
        void rank1st(double& d) const;

        /**
         * This function reports the most likely class, quaternion of rotation,
         * translation vector and defocus factor.
         * 
         * @param cls  the most likely class
         * @param quat the most likely quaternion of rotation
         * @param tran the most likely translation vector
         * @param d    the most likely defocus factor
         */
        void rank1st(size_t& cls,
                     dvec4& quat,
                     dvec2& tran,
                     double& d) const;

        /**
         * This function reports the most likely class, rotation matrix in 2D,
         * translation vector and defocus factor.
         * 
         * @param cls  the most likely class
         * @param rot  the most likely rotation matrix in 2D
         * @param tran the most likely translation vector
         * @param d    the most likely defocus factor
         */
        void rank1st(size_t& cls,
                     dmat22& rot,
                     dvec2& tran,
                     double& d) const;
        /**
         * This function reports the most likely class, rotation matrix in 3D,
         * translation vector and defocus factor.
         * 
         * @param cls  the most likely class
         * @param rot  the most likely rotation matrix in 3D
         * @param tran the most likely translation vector
         * @param d    the most likely defocus factor
         */
        void rank1st(size_t& cls,
                     dmat33& rot,
                     dvec2& tran,
                     double& d) const;

        /**
         * This function gives the class of a random particle.
         *
         * @param cls the class of a random particle
         */
        void rand(size_t& cls) const;

        /**
         * This function gives the quaternion of rotation of a random particle.
         *
         * @param quat the quaternion of rotation of a random particle
         */
        void rand(dvec4& quat) const;

        /**
         * This function gives the rotation matrix in 2D of a random particle.
         *
         * @param rot the rotation matrix in 2D of a random particle
         */
        void rand(dmat22& rot) const;

        /**
         * This function gives the rotation matrix in 3D of a random particle.
         *
         * @param rot the rotation martrix in 3D of a random particle
         */
        void rand(dmat33& rot) const;

        /**
         * This function gives the translation vector of a random particle.
         *
         * @param tran the translation vector of a random particle
         */
        void rand(dvec2& tran) const;

        /**
         * This function gives the defocus factor of a random particle.
         *
         * @param d the defocus factor
         */
        void rand(double& d) const;

        /**
         * This function gives the class, quaternion of rotation, translation
         * vector and defocus factor of a random particle.
         *
         * @param cls  the class of a random particle
         * @param quat the quaternion of rotation of a random particle
         * @param tran the translation vector of a random particle
         * @param d    the defocus factor of a random particle
         */
        void rand(size_t& cls,
                  dvec4& quat,
                  dvec2& tran,
                  double& d) const;

        /**
         * This function gives the class, rotation matrix in 2D, translation
         * dvector and defocus factor of a random particle.
         *
         * @param cls  the class of a random particle of a random particle
         * @param rot  the rotation matrix in 2D of a random particle
         * @param tran the translation vector of a random particle
         * @param d    the defocus factor of a random particle
         */
        void rand(size_t& cls,
                  dmat22& rot,
                  dvec2& tran,
                  double& d) const;

        /**
         * This function gives the class, rotation matrix in 3D, translation
         * vector and defocus factor of a random particle.
         *
         * @param cls  the class of a random particle of a random particle
         * @param rot  the rotation matrix in 3D of a random particle
         * @param tran the translation vector of a random particle
         * @param d    the defocus factor of a random particle
         */
        void rand(size_t& cls,
                  dmat33& rot,
                  dvec2& tran,
                  double& d) const;

        void shuffle(const ParticleType pt);

        /**
         * This function shuffles the sampling points.
         */
        void shuffle();

        void balanceWeight(const ParticleType pt);

        /**
         * This function will copy the content to another Particle object.
         *
         * @param that the destination object
         */
        void copy(Particle& that) const;

        /**
         * This function will copy the content to another Particle object.
         */
        Particle copy() const;
    
    private:

        /**
         * This function symmetrises the particles in this particle filter
         * according to the symmetry information. This operation will be only
         * performed in 3D mode.
         */
        void symmetrise(const dvec4* anchor = NULL);

        /**
         * This function re-centres in the translation of the particles in this
         * particle filter.
         */
        void reCentre();

        /**
         * This function clears up the content in this particle filter.
         */
        void clear();
};

/**
 * This function displays the information in this particle filter.
 *
 * @param particle the particle filter
 */
void display(const Particle& par);

/**
 * This function save this particle filter to a file.
 *
 * @param filename the file name for saving
 * @param particle the particle filter to be saved
 */
void save(const char filename[],
          const Particle& par,
          const bool saveU = false);

void save(const char filename[],
          const Particle& par,
          const ParticleType pt,
          const bool saveU = false);

/**
 * This function load a particle filter from a file.
 *
 * @param particle the particle filter to be loaded
 * @param filename the file name for loading
 */
/***
void load(Particle& particle,
          const char filename[]);
          ***/

#endif  //PARTICLE_H
