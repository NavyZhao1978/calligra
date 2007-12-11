/*
 * kis_image_alignment.h -- Part of Krita
 *
 * Copyright (c) 2007 Cyrille Berger (cberger@cberger.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _KIS_IMAGE_ALIGNMENT_H_
#define _KIS_IMAGE_ALIGNMENT_H_

#include <vector>

#include <QRect>

#include <eigen/matrix.h>

#include <kis_types.h>
#include "kis_interest_points_detector.h"


class KisImageAlignment {
    public:
        struct ImageInfo {
            KisPaintDeviceSP device;
            QRect rect;
            lInterestPoints points;
        };
        struct Result {
            double a,b,c; // Distortion parameters
            Eigen::Matrix3d homography; ///< homography parameters
        };
    public:
        KisImageAlignment(KisInterestPointsDetector* );
        ~KisImageAlignment();
        std::vector<Result> align(QList<ImageInfo> info);
    private:
        struct Private;
        Private* const d;
};


#endif
