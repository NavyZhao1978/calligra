/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003   Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004   Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kexiproperty.h"
#include "kexi_version.h"
#include "kexipropertybuffer.h"

#include <qstringlist.h>
#include <kdebug.h>
#include <qsizepolicy.h>
#include <qsize.h>
#include <qrect.h>
#include <qfont.h>
#include <qcursor.h>
#include <qpixmap.h>

#include <klocale.h>
#include <kglobal.h>

QT_STATIC_CONST_IMPL KexiProperty KexiProperty::null;


class SPHelper
{
	public:
	SPHelper() {
		list << "Fixed" << "Maximum" << "Minimum" << "Preferred" << "Expanding"
			<< "MinimumExpanding" << "Ignored";
	}
	~SPHelper() {;}

//	static QStringList list();
	QString valueToKey(int value);
	QSizePolicy::SizeType keyToValue(const QString &key);

	QStringList list;
//	QValueVector<QMap<Intm_v2key
};

QString
SPHelper::valueToKey(int value)
{
	switch(value)
	{
		case QSizePolicy::Fixed: return QString("Fixed");
		case QSizePolicy::Minimum: return QString("Minimum");
		case QSizePolicy::Maximum: return QString("Maximum");
		case QSizePolicy::Preferred: return QString("Preferred");
		case QSizePolicy::MinimumExpanding: return QString("MinimumExpanding");
		case QSizePolicy::Expanding: return QString("Expanding");
		case QSizePolicy::Ignored: return QString("Ignored");
		default: return QString::null;
	}
}

QSizePolicy::SizeType
SPHelper::keyToValue(const QString &key)
{
	if(key == "Fixed") return QSizePolicy::Fixed;
	if(key == "Minimum") return QSizePolicy::Minimum;
	if(key == "Maximum") return QSizePolicy::Maximum;
	if(key == "Preferred") return QSizePolicy::Preferred;
	if(key == "MinimumExpanding") return QSizePolicy::MinimumExpanding;
	if(key == "Expanding") return QSizePolicy::Expanding;
	if(key == "Ignored") return QSizePolicy::Ignored;

	return QSizePolicy::Expanding;
}

//singleton

SPHelper spHelper;

//===================================================

// Helper method for cursor
QString
valueToCursorName(int shape)
{
	switch(shape)
	{
		case Qt::ArrowCursor: return i18n("Arrow");
		case Qt::UpArrowCursor: return i18n("Up Arrow");
		case Qt::CrossCursor: return i18n("Cross");
		case Qt::WaitCursor: return i18n("Waiting");
		case Qt::IbeamCursor: return i18n("iBeam");
		case Qt::SizeVerCursor: return i18n("Size Vertical");
		case Qt::SizeHorCursor: return i18n("Size Horizontal");
		case Qt::SizeFDiagCursor: return i18n("Size Slash");
		case Qt::SizeBDiagCursor: return i18n("Size Backslash");
		case Qt::SizeAllCursor: return i18n("Size All");
		case Qt::BlankCursor: return i18n("Blank");
		case Qt::SplitVCursor: return i18n("Split Vertical");
		case Qt::SplitHCursor: return i18n("Split Horizontal");
		case Qt::PointingHandCursor: return i18n("Pointing Hand");
		case Qt::ForbiddenCursor: return i18n("Forbidden");
		case Qt::WhatsThisCursor: return i18n("What's This");
		default: return QString();
	}
}

//===================================================

KexiProperty::ListData::ListData(const QStringList& keys_, const QStringList& names_)
 : keys(keys_)
 , names(names_)
 , fixed(true)
{
}

KexiProperty::ListData::ListData()
 : fixed(true)
{
}

KexiProperty::ListData::~ListData()
{
}

//===================================================

KexiProperty::KexiProperty(const QCString &name, QVariant value, const QString &desc)
{
	m_name = name;
	m_desc = desc;
	init(value);
}

