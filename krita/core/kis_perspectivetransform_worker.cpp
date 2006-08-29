/*
 * This file is part of Krita
 *
 *  Copyright (c) 2006 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_perspectivetransform_worker.h"

#include "kis_iterators_pixel.h"
#include "kis_paint_device.h"
#include "kis_perspective_math.h"
#include "kis_progress_display_interface.h"
#include "kis_random_sub_accessor.h"
#include "kis_selection.h"

#if 1

#include <iostream.h>
#include <stdlib.h>
#include <math.h>
//#define NDEBUG // uncomment to remove checking of assert()
#include <assert.h>
#define DEFAULT_ALLOC 2

namespace math { // TODO: use eigen

template <class ElType> class matrix;

template <class ElType>
class vector
{
    public:
        friend class matrix<ElType>;
     ElType * data;
     int len;
     int length()const;
     vector();
     vector(int n);
     ~vector(){ delete [] data;}
     //Copy operator
     vector(const vector<ElType>& v) ;
     //assignment operator
    vector<ElType>& operator =(const vector<ElType> &original);
    ElType& operator[](int i)const  ;
    void zero();
    vector<ElType> operator+(const vector<ElType>& v);
    vector<ElType> operator-(const vector<ElType>&v);
    void  rprint()const;  //print entries on a single line
    void resize(int n);
    int operator==(const vector<ElType>& v)const;
    friend   vector<ElType> operator*(ElType c,vector<ElType>& v );
    friend   vector<ElType> operator*(vector<ElType>& v,ElType c );
    friend ostream& operator<<(ostream& s,vector<ElType>& v);
};
template <class ElType>
void vector<ElType>::zero()
{
    for(int i=0;i<len;i++) data[i]=(ElType)0;
}
template <class ElType>
int vector<ElType>::length()const
{
    return len;
}
template <class ElType>
ElType& vector<ElType>::operator[](int i)const
{
    assert(i>=0 && i < len);
    return data[i];
}

template <class ElType>
vector<ElType>::vector()
{
    data=new ElType[ DEFAULT_ALLOC];
    assert(data!=0);
    len=  DEFAULT_ALLOC;
}
template <class ElType>
vector<ElType>::vector(int n)
{
    data = new ElType[len=n];
    assert(data!=0);
}
template <class ElType>
vector<ElType>::vector(const vector<ElType>& v)
{
    data=new ElType[len=v.len];
    assert(data!=0);
    for(int i=0;i<len;i++) data[i]=v.data[i];
}
template <class ElType>
vector<ElType>& vector<ElType>::operator =(const vector<ElType> &original)
{
        if(this != &original)
        {
            delete [] data;
            data= new ElType[len=original.len];
            assert(data!=0);
            for(int i=0;i<len;i++) data[i]=original.data[i];
        }
        return *this;
  }
template <class ElType>
vector<ElType> vector<ElType>::operator+(const vector<ElType>& v)
{
    vector<ElType> sum(len);
    for(int i=0;i<len;i++) sum[i] = data[i]+v.data[i];
    return sum;

}
template <class ElType>
vector<ElType> vector<ElType>::operator-(const vector<ElType>& v)
{
        vector<ElType> sum(len);
        for(int i=0;i<len;i++) sum[i] = data[i]-v.data[i];
        return sum;
}
template <class ElType>
void  vector<ElType>::rprint()const  //print entries on a single line
{
        int i;
        cout << "VECTOR: ";
        cout << "(";
        for(i=0;i<len-1;i++) cout << data[i] << ",";
        cout << data[len-1] << ")" << endl;
        return;
}
template <class ElType>
void vector<ElType>::resize(int n)
{
        delete[]data;
        data = new ElType[len=n];
        assert(data !=0);
}
template <class ElType>
int vector<ElType>::operator==(const vector<ElType>& v)const
{
        if(len != v.len) return 0;
        for(int i=0;i<len;i++) if(data[i]!=v.data[i]) return 0;
        return 1;
}
template <class ElType>
vector<ElType> operator*(ElType c,vector<ElType>& v )
{
        vector<ElType> ans(v.len);
        for(int i=0;i<v.len;i++) ans[i]=c*v[i];
        return ans;
}
template <class ElType>
vector<ElType> operator*(vector<ElType>& v,ElType c )
    {
        vector<ElType> ans(v.len);
        for(int i=0;i<v.len;i++) ans[i]=c*v[i];
        return ans;
    }
template <class ElType>
ostream& operator<<(ostream& s,vector<ElType>& v)
{
      s << "(";
    for(int i=0;i<v.len-1;i++) s << v.data[i] << ", ";
    s << v.data[v.len-1]<<")"<<endl;
    return s;
}

template <class ElType>
class matrix
{
    public:
        vector<ElType> *m;
    int rows,cols;
    matrix();
    matrix( int r, int c);
    matrix(const matrix<ElType> &s);
    ~matrix();
    matrix& operator =(const matrix<ElType>& s);
    vector<ElType>& operator[](const int i);
    vector<ElType> operator*(const vector<ElType>&);
    friend matrix<ElType>  operator*(const ElType&, const matrix<ElType>&);
    friend matrix<ElType> operator*(const matrix<ElType>&, const ElType&);
    matrix<ElType> operator*(const matrix<ElType>& a);
    matrix<ElType> operator+(const matrix<ElType>& a);
    matrix<ElType> operator-(const matrix<ElType>& a);
    matrix<ElType> transpose();
    //matrix<ElType> inverse();
    friend ostream& operator<<(ostream& s,matrix<ElType>& m);
    friend void ludcmp(matrix<ElType>& a,vector<int>& indx,double &d);
    friend void lubksb(matrix<ElType>&a,vector<int>& indx,vector<ElType>&b);
};
template <class ElType>
matrix<ElType>::matrix()
{
    m = new vector<ElType>[DEFAULT_ALLOC];
    assert(m !=0);
    rows=cols=DEFAULT_ALLOC;
    for(int i=0;i<rows;i++)
    {
        vector<ElType> v;
        m[i]= v;
    }
}

template <class ElType>
matrix<ElType>::matrix(int r, int c)
{
    m= new vector<ElType>[r];
    assert(m != 0);
    rows=r;
    cols=c;
    for(int i=0;i<r;i++)
    {
        vector<ElType> v(cols);
        m[i]=v;
    }
}
template <class ElType>
matrix<ElType>::matrix(const matrix<ElType> &s)
{
    int i;
    rows=s.rows;
    m = new vector<ElType>[rows];
    assert(m!=0);
    cols =s.cols;
    for(i=0;i<rows;i++)
    {
      m[i]=s.m[i];
    }
}
template <class ElType>
matrix<ElType>::~matrix()
{
    delete [] m;
}

template <class ElType>
matrix<ElType>& matrix<ElType>::operator =(const matrix<ElType> &s)
{
    if(this != &s)
    {
        delete []m;
        rows= s.rows;
        cols=s.cols;
        m = new vector<ElType>[rows];
        assert(m !=0);
        for(int i=0;i<rows;i++) m[i]=s.m[i];
    }
    return *this;
}
template <class ElType>
vector<ElType>& matrix<ElType>::operator[](const int i)
{
    assert(i>=0 && i < rows);
    return m[i];
}
template <class ElType>
vector<ElType> matrix<ElType>::operator*(const vector<ElType>& v)
{
    int i,j;
    assert(cols == v.len);
    vector<ElType> ans(rows);
    for(i=0;i<rows;i++)
    {
        ans.data[i]=0.0;
        for(j=0;j<cols;j++) ans.data[i] += m[i][j]*v.data[j];
    }
    return ans;
}
template <class ElType>
matrix<ElType> operator*(const ElType& x,const matrix<ElType>& s)
{
    matrix<ElType> ans(s.rows,s.cols);
    for(int i=0;i<ans.rows;i++)
      {
        ans.m[i]= x*s.m[i];
      }
    return ans;
}
template <class ElType>
matrix<ElType> matrix<ElType>::transpose()
{
  matrix<ElType> ans(cols,rows);
  for(int i=0;i<rows;i++)
    {
      for(int j=0;j<cols;j++) ans[j][i]=m[i][j];
      }
     return ans;
}
template <class ElType>
matrix<ElType> operator*(const matrix<ElType>& s,const ElType& x)
{
    matrix<ElType> ans(s.rows,s.cols);
    for(int i=0;i<ans.rows;i++)
      {
        ans.m[i]= x*s.m[i];
      }
    return ans;
}
template <class ElType>
matrix<ElType>  matrix<ElType> ::operator*(const matrix<ElType>&  a)
{

    assert(cols == a.rows);

    matrix<ElType>  ans(rows,a.cols);
    for(int i=0;i<rows;i++)
    {
        for(int j=0;j<a.cols;j++)
        {
            ans.m[i][j]=0.0;
            for(int k=0;k<cols;k++)
            {
                ans.m[i][j] += m[i][k]*a.m[k][j];
            }
        }
    }
    return ans;
}
template <class ElType>
matrix<ElType>  matrix<ElType> ::operator+(const matrix<ElType> & a)
{
    int i,j;

    assert(rows== a.rows);
    assert(cols== a.cols);

    matrix<ElType>  ans(a.rows,a.cols);
    for(i=0;i<a.rows;i++)
    {
        for(j=0;j<a.cols;j++)
          {
            ans.m[i][j] = m[i][j] + a.m[i][j];  //faster than assigning vectors?
        }
    }
    return ans;
}
template <class ElType>
matrix<ElType> matrix<ElType>::operator-(const matrix<ElType>& a)
{
    int i,j;
    assert(rows == a.rows);
    assert(cols == a.cols);
    matrix ans(rows,cols);
    for(i=0;i<rows;i++)
    {
        for(j=0;j<cols;j++)
        ans.m[i][j] = m[i][j] - a.m[i][j];
    }
    return ans;
}
template <class ElType>
ostream& operator<<(ostream& s,matrix<ElType>& m)
{
    for(int i=0; i<m.rows;i++) s << m[i];
    return s;
}

#define TINY 1.0e-20;
//we assume fabs(ElType) is defined
//assignment of doubles to ElType is defined
template <class ElType>
void ludcmp(matrix<ElType>& a, vector<int>& indx,double& d)
{
    int i,imax,j,k;
    ElType  big,dum,sum,temp;
    int n=a.rows;
    vector<ElType> vv(n);
    assert(a.rows == a.cols);
    d=1.0;
    for (i=0;i<n;i++)
    {
        big=0.0;
        kdDebug() << "new search" << endl;
        for (j=0;j<n;j++) { if ((temp=fabs(a[i][j])) > big) big=temp;
            kdDebug() << temp << " " << fabs(a[i][j]) << " "<< big <<endl; }
        if (big == 0.0) { cerr << "Singular matrix in routine LUDCMP" << endl; big = TINY;}
        vv[i]=1.0/big;
    }
    for (j=0;j<n;j++)
    {
        for (i=0;i<j;i++)
        {
            sum=a[i][j];
            for (k=0;k<i;k++) sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
        }
        big=0.0;
        for (i=j;i<n;i++)
        {
            sum=a[i][j];
            for (k=0;k<j;k++) sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
            if ( (dum=vv[i]*fabs(sum)) >= big)
            {
                big=dum;
                imax=i;
            }
        }
        if (j != imax)
        {
            for (k=0;k<n;k++)
            {
                dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            d = -(d);
            vv[imax]=vv[j];
        }
        indx[j]=imax;
        if (a[j][j] == 0.0) a[j][j]=TINY;
        if (j != n-1) {
            dum=1.0/(a[j][j]);
            for (i=j+1;i<n;i++) a[i][j] *= dum;
        }
    }
}
#undef TINY
template <class ElType>
void lubksb(matrix<ElType>& a,vector<int>& indx,vector<ElType>& b)
{
    int i,ip,j;
    ElType sum;
    int n=a.rows;
    for (i=0;i<n;i++)
    {
        ip=indx[i];
        sum=b[ip];
        b[ip]=b[i];
        for (j=0;j<=i-1;j++) sum -= a[i][j]*b[j];
        b[i]=sum;
    }
    for (i=n-1;i>=0;i--)
    {
        sum=b[i];
        for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
        b[i]=sum/a[i][i];
    }
}



}
#endif

KisPerspectiveTransformWorker::KisPerspectiveTransformWorker(KisPaintDeviceSP dev, const KisPoint& topLeft, const KisPoint& topRight, const KisPoint& bottomLeft, const KisPoint& bottomRight, KisProgressDisplayInterface *progress)
    : KisProgressSubject(), m_dev(dev), m_cancelRequested(false), m_progress(progress)

{
    QRect m_r;
    if(m_dev->hasSelection())
        m_r = m_dev->selection()->selectedExactRect();
    else
        m_r = m_dev->exactBounds();
/*    if(m_dev->hasSelection())
        m_dev->selection()->clear();*/
    kdDebug() << "r = " << m_r << endl;

    math::matrix<double> a(10,10);
    math::vector<double> b(10);
    math::vector<int> indx(10);
    double d = 0.;
    for(int i = 0; i <= 9; i++)
    {
        for(int j = 0; j <= 9; j++)
        {
            a[i][j] = 0.;
        }
        b[i] = 0.;
        indx[i] = 0;
    }

        // topLeft
    a[0][0] = topLeft.x();
    a[0][1] = topLeft.y();
    a[0][2] = 1;
    a[0][6] = -m_r.left() * topLeft.x();
    a[0][7] = -m_r.left() * topLeft.y();
    a[0][8] = -m_r.left();
    a[1][3] = topLeft.x();
    a[1][4] = topLeft.y();
    a[1][5] = 1;
    a[1][6] = -m_r.top() * topLeft.x();
    a[1][7] = -m_r.top() * topLeft.y();
    a[1][8] = -m_r.top();
        // topRight
    a[2][0] = topRight.x();
    a[2][1] = topRight.y();
    a[2][2] = 1;
    a[2][6] = -m_r.right() * topRight.x();
    a[2][7] = -m_r.right() * topRight.y();
    a[2][8] = -m_r.right();
    a[3][3] = topRight.x();
    a[3][4] = topRight.y();
    a[3][5] = 1;
    a[3][6] = -m_r.top() * topRight.x();
    a[3][7] = -m_r.top() * topRight.y();
    a[3][8] = -m_r.top();
        // bottomLeft
    a[4][0] = bottomLeft.x();
    a[4][1] = bottomLeft.y();
    a[4][2] = 1;
    a[4][6] = -m_r.left() * bottomLeft.x();
    a[4][7] = -m_r.left() * bottomLeft.y();
    a[4][8] = -m_r.left();
    a[5][3] = bottomLeft.x();
    a[5][4] = bottomLeft.y();
    a[5][5] = 1;
    a[5][6] = -m_r.bottom() * bottomLeft.x();
    a[5][7] = -m_r.bottom() * bottomLeft.y();
    a[5][8] = -m_r.bottom();
        // bottomRight
    a[6][0] = bottomRight.x();
    a[6][1] = bottomRight.y();
    a[6][2] = 1;
    a[6][6] = -m_r.right() * bottomRight.x();
    a[6][7] = -m_r.right() * bottomRight.y();
    a[6][8] = -m_r.right();
    a[7][3] = bottomRight.x();
    a[7][4] = bottomRight.y();
    a[7][5] = 1;
    a[7][6] = -m_r.bottom() * bottomRight.x();
    a[7][7] = -m_r.bottom() * bottomRight.y();
    a[7][8] = -m_r.bottom();
    a[8][8] = 1;
    b[8] = 1;
