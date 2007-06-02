/*
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
#include "dynamicbrush.h"

#include <kactioncollection.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <kis_paintop_registry.h>
#include <kis_view2.h>

#include "kis_dynamicop.h"
#include "kis_dynamic_programs_editor.h"

// TEMP
#include <kis_dynamic_brush.h>
#include <kis_dynamic_brush_registry.h>
#include <kis_dynamic_program_registry.h>
#include <kis_filters_list_dynamic_program.h>
#include <kis_size_transformation.h>
#include <kis_rotation_transformation.h>
#include <kis_dynamic_sensor.h>
// TEMP

typedef KGenericFactory<DynamicBrush> DynamicBrushFactory;
K_EXPORT_COMPONENT_FACTORY(kritadynamicbrushpaintop, DynamicBrushFactory("kritacore"))

DynamicBrush::DynamicBrush(QObject *parent, const QStringList &)
    : KParts::Plugin(parent)
{
    setComponentData(DynamicBrushFactory::componentData());

    // This is not a gui plugin; only load it when the doc is created.
    KisPaintOpRegistry *r = KisPaintOpRegistry::instance();
    r->add (new KisDynamicOpFactory);
    
    {
        // TODO: remove this, temp stuff for testing only
        {
            KisFiltersListDynamicProgram* programSpeed = new KisFiltersListDynamicProgram("speed");
            programSpeed->appendTransformation( new KisSizeTransformation(KisDynamicSensor::id2Sensor("speed"), KisDynamicSensor::id2Sensor("speed") ) );
            KisDynamicProgramRegistry::instance()->add( programSpeed);
        }
        {
            KisFiltersListDynamicProgram* programPressure = new KisFiltersListDynamicProgram("pressure");
            programPressure->appendTransformation( new KisSizeTransformation( KisDynamicSensor::id2Sensor("pressure"), KisDynamicSensor::id2Sensor("pressure") ) );
            KisDynamicProgramRegistry::instance()->add(  programPressure);
        }
        {
            KisFiltersListDynamicProgram* programRotation = new KisFiltersListDynamicProgram("rotation");
            programRotation->appendTransformation( new KisRotationTransformation( KisDynamicSensor::id2Sensor("drawingangle") ) );
            KisDynamicProgramRegistry::instance()->add(  programRotation);
        }
        {
            KisFiltersListDynamicProgram* programTime = new KisFiltersListDynamicProgram("time");
            programTime->appendTransformation( new KisRotationTransformation( KisDynamicSensor::id2Sensor("time") ) );
            KisDynamicProgramRegistry::instance()->add( programTime);
        }
    }
    if ( parent->inherits("KisView2") )
    {
        m_view = (KisView2*) parent;

        setXMLFile(KStandardDirs::locate("data","kritaplugins/dynamicbrush.rc"), true);

        KAction *action  = new KAction(i18n("Edit dynamic programs"), this);
        actionCollection()->addAction("EditDynamicPrograms", action );
        connect(action, SIGNAL(triggered()), this, SLOT(slotEditDynamicPrograms()));
    }

}

DynamicBrush::~DynamicBrush()
{
}

void DynamicBrush::slotEditDynamicPrograms()
{
    kDebug() << " BOUH " << endl;
    KisDynamicProgramsEditor editor(m_view);
    editor.exec();
}

#include "dynamicbrush.moc"