/*KexiProperty::KexiProperty(const QCString &name, const QString &value,
 const QStringList &key_list, const QStringList &name_list,
 const QString &desc)*/
KexiProperty::KexiProperty(const QCString &name, const QString &value,
	ListData* listData, const QString &desc)
{
	m_name = name;
	m_desc = desc;
	init(value);
	setListData(listData);
}

KexiProperty::KexiProperty()
{
	m_list = 0;
	init(QVariant());
}

KexiProperty::~KexiProperty()
{
	delete m_list;
	delete m_children_list;
	delete m_children_dict;
}

void KexiProperty::init(QVariant value)
{
	m_changed = false;
	m_visible = true;
	m_list = 0;
	m_autosync = cancelled;
	m_children_dict = 0;
	m_children_list = 0;
	m_parent = 0;
//	if (!m_desc.isEmpty())
//		kdDebug() << "KexiProperty::KexiProperty(): labeled property (" << m_desc << ")" << endl;


	//automatically add children if necessary
	switch(value.type())
	{
		case QVariant::Size:
		{
			QSize s = value.toSize();
			addChild( new KexiProperty("width", QVariant(s.width()), i18n("width")) );
			addChild( new KexiProperty("height", QVariant(s.height()), i18n("height")) );
			break;
		}
		case QVariant::Point:
		{
			QPoint p = value.toPoint();
			addChild( new KexiProperty("x", p.x(), i18n("x")) );
			addChild( new KexiProperty("y", p.y(), i18n("y")) );
			break;
		}
		case QVariant::Rect:
		{
			QRect r = value.toRect();
			addChild( new KexiProperty("x", r.x(), i18n("x")) );
			addChild( new KexiProperty("y", r.y(), i18n("y")) );
			addChild( new KexiProperty("width", r.width(), i18n("width")) );
			addChild( new KexiProperty("height", r.height(), i18n("height")) );
			break;
		}
		case QVariant::SizePolicy:
		{
			QSizePolicy p = value.toSizePolicy();

			ListData *listData = new ListData();
			listData->keys = spHelper.list;
			listData->names = spHelper.list;
			addChild( new KexiProperty("horSizeType", spHelper.valueToKey(p.horData()),
				listData, i18n("horSizeType")) );

			addChild( new KexiProperty("verSizeType", spHelper.valueToKey(p.verData()), 
				new ListData(*listData)/*copy*/, i18n("verSizeType")) );

			addChild( new KexiProperty("hStretch", (int)p.horStretch(), i18n("hStretch") ) );
			addChild( new KexiProperty("vStretch", (int)p.verStretch(), i18n("vStretch") ) );
			break;
		}
		default:
		{
			break;
		}
	}

	if (!m_name.isEmpty())
		setValue( value, false, false );
}

KexiProperty::KexiProperty(const KexiProperty &property)
{
	*this = property;
/*	m_name = property.m_name;
	m_value = property.m_value;
	m_changed = property.m_changed;
	m_visible = property.m_visible;
	m_desc = property.m_desc;
	m_autosync = property.m_autosync;
	if (property.m_children)
		m_children = new KexiProperty::Map(*property.m_children);
	else
		m_children = 0;

	m_parent = property.m_parent;

	if(property.m_list)
		m_list = new QStringList(*(property.m_list));
	else
		m_list=0;*/
}

const KexiProperty&
KexiProperty::operator=(const KexiProperty &property)
{
	if(&property==this)
		return *this;

	if(m_list) {
		delete m_list;
		m_list=0;
	}
	if(m_children_list) {
		m_children_list->clear();
		m_children_dict->clear();
	}

	m_name = property.m_name;
	m_value = property.m_value;
	m_changed = property.m_changed;
	m_visible = property.m_visible;
	m_desc = property.m_desc;
	m_autosync = property.m_autosync;

	if (property.m_children_list) {
		//copy children
//		m_children_dict = new KexiProperty::Dict(property.m_children_dict->size());
//		m_children_dict->setAutoDelete(true);
//		m_children_list = new KexiProperty::List();
		KexiProperty::ListIterator it(*property.m_children_list);
		for (;it.current();++it) {
			addChild( new KexiProperty(*it.current()) );
		}
	} else {
		m_children_dict = 0;
		m_children_list = 0;
	}

	m_parent = property.m_parent;

	if(property.m_list) {
		m_list = new ListData();
		*m_list = *(property.m_list);
	} else {
		m_list=0;
	}
	return *this;
}

