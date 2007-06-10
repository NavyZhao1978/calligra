/* This file is part of the KDE project
   Copyright 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KSPREAD_CELL_STORAGE
#define KSPREAD_CELL_STORAGE

#include <QPair>
#include <QRect>
#include <QVector>

#include "Cell.h"
#include "Condition.h"
#include "Formula.h"
#include "Global.h"
#include "PointStorage.h"
#include "Validity.h"
#include "Value.h"

#include "database/DatabaseRange.h"

namespace KSpread
{
class Cell;
class CellStorageUndoData;
class CommentStorage;
class ConditionsStorage;
class FormulaStorage;
class FusionStorage;
class LinkStorage;
class Region;
class Sheet;
class StyleStorage;
class ValidityStorage;
class ValueStorage;

/**
 * The cell storage.
 * A wrapper around a couple of storages, which hold the cell data.
 * Provides methods to iterate over the non-empty cells.
 * Emits Damages on changes.
 * Capable of recording the old data for undoing.
 *
 * \author Stefan Nikolaus <stefan.nikolaus@kdemail.net>
 *
 * \note If you fill the storage, do it row-wise. That's more performant.
 */
class KSPREAD_EXPORT CellStorage : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * Creates an empty storage.
     */
    CellStorage( Sheet* sheet );

    /**
     * Copy constructor.
     * \note Take care: does not perform a deep copy!
     */
    CellStorage( const CellStorage& other );

    /**
     * Destructor.
     */
    ~CellStorage();

    /**
     * Removes all data at \p col , \p row .
     */
    void take( int col, int row );

    /**
     * \return the comment associated with the Cell at \p column , \p row .
     */
    QString comment( int column, int row ) const;
    void setComment( const Region& region, const QString& comment );

    /**
     * \return the conditional formattings associated with the Cell at \p column , \p row .
     */
    Conditions conditions( int column, int row ) const;
    void setConditions( const Region& region, Conditions conditions );

    /**
     * \return the database range associated with the Cell at \p column , \p row .
     */
    DatabaseRange databaseRange( int column, int row ) const;
    void setDatabaseRange( const Region& region, const DatabaseRange& databaseRange );

    /**
     * \return the formula associated with the Cell at \p column , \p row .
     */
    Formula formula( int column, int row ) const;
    void setFormula( int column, int row, const Formula& formula );

    /**
     * \return the hyperlink associated with the Cell at \p column , \p row .
     */
    QString link( int column, int row ) const;
    void setLink( int column, int row, const QString& link );

    /**
     * \return the Style associated with the Cell at \p column , \p row .
     */
    Style style( int column, int row ) const;

    /**
     * \return the Style associated with \p rect.
     */
    Style style( const QRect& rect ) const;
    void setStyle( const Region& region, const Style& style );

    /**
     * \return the user input associated with the Cell at \p column , \p row .
     */
    QString userInput( int column, int row ) const;
    void setUserInput( int column, int row, const QString& input );

    /**
     * \return the validity checks associated with the Cell at \p column , \p row .
     */
    Validity validity( int column, int row ) const;
    void setValidity( const Region& region, Validity validity );

    /**
     * \return the value associated with the Cell at \p column , \p row .
     */
    Value value( int column, int row ) const;

    /**
     * Creates a value array containing the values in \p region.
     */
    Value valueRegion( const Region& region ) const;
    void setValue( int column, int row, const Value& value );

    /**
     */
    bool doesMergeCells( int column, int row ) const;
    bool isPartOfMerged( int column, int row ) const;
    void mergeCells( int column, int row, int numXCells, int numYCells );
    Cell masterCell( int column, int row ) const;
    int mergedXCells( int column, int row ) const;
    int mergedYCells( int column, int row ) const;

    /**
     * \return \c true, if the cell's value is a matrix and obscures other cells
     */
    bool locksCells( int column, int row ) const;
    bool isLocked( int column, int row ) const;
    void lockCells( const QRect& rect );
    void unlockCells( int column, int row );
    QRect lockedCells( int column, int row ) const;

    /**
     * Insert \p number columns at \p position .
     * \return the data, that became out of range (shifted over the end)
     */
    void insertColumns( int position, int number = 1 );

    /**
     * Removes \p number columns at \p position .
     * \return the removed data
     */
    void removeColumns( int position, int number = 1 );

    /**
     * Insert \p number rows at \p position .
     * \return the data, that became out of range (shifted over the end)
     */
    void insertRows( int position, int number = 1 );

    /**
     * Removes \p number rows at \p position .
     * \return the removed data
     */
    void removeRows( int position, int number = 1 );

    /**
     * Shifts the data right of \p rect to the left by the width of \p rect .
     * The data formerly contained in \p rect becomes overridden.
     */
    void removeShiftLeft( const QRect& rect );

    /**
     * Shifts the data in and right of \p rect to the right by the width of \p rect .
     */
    void insertShiftRight( const QRect& rect );

    /**
     * Shifts the data below \p rect to the top by the height of \p rect .
     * The data formerly contained in \p rect becomes overridden.
     */
    void removeShiftUp( const QRect& rect );

    /**
     * Shifts the data in and below \p rect to the bottom by the height of \p rect .
     */
    void insertShiftDown( const QRect& rect );

    /**
     * Retrieve the first used data in \p col .
     * Can be used in conjunction with nextInColumn() to loop through a column.
     * \return the first used data in \p col or the default data, if the column is empty.
     */
    Cell firstInColumn( int col ) const;

    /**
     * Retrieve the first used data in \p row .
     * Can be used in conjunction with nextInRow() to loop through a row.
     * \return the first used data in \p row or the default data, if the row is empty.
     */
    Cell firstInRow( int row ) const;

    /**
     * Retrieve the last used data in \p col .
     * Can be used in conjunction with prevInColumn() to loop through a column.
     * \return the last used data in \p col or the default data, if the column is empty.
     */
    Cell lastInColumn( int col ) const;

    /**
     * Retrieve the last used data in \p row .
     * Can be used in conjunction with prevInRow() to loop through a row.
     * \return the last used data in \p row or the default data, if the row is empty.
     */
    Cell lastInRow( int row ) const;

    /**
     * Retrieve the next used data in \p col after \p row .
     * Can be used in conjunction with firstInColumn() to loop through a column.
     * \return the next used data in \p col or the default data, there is no further data.
     */
    Cell nextInColumn( int col, int row ) const;

    /**
     * Retrieve the next used data in \p row after \p col .
     * Can be used in conjunction with firstInRow() to loop through a row.
     * \return the next used data in \p row or the default data, if there is no further data.
     */
    Cell nextInRow( int col, int row ) const;

    /**
     * Retrieve the previous used data in \p col after \p row .
     * Can be used in conjunction with lastInColumn() to loop through a column.
     * \return the previous used data in \p col or the default data, there is no further data.
     */
    Cell prevInColumn( int col, int row ) const;

    /**
     * Retrieve the previous used data in \p row after \p col .
     * Can be used in conjunction with lastInRow() to loop through a row.
     * \return the previous used data in \p row or the default data, if there is no further data.
     */
    Cell prevInRow( int col, int row ) const;

    /**
     * The maximum occupied column, i.e. the horizontal storage dimension.
     * \return the maximum column
     */
    int columns() const;

    /**
     * The maximum occupied row, i.e. the vertical storage dimension.
     * \return the maximum row
     */
    int rows() const;

    /**
     * Creates a substorage consisting of the values in \p region.
     * \return a subset of the storage stripped down to the values in \p region
     */
    CellStorage subStorage( const Region& region ) const;

    const CommentStorage* commentStorage() const;
    const ConditionsStorage* conditionsStorage() const;
    const FormulaStorage* formulaStorage() const;
    const FusionStorage* fusionStorage() const;
    const LinkStorage* linkStorage() const;
    StyleStorage* styleStorage() const;
    const ValidityStorage* validityStorage() const;
    const ValueStorage* valueStorage() const;

    /**
     * Start the undo recording.
     * While recording the undo data of each storage operation is saved in
     * the undo data object, that is returned when the recording is stopped.
     * \see stopUndoRecording
     * \see undo
     */
    void startUndoRecording();

    /**
     * Stops the undo recording and return the undo data object.
     * If no undo data occurred, it returns null.
     * \see startUndoRecording
     * \see undo
     * \return the undo object
     */
    CellStorageUndoData* stopUndoRecording();

    /**
     * Reapplies the data stored in \p undoData.
     * \see startUndoRecording
     * \see stopUndoRecording
     */
    void undo( CellStorageUndoData* undoData );

private:
    // do not allow assignment
    CellStorage& operator=( const CellStorage& );

    class Private;
    Private * const d;
};


class FormulaStorage : public PointStorage<Formula>
{
public:
    FormulaStorage& operator=( const PointStorage<Formula>& o )
    {
        PointStorage<Formula>::operator=( o );
        return *this;
    }
};

class UserInputStorage : public PointStorage<QString>
{
public:
    UserInputStorage& operator=( const PointStorage<QString>& o )
    {
        PointStorage<QString>::operator=( o );
        return *this;
    }
};

class LinkStorage : public PointStorage<QString>
{
public:
    LinkStorage& operator=( const PointStorage<QString>& o )
    {
        PointStorage<QString>::operator=( o );
        return *this;
    }
};

class ValueStorage : public PointStorage<Value>
{
public:
    ValueStorage()
        : PointStorage<Value>()
    {
    }

    ValueStorage( const PointStorage<Value>& o )
        : PointStorage<Value>( o )
    {
    }

    ValueStorage& operator=( const PointStorage<Value>& o )
    {
        PointStorage<Value>::operator=( o );
        return *this;
    }
};

} // namespace KSpread

#endif // KSPREAD_CELL_STORAGE