//     kdDebug() << " a := { { " << a[0][0] << " , " << a[0][1] << " , " << a[0][2] << " , " << a[0][3] << " , " << a[0][4] << " , " << a[0][5] << " , " << a[0][6] << " , " << a[0][7] << " , " << a[0][8] << " } , { " << a[1][0] << " , " << a[1][1] << " , " << a[1][2] << " , " << a[1][3] << " , " << a[1][4] << " , " << a[1][5] << " , " << a[1][6] << " , " << a[1][7] << " , " << a[1][8] << " } , { " << a[2][0] << " , " << a[2][1] << " , " << a[2][2] << " , " << a[2][3] << " , " << a[2][4] << " , " << a[2][5] << " , " << a[2][6] << " , " << a[2][7] << " , " << a[2][8] << " } , { " << a[3][0] << " , " << a[3][1] << " , " << a[3][2] << " , " << a[3][3] << " , " << a[3][4] << " , " << a[3][5] << " , " << a[3][6] << " , " << a[3][7] << " , " << a[3][8] << " } , { " << a[4][0] << " , " << a[4][1] << " , " << a[4][2] << " , " << a[4][3] << " , " << a[4][4] << " , " << a[4][5] << " , " << a[4][6] << " , " << a[4][7] << " , " << a[4][8] << " } , { " << a[5][0] << " , " << a[5][1] << " , " << a[5][2] << " , " << a[5][3] << " , " << a[5][4] << " , " << a[5][5] << " , " << a[5][6] << " , " << a[5][7] << " , " << a[5][8] << " } , { " << a[6][0] << " , " << a[6][1] << " , " << a[6][2] << " , " << a[6][3] << " , " << a[6][4] << " , " << a[6][5] << " , " << a[6][6] << " , " << a[6][7] << " , " << a[6][8] << " } , { "<< a[7][0] << " , " << a[7][1] << " , " << a[7][2] << " , " << a[7][3] << " , " << a[7][4] << " , " << a[7][5] << " , " << a[7][6] << " , " << a[7][7] << " , " << a[7][8] << " } , { "<< a[8][0] << " , " << a[8][1] << " , " << a[8][2] << " , " << a[8][3] << " , " << a[8][4] << " , " << a[8][5] << " , " << a[8][6] << " , " << a[8][7] << " , " << a[8][8] << " } }; " << endl;
    math::ludcmp<double>(a,indx,d);
    math::lubksb<double>(a,indx,b);

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            kdDebug() << "sol[" << 3*i+j << "]=" << b[3*i+j] << endl;
            m_matrix[i][j] = b[3*i+j];
        }
    }
    kdDebug() << "r = " << m_r << endl;
}