QVariant::Type KexiProperty::type() const
{
	if(m_list)
		return QVariant::StringList;
	else
		return m_value.type();
}

void KexiProperty::setValue(const QVariant &v, bool updateChildren, bool saveOldValue)
{
	if (m_name.isEmpty()) {
		kexiwarn << "KexiProperty::setValue(): COULD NOT SET value to a null property" << endl;
		return;
	}
//	kdDebug() << m_name << ": setValue('" << v.toString() << "' type=" << v.typeName() << ")" << endl;
	if (m_value.type() != v.type() && !m_value.isNull() && !v.isNull()
		 && !((m_value.type()==QVariant::Int && v.type()==QVariant::UInt) || (m_value.type()==QVariant::UInt && v.type()==QVariant::Int))) 
	{
		kexiwarn << "KexiProperty::setValue(): INCOMPATIBLE TYPES! " <<m_value.typeName() <<" and " << v.typeName() << endl;
	}

	//1. Check if the value should be changed
	bool ch;
	if (m_value.type()==QVariant::DateTime
		|| m_value.type()==QVariant::Time) {
		//for date and datetime types: compare with strings, because there
		//can be miliseconds difference
		ch = m_value.toString() != v.toString();
	}
	else if (m_value.type()==QVariant::String) {
		//property is changed for string type,
		//if one of value is empty and other isn't..
		ch = (m_value.toString().isEmpty() != v.toString().isEmpty()
		//..or both are not empty and values differ
			|| !m_value.toString().isEmpty() && !v.toString().isEmpty() && m_value != v);
	}
	else if ((m_value.type()==QVariant::Pixmap && m_value.toPixmap().isNull() && v.isNull())
			|| (v.type()==QVariant::Pixmap && v.toPixmap().isNull() && m_value.isNull()))
		return;
	else
		ch = (m_value != v);

	if (!ch)
		return;

	if (saveOldValue) {
//		if (m_value == v)
//			return;
		if (!m_changed) {
			m_oldValue = m_value; //store old
		}
//			m_changed = true;
		setChanged(true);
		m_value = v;
		if (m_parent) {
			m_parent->setChanged( true ); //inform the parent
			m_parent->updateValueForChild(m_name, m_value, saveOldValue);
		}
	}
	else {
		m_value = v;
		m_oldValue = QVariant(); //clear old
		setChanged(false);
	}

	if (!m_buf.isNull()) {
		emit m_buf->propertyChanged(*m_buf, *this);
		emit m_buf->propertyChanged();
	}

	if (!updateChildren)
		return;

	//automatically update children's value if necessary
	switch(m_value.type())
	{
		case QVariant::Size:
		{
			QSize s = m_value.toSize();
			setChildValue("width",s.width(),saveOldValue);
			setChildValue("height",s.height(),saveOldValue);
			break;
		}
		case QVariant::Point:
		{
			QPoint p = m_value.toPoint();
			setChildValue("x",p.x(),saveOldValue);
			setChildValue("y",p.y(),saveOldValue);
			break;
		}
		case QVariant::Rect:
		{
			QRect r = m_value.toRect();
			setChildValue("x",r.x(),saveOldValue);
			setChildValue("y",r.y(),saveOldValue);
			setChildValue("width",r.width(),saveOldValue);
			setChildValue("height",r.height(),saveOldValue);
			break;
		}
		case QVariant::SizePolicy:
		{
			QSizePolicy p = m_value.toSizePolicy();
			setChildValue("horSizeType",QVariant(spHelper.valueToKey(p.horData())),saveOldValue);
			setChildValue("verSizeType",QVariant(spHelper.valueToKey(p.verData())),saveOldValue);
			setChildValue("hStretch",(int)p.horStretch(),saveOldValue);
			setChildValue("vStretch",(int)p.verStretch(),saveOldValue);
			break;
		}
		default:
			break;
	}
}

