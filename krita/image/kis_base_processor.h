/*
 *  Copyright (c) 2008 Boudewijn Rempt <boud@valdyas.org>
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
#ifndef _KIS_BASE_PROCESSOR_H_
#define _KIS_BASE_PROCESSOR_H_

#include <list>

#include <QString>

#include <klocale.h>

#include "KoID.h"
#include "KoColorSpace.h"

#include "kis_types.h"
#include "kis_shared.h"
#include "kis_image.h"

#include "krita_export.h"

class QWidget;

class KoUpdater;

class KisBookmarkedConfigurationManager;
class KisFilterConfiguration;
class KisFilterConfigWidget;

/**
 * Base class for classes that process areas of pixels.
 * Processors can either read in pixels and write out pixels
 * or just write out pixels, using a certain set of configuation
 * pixels.
 *
 * in-out processing is typically filtering: @see KisFilter.
 * out-only processing is typiccaly generating: @see KisGenerator.
 * 
 * Information about the area that needs to be processed is contained
 * @see KisProcessingInformation and @see KisConstProcessingInformation.
 */
class KRITAIMAGE_EXPORT KisBaseProcessor : public KisShared {

public:


    KisBaseProcessor(const KoID& id, const KoID & category, const QString & entry);

    virtual ~KisBaseProcessor();
    /**
     * Return the configuration set as the default by the user or the default 
     * configuration from the filter writer as returned by factoryConfiguration.
     *
     * This configuration is used by default for the configuration widget and 
     * given to the process function if there is no configuration widget.
     *
     * @return the default configuration of this widget
     */
    virtual KisFilterConfiguration * defaultConfiguration(const KisPaintDeviceSP) const;

    /**
     * @return the bookmark manager for this processor
     */
    KisBookmarkedConfigurationManager* bookmarkManager();

    /**
     * @return the bookmark manager for this processor
     */
    const KisBookmarkedConfigurationManager* bookmarkManager() const;

    /**
     * Used when threading is used -- the overlap margin is passed to the
     * filter to use to compute pixels, but the margin is not pasted into the
     * resulting image. Use this for convolution filters, for instance.
     */
    virtual int overlapMarginNeeded( const KisFilterConfiguration* = 0 ) const;

     /**
     * Similar to overlapMarginNeeded: some filters will alter a lot of pixels that are
     * near to each other at the same time. So when you changed a single rectangle
     * in a device, the actual rectangle that will feel the influence of this change
     * might be bigger. Use this function to determine that rect.
     * The default implementation makes a guess using overlapMarginNeeded.
      */
    virtual QRect enlargeRect(const QRect & rect, const KisFilterConfiguration* = 0) const;

    /// @return Unique identification for this processor
    QString id() const;
    
    /// @return User-visible identification for this processor
    QString name() const;

    /// @return the submenu in the filters menu does processor want to go?
    KoID menuCategory() const;

    /// @return the i18n'ed string this filter wants to show itself in the menu
    QString menuEntry() const;

    /**
     * Create the configuration widget for this processor.
     *
     * @param parent the Qt owner widget of this widget
     * @param dev the paintdevice this filter will act on
     */
    virtual KisFilterConfigWidget * createConfigurationWidget(QWidget * parent, const KisPaintDeviceSP dev, const KisImageSP image = 0) const;
    // "Support" functions
public:
    /**
     * If true, this filter can be used in painting tools as a paint operation
     */
    bool supportsPainting() const;

    /// This filter can be displayed in a preview dialog
    bool supportsPreview() const;

    /// This filter can be used in adjustment layers
    bool supportsAdjustmentLayers() const;

    /**
     * Can this filter work incrementally when painting, or do we need to work
     * on the state as it was before painting started. The former is faster.
     */
    bool supportsIncrementalPainting() const;

    /**
     * This filter supports cutting up the work area and filtering
     * each chunk in a separate thread. Filters that need access to the
     * whole area for correct computations should return false.
     */
    bool supportsThreading() const;
    
    /**
     * Determine the colorspace independence of this filter.
     * @see ColorSpaceIndependence
     *
     * @return the degree of independence
     */
    ColorSpaceIndependence colorSpaceIndependence() const;

protected:

    void setSupportsPainting(bool v);
    void setSupportsPreview(bool v);
    void setSupportsAdjustmentLayers(bool v);
    void setSupportsIncrementalPainting(bool v);
    void setSupportsThreading(bool v);
    void setColorSpaceIndependence(ColorSpaceIndependence v);
    
protected:
    
    void setBookmarkManager(KisBookmarkedConfigurationManager* );
    
    /// @return the name of config group in KConfig
    virtual QString configEntryGroup() const = 0;

    /// @return the default configuration as defined by whoever wrote the plugin
    virtual KisFilterConfiguration* factoryConfiguration(const KisPaintDeviceSP) const;

private:
    struct Private;
    Private* const d;
};


#endif