KisPerspectiveTransformWorker::~KisPerspectiveTransformWorker()
{
}

double norm2(const KisPoint& p)
{
    return sqrt(p.x() * p.x() + p.y() * p.y() );
}

void KisPerspectiveTransformWorker::run()
{
    kdDebug() << "r = " << m_r << endl;
    
    //TODO: understand why my caching of the rect didn't work...
    if(m_dev->hasSelection())
        m_r = m_dev->selection()->selectedExactRect();
    else
        m_r = m_dev->exactBounds();
    KisColorSpace * cs = m_dev->colorSpace();
    Q_UINT8 pixelSize = m_dev->pixelSize();
    
    if(m_dev->hasSelection())
        m_dev->selection()->clear();
    
    kdDebug() << "r = " << m_r << endl;
    KisRectIteratorPixel dstIt = m_dev->createRectIterator(m_r.x(), m_r.y(), m_r.width(), m_r.height(), true); 
    KisPaintDeviceSP srcdev = new KisPaintDevice(*m_dev.data());
    { // ensure that the random sub accessor is deleted first
        KisRandomSubAccessorPixel srcAcc = srcdev->createRandomSubAccessor();
        // Initialise progress
        if(m_progress)
            m_progress->setSubject(this, true, true);
        m_lastProgressReport = 0;
        m_progressStep = 0;
        m_progressTotalSteps = m_r.width() * m_r.height();
        //Action
        while(!dstIt.isDone())
        {
            if(dstIt.isSelected())
            {
                KisPoint p;
                double sf = ( dstIt.x() * m_matrix[2][0] + dstIt.y() * m_matrix[2][1] + 1.0);
                sf = (sf == 0.) ? 1. : 1./sf;
                p.setX( ( dstIt.x() * m_matrix[0][0] + dstIt.y() * m_matrix[0][1] + m_matrix[0][2] ) * sf );
                p.setY( ( dstIt.x() * m_matrix[1][0] + dstIt.y() * m_matrix[1][1] + m_matrix[1][2] ) * sf );
    
                srcAcc.moveTo( p );
                srcAcc.sampledOldRawData( dstIt.rawData() );
                // TODO: Should set alpha = alpha*(1-selectedness)
                cs->setAlpha( dstIt.rawData(), 255, 1);
            } else {
                cs->setAlpha( dstIt.rawData(), 0, 1);
            }
            m_progressStep ++;
            if(m_lastProgressReport != (m_progressStep * 100) / m_progressTotalSteps)
            {
                m_lastProgressReport = (m_progressStep * 100) / m_progressTotalSteps;
                emit notifyProgress(m_lastProgressReport);
            }
            if (m_cancelRequested) {
                break;
            }
            ++dstIt;
        }
    }
}