void KexiProperty::setValue(const QVariant &v, bool saveOldValue)
{
	setValue(v, true, saveOldValue);
}

//void KexiProperty::setList(const QStringList &key_list, const QStringList &name_list)
void KexiProperty::setListData(ListData* listData)
{
	delete m_list;
	m_list = listData;
}

QVariant KexiProperty::value() const
{
	return m_value;
}

QString KexiProperty::valueText() const
{
	if (!m_list)
		return KexiProperty::format( m_value );
	//special case: return text
	int idx = m_list->keys.findIndex( m_value.toString() );
	if (idx<0) {
		kdWarning() << "KexiProperty::value(): NO SUCH KEY '" << m_value.toString() << "'" << endl;
		return m_value.toString();
	}
	return m_list->names[ idx ];
}

void KexiProperty::setChildValue(const QCString& childName, const QVariant &v, bool saveOldValue)
{
	KexiProperty * prop = child(childName);
	if (!prop) {
		kdWarning()<< "KexiProperty::setChildValue() NO SUCH CHILD: " << childName << endl;
		return;
	}
	prop->setValue(v, saveOldValue);
}

void KexiProperty::updateValueForChild(const QCString& childName,
	const QVariant &v, bool saveOldValue)
{
	debug();

	switch(m_value.type())
	{
		case QVariant::Size:
		{
			QSize s = m_value.toSize();
			if (childName=="width")
				s.setWidth(v.toInt());
			else if (childName=="height")
				s.setHeight(v.toInt());
			else
				break;
			setValue(s, false, saveOldValue);
			break;
		}
		case QVariant::Point:
		{
			QPoint p = m_value.toPoint();
			if (childName=="x")
				p.setX(v.toInt());
			else if (childName=="y")
				p.setY(v.toInt());
			else
				break;
			setValue(p, false, saveOldValue);
			break;
		}
		case QVariant::Rect:
		{
			QRect r = m_value.toRect();
			if (childName=="x")
				r.moveLeft(v.toInt());
			else if (childName=="y")
				r.moveTop(v.toInt());
			else if (childName=="width")
				r.setWidth(v.toInt());
			else if (childName=="height")
				r.setHeight(v.toInt());
			else
				break;
			setValue(r, false, saveOldValue);
			break;
		}
		case QVariant::SizePolicy:
		{
			QSizePolicy p = m_value.toSizePolicy();
			if (childName=="horSizeType")
				p.setHorData( spHelper.keyToValue(v.toString()) );
			else if (childName=="verSizeType")
				p.setVerData( spHelper.keyToValue(v.toString()) );
			else if (childName=="hStretch")
				p.setHorStretch( v.toInt() );
			else if (childName=="vStretch")
				p.setVerStretch( v.toInt() );
			else
				break;
			setValue(p, false, saveOldValue);
			break;
		}
		default:
			break;
	}
}

void KexiProperty::resetValue()
{
	if (!m_changed)
		return;
	setValue( oldValue(), false );
	if (!m_buf.isNull())
		emit m_buf->propertyReset(*m_buf, *this);
}

bool KexiProperty::changed() const
{
	return m_changed;
}

