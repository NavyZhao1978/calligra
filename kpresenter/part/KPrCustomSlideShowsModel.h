/* This file is part of the KDE project
* Copyright (C) 2011 Paul Mendez <paulestebanms@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301, USA.
*/

#ifndef KPRCUSTOMSLIDESHOWSMODEL_H
#define KPRCUSTOMSLIDESHOWSMODEL_H

#include <QAbstractListModel>
#include <QSize>

class KPrCustomSlideShows;
class KoPAPageBase;
class KPrDocument;

class KPrCustomSlideShowsModel : public QAbstractListModel
{
    Q_OBJECT
public:

    //List the actions that you could perform with slides
    //within a custom show.
    enum CustomShowActions
    {
        SLIDES_ADD = 0,        ///< add slides
        SLIDES_DELETE    = 1,  ///< delete slides
        SLIDES_MOVE     = 2,  ///< move slides
    };

    explicit KPrCustomSlideShowsModel(KPrDocument *document, QObject *parent = 0);

    virtual ~KPrCustomSlideShowsModel();

    QVariant data(const QModelIndex &index, int role) const;

    int rowCount(const QModelIndex &parent) const;

    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;

    virtual QStringList mimeTypes() const;

    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;

    virtual Qt::DropActions supportedDropActions() const;

    //virtual bool removeRows(int row, int count, const QModelIndex &parent);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    virtual bool dropMimeData (const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    /**
     * @brief Set the current KPrCustomSlideShows
     *
     * @param customShows the KPrCustomSlideShows class
     */
    void setCustomSlideShows(KPrCustomSlideShows *customShows);

    /**
     * @brief Returns the current slide show name
     *
     * @return the current slide show name
     */
    QString currentSlideShow();

    /**
     * @brief Set the current custom slide show by name
     *
     * @param name of the custom slide show
     */
    void setCurrentSlideShow(QString name);

    /**
     * @brief Set the current custom slide show by index
     *
     * @param index of the custom slide show within the KPrCustomSlideShows class
     */
    void setCurrentSlideShow(int index);

    /**
     * Setter of the size with a rect
     *
     * @param size which is a QRect
     */
    void setIconSize(QSize size);

    /**
     * @brief Returns the current slide show names list
     *
     * @return the current slide show names list
     */
    QStringList customShowsNamesList() const;

    /**
     * @brief setter of the current document
     *
     * @param the document
     */
    void setDocument(KPrDocument *document);

    /**
     * @brief updates the list of slides of a custom slide show
     *
     * @param name of the custom slide show
     * @param newCustomShow list of slides
     */
    void updateCustomShow(QString name, QList<KoPAPageBase *> newCustomShow);

    /**
     * @brief Deletes all the ocurrencies of a given list of slides from all the known custom SlideShows
     *
     * @param slideShow list of slides to be removed
     */
    void removeSlidesFromAll(QList<KoPAPageBase *> pages);

    /**
     * @brief Deletes all the ocurrencies of a given list of slides indexes from current custom SlideShows
     *
     * @param slideIndexes list of slide indexes to be removed
     */
    void removeSlidesByIndexes(QModelIndexList slideIndexes);

    /**
     * @brief Add a given list of slides to the current custom slide show
     *
     * @param pages list of slides to be added.
     * @param row the insertion will be performed before the given row
     */
    void addSlides(QList<KoPAPageBase *> pages, int row);

    /**
     * @brief Manage all edit actions on a custom slide show
     *
     * @param c_action action to be performed according to CustomShowsAction Enum
     * @param slides list of slides to be added/deleted or moved
     * @param beginRow destination row for copy and move actions
     * @return true if the action success
     */
    bool doCustomSlideShowAction(CustomShowActions c_action, QList<KoPAPageBase *> slides, int beginRow);

    /**
     * @brief Insert a new Custom SlideShow
     *
     * @param name The name of the new SlideShow
      */
    void addNewCustomShow(const QString &name);

    /**
     * @brief Replaces a custom SlideShow name with the given SlideShow name
     *
     * @param oldName The name of the SlideShow to be renamed
     * @param newName The new name of the SlideShow
     */
    void renameCustomShow(const QString &oldName, const QString &newName);

    /**
     * @brief Delete a Custom SlideShow by its name
     *
     * @param name The name of the SlideShow to be removed
     */
    void removeCustomShow(const QString &name);

    void updateCustomSlideShowsList(const QString &name);

signals:
    void customSlideShowsChanged();

public slots:

private:
    KPrCustomSlideShows *m_customSlideShows;
    QString m_currentCustomSlideShowName;
    QSize m_iconSize;
    KPrDocument *m_document;

};

#endif // KPRCUSTOMSLIDESHOWSMODEL_H
