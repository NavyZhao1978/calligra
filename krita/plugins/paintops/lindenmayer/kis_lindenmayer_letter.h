#ifndef KIS_LINDENMAYER_LETTER_H
#define KIS_LINDENMAYER_LETTER_H

#include <Eigen/Core>
#include <QList>
#include <QPointF>
#include <QVariant>
#include <QHash>

class KisLindenmayerPaintOp;
class KisPaintInformation;

class KisLindenmayerLetter
{
private:
    const KisLindenmayerPaintOp* m_paintOp;
    // we don't use an enum here, because we allow custom parameters for gramer internal computations
    QHash<QString, QVariant> m_parameters;
    bool m_markedForRemoving;

    // angles are relative to the vertical axis, +90 degrees points to right, -90 to left. positive +90+180=+270 points to the left. watch out, what happens at +180 or -180

    //default parameters:
    // position, angle, length, age, startWidth, endWidth

    // computed parameters:
    // endPosition, angleToSun (points towards the mouse position), distanceToSun

    // creating copies should be done with createCopy(), because it will reset the age to 0. i don't want to do this in the operator, because it would change the meaning.
    KisLindenmayerLetter(const KisLindenmayerLetter& other);
public:
    KisLindenmayerLetter(QPointF position, float angle, const KisLindenmayerPaintOp* paintOp);

    QPointF position() const {
        return getParameter("position").toPointF();
    }

    QPointF lineEndPosition() const {
        return getComputedParameter("endPosition").toPointF();
    }

    float length() const { return getParameter("length").toFloat(); }

    QVariant getComputedParameter(QString key) const;

    QVariant getParameter(QString key) const {
        if(key == "endPosition" || key == "angleToSun" || key == "distanceToSun")
            return getComputedParameter(key);

        return m_parameters.value(key, QVariant(false));
    }

    void setParameter(QString key, QVariant parameter) {
        m_parameters.insert(key, parameter);
    }

    KisLindenmayerLetter* createCopy() const {
        KisLindenmayerLetter* newLetter = new KisLindenmayerLetter(*this);
        newLetter->setParameter("age", 0.0f);
        return newLetter;
    }

    void markForRemoving() {
        m_markedForRemoving = true;
    }

    bool markedForRemoving() const {
        return m_markedForRemoving;
    }

private:
    Eigen::Vector2f eigenPosition() const {
        QPointF qposition = position();
        return Eigen::Vector2f(qposition.x(), qposition.y());
    }
};

#endif // KIS_LINDENMAYER_LETTER_H