void KexiProperty::setChanged(bool set)
{
	if (m_changed==set)
		return;
	m_changed=set;
	if (!m_changed) {
		m_oldValue = QVariant();
		//if there's parent property, set it to unchanged if no child is changed
		if (m_parent && m_parent->m_children_list) {
			KexiProperty::ListIterator it(*m_parent->m_children_list);
			for (;it.current();++it) {
				if (it.current()->changed()) {
					m_parent->setChanged(true);
					return;
				}
			}
			m_parent->setChanged(false);
		}
	}else {
		m_oldValue = m_value; //store
		if (m_parent)
			m_parent->setChanged( true );
	}
}

void KexiProperty::addChild(KexiProperty *prop)
{
	if (!m_children_dict) {
		m_children_dict = new KexiProperty::Dict(53);
		m_children_dict->setAutoDelete(true);
		m_children_list = new KexiProperty::List();
	}
	m_children_dict->insert(prop->name(), prop);
	m_children_list->append( prop );
	prop->m_parent = this;
}

bool KexiProperty::isVisible() const
{
	if (m_parent) {
		if (!m_parent->isVisible())
			return false;
	}
	return m_visible;
}

KexiProperty* KexiProperty::child(const QCString& name)
{
	if (!m_children_dict)
		return 0;
	return m_children_dict->find(name);
}

QString KexiProperty::debugString() const
{
	QString dbg = "KexiProperty( name='" + QString(m_name) + "' desc='" + m_desc
		+ "' val=" + (m_value.isValid() ? m_value.toString() : "<INVALID>");
	dbg += (QString::fromLatin1("type=") + QVariant::typeToName(type()));
	if (m_list) {
		dbg += (QString::fromLatin1(" keys=[") + m_list->keys.join(",") 
			+ QString("] values=[") + m_list->names.join(",")
			+ QString("] fixed=") + (m_list->fixed?"true":"false"));
	}
	if (!m_oldValue.isValid())
		dbg += (", oldVal='" + m_oldValue.toString() + "'");
	dbg += (QString(m_changed ? " " : " un") + "changed");
	dbg += (m_visible ? " visible" : " hidden");
	dbg+=" )";
	return dbg;
}

void KexiProperty::debug()
{
	kdDebug() << debugString() << endl;
}

QString
KexiProperty::format(const QVariant &v)
{
	switch(v.type())
	{
		case QVariant::Size:
		{
			QSize s = v.toSize();
			return QString("[" + QString::number(s.width()) + "," + QString::number(s.height()) + "]");
		}
		case QVariant::Rect:
		{
			QRect r = v.toRect();
			QString x = QString::number(r.x());
			QString y = QString::number(r.y());
			QString w = QString::number(r.width());
			QString h = QString::number(r.height());

			return QString("[" + x + "," + y + "," + w + "," + h + "]");
		}
		case QVariant::Bool:
		{
			if(v.toBool())
			{
				return i18n("True");
			}

			return i18n("False");
		}
		case QVariant::Font:
		{
			QFont f = v.toFont();
			int w = f.weight();
			QString wstr;
			if (w<=QFont::Light) wstr=i18n("Light")+" ";
			else if (w<=QFont::Normal) wstr="";
			else wstr=i18n("Bold")+" ";
			if (f.italic())
				wstr += (i18n("Italic")+" ");
			return f.family() + " " + wstr + QString::number(f.pointSize());
		}
		case QVariant::Double:
		{
			return QString(KGlobal::locale()->formatNumber(v.toDouble()));
		}
		case QVariant::StringList:
		{
			return v.toStringList().join("|");
		}
		case QVariant::SizePolicy:
		{
			QSizePolicy p = v.toSizePolicy();
			return QString(spHelper.valueToKey(p.horData()) + "/" + spHelper.valueToKey(p.verData()));
		}
		case QVariant::Cursor:
		{
			QCursor c = v.toCursor();
			return valueToCursorName(c.shape());
		}
		default:
		{
			return v.toString();
		}
	}
}

// TMP
void
KexiProperty::execute(const QString &value)
{
	emit m_buf->propertyExecuted(*m_buf, *this, value);
}

