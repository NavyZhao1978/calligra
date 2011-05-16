/* This file is part of the KDE project
 * Copyright (C) 2006-2010 Thomas Zander <zander@kde.org>
 * Copyright (C) 2008 Thorsten Zachmann <zachmann@kde.org>
 * Copyright (C) 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * Copyright (C) 2008 Pierre Stirnweiss <pierre.stirnweiss_koffice@gadz.org>
 * Copyright (C) 2009 KO GmbH <cbo@kogmbh.com>
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

#include "TextTool.h"
#include "TextEditingPluginContainer.h"
#include "dialogs/SimpleCharacterWidget.h"
#include "dialogs/SimpleParagraphWidget.h"
#include "dialogs/SimpleTableWidget.h"
#include "dialogs/SimpleStylesWidget.h"
#include "dialogs/ParagraphSettingsDialog.h"
#include "dialogs/StyleManagerDialog.h"
#include "dialogs/InsertCharacter.h"
#include "dialogs/FontDia.h"
#include "dialogs/TableDialog.h"
#include "dialogs/ChangeConfigureDialog.h"
#include "dialogs/ChangeTrackingOptionsWidget.h"
#include "dialogs/SimpleTableWidget.h"
#include "commands/TextCutCommand.h"
#include "commands/TextPasteCommand.h"
#include "commands/ChangeListCommand.h"
#include "commands/ChangeListLevelCommand.h"
#include "commands/ListItemNumberingCommand.h"
#include "commands/ShowChangesCommand.h"
#include "commands/ChangeTrackedDeleteCommand.h"
#include "commands/DeleteCommand.h"
#include "commands/AutoResizeCommand.h"
#include "FontSizeAction.h"

#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoSelection.h>
#include <KoShapeManager.h>
#include <KoPointerEvent.h>
#include <KoColorBackground.h>
#include <KoColorPopupAction.h>
#include <KoTextDocumentLayout.h>
#include <KoParagraphStyle.h>
#include <KoTextEditingPlugin.h>
#include <KoTextEditingRegistry.h>
#include <KoInlineTextObjectManager.h>
#include <KoStyleManager.h>
#include <KoTextOdfSaveHelper.h>
#include <KoTextDrag.h>
#include <KoTextPaste.h>
#include <KoTextDocument.h>
#include <KoTextEditor.h>
#include <KoGlobal.h>
#include <KoChangeTracker.h>
#include <KoChangeTrackerElement.h>
#include <KoBookmark.h>
#include <KoBookmarkManager.h>

#include <KoTextLayoutRootArea.h>

#include <kdebug.h>
#include <KRun>
#include <KStandardShortcut>
#include <KFontChooser>
#include <KFontAction>
#include <KAction>
#include <KLocale>
#include <KStandardAction>
#include <KMimeType>
#include <KMessageBox>
#include <KUser>
#include <QTabWidget>
#include <QTextDocumentFragment>
#include <QToolTip>

#include <rdf/KoDocumentRdfBase.h>

static bool hit(const QKeySequence &input, KStandardShortcut::StandardShortcut shortcut)
{
    foreach (const QKeySequence & ks, KStandardShortcut::shortcut(shortcut).toList()) {
        if (input == ks)
            return true;
    }
    return false;
}

TextTool::TextTool(KoCanvasBase *canvas)
        : KoToolBase(canvas),
        m_textShape(0),
        m_textShapeData(0),
        m_changeTracker(0),
        m_allowActions(true),
        m_allowAddUndoCommand(true),
        m_trackChanges(false),
        m_allowResourceManagerUpdates(true),
        m_prevCursorPosition(-1),
        m_caretTimer(this),
        m_caretTimerState(true)
        , m_currentCommand(0),
        m_currentCommandHasChildren(false),
        m_specialCharacterDocker(0),
        m_textTyping(false),
        m_textDeleting(false),
        m_changeTipTimer(this),
        m_changeTipCursorPos(0)
        , m_delayedEnsureVisible(false)
{
    setTextMode(true);

    createActions();

    m_textEditingPlugins = canvas->resourceManager()->
        resource(TextEditingPluginContainer::ResourceId).value<TextEditingPluginContainer*>();
    if (m_textEditingPlugins == 0) {
        m_textEditingPlugins = new TextEditingPluginContainer(canvas->resourceManager());
        QVariant variant;
        variant.setValue(m_textEditingPlugins);
        canvas->resourceManager()->setResource(TextEditingPluginContainer::ResourceId, variant);
    }

    foreach (KoTextEditingPlugin* plugin, m_textEditingPlugins->values()) {
        connect(plugin, SIGNAL(startMacro(const QString &)),
                this, SLOT(startMacro(const QString &)));
        connect(plugin, SIGNAL(stopMacro()), this, SLOT(stopMacro()));
        QHash<QString, KAction*> actions = plugin->actions();
        QHash<QString, KAction*>::iterator i = actions.begin();
        while (i != actions.end()) {
            addAction(i.key(), i.value());
            ++i;
        }
    }

    // setup the context list.
    QSignalMapper *signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(startTextEditingPlugin(QString)));
    QList<QAction*> list;
    list.append(this->action("text_default"));
    list.append(this->action("format_font"));
    foreach (const QString &key, KoTextEditingRegistry::instance()->keys()) {
        KoTextEditingFactory *factory =  KoTextEditingRegistry::instance()->value(key);
        if (factory->showInMenu()) {
            KAction *a = new KAction(i18n("Apply %1", factory->title()), this);
            connect(a, SIGNAL(triggered()), signalMapper, SLOT(map()));
            signalMapper->setMapping(a, factory->id());
            list.append(a);
            addAction(QString("apply_%1").arg(factory->id()), a);
        }
    }
    setPopupActionList(list);

    connect(canvas->shapeManager()->selection(), SIGNAL(selectionChanged()), this, SLOT(shapeAddedToCanvas()));

    m_caretTimer.setInterval(500);
    connect(&m_caretTimer, SIGNAL(timeout()), this, SLOT(blinkCaret()));

    m_changeTipTimer.setInterval(500);
    m_changeTipTimer.setSingleShot(true);
    connect(&m_changeTipTimer, SIGNAL(timeout()), this, SLOT(showChangeTip()));
}

void TextTool::createActions()
{
    m_actionFormatBold  = new KAction(KIcon("format-text-bold"), i18n("Bold"), this);
    addAction("format_bold", m_actionFormatBold);
    m_actionFormatBold->setShortcut(Qt::CTRL + Qt::Key_B);
    m_actionFormatBold->setCheckable(true);
    connect(m_actionFormatBold, SIGNAL(triggered(bool)), this, SLOT(bold(bool)));

    m_actionFormatItalic  = new KAction(KIcon("format-text-italic"), i18n("Italic"), this);
    addAction("format_italic", m_actionFormatItalic);
    m_actionFormatItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    m_actionFormatItalic->setCheckable(true);
    connect(m_actionFormatItalic, SIGNAL(triggered(bool)), this, SLOT(italic(bool)));

    m_actionFormatUnderline  = new KAction(KIcon("format-text-underline"), i18nc("Text formatting", "Underline"), this);
    addAction("format_underline", m_actionFormatUnderline);
    m_actionFormatUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    m_actionFormatUnderline->setCheckable(true);
    connect(m_actionFormatUnderline, SIGNAL(triggered(bool)), this, SLOT(underline(bool)));

    m_actionFormatStrikeOut  = new KAction(KIcon("format-text-strikethrough"), i18n("Strike Out"), this);
    addAction("format_strike", m_actionFormatStrikeOut);
    m_actionFormatStrikeOut->setCheckable(true);
    connect(m_actionFormatStrikeOut, SIGNAL(triggered(bool)), this, SLOT(strikeOut(bool)));

    QActionGroup *alignmentGroup = new QActionGroup(this);
    m_actionAlignLeft  = new KAction(KIcon("format-justify-left"), i18n("Align Left"), this);
    addAction("format_alignleft", m_actionAlignLeft);
    m_actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    m_actionAlignLeft->setCheckable(true);
    alignmentGroup->addAction(m_actionAlignLeft);
    connect(m_actionAlignLeft, SIGNAL(triggered(bool)), this, SLOT(alignLeft()));

    m_actionAlignRight  = new KAction(KIcon("format-justify-right"), i18n("Align Right"), this);
    addAction("format_alignright", m_actionAlignRight);
    m_actionAlignRight->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_R);
    m_actionAlignRight->setCheckable(true);
    alignmentGroup->addAction(m_actionAlignRight);
    connect(m_actionAlignRight, SIGNAL(triggered(bool)), this, SLOT(alignRight()));

    m_actionAlignCenter  = new KAction(KIcon("format-justify-center"), i18n("Align Center"), this);
    addAction("format_aligncenter", m_actionAlignCenter);
    m_actionAlignCenter->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_C);
    m_actionAlignCenter->setCheckable(true);
    alignmentGroup->addAction(m_actionAlignCenter);
    connect(m_actionAlignCenter, SIGNAL(triggered(bool)), this, SLOT(alignCenter()));

    m_actionAlignBlock  = new KAction(KIcon("format-justify-fill"), i18n("Align Block"), this);
    addAction("format_alignblock", m_actionAlignBlock);
    m_actionAlignBlock->setShortcut(Qt::CTRL + Qt::ALT + Qt::Key_R);
    m_actionAlignBlock->setCheckable(true);
    alignmentGroup->addAction(m_actionAlignBlock);
    connect(m_actionAlignBlock, SIGNAL(triggered(bool)), this, SLOT(alignBlock()));

    m_actionFormatSuper = new KAction(KIcon("format-text-superscript"), i18n("Superscript"), this);
    addAction("format_super", m_actionFormatSuper);
    m_actionFormatSuper->setCheckable(true);
    connect(m_actionFormatSuper, SIGNAL(triggered(bool)), this, SLOT(superScript(bool)));

    m_actionFormatSub = new KAction(KIcon("format-text-subscript"), i18n("Subscript"), this);
    addAction("format_sub", m_actionFormatSub);
    m_actionFormatSub->setCheckable(true);
    connect(m_actionFormatSub, SIGNAL(triggered(bool)), this, SLOT(subScript(bool)));

    KAction *action = new KAction(
        KIcon(QApplication::isRightToLeft() ? "format-indent-less" : "format-indent-more"),
        i18n("Increase Indent"), this);
    addAction("format_increaseindent", action);
    connect(action, SIGNAL(triggered()), this, SLOT(increaseIndent()));

    m_actionFormatDecreaseIndent = new KAction(
        KIcon(QApplication::isRightToLeft() ? "format-indent-more" : "format-indent-less"),
        i18n("Decrease Indent"), this);
    addAction("format_decreaseindent", m_actionFormatDecreaseIndent);
    connect(m_actionFormatDecreaseIndent, SIGNAL(triggered()), this, SLOT(decreaseIndent()));

    action = new KAction(KIcon("format-list-unordered"),  i18n("Bullet list"), this);
    addAction("format_bulletlist", action);

    action = new KAction(KIcon("format-list-ordered"),  i18n("Numbered list"), this);
    addAction("format_numberlist", action);

    action = new KAction(i18n("Increase Font Size"), this);
    action->setShortcut(Qt::CTRL + Qt::Key_Greater);
    addAction("fontsizeup", action);
    connect(action, SIGNAL(triggered()), this, SLOT(increaseFontSize()));

    action = new KAction(i18n("Decrease Font Size"), this);
    action->setShortcut(Qt::CTRL + Qt::Key_Less);
    addAction("fontsizedown", action);
    connect(action, SIGNAL(triggered()), this, SLOT(decreaseFontSize()));

    m_actionFormatFontFamily = new KFontAction(KFontChooser::SmoothScalableFonts, this);
    addAction("format_fontfamily", m_actionFormatFontFamily);
    connect(m_actionFormatFontFamily, SIGNAL(triggered(const QString &)),
            this, SLOT(setFontFamily(const QString &)));

    // ------------------- Actions with a key binding and no GUI item
    action  = new KAction(i18n("Insert Non-Breaking Space"), this);
    addAction("nonbreaking_space", action);
    action->setShortcut(Qt::CTRL + Qt::Key_Space);
    connect(action, SIGNAL(triggered()), this, SLOT(nonbreakingSpace()));

    action  = new KAction(i18n("Insert Non-Breaking Hyphen"), this);
    addAction("nonbreaking_hyphen", action);
    action->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Minus);
    connect(action, SIGNAL(triggered()), this, SLOT(nonbreakingHyphen()));

    action  = new KAction(i18n("Insert Index"), this);
    action->setShortcut(Qt::CTRL + Qt::Key_T);
    addAction("insert_index", action);
    connect(action, SIGNAL(triggered()), this, SLOT(insertIndexMarker()));

    action  = new KAction(i18n("Insert Soft Hyphen"), this);
    addAction("soft_hyphen", action);
    //action->setShortcut(Qt::CTRL + Qt::Key_Minus); // TODO this one is also used for the kde-global zoom-out :(
    connect(action, SIGNAL(triggered()), this, SLOT(softHyphen()));

    action  = new KAction(i18n("Line Break"), this);
    addAction("line_break", action);
    action->setShortcut(Qt::SHIFT + Qt::Key_Return);
    connect(action, SIGNAL(triggered()), this, SLOT(lineBreak()));

    action  = new KAction(i18n("Font..."), this);
    addAction("format_font", action);
    action->setShortcut(Qt::ALT + Qt::CTRL + Qt::Key_F);
    action->setToolTip(i18n("Change character size, font, boldface, italics etc."));
    action->setWhatsThis(i18n("Change the attributes of the currently selected characters."));
    connect(action, SIGNAL(triggered()), this, SLOT(selectFont()));

    m_actionFormatFontSize = new FontSizeAction(i18n("Font Size"), this);
    addAction("format_fontsize", m_actionFormatFontSize);
    connect(m_actionFormatFontSize, SIGNAL(fontSizeChanged(qreal)), this, SLOT(setFontSize(qreal)));

    m_actionFormatTextColor = new KoColorPopupAction(this);
    m_actionFormatTextColor->setIcon(KIcon("format-text-color"));
    m_actionFormatTextColor->setToolTip(i18n("Text Color..."));
    addAction("format_textcolor", m_actionFormatTextColor);
    connect(m_actionFormatTextColor, SIGNAL(colorChanged(const KoColor &)), this, SLOT(setTextColor(const KoColor &)));

    m_actionFormatBackgroundColor = new KoColorPopupAction(this);
    m_actionFormatBackgroundColor->setIcon(KIcon("format-fill-color"));
    m_actionFormatBackgroundColor->setToolTip(i18n("Background Color..."));
    m_actionFormatBackgroundColor->setText(i18n("Background"));
    addAction("format_backgroundcolor", m_actionFormatBackgroundColor);
    connect(m_actionFormatBackgroundColor, SIGNAL(colorChanged(const KoColor &)), this, SLOT(setBackgroundColor(const KoColor &)));

    m_growWidthAction = new KAction(KIcon("zoom-fit-best"), i18n("Grow To Fit Width"), this);
    addAction("grow_to_fit_width", m_growWidthAction);
    m_growWidthAction->setCheckable(true);
    connect(m_growWidthAction, SIGNAL(triggered(bool)), this, SLOT(setGrowWidthToFit(bool)));

    m_growHeightAction = new KAction(KIcon("zoom-fit-best"), i18n("Grow To Fit Height"), this);
    addAction("grow_to_fit_height", m_growHeightAction);
    m_growHeightAction->setCheckable(true);
    connect(m_growHeightAction, SIGNAL(triggered(bool)), this, SLOT(setGrowHeightToFit(bool)));

    m_shrinkToFitAction = new KAction(KIcon("zoom-fit-best"), i18n("Shrink To Fit"), this);
    addAction("shrink_to_fit", m_shrinkToFitAction);
    m_shrinkToFitAction->setCheckable(true);
    connect(m_shrinkToFitAction, SIGNAL(triggered(bool)), this, SLOT(setShrinkToFit(bool)));

    action = new KAction(i18n("Default Format"), this);
    addAction("text_default", action);
    action->setToolTip(i18n("Change text attributes to their default values"));
    connect(action, SIGNAL(triggered()), this, SLOT(setDefaultFormat()));

    action = new KAction(i18n("Table..."), this);
    addAction("insert_table", action);
    action->setToolTip(i18n("Insert a table into the document."));
    connect(action, SIGNAL(triggered()), this, SLOT(insertTable()));

    action  = new KAction(KIcon("edit-table-insert-row-above"), i18n("Row Above"), this);
    action->setToolTip(i18n("Insert Row Above"));
    addAction("insert_tablerow_above", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableRowAbove()));

    action  = new KAction(KIcon("edit-table-insert-row-below"), i18n("Row Below"), this);
    action->setToolTip(i18n("Insert Row Below"));
    addAction("insert_tablerow_below", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableRowBelow()));

    action  = new KAction(KIcon("edit-table-insert-column-left"), i18n("Column Left"), this);
    action->setToolTip(i18n("Insert Column Left"));
    addAction("insert_tablecolumn_left", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableColumnLeft()));

    action  = new KAction(KIcon("edit-table-insert-column-right"), i18n("Column Right"), this);
    action->setToolTip(i18n("Insert Column Right"));
    addAction("insert_tablecolumn_right", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableColumnRight()));
    action  = new KAction(KIcon("edit-table-delete-column"), i18n("Column"), this);
    action->setToolTip(i18n("Delete Column"));
    addAction("delete_tablecolumn", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteTableColumn()));

    action  = new KAction(KIcon("edit-table-delete-row"), i18n("Row"), this);
    action->setToolTip(i18n("Delete Row"));
    addAction("delete_tablerow", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteTableRow()));

    action  = new KAction(KIcon("merge"), i18n("Merge Cells"), this);
    addAction("merge_tablecells", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(mergeTableCells()));

    action  = new KAction(KIcon("split"), i18n("Split Cells"), this);
    addAction("split_tablecells", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(splitTableCells()));

    action  = new KAction(KIcon("edit-table-insert-row-above"), i18n("Row Above"), this);
    action->setToolTip(i18n("Insert Row Above"));
    addAction("insert_tablerow_above", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableRowAbove()));

    action  = new KAction(KIcon("edit-table-insert-row-below"), i18n("Row Below"), this);
    action->setToolTip(i18n("Insert Row Below"));
    addAction("insert_tablerow_below", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableRowBelow()));

    action  = new KAction(KIcon("edit-table-insert-column-left"), i18n("Column Left"), this);
    action->setToolTip(i18n("Insert Column Left"));
    addAction("insert_tablecolumn_left", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableColumnLeft()));

    action  = new KAction(KIcon("edit-table-insert-column-right"), i18n("Column Right"), this);
    action->setToolTip(i18n("Insert Column Right"));
    addAction("insert_tablecolumn_right", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(insertTableColumnRight()));

    action  = new KAction(KIcon("edit-table-delete-column"), i18n("Column"), this);
    action->setToolTip(i18n("Delete Column"));
    addAction("delete_tablecolumn", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteTableColumn()));

    action  = new KAction(KIcon("edit-table-delete-row"), i18n("Row"), this);
    action->setToolTip(i18n("Delete Row"));
    addAction("delete_tablerow", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteTableRow()));

    action  = new KAction(KIcon("merge"), i18n("Merge Cells"), this);
    addAction("merge_tablecells", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(mergeTableCells()));

    action  = new KAction(KIcon("split"), i18n("Split Cells"), this);
    addAction("split_tablecells", action);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(splitTableCells()));

    action = new KAction(i18n("Paragraph..."), this);
    addAction("format_paragraph", action);
    action->setShortcut(Qt::ALT + Qt::CTRL + Qt::Key_P);
    action->setToolTip(i18n("Change paragraph margins, text flow, borders, bullets, numbering etc."));
    action->setWhatsThis(i18n("Change paragraph margins, text flow, borders, bullets, numbering etc.<p>Select text in multiple paragraphs to change the formatting of all selected paragraphs.<p>If no text is selected, the paragraph where the cursor is located will be changed.</p>"));
    connect(action, SIGNAL(triggered()), this, SLOT(formatParagraph()));

    m_actionShowChanges = new KAction(i18n("Show Changes"), this);
    m_actionShowChanges->setCheckable(true);
    addAction("edit_show_changes", m_actionShowChanges);
    connect(m_actionShowChanges, SIGNAL(triggered(bool)), this, SLOT(toggleShowChanges(bool)));

    m_actionRecordChanges = new KAction(i18n("Record Changes"), this);
    m_actionRecordChanges->setCheckable(true);
    addAction("edit_record_changes", m_actionRecordChanges);
    connect(m_actionRecordChanges, SIGNAL(triggered(bool)), this, SLOT(toggleRecordChanges(bool)));

    m_configureChangeTracking = new KAction(i18n("Configure Change Tracking..."), this);
    addAction("configure_change_tracking", m_configureChangeTracking);
    connect(m_configureChangeTracking, SIGNAL(triggered()), this, SLOT(configureChangeTracking()));

    action = new KAction(i18n("Style Manager..."), this);
    action->setShortcut(Qt::ALT + Qt::CTRL + Qt::Key_S);
    action->setToolTip(i18n("Change attributes of styles"));
    action->setWhatsThis(i18n("Change font and paragraph attributes of styles.<p>Multiple styles can be changed using the dialog box."));
    addAction("format_stylist", action);
    connect(action, SIGNAL(triggered()), this, SLOT(showStyleManager()));

    action = KStandardAction::selectAll(this, SLOT(selectAll()), this);
    addAction("edit_selectall", action);

    action = new KAction(i18n("Special Character..."), this);
    action->setIcon(KIcon("character-set"));
    action->setShortcut(Qt::ALT + Qt::SHIFT + Qt::Key_C);
    addAction("insert_specialchar", action);
    action->setToolTip(i18n("Insert one or more symbols or characters not found on the keyboard"));
    action->setWhatsThis(i18n("Insert one or more symbols or characters not found on the keyboard."));
    connect(action, SIGNAL(triggered()), this, SLOT(insertSpecialCharacter()));

    action = new KAction(i18n("Repaint"), this);
    action->setIcon(KIcon("view-refresh"));
    addAction("repaint", action);
    connect(action, SIGNAL(triggered()), this, SLOT(relayoutContent()));

#ifndef NDEBUG
    action = new KAction("Paragraph Debug", this); // do NOT add i18n!
    action->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_P);
    addAction("detailed_debug_paragraphs", action);
    connect(action, SIGNAL(triggered()), this, SLOT(debugTextDocument()));
    action = new KAction("Styles Debug", this); // do NOT add i18n!
    action->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::ALT + Qt::Key_S);
    addAction("detailed_debug_styles", action);
    connect(action, SIGNAL(triggered()), this, SLOT(debugTextStyles()));
#endif
}

#ifndef NDEBUG
#include "tests/MockShapes.h"
#include <KUndoStack>

TextTool::TextTool(MockCanvas *canvas)  // constructor for our unit tests;
    : KoToolBase(canvas),
    m_textShape(0),
    m_textShapeData(0),
    m_changeTracker(0),
    m_allowActions(true),
    m_allowAddUndoCommand(true),
    m_trackChanges(false),
    m_allowResourceManagerUpdates(true),
    m_prevCursorPosition(-1),
    m_caretTimer(this),
    m_caretTimerState(true),
    m_currentCommand(0),
    m_currentCommandHasChildren(false),
    m_specialCharacterDocker(0),
    m_textEditingPlugins(0),
    m_changeTipTimer(this),
    m_changeTipCursorPos(0)
    , m_delayedEnsureVisible(false)
{
    // we could init some vars here, but we probably don't have to
    KGlobal::setLocale(new KLocale("en"));
    QTextDocument *document = new QTextDocument();

    KoInlineTextObjectManager *inlineManager = new KoInlineTextObjectManager();
    KoTextDocument(document).setInlineTextObjectManager(inlineManager);

    m_textEditor = new KoTextEditor(document);
    KoTextDocument(document).setTextEditor(m_textEditor.data());

    m_changeTracker = new KoChangeTracker();
    KoTextDocument(document).setChangeTracker(m_changeTracker);

    KoTextDocument(document).setUndoStack(new KUndoStack());

#if 0
    KoTextDocumentLayout *layout = new KoTextDocumentLayout(document);
    document->setDocumentLayout(layout);
#else
    #ifdef __GNUC__
        #warning FIXME: port to textlayout-rework
    #endif
#endif
}
#endif

TextTool::~TextTool()
{
}

void TextTool::showChangeTip()
{
    if (!m_textShapeData || !m_changeTipCursorPos || !m_changeTracker->displayChanges())
        return;

    QTextCursor c(m_textShapeData->document());
    c.setPosition(m_changeTipCursorPos);
    if (m_changeTracker && m_changeTracker->containsInlineChanges(c.charFormat())) {
        KoChangeTrackerElement *element = m_changeTracker->elementById(c.charFormat().property(KoCharacterStyle::ChangeTrackerId).toInt());
        if (element->isEnabled()) {
            QString changeType;
            if (element->getChangeType() == KoGenChange::InsertChange)
                changeType = i18n("Insertion");
            else if (element->getChangeType() == KoGenChange::DeleteChange)
                changeType = i18n("Deletion");
            else
                changeType = i18n("Formatting");

            QString change = "<p align=center style=\'white-space:pre\' ><b>" + changeType + "</b><br/>";

            QString date = element->getDate();
            //Remove the T which separates the Data and Time.
            date[10] = ' ';
            change += element->getCreator() + " " + date + "</p>";

            int toolTipWidth = QFontMetrics(QToolTip::font()).boundingRect(element->getDate() + ' ' + element->getCreator()).width();
            m_changeTipPos.setX(m_changeTipPos.x() - toolTipWidth/2);

            QToolTip::showText(m_changeTipPos,change,canvas()->canvasWidget());

        }
    }
}

void TextTool::blinkCaret()
{
    if (!(canvas()->canvasWidget() ? canvas()->canvasWidget()->hasFocus() : canvas()->canvasItem()->hasFocus())) {
        m_caretTimer.stop();
        m_caretTimerState = false; // not visible.
    }
    else {
        m_caretTimerState = !m_caretTimerState;
    }
    repaintCaret();
}

void TextTool::relayoutContent()
{
    KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(m_textShapeData->document()->documentLayout());
    Q_ASSERT(lay);
    foreach (KoTextLayoutRootArea *rootArea, lay->rootAreas()) {
        rootArea->setDirty();
    }
    lay->emitLayoutIsDirty();
}

void TextTool::paint(QPainter &painter, const KoViewConverter &converter)
{
    if (m_textEditor.isNull())
        return;
    if (canvas()
            && (( canvas()->canvasWidget() && canvas()->canvasWidget()->hasFocus())
                  || (canvas()->canvasItem() && canvas()->canvasItem()->hasFocus())
               )
            && !m_caretTimer.isActive()) { // make sure we blink
        m_caretTimer.start();
    }

    if (!m_textShapeData)
        return;
    if (m_textShapeData->isDirty())
        return;

    qreal zoomX, zoomY;
    converter.zoom(&zoomX, &zoomY);

    painter.save();
    QTransform shapeMatrix = m_textShape->absoluteTransformation(&converter);
    shapeMatrix.scale(zoomX, zoomY);
    painter.setTransform(shapeMatrix * painter.transform());
    painter.setClipRect(m_textShape->outlineRect(), Qt::IntersectClip);
    painter.translate(0, -m_textShapeData->documentOffset());

    if (m_caretTimerState) {
        // Lets draw the caret ourselves, as the Qt method doesn't take cursor
        // charFormat into consideration.
        QTextBlock block = m_textEditor.data()->block();
        if (block.isValid()) {
            int posInParag = m_textEditor.data()->position() - block.position();
            if (posInParag <= block.layout()->preeditAreaPosition())
                posInParag += block.layout()->preeditAreaText().length();

            QTextLine tl = block.layout()->lineForTextPosition(m_textEditor.data()->position() - block.position());
            if (tl.isValid()) {
                QPen caretPen = QPen(QColor(0,0,0),0);
                painter.setPen(caretPen);
                painter.setRenderHint(QPainter::Antialiasing,false);
                QRectF rect = caretRect(m_textEditor.data()->cursor());
                if (tl.ascent() > 0) {
                    QFontMetricsF fm(m_textEditor.data()->charFormat().font(), painter.device());
                    rect.setY(rect.y() + tl.ascent() - qMin(tl.ascent(), fm.ascent()));
                    rect.setHeight(qMin(tl.ascent(), fm.ascent()) + qMin(tl.descent(), fm.descent()));
                } else {
                    //line only filled with characters-without-size (eg anchors)
                    // layout will make sure line has height of block font
                    QFontMetricsF fm(block.charFormat().font(), painter.device());
                    rect.setHeight(fm.ascent() + fm.descent());
                }
                painter.drawLine(rect.topLeft(), rect.bottomLeft());
                caretPen.setColor(QColor(255,255,255));
                caretPen.setStyle(Qt::DotLine);
                painter.setPen(caretPen);
                painter.drawLine(rect.topLeft(), rect.bottomLeft());
            }
        }
    }

    painter.restore();
}

void TextTool::updateSelectedShape(const QPointF &point)
{
    if (m_textShape && !m_textShape->boundingRect().contains(point)) {
        QRectF area(point, QSizeF(1, 1));
        if (m_textEditor.data()->hasSelection())
            repaintSelection();
        else
            repaintCaret();
        foreach (KoShape *shape, canvas()->shapeManager()->shapesAt(area, true)) {
            if (shape->isContentProtected())
                continue;
            TextShape *textShape = dynamic_cast<TextShape*>(shape);
            if (textShape) {
                KoTextShapeData *d = static_cast<KoTextShapeData*>(textShape->userData());
                const bool sameDocument = m_textShapeData ? d->document() == m_textShapeData->document() : false;
                m_textShape = textShape;
                if (sameDocument)
                    break; // stop looking.
            }
        }

        setShapeData(static_cast<KoTextShapeData*>(m_textShape->userData()));

        // This is how we inform the rulers of the active range
        // For now we will not consider table cells, but just give the shape dimensions
        QVariant v;
        QRectF rect(QPoint(), m_textShape->size());
        rect = m_textShape->absoluteTransformation(0).mapRect(rect);
        v.setValue(rect);
        canvas()->resourceManager()->setResource(KoCanvasResource::ActiveRange, v);
    }
}

void TextTool::mousePressEvent(KoPointerEvent *event)
{
    if (m_textEditor.isNull())
        return;
    if (event->button() != Qt::RightButton)
        updateSelectedShape(event->point);
    KoSelection *selection = canvas()->shapeManager()->selection();
    if (m_textShape && !selection->isSelected(m_textShape) && m_textShape->isSelectable()) {
        selection->deselectAll();
        selection->select(m_textShape);
    }

    const bool canMoveCaret = !m_textEditor.data()->hasSelection() || event->button() !=  Qt::RightButton;
    if (canMoveCaret) {
        bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        if (m_textEditor.data()->hasSelection())
            repaintSelection(); // will erase selection
        else
            repaintCaret();
        int position = pointToPosition(event->point);
        if (position != -1) {
            m_textEditor.data()->setPosition(position, shiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        }
        if (shiftPressed) // altered selection.
            repaintSelection();
        else
            repaintCaret();

        updateSelectionHandler();
        updateStyleManager();
    }
    updateActions();

    //activate context-menu for spelling-suggestions
    if (event->button() == Qt::RightButton) {
        KoTextEditingPlugin *plugin = m_textEditingPlugins->spellcheck();
        if (plugin)
            plugin->setCurrentCursorPosition(m_textEditor.data()->document(), m_textEditor.data()->position());

        event->ignore();
    }

    if (event->button() ==  Qt::MidButton) // Paste
        paste();
}

const QTextCursor TextTool::cursor()
{
    return *(m_textEditor.data()->cursor());
}

void TextTool::setShapeData(KoTextShapeData *data)
{
    bool docChanged = !data || !m_textShapeData || m_textShapeData->document() != data->document();
    if (m_textShapeData) {
        disconnect(m_textShapeData, SIGNAL(destroyed (QObject*)), this, SLOT(shapeDataRemoved()));
    }
    m_textShapeData = data;
    if (!m_textShapeData)
        return;
    connect(m_textShapeData, SIGNAL(destroyed (QObject*)), this, SLOT(shapeDataRemoved()));
    if (docChanged) {
        if (!m_textEditor.isNull())
            disconnect(m_textEditor.data(), SIGNAL(isBidiUpdated()), this, SLOT(isBidiUpdated()));
        m_textEditor = KoTextDocument(m_textShapeData->document()).textEditor();
        Q_ASSERT(m_textEditor.data());
        connect(m_textEditor.data(), SIGNAL(isBidiUpdated()), this, SLOT(isBidiUpdated()));
    }
    m_textEditor.data()->updateDefaultTextDirection(m_textShapeData->pageDirection());
}

void TextTool::updateSelectionHandler()
{
    if (m_textEditor) {
        emit selectionChanged(m_textEditor.data()->hasSelection());
        if (m_textEditor.data()->hasSelection()) {
            QClipboard *clipboard = QApplication::clipboard();
            if (clipboard->supportsSelection())
                clipboard->setText(m_textEditor.data()->selectedText(), QClipboard::Selection);
        }
    }

    KoResourceManager *p = canvas()->resourceManager();
    m_allowResourceManagerUpdates = false;
    if (m_textEditor && m_textShapeData) {
        p->setResource(KoText::CurrentTextPosition, m_textEditor.data()->position());
        p->setResource(KoText::CurrentTextAnchor, m_textEditor.data()->anchor());
        QVariant variant;
        variant.setValue<void*>(m_textShapeData->document());
        p->setResource(KoText::CurrentTextDocument, variant);
    } else {
        p->clearResource(KoText::CurrentTextPosition);
        p->clearResource(KoText::CurrentTextAnchor);
        p->clearResource(KoText::CurrentTextDocument);
    }
    m_allowResourceManagerUpdates = true;
}

void TextTool::copy() const
{
    if (!m_textShapeData || m_textEditor.isNull() || !m_textEditor.data()->hasSelection())
        return;
    int from = m_textEditor.data()->position();
    int to = m_textEditor.data()->anchor();
    KoTextOdfSaveHelper saveHelper(m_textShapeData, from, to);
    KoTextDrag drag;

    if (KoDocumentRdfBase *rdf = KoDocumentRdfBase::fromResourceManager(canvas())) {
        saveHelper.setRdfModel(rdf->model());
    }
    drag.setOdf(KoOdf::mimeType(KoOdf::Text), saveHelper);
    QTextDocumentFragment fragment = m_textEditor.data()->selection();
    drag.setData("text/html", fragment.toHtml("utf-8").toUtf8());
    drag.setData("text/plain", fragment.toPlainText().toUtf8());
    drag.addToClipboard();
}

void TextTool::deleteSelection()
{
    if (m_actionRecordChanges->isChecked())
      m_textEditor.data()->addCommand(new ChangeTrackedDeleteCommand(ChangeTrackedDeleteCommand::NextChar, this));
    else
      m_textEditor.data()->addCommand(new DeleteCommand(DeleteCommand::NextChar, this));
    editingPluginEvents();
}

bool TextTool::paste()
{
    const QMimeData *data = QApplication::clipboard()->mimeData(QClipboard::Clipboard);

    // on windows we do not have data if we try to paste this selection
    if (!data) return false;

    m_prevCursorPosition = m_textEditor.data()->position();
    m_textEditor.data()->addCommand(new TextPasteCommand(QClipboard::Clipboard, this));
    editingPluginEvents();
    return true;
}

void TextTool::cut()
{
    m_textEditor.data()->addCommand(new TextCutCommand(this));
}

QStringList TextTool::supportedPasteMimeTypes() const
{
    QStringList list;
    list << "text/plain" << "text/html" << "application/vnd.oasis.opendocument.text";
    return list;
}

int TextTool::pointToPosition(const QPointF & point) const
{
    if (!m_textShape || !m_textShapeData) {
        return -1;
    }
    QPointF p = m_textShape->convertScreenPos(point);
    KoTextLayoutRootArea *rootArea = m_textShapeData->rootArea();
    int caretPos = rootArea ? rootArea->hitTest(p, Qt::FuzzyHit) : -1;
    return caretPos;
}

void TextTool::mouseDoubleClickEvent(KoPointerEvent *event)
{
    if (canvas()->shapeManager()->shapeAt(event->point) != m_textShape) {
        event->ignore(); // allow the event to be used by another
        return;
    }
    m_textEditor.data()->clearSelection();
    int pos = m_textEditor.data()->position();
    m_textEditor.data()->movePosition(QTextCursor::WordLeft);
    m_textEditor.data()->movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
    if (qAbs(pos - m_textEditor.data()->position()) <= 1) // clicked between two words
        m_textEditor.data()->movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);

    repaintSelection();
    updateSelectionHandler();
}

void TextTool::mouseMoveEvent(KoPointerEvent *event)
{
    m_changeTipPos = event->globalPos();

    if (event->buttons()) {
        updateSelectedShape(event->point);
    }

    m_changeTipTimer.stop();

    if (QToolTip::isVisible())
        QToolTip::hideText();

    int position = pointToPosition(event->point);

    if (event->buttons() == Qt::NoButton) {
        //if (!m_textShapeData ){//FIXME|| m_textShapeData->endPosition() == position) {
        if (!m_textShapeData || position < 0) {
            useCursor(Qt::IBeamCursor);
            return;
        }

        QTextCursor cursor(*(m_textEditor.data()->cursor()));
        cursor.setPosition(position);

        QTextCursor mouseOver(m_textShapeData->document());
        mouseOver.setPosition(position);

        if (m_changeTracker && m_changeTracker->containsInlineChanges(mouseOver.charFormat())) {
            m_changeTipTimer.start();
            m_changeTipCursorPos = position;
        }

        if (cursor.charFormat().isAnchor()) {
            useCursor(Qt::PointingHandCursor);
            return;
        }

        // check if mouse pointer is not over shape with hyperlink
        KoShape *selectedShape = canvas()->shapeManager()->shapeAt(event->point);
        if (selectedShape != 0 && selectedShape != m_textShape && selectedShape->hyperLink().size() != 0) {
            useCursor(Qt::PointingHandCursor);
            return;
        }

        useCursor(Qt::IBeamCursor);
        return;
    }

    useCursor(Qt::IBeamCursor);
    if (position == m_textEditor.data()->position()) return;
    if (position >= 0) {
        if (m_textEditor.data()->hasSelection())
            repaintSelection(); // will erase selection
        else
            repaintCaret();

        m_textEditor.data()->setPosition(position, QTextCursor::KeepAnchor);

        if (m_textEditor.data()->hasSelection())
            repaintSelection();
        else
            repaintCaret();
    }

    updateSelectionHandler();
}

void TextTool::mouseReleaseEvent(KoPointerEvent *event)
{
    event->ignore();
    editingPluginEvents();

    if (!m_textShapeData)
        return;

    // check if mouse pointer is not over some shape with hyperlink
    KoShape *selectedShape = canvas()->shapeManager()->shapeAt(event->point);
    if (selectedShape != 0 && selectedShape != m_textShape && selectedShape->hyperLink().size() != 0) {
        QString url = selectedShape->hyperLink();
        runUrl(event, url);
        return;
    }

    // Is there an anchor here ?
    if (m_textEditor.data()->charFormat().isAnchor() && !m_textEditor.data()->hasSelection()) {
        QString anchor = m_textEditor.data()->charFormat().anchorHref();
        // local uri has this prefix but bookmark does not contain it, so strip it
        if ( anchor.startsWith("#") ){
            anchor = anchor.right(anchor.size()-1);
        }
        if (!anchor.isEmpty()) {
            KoTextDocument document(m_textShapeData->document());
            KoInlineTextObjectManager *inlineManager = document.inlineTextObjectManager();
            if (inlineManager) {
                QList<QString> bookmarks = inlineManager->bookmarkManager()->bookmarkNameList();
                // Which are the bookmarks we have ?
                foreach(const QString& s, bookmarks) {
                    // Is this bookmark the good one ?
                    if (s == anchor) {
                        // if Yes, let's jump to it
                        KoBookmark *bookmark = inlineManager->bookmarkManager()->retrieveBookmark(s);
                        m_textEditor.data()->setPosition(bookmark->position());
                        ensureCursorVisible();
                        event->accept();
                        return;
                    }
                }
            }

            runUrl(event, anchor);
            return;
        } else {
            QStringList anchorList = m_textEditor.data()->charFormat().anchorNames();
            QString anchorName;
            if (!anchorList.isEmpty()) {
                anchorName = anchorList.takeFirst();
            }
            KoTextDocument document(m_textShapeData->document());
            KoBookmark *bookmark = document.inlineTextObjectManager()->bookmarkManager()->retrieveBookmark(anchorName);
            if (bookmark) {
                m_textEditor.data()->setPosition(bookmark->position());
                ensureCursorVisible();
            } else {
                kDebug(32500) << "A bookmark should exist but has not been found";
            }
        }
    }
}

void TextTool::keyPressEvent(QKeyEvent *event)
{
    int destinationPosition = -1; // for those cases where the moveOperation is not relevant;
    QTextCursor::MoveOperation moveOperation = QTextCursor::NoMove;
    KoTextEditor *textEditor = m_textEditor.data();
    Q_ASSERT(textEditor);
    if (event->key() == Qt::Key_Backspace) {
        if (!textEditor->hasSelection() && textEditor->block().textList()
            && (textEditor->position() == textEditor->block().position())
            && !(m_actionRecordChanges->isChecked())) {
            if (!textEditor->blockFormat().boolProperty(KoParagraphStyle::UnnumberedListItem)) {
                // backspace at beginning of numbered list item, makes it unnumbered
                ListItemNumberingCommand *lin = new ListItemNumberingCommand(textEditor->block(), false);
                addCommand(lin);
            } else {
                // backspace on numbered, empty parag, removes numbering.
                ChangeListCommand *clc = new ChangeListCommand(*textEditor->cursor(), KoListStyle::None, 0 /* level */);
                addCommand(clc);
            }
        } else if (textEditor->position() > 0 || textEditor->hasSelection()) {
            if (!textEditor->hasSelection() && event->modifiers() & Qt::ControlModifier) // delete prev word.
                textEditor->movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
            if (m_actionRecordChanges->isChecked())
                textEditor->addCommand(new ChangeTrackedDeleteCommand(
                            ChangeTrackedDeleteCommand::PreviousChar, this));
            else
                textEditor->addCommand(new DeleteCommand(DeleteCommand::PreviousChar, this));
            editingPluginEvents();
        }
    } else if ((event->key() == Qt::Key_Tab)
        && ((!textEditor->hasSelection() && (textEditor->position() == textEditor->block().position())) || (textEditor->block().document()->findBlock(textEditor->anchor()) != textEditor->block().document()->findBlock(textEditor->position()))) && textEditor->block().textList()) {
        ChangeListLevelCommand::CommandType type = ChangeListLevelCommand::IncreaseLevel;
        ChangeListLevelCommand *cll = new ChangeListLevelCommand(*textEditor->cursor(), type, 1);
        addCommand(cll);
        editingPluginEvents();
    } else if ((event->key() == Qt::Key_Backtab)
        && ((!textEditor->hasSelection() && (textEditor->position() == textEditor->block().position())) || (textEditor->block().document()->findBlock(textEditor->anchor()) != textEditor->block().document()->findBlock(textEditor->position()))) && textEditor->block().textList() && !(m_actionRecordChanges->isChecked())) {
        ChangeListLevelCommand::CommandType type = ChangeListLevelCommand::DecreaseLevel;
        ChangeListLevelCommand *cll = new ChangeListLevelCommand(*textEditor->cursor(), type, 1);
        addCommand(cll);
        editingPluginEvents();
    } else if (event->key() == Qt::Key_Delete) {
        if (!textEditor->hasSelection() && event->modifiers() & Qt::ControlModifier) // delete next word.
            textEditor->movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        // the event only gets through when the Del is not used in the app
        // if the app forwards Del then deleteSelection is used
        if (m_actionRecordChanges->isChecked())
          textEditor->addCommand(new ChangeTrackedDeleteCommand(ChangeTrackedDeleteCommand::NextChar, this));
        else
          textEditor->addCommand(new DeleteCommand(DeleteCommand::NextChar, this));
        editingPluginEvents();
    } else if ((event->key() == Qt::Key_Left) && (event->modifiers() & Qt::ControlModifier) == 0) {
        moveOperation = QTextCursor::Left;
    } else if ((event->key() == Qt::Key_Right) && (event->modifiers() & Qt::ControlModifier) == 0) {
        moveOperation = QTextCursor::Right;
    } else if ((event->key() == Qt::Key_Up) && (event->modifiers() & Qt::ControlModifier) == 0) {
        moveOperation = QTextCursor::Up;
    } else if ((event->key() == Qt::Key_Down) && (event->modifiers() & Qt::ControlModifier) == 0) {
        moveOperation = QTextCursor::Down;
    } else {
        // check for shortcuts.
        QKeySequence item(event->key() | ((Qt::ControlModifier | Qt::AltModifier) & event->modifiers()));
        if (hit(item, KStandardShortcut::Begin))
            // Goto beginning of the document. Default: Ctrl-Home
            destinationPosition = 0;
        else if (hit(item, KStandardShortcut::End)) {
            // Goto end of the document. Default: Ctrl-End
            if (m_textShapeData) {
                QTextBlock last = m_textShapeData->document()->lastBlock();
                destinationPosition = last.position() + last.length() - 1;
            }
        } else if (hit(item, KStandardShortcut::Prior)) { // page up
            // Scroll up one page. Default: Prior
            QPointF point = caretRect(textEditor->cursor()).topLeft();
            qreal moveDistance = canvas()->viewConverter()->viewToDocument(QSizeF(0,canvas()->canvasController()->visibleHeight())).height() * 0.8;
            point.setY(point.y() - moveDistance);
            destinationPosition = m_textShapeData->rootArea()->hitTest(point, Qt::FuzzyHit);
        }
        else if (hit(item, KStandardShortcut::Next)) {
            // Scroll down one page. Default: Next
            QPointF point = caretRect(textEditor->cursor()).topLeft();
            qreal moveDistance = canvas()->viewConverter()->viewToDocument(QSizeF(0,canvas()->canvasController()->visibleHeight())).height() * 0.8;
            point.setY(point.y() + moveDistance);
            destinationPosition = m_textShapeData->rootArea()->hitTest(point, Qt::FuzzyHit);
        }
        else if (hit(item, KStandardShortcut::BeginningOfLine))
            // Goto beginning of current line. Default: Home
            moveOperation = QTextCursor::StartOfLine;
        else if (hit(item, KStandardShortcut::EndOfLine))
            // Goto end of current line. Default: End
            moveOperation = QTextCursor::EndOfLine;
        else if (hit(item, KStandardShortcut::BackwardWord))
            moveOperation = QTextCursor::WordLeft;
        else if (hit(item, KStandardShortcut::ForwardWord))
            moveOperation = QTextCursor::WordRight;
#ifdef Q_WS_MAC
        // Don't reject "alt" key, it may be used for typing text on Mac OS
        else if ((event->modifiers() & Qt::ControlModifier) || event->text().length() == 0) {
#else
        else if ((event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) || event->text().length() == 0) {
#endif
            event->ignore();
            return;
        } else if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)) {
            textEditor->newLine();
            updateActions();
            editingPluginEvents();
        } else if ((event->key() == Qt::Key_Tab || !(event->text().length() == 1 && !event->text().at(0).isPrint()))) { // insert the text
            m_prevCursorPosition = textEditor->position();
            textEditor->insertText(event->text());
            editingPluginEvents();
        }
    }
    if (moveOperation != QTextCursor::NoMove || destinationPosition != -1) {
        useCursor(Qt::BlankCursor);
        bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
        if (textEditor->hasSelection())
            repaintSelection(); // will erase selection
        else
            repaintCaret();
        QTextBlockFormat format = textEditor->blockFormat();

        KoText::Direction dir = static_cast<KoText::Direction>(format.intProperty(KoParagraphStyle::TextProgressionDirection));
        bool isRtl;
        if (dir == KoText::AutoDirection)
            isRtl = textEditor->block().text().isRightToLeft();
        else
            isRtl =  dir == KoText::RightLeftTopBottom;

        if (isRtl) { // if RTL toggle direction of cursor movement.
            switch (moveOperation) {
            case QTextCursor::Left: moveOperation = QTextCursor::Right; break;
            case QTextCursor::Right: moveOperation = QTextCursor::Left; break;
            case QTextCursor::WordRight: moveOperation = QTextCursor::WordLeft; break;
            case QTextCursor::WordLeft: moveOperation = QTextCursor::WordRight; break;
            default: break;
            }
        }
        int prevPosition = textEditor->position();
        if (moveOperation != QTextCursor::NoMove)
            textEditor->movePosition(moveOperation,
                shiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        else
            textEditor->setPosition(destinationPosition,
                shiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        if (moveOperation == QTextCursor::Down && prevPosition == textEditor->position()) {
            // change behavior a little big from Qt; at the bottom of the doc we go to the end of the doc
            textEditor->movePosition(QTextCursor::End,
                shiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
        }
        if (shiftPressed) // altered selection.
            repaintSelection();
        else
            repaintCaret();
        updateActions();
        editingPluginEvents();
    }
    if (m_caretTimer.isActive()) { // make the caret not blink but decide on the action if its visible or not.
        m_caretTimer.stop();
        m_caretTimer.start();
        m_caretTimerState = moveOperation != QTextCursor::NoMove; // turn caret off while typing
        if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return
                || event->key() == Qt::Key_Backspace) {// except the enter/backspace key
            m_caretTimerState = true;
        }
    }
    ensureCursorVisible();

    updateSelectionHandler();
}

QVariant TextTool::inputMethodQuery(Qt::InputMethodQuery query, const KoViewConverter &converter) const
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (!textEditor || !m_textShapeData)
        return QVariant();

    switch (query) {
    case Qt::ImMicroFocus: {
        // The rectangle covering the area of the input cursor in widget coordinates.
        QRectF rect = caretRect(textEditor->cursor());
        rect.moveTop(rect.top() - m_textShapeData->documentOffset());
        QTransform shapeMatrix = m_textShape->absoluteTransformation(&converter);
        qreal zoomX, zoomY;
        converter.zoom(&zoomX, &zoomY);
        shapeMatrix.scale(zoomX, zoomY);
        rect = shapeMatrix.mapRect(rect);

        return rect.toRect();
    }
    case Qt::ImFont:
        // The currently used font for text input.
        return textEditor->charFormat().font();
    case Qt::ImCursorPosition:
        // The logical position of the cursor within the text surrounding the input area (see ImSurroundingText).
        return textEditor->position() - textEditor->block().position();
    case Qt::ImSurroundingText:
        // The plain text around the input area, for example the current paragraph.
        return textEditor->block().text();
    case Qt::ImCurrentSelection:
        // The currently selected text.
        return textEditor->selectedText();
    default:
        ; // Qt 4.6 adds ImMaximumTextLength and ImAnchorPosition
    }
    return QVariant();
}

void TextTool::inputMethodEvent(QInputMethodEvent *event)
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (textEditor == 0)
        return;
    if (event->replacementLength() > 0) {
        textEditor->setPosition(textEditor->position() + event->replacementStart());
        for (int i = event->replacementLength(); i > 0; --i) {
            if (m_actionRecordChanges->isChecked())
              textEditor->addCommand(new ChangeTrackedDeleteCommand(ChangeTrackedDeleteCommand::NextChar, this));
            else
              textEditor->addCommand(new DeleteCommand(DeleteCommand::NextChar, this));
        }
    }
    QTextBlock block = textEditor->block();
    QTextLayout *layout = block.layout();
    Q_ASSERT(layout);
    if (!event->commitString().isEmpty()) {
        QKeyEvent ke(QEvent::KeyPress, -1, 0, event->commitString());
        keyPressEvent(&ke);
        layout->setPreeditArea(-1, QString());
    } else {
        layout->setPreeditArea(textEditor->position() - block.position(),
                event->preeditString());
        textEditor->document()->markContentsDirty(textEditor->position(), 1);
    }
    event->accept();
}

void TextTool::ensureCursorVisible()
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (!textEditor || !m_textShapeData)
        return;

    const int position = textEditor->position();

    KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(m_textShapeData->document()->documentLayout());
    Q_ASSERT(lay);
    KoTextLayoutRootArea *rootArea = lay->rootAreaForPosition(position);

    if (rootArea && m_textShapeData->rootArea() != rootArea) {
        // If we have changed root area we need to update m_textShape and m_textShapeData
        m_textShape = static_cast<TextShape*>(rootArea->associatedShape());
        Q_ASSERT(m_textShape);
        disconnect(m_textShapeData, SIGNAL(destroyed (QObject*)), this, SLOT(shapeDataRemoved()));
        m_textShapeData = static_cast<KoTextShapeData*>(m_textShape->userData());
        Q_ASSERT(m_textShapeData);
        connect(m_textShapeData, SIGNAL(destroyed (QObject*)), this, SLOT(shapeDataRemoved()));
    }

    QRectF cursorPos = caretRect(textEditor->cursor());
    if (! cursorPos.isValid()) { // paragraph is not yet layouted.
        // The number one usecase for this is when the user pressed enter.
        // try to do it on next caret blink
        m_delayedEnsureVisible = true;
    }
    cursorPos.moveTop(cursorPos.top() - m_textShapeData->documentOffset());
    canvas()->ensureVisible(m_textShape->absoluteTransformation(0).mapRect(cursorPos));
}

void TextTool::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
}

void TextTool::updateActions()
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (textEditor == 0)
        return;
    m_allowActions = false;
    QTextCharFormat cf = textEditor->charFormat();
    m_actionFormatBold->setChecked(cf.fontWeight() > QFont::Normal);
    m_actionFormatItalic->setChecked(cf.fontItalic());
    m_actionFormatUnderline->setChecked(cf.intProperty(KoCharacterStyle::UnderlineType) != KoCharacterStyle::NoLineType);
    m_actionFormatStrikeOut->setChecked(cf.intProperty(KoCharacterStyle::StrikeOutType) != KoCharacterStyle::NoLineType);
    bool super = false, sub = false;
    switch (cf.verticalAlignment()) {
    case QTextCharFormat::AlignSuperScript:
        super = true;
        break;
    case QTextCharFormat::AlignSubScript:
        sub = true;
        break;
    default:;
    }
    m_actionFormatSuper->setChecked(super);
    m_actionFormatSub->setChecked(sub);
    m_actionFormatFontSize->setFontSize(cf.fontPointSize());
    m_actionFormatFontFamily->setFont(cf.font().family());

    KoTextShapeData::ResizeMethod resizemethod = KoTextShapeData::AutoResize;
    if(m_textShapeData) {
        resizemethod = m_textShapeData->resizeMethod();
    }
    m_shrinkToFitAction->setEnabled(resizemethod != KoTextShapeData::AutoResize);
    m_shrinkToFitAction->setChecked(resizemethod == KoTextShapeData::ShrinkToFitResize);

    m_growWidthAction->setEnabled(resizemethod != KoTextShapeData::AutoResize);
    m_growWidthAction->setChecked(resizemethod == KoTextShapeData::AutoGrowWidth || resizemethod == KoTextShapeData::AutoGrowWidthAndHeight);

    m_growHeightAction->setEnabled(resizemethod != KoTextShapeData::AutoResize);
    m_growHeightAction->setChecked(resizemethod == KoTextShapeData::AutoGrowHeight || resizemethod == KoTextShapeData::AutoGrowWidthAndHeight);

    QTextBlockFormat bf = textEditor->blockFormat();
    if (bf.alignment() == Qt::AlignLeading || bf.alignment() == Qt::AlignTrailing) {
        bool revert = (textEditor->block().layout()->textOption().textDirection() == Qt::LeftToRight) != QApplication::isLeftToRight();
        if (bf.alignment() == (Qt::AlignLeading ^ revert))
            m_actionAlignLeft->setChecked(true);
        else
            m_actionAlignRight->setChecked(true);
    } else if (bf.alignment() == Qt::AlignHCenter)
        m_actionAlignCenter->setChecked(true);
    if (bf.alignment() == Qt::AlignJustify)
        m_actionAlignBlock->setChecked(true);
    else if (bf.alignment() == (Qt::AlignLeft | Qt::AlignAbsolute))
        m_actionAlignLeft->setChecked(true);
    else if (bf.alignment() == (Qt::AlignRight | Qt::AlignAbsolute))
        m_actionAlignRight->setChecked(true);

    m_actionFormatDecreaseIndent->setEnabled(textEditor->blockFormat().leftMargin() > 0.);

    if (m_changeTracker && m_changeTracker->displayChanges())
        m_actionShowChanges->setChecked(true);
    if (m_changeTracker && m_changeTracker->recordChanges())
        m_actionRecordChanges->setChecked(true);

    m_allowActions = true;

    emit charFormatChanged(cf);
    emit blockFormatChanged(bf);
    emit blockChanged(textEditor->block());
}

void TextTool::updateStyleManager()
{
    if (!m_textShapeData)
        return;
    KoStyleManager *styleManager = KoTextDocument(m_textShapeData->document()).styleManager();
    emit styleManagerChanged(styleManager);

    //TODO move this to its own method
    m_changeTracker = KoTextDocument(m_textShapeData->document()).changeTracker();
}

void TextTool::activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes)
{
    kDebug();

    Q_UNUSED(toolActivation);
    m_caretTimer.start();
    foreach (KoShape *shape, shapes) {
        m_textShape = dynamic_cast<TextShape*>(shape);
        if (m_textShape)
            break;
    }
    if (!m_textShape) { // none found
        emit done();
        // This is how we inform the rulers of the active range
        // No shape means no active range
        canvas()->resourceManager()->setResource(KoCanvasResource::ActiveRange, QVariant(QRectF()));
        return;
    }

    // This is how we inform the rulers of the active range
    // For now we will not consider table cells, but just give the shape dimensions
    QVariant v;
    QRectF rect(QPoint(), m_textShape->size());
    rect = m_textShape->absoluteTransformation(0).mapRect(rect);
    v.setValue(rect);
    canvas()->resourceManager()->setResource(KoCanvasResource::ActiveRange, v);

    setShapeData(static_cast<KoTextShapeData*>(m_textShape->userData()));
    useCursor(Qt::IBeamCursor);

    // restore the selection from a previous time we edited this document.
    for (int i = 0; i < m_previousSelections.count(); i++) {
        TextSelection selection = m_previousSelections.at(i);
        if (selection.document == m_textShapeData->document()) {
            KoTextEditor *textEditor = m_textEditor.data();
            if (textEditor) {
                textEditor->setPosition(selection.anchor);
                textEditor->setPosition(selection.position, QTextCursor::KeepAnchor);
            }
            m_previousSelections.removeAt(i);
            break;
        }
    }

    repaintSelection();
    updateSelectionHandler();
    updateActions();
    updateStyleManager();
    if (m_specialCharacterDocker)
        m_specialCharacterDocker->setEnabled(true);
    readConfig();
}

void TextTool::deactivate()
{
    kDebug();

    m_caretTimer.stop();
    m_caretTimerState = false;
    repaintCaret();
    m_textShape = 0;

    // This is how we inform the rulers of the active range
    // No shape means no active range
    canvas()->resourceManager()->setResource(KoCanvasResource::ActiveRange, QVariant(QRectF()));

    if (m_textEditor.data() && m_textShapeData) {
        TextSelection selection;
        selection.document = m_textShapeData->document();
        selection.position = m_textEditor.data()->position();
        selection.anchor = m_textEditor.data()->anchor();
        m_previousSelections.append(selection);
    }
    setShapeData(0);
    if (m_previousSelections.count() > 20) // don't let it grow indefinitely
        m_previousSelections.removeAt(0);

    updateSelectionHandler();
    if (m_specialCharacterDocker) {
        m_specialCharacterDocker->setEnabled(false);
        m_specialCharacterDocker->setVisible(false);
    }
}

void TextTool::repaintDecorations()
{
    if (m_textShapeData)
        repaintSelection();
}

void TextTool::repaintCaret()
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (!textEditor || !m_textShapeData)
        return;

    KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(m_textShapeData->document()->documentLayout());
    Q_ASSERT(lay);
    KoTextLayoutRootArea *rootArea = lay->rootAreaForPosition(textEditor->position());

    if (rootArea) {
        // If we have changed root area we need to update m_textShape and m_textShapeData
        if (m_delayedEnsureVisible) {
            m_delayedEnsureVisible = false;
            ensureCursorVisible();
            return;
        }

        TextShape *textShape = static_cast<TextShape*>(rootArea->associatedShape());
        Q_ASSERT(textShape);
        Q_ASSERT(textShape->textShapeData());

        QRectF repaintRect = caretRect(textEditor->cursor());
        repaintRect.moveTop(repaintRect.top() - textShape->textShapeData()->documentOffset());
        if (repaintRect.isValid()) {
            repaintRect = textShape->absoluteTransformation(0).mapRect(repaintRect);
            canvas()->updateCanvas(repaintRect);
        }
    }
}

void TextTool::repaintSelection()
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (textEditor == 0)
        return;
    QTextCursor cursor = *textEditor->cursor();

    QList<TextShape *> shapes;
    KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(m_textShapeData->document()->documentLayout());
    Q_ASSERT(lay);
    foreach (KoShape* shape, lay->shapes()) {
        TextShape *textShape = dynamic_cast<TextShape*>(shape);
        if (textShape == 0) // when the shape is being deleted its no longer a TextShape but a KoShape
            continue;

        if (textShape->textShapeData()->isCursorVisible(&cursor)) {
            //Q_ASSERT(!shapes.contains(textShape));
            if (!shapes.contains(textShape))
                shapes.append(textShape);
        }
    }

    // loop over all shapes that contain the text and update per shape.
    QRectF repaintRect = textRect(cursor);
    foreach (TextShape *ts, shapes) {
        QRectF rect = repaintRect;
        rect.moveTop(rect.y() - ts->textShapeData()->documentOffset());
        rect = ts->absoluteTransformation(0).mapRect(rect);
        QRectF r = ts->boundingRect().intersected(rect);
        canvas()->updateCanvas(r);
    }
}

QRectF TextTool::caretRect(QTextCursor *cursor) const
{
    QTextCursor tmpCursor(*cursor);
    tmpCursor.setPosition(cursor->position()); // looses the anchor

    return textRect(tmpCursor);
}

QRectF TextTool::textRect(QTextCursor &cursor) const
{
    if (!m_textShapeData)
        return QRectF();
    KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(m_textShapeData->document()->documentLayout());
    return lay->selectionBoundingBox(cursor);
}

KoToolSelection* TextTool::selection()
{
    return m_textEditor.data();
}

QMap<QString, QWidget *> TextTool::createOptionWidgets()
{
    QMap<QString, QWidget *> widgets;
    SimpleCharacterWidget *scw = new SimpleCharacterWidget(this, 0);
    SimpleParagraphWidget *spw = new SimpleParagraphWidget(this, 0);
    SimpleStylesWidget *ssw = new SimpleStylesWidget(0);
    SimpleTableWidget *stw = new SimpleTableWidget(this, 0);

    // Connect to/with simple character widget (docker)
    connect(this, SIGNAL(styleManagerChanged(KoStyleManager *)), scw, SLOT(setStyleManager(KoStyleManager *)));
    connect(scw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));


    // Connect to/with simple paragraph widget (docker)
    connect(this, SIGNAL(styleManagerChanged(KoStyleManager *)), spw, SLOT(setStyleManager(KoStyleManager *)));
    connect(this, SIGNAL(blockChanged(const QTextBlock&)), spw, SLOT(setCurrentBlock(const QTextBlock&)));
    connect(spw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));
    connect(spw, SIGNAL(insertTableQuick(int, int)), this, SLOT(insertTableQuick(int, int)));

    // Connect to/with simple styles widget (docker)
    connect(this, SIGNAL(styleManagerChanged(KoStyleManager *)), ssw, SLOT(setStyleManager(KoStyleManager *)));
    connect(ssw, SIGNAL(paragraphStyleSelected(KoParagraphStyle *)), this, SLOT(setStyle(KoParagraphStyle*)));
    connect(ssw, SIGNAL(characterStyleSelected(KoCharacterStyle *)), this, SLOT(setStyle(KoCharacterStyle*)));
    connect(ssw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));

    // Connect to/with simple table widget (docker)
    connect(this, SIGNAL(styleManagerChanged(KoStyleManager *)), stw, SLOT(setStyleManager(KoStyleManager *)));
    connect(stw, SIGNAL(doneWithFocus()), this, SLOT(returnFocusToCanvas()));

    updateStyleManager();
    if (m_textShape)
        updateActions();
    widgets.insert(i18n("Character"), scw);
    widgets.insert(i18n("Paragraph"), spw);
    widgets.insert(i18n("Styles"), ssw);
    widgets.insert(i18n("Table"), stw);
    return widgets;
}

void TextTool::returnFocusToCanvas()
{
    canvas()->canvasWidget()->setFocus();
}

void TextTool::addUndoCommand()
{
    return;
/*    if (! m_allowAddUndoCommand) return;
    class UndoTextCommand : public QUndoCommand
    {
    public:
        UndoTextCommand(QTextDocument *document, TextTool *tool, QUndoCommand *parent = 0)
                : QUndoCommand(i18n("Text"), parent),
                m_document(document),
                m_tool(tool) {
        }

        void undo() {
            if (m_document.isNull())
                return;
            if (!(m_tool.isNull()) && (m_tool->m_textShapeData) && (m_tool->m_textShapeData->document() == m_document)) {
                m_tool->stopMacro();
                m_tool->m_allowAddUndoCommand = false;


                m_document->undo(&m_tool->m_caret);
            } else
                m_document->undo();
            if (! m_tool.isNull())
                m_tool->m_allowAddUndoCommand = true;
        }

        void redo() {
            if (m_document.isNull())
                return;

            if (!(m_tool.isNull()) && (m_tool->m_textShapeData) && (m_tool->m_textShapeData->document() == m_document)) {
                m_tool->m_allowAddUndoCommand = false;
                m_document->redo(&m_tool->m_caret);
            } else
                m_document->redo();
            if (! m_tool.isNull())
                m_tool->m_allowAddUndoCommand = true;
        }

        QPointer<QTextDocument> m_document;
        QPointer<TextTool> m_tool;
    };
    kDebug() << "in TextTool addCommand";
    if (m_currentCommand) {
        new UndoTextCommand(m_textShapeData->document(), this, m_currentCommand);
        if (! m_currentCommandHasChildren)
            canvas()->addCommand(m_currentCommand);
        m_currentCommandHasChildren = true;
    } else
        canvas()->addCommand(new UndoTextCommand(m_textShapeData->document(), this));
*/}

void TextTool::addCommand(QUndoCommand *command)
{
/*    m_currentCommand = command;
    TextCommandBase *cmd = dynamic_cast<TextCommandBase*>(command);
    if (cmd)
        cmd->setTool(this);
    m_currentCommandHasChildren = true; //to avoid adding it again on the first child UndoTextCommand (infinite loop)
    canvas()->addCommand(command); // will execute it.
    m_currentCommand = 0;
    m_currentCommandHasChildren = false;
*/
    Q_ASSERT(!m_textEditor.isNull());
    m_textEditor.data()->addCommand(command);
}

void TextTool::startEditing(QUndoCommand* command)
{
    m_currentCommand = command;
    m_currentCommandHasChildren = true;
}

void TextTool::stopEditing()
{
    m_currentCommand = 0;
    m_currentCommandHasChildren = false;
}

void TextTool::bold(bool bold)
{
    m_textEditor.data()->bold(bold);
}

void TextTool::italic(bool italic)
{
    m_textEditor.data()->italic(italic);
}

void TextTool::underline(bool underline)
{
    m_textEditor.data()->underline(underline);
}

void TextTool::strikeOut(bool strikeOut)
{
    m_textEditor.data()->strikeOut(strikeOut);
}

void TextTool::nonbreakingSpace()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->insertText(QString(QChar(Qt::Key_nobreakspace)));
}

void TextTool::nonbreakingHyphen()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->insertText(QString(QChar(0x2013)));
}

void TextTool::softHyphen()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->insertText(QString(QChar(Qt::Key_hyphen)));
}

void TextTool::lineBreak()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->insertText(QString(QChar(0x2028)));
}

void TextTool::alignLeft()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    Qt::Alignment align = Qt::AlignLeading;
    if (m_textEditor.data()->block().layout()->textOption().textDirection() != Qt::LeftToRight)
        align |= Qt::AlignTrailing;
    m_textEditor.data()->setHorizontalTextAlignment(align);
}

void TextTool::alignRight()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    Qt::Alignment align = Qt::AlignTrailing;
    if (m_textEditor.data()->block().layout()->textOption().textDirection() == Qt::RightToLeft)
        align = Qt::AlignLeading;
    m_textEditor.data()->setHorizontalTextAlignment(align);
}

void TextTool::alignCenter()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->setHorizontalTextAlignment(Qt::AlignHCenter);
}

void TextTool::alignBlock()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->setHorizontalTextAlignment(Qt::AlignJustify);
}

void TextTool::superScript(bool on)
{
    if (!m_allowActions || !m_textEditor.data()) return;
    if (on)
        m_actionFormatSub->setChecked(false);
    m_textEditor.data()->setVerticalTextAlignment(on ? Qt::AlignTop : Qt::AlignVCenter);
}

void TextTool::subScript(bool on)
{
    if (!m_allowActions || !m_textEditor.data()) return;
    if (on)
        m_actionFormatSuper->setChecked(false);
    m_textEditor.data()->setVerticalTextAlignment(on ? Qt::AlignBottom : Qt::AlignVCenter);
}

void TextTool::increaseIndent()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->increaseIndent();
    m_actionFormatDecreaseIndent->setEnabled(m_textEditor.data()->blockFormat().leftMargin() > 0.);
}

void TextTool::decreaseIndent()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->decreaseIndent();
    m_actionFormatDecreaseIndent->setEnabled(m_textEditor.data()->blockFormat().leftMargin() > 0.);
}

void TextTool::decreaseFontSize()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->decreaseFontSize();
}

void TextTool::increaseFontSize()
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->increaseFontSize();
}

void TextTool::setFontFamily(const QString &font)
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->setFontFamily(font);
}

void TextTool::setFontSize (qreal size)
{
    if (!m_allowActions || !m_textEditor.data()) return;
    m_textEditor.data()->setFontSize(size);
}

void TextTool::setDefaultFormat()
{
    m_textEditor.data()->setDefaultFormat();
}

void TextTool::insertIndexMarker()
{
    // TODO handle result when we figure out how to report errors from a tool.
    m_textEditor.data()->insertIndexMarker();
}

void TextTool::setStyle(KoCharacterStyle *style)
{
    m_textEditor.data()->setStyle(style);
    emit charFormatChanged(m_textEditor.data()->charFormat());
}

void TextTool::setStyle(KoParagraphStyle *style)
{
    m_textEditor.data()->setStyle(style);
    emit blockFormatChanged(m_textEditor.data()->blockFormat());
    emit charFormatChanged(m_textEditor.data()->charFormat());
}

void TextTool::insertTable()
{
    TableDialog *dia = new TableDialog(0);
    if (dia->exec() == TableDialog::Accepted)
        m_textEditor.data()->insertTable(dia->rows(), dia->columns());
    delete dia;
}

void TextTool::insertTableQuick(int rows, int columns)
{
    m_textEditor.data()->insertTable(rows, columns);
}

void TextTool::insertTableRowAbove()
{
    m_textEditor.data()->insertTableRowAbove();
}

void TextTool::insertTableRowBelow()
{
    m_textEditor.data()->insertTableRowBelow();
}

void TextTool::insertTableColumnLeft()
{
    m_textEditor.data()->insertTableColumnLeft();
}

void TextTool::insertTableColumnRight()
{
    m_textEditor.data()->insertTableColumnRight();
}

void TextTool::deleteTableColumn()
{
    m_textEditor.data()->deleteTableColumn();
}

void TextTool::deleteTableRow()
{
    m_textEditor.data()->deleteTableRow();
}

void TextTool::mergeTableCells()
{
    m_textEditor.data()->mergeTableCells();
}

void TextTool::splitTableCells()
{
    m_textEditor.data()->splitTableCells();
}

void TextTool::formatParagraph()
{
    ParagraphSettingsDialog *dia = new ParagraphSettingsDialog(this, m_textEditor.data()->cursor());//TODO  check this with KoTextEditor
    dia->setUnit(canvas()->unit());
    connect(dia, SIGNAL(startMacro(const QString&)), this, SLOT(startMacro(const QString&)));//TODO
    connect(dia, SIGNAL(stopMacro()), this, SLOT(stopMacro()));

    dia->exec();
    delete dia;
}

void TextTool::toggleShowChanges(bool on)//TODO transfer this in KoTextEditor
{
    m_actionShowChanges->setChecked(on);
    ShowChangesCommand *command = new ShowChangesCommand(on, m_textShapeData->document(), this->canvas());
    connect(command, SIGNAL(toggledShowChange(bool)), m_actionShowChanges, SLOT(setChecked(bool)));
    m_textEditor.data()->addCommand(command);
}

void TextTool::toggleRecordChanges(bool on)
{
    m_actionRecordChanges->setChecked(on);
    if (m_changeTracker)
        m_changeTracker->setRecordChanges(on);
}

void TextTool::configureChangeTracking()
{
    if (m_changeTracker) {
        QColor insertionBgColor, deletionBgColor, formatChangeBgColor;
        insertionBgColor = m_changeTracker->getInsertionBgColor();
        deletionBgColor = m_changeTracker->getDeletionBgColor();
        formatChangeBgColor = m_changeTracker->getFormatChangeBgColor();
        QString authorName = m_changeTracker->authorName();
        KoChangeTracker::ChangeSaveFormat changeSaveFormat = m_changeTracker->saveFormat();

        ChangeConfigureDialog changeDialog(insertionBgColor, deletionBgColor, formatChangeBgColor, authorName, changeSaveFormat, canvas()->canvasWidget());

        if (changeDialog.exec()) {
            m_changeTracker->setInsertionBgColor(changeDialog.getInsertionBgColor());
            m_changeTracker->setDeletionBgColor(changeDialog.getDeletionBgColor());
            m_changeTracker->setFormatChangeBgColor(changeDialog.getFormatChangeBgColor());
            m_changeTracker->setAuthorName(changeDialog.authorName());
            m_changeTracker->setSaveFormat(changeDialog.saveFormat());
            writeConfig();
        }
    }
}

void TextTool::testSlot(bool on)
{
    kDebug(32500) << "signal received. bool:" << on;
}

void TextTool::selectAll()
{
    KoTextEditor *textEditor = m_textEditor.data();
    if (!textEditor || !m_textShapeData)
        return;
    const int selectionLength = qAbs(textEditor->position() - textEditor->anchor());
    QTextBlock lastBlock = m_textShapeData->document()->lastBlock();
    textEditor->setPosition(lastBlock.position() + lastBlock.length() - 1);
    textEditor->setPosition(0, QTextCursor::KeepAnchor);
    repaintSelection();
    if (selectionLength != qAbs(textEditor->position() - textEditor->anchor())) // it actually changed
        emit selectionChanged(true);
}

void TextTool::startMacro(const QString &title)
{
    if (title != i18n("Key Press") && title !=i18n("Autocorrection")) //dirty hack while waiting for refactor of text editing
        m_textTyping = false;
    else
        m_textTyping = true;

    if (title != i18n("Delete") && title != i18n("Autocorrection")) //same dirty hack as above
        m_textDeleting = false;
    else
        m_textDeleting = true;

    if (m_currentCommand) return;

    class MacroCommand : public QUndoCommand
    {
    public:
        MacroCommand(const QString &title) : QUndoCommand(title), m_first(true) {}
        virtual void redo() {
            if (! m_first)
                QUndoCommand::redo();
            m_first = false;
        }
        virtual bool mergeWith(const QUndoCommand *) {
            return false;
        }
        bool m_first;
    };

    m_currentCommand = new MacroCommand(title);
    m_currentCommandHasChildren = false;
}

void TextTool::stopMacro()
{
    if (!m_currentCommand)
        return;
    if (! m_currentCommandHasChildren)
        delete m_currentCommand;
    m_currentCommand = 0;
}

void TextTool::showStyleManager()
{
    if (!m_textShapeData)
        return;
    KoStyleManager *styleManager = KoTextDocument(m_textShapeData->document()).styleManager();
    Q_ASSERT(styleManager);
    if (!styleManager)
        return;  //don't crash
    StyleManagerDialog *dia = new StyleManagerDialog(canvas()->canvasWidget());
    dia->setStyleManager(styleManager);
    dia->setUnit(canvas()->unit());
    dia->show();
}

void TextTool::startTextEditingPlugin(const QString &pluginId)
{
    KoTextEditingPlugin *plugin = m_textEditingPlugins->plugin(pluginId);
    if (plugin) {
        if (m_textEditor.data()->hasSelection()) {
            int from = m_textEditor.data()->position();
            int to = m_textEditor.data()->anchor();
            if (from > to) // make sure we call the plugin consistently
                qSwap(from, to);
            plugin->checkSection(m_textShapeData->document(), from, to);
        } else
            plugin->finishedWord(m_textShapeData->document(), m_textEditor.data()->position());
    }
}

bool TextTool::isBidiDocument() const
{
    if (m_textEditor)
        return m_textEditor.data()->isBidiDocument();
    return false;
}

void TextTool::resourceChanged(int key, const QVariant &var)
{
    if (m_textEditor.isNull())
        return;
    if (!m_textShapeData)
        return;
    if (m_allowResourceManagerUpdates == false)
        return;
    if (key == KoText::CurrentTextPosition) {
        repaintSelection();
        m_textEditor.data()->setPosition(var.toInt());
        ensureCursorVisible();
    } else if (key == KoText::CurrentTextAnchor) {
        repaintSelection();
        int pos = m_textEditor.data()->position();
        m_textEditor.data()->setPosition(var.toInt());
        m_textEditor.data()->setPosition(pos, QTextCursor::KeepAnchor);
    } else return;

    repaintSelection();
}

void TextTool::isBidiUpdated()
{
    emit blockChanged(m_textEditor.data()->block()); // make sure that the dialogs follow this change
}

void TextTool::insertSpecialCharacter()
{
    if (m_specialCharacterDocker == 0) {
        m_specialCharacterDocker = new InsertCharacter(canvas()->canvasWidget());
        connect(m_specialCharacterDocker, SIGNAL(insertCharacter(const QString&)),
                this, SLOT(insertString(const QString&)));
    }

    m_specialCharacterDocker->show();
}

void TextTool::insertString(const QString& string)
{
    m_textEditor.data()->insertText(string);
}

void TextTool::selectFont()
{
    FontDia *fontDlg = new FontDia(m_textEditor.data()->cursor());//TODO check this with KoTextEditor
    connect(fontDlg, SIGNAL(startMacro(const QString &)), this, SLOT(startMacro(const QString &)));
    connect(fontDlg, SIGNAL(stopMacro()), this, SLOT(stopMacro()));
    fontDlg->exec();
    delete fontDlg;
}

void TextTool::shapeAddedToCanvas()
{
    kDebug();
    if (m_textShape) {
        KoSelection *selection = canvas()->shapeManager()->selection();
        KoShape *shape = selection->firstSelectedShape();
        if (shape != m_textShape && canvas()->shapeManager()->shapes().contains(m_textShape)) {
            // this situation applies when someone, not us, changed the selection by selecting another
            // text shape. Possibly by adding one.
            // Deselect the new shape again, so we can keep editing what we were already editing
            selection->select(m_textShape);
            selection->deselect(shape);
        }
    }
}

void TextTool::shapeDataRemoved()
{
    m_textShapeData = 0;
    m_textShape = 0;
    if (!m_textEditor.isNull() && !m_textEditor.data()->cursor()->isNull()) {
        const QTextDocument *doc = m_textEditor.data()->document();
        Q_ASSERT(doc);
        KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(doc->documentLayout());
        if (!lay || lay->shapes().isEmpty()) {
            emit done();
            return;
        }
        m_textShape = static_cast<TextShape*>(lay->shapes().first());
        m_textShapeData = static_cast<KoTextShapeData*>(m_textShape->userData());
        connect(m_textShapeData, SIGNAL(destroyed (QObject*)), this, SLOT(shapeDataRemoved()));
    }
}

// ---------- editing plugins methods.
void TextTool::editingPluginEvents()
{
    if (m_prevCursorPosition == -1 || m_prevCursorPosition == m_textEditor.data()->position()) {
        kDebug()<<"m_prevCursorPosition="<<m_prevCursorPosition<<"m_textEditor.data()->position()="<<m_textEditor.data()->position();
        return;
    }

    QTextBlock block = m_textEditor.data()->block();
    if (! block.contains(m_prevCursorPosition)) {
        kDebug()<<"m_prevCursorPosition="<<m_prevCursorPosition;
        finishedWord();
        finishedParagraph();
        m_prevCursorPosition = -1;
    } else {
        int from = m_prevCursorPosition;
        int to = m_textEditor.data()->position();
        if (from > to)
            qSwap(from, to);
        QString section = block.text().mid(from - block.position(), to - from);
        kDebug()<<"from="<<from<<"to="<<to;
        if (section.contains(' ')) {
            finishedWord();
            m_prevCursorPosition = -1;
        }
    }
}

void TextTool::finishedWord()
{
    kDebug();
    if (m_textShapeData)
        foreach (KoTextEditingPlugin* plugin, m_textEditingPlugins->values())
            plugin->finishedWord(m_textShapeData->document(), m_prevCursorPosition);
}

void TextTool::finishedParagraph()
{
    kDebug();
    if (m_textShapeData)
        foreach (KoTextEditingPlugin* plugin, m_textEditingPlugins->values())
            plugin->finishedParagraph(m_textShapeData->document(), m_prevCursorPosition);
}

void TextTool::setTextColor(const KoColor &color)
{
    m_textEditor.data()->setTextColor(color.toQColor());
}

void TextTool::setBackgroundColor(const KoColor &color)
{
    m_textEditor.data()->setTextBackgroundColor(color.toQColor());
}

void TextTool::setGrowWidthToFit(bool enabled)
{
    m_textEditor.data()->addCommand(new AutoResizeCommand(m_textShapeData, KoTextShapeData::AutoGrowWidth, enabled));
    updateActions();
}

void TextTool::setGrowHeightToFit(bool enabled)
{
    m_textEditor.data()->addCommand(new AutoResizeCommand(m_textShapeData, KoTextShapeData::AutoGrowHeight, enabled));
    updateActions();
}

void TextTool::setShrinkToFit(bool enabled)
{
    m_textEditor.data()->addCommand(new AutoResizeCommand(m_textShapeData, KoTextShapeData::ShrinkToFitResize, enabled));
    updateActions();
}

void TextTool::readConfig()
{
    if (m_changeTracker) {
        QColor bgColor, defaultColor;
        QString changeAuthor;
        int changeSaveFormat = KoChangeTracker::DELTAXML;
        KConfigGroup interface = KoGlobal::kofficeConfig()->group("Change-Tracking");
        if (interface.exists()) {
            bgColor = interface.readEntry("insertionBgColor", defaultColor);
            m_changeTracker->setInsertionBgColor(bgColor);
            bgColor = interface.readEntry("deletionBgColor", defaultColor);
            m_changeTracker->setDeletionBgColor(bgColor);
            bgColor = interface.readEntry("formatChangeBgColor", defaultColor);
            m_changeTracker->setFormatChangeBgColor(bgColor);
            changeAuthor = interface.readEntry("changeAuthor", changeAuthor);
            if (changeAuthor == "") {
                KUser user(KUser::UseRealUserID);
                m_changeTracker->setAuthorName(user.property(KUser::FullName).toString());
            } else {
                m_changeTracker->setAuthorName(changeAuthor);
            }
            changeSaveFormat = interface.readEntry("changeSaveFormat", changeSaveFormat);
            m_changeTracker->setSaveFormat((KoChangeTracker::ChangeSaveFormat)(changeSaveFormat));
        }
    }
}

void TextTool::writeConfig()
{
    if (m_changeTracker) {
        KConfigGroup interface = KoGlobal::kofficeConfig()->group("Change-Tracking");
        interface.writeEntry("insertionBgColor", m_changeTracker->getInsertionBgColor());
        interface.writeEntry("deletionBgColor", m_changeTracker->getDeletionBgColor());
        interface.writeEntry("formatChangeBgColor", m_changeTracker->getFormatChangeBgColor());
        KUser user(KUser::UseRealUserID);
        QString changeAuthor = m_changeTracker->authorName();
        if (changeAuthor != user.property(KUser::FullName).toString()) {
            interface.writeEntry("changeAuthor", changeAuthor);
        }
        interface.writeEntry("changeSaveFormat", (int)(m_changeTracker->saveFormat()));
    }
}

void TextTool::runUrl(KoPointerEvent *event, QString &url)
{
    bool isLocalLink = (url.indexOf("file:") == 0);
    QString type = KMimeType::findByUrl(url, 0, isLocalLink)->name();

    if (KRun::isExecutableFile(url, type)) {
        QString question = i18n("This link points to the program or script '%1'.\n"
                                "Malicious programs can harm your computer. "
                                "Are you sure that you want to run this program?", url);
        // this will also start local programs, so adding a "don't warn again"
        // checkbox will probably be too dangerous
        int choice = KMessageBox::warningYesNo(0, question, i18n("Open Link?"));
        if (choice != KMessageBox::Yes)
            return;
    }

    event->accept();
    new KRun(url, 0);
}

void TextTool::debugTextDocument()
{
#ifndef NDEBUG
    if (!m_textShapeData)
        return;
    const int CHARSPERLINE = 80; // TODO Make configurable using ENV var?
    const int CHARPOSITION = 278301935;
    KoTextDocument document(m_textShapeData->document());
    KoStyleManager *styleManager = document.styleManager();
    KoInlineTextObjectManager *inlineManager = document.inlineTextObjectManager();

    QTextBlock block = m_textShapeData->document()->begin();
    for (;block.isValid(); block = block.next()) {
        QVariant var = block.blockFormat().property(KoParagraphStyle::StyleId);
        if (!var.isNull()) {
            KoParagraphStyle *ps = styleManager->paragraphStyle(var.toInt());
            kDebug(32500) << "--- Paragraph Style:" << (ps ? ps->name() : QString()) << var.toInt();
        }
        var = block.charFormat().property(KoCharacterStyle::StyleId);
        if (!var.isNull()) {
            KoCharacterStyle *cs = styleManager->characterStyle(var.toInt());
            kDebug(32500) << "--- Character Style:" << (cs ? cs->name() : QString()) << var.toInt();
        }
        int lastPrintedChar = -1;
        QTextBlock::iterator it;
        QString fragmentText;
        QList<QTextCharFormat> inlineCharacters;
        for (it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment fragment = it.fragment();
            if (!fragment.isValid())
                continue;
            QTextCharFormat fmt = fragment.charFormat();
            kDebug(32500) << "changeId: " << fmt.property(KoCharacterStyle::ChangeTrackerId);
            const int fragmentStart = fragment.position() - block.position();
            for (int i = fragmentStart; i < fragmentStart + fragment.length(); i += CHARSPERLINE) {
                if (lastPrintedChar == fragmentStart-1)
                    fragmentText += '|';
                if (lastPrintedChar < fragmentStart || i > fragmentStart) {
                    QString debug = block.text().mid(lastPrintedChar, CHARSPERLINE);
                    lastPrintedChar += CHARSPERLINE;
                    if (lastPrintedChar > block.length())
                        debug += "\\n";
                    kDebug(32500) << debug;
                }
                var = fmt.property(KoCharacterStyle::StyleId);
                QString charStyleLong, charStyleShort;
                if (! var.isNull()) { // named style
                    charStyleShort = QString::number(var.toInt());
                    KoCharacterStyle *cs = styleManager->characterStyle(var.toInt());
                    if (cs)
                        charStyleLong = cs->name();
                }
                if (inlineManager && fmt.hasProperty(KoCharacterStyle::InlineInstanceId)) {
                    QTextCharFormat inlineFmt = fmt;
                    inlineFmt.setProperty(CHARPOSITION, fragmentStart);
                    inlineCharacters << inlineFmt;
                }

                if (fragment.length() > charStyleLong.length())
                    fragmentText += charStyleLong;
                else if (fragment.length() > charStyleShort.length())
                    fragmentText += charStyleShort;
                else if (fragment.length() >= 2)
                    fragmentText += QChar(8230); // elipses



                int rest =  fragmentStart - (lastPrintedChar-CHARSPERLINE) + fragment.length() - fragmentText.length();
                rest = qMin(rest, CHARSPERLINE - fragmentText.length());
                if (rest >= 2)
                    fragmentText = QString("%1%2").arg(fragmentText).arg(' ', rest);
                if (rest >= 0)
                    fragmentText += '|';
                if (fragmentText.length() >= CHARSPERLINE) {
                    kDebug(32500) << fragmentText;
                    fragmentText.clear();
                }
            }
        }
        if (!fragmentText.isEmpty()) {
            kDebug(32500) << fragmentText;
        }
        else if (block.length() == 1) { // no actual tet
            kDebug(32500) << "\\n";
        }
        foreach (QTextCharFormat cf, inlineCharacters) {
            KoInlineObject *object= inlineManager->inlineTextObject(cf);
            kDebug(32500) << "At pos:" << cf.intProperty(CHARPOSITION) << object;
            // kDebug(32500) << "-> id:" << cf.intProperty(577297549);
        }
        QTextList *list = block.textList();
        if (list) {
            if (list->format().hasProperty(KoListStyle::StyleId)) {
                KoListStyle *ls = styleManager->listStyle(list->format().intProperty(KoListStyle::StyleId));
                kDebug(32500) << "   List style applied:" << ls->styleId() << ls->name();
            }
            else
                kDebug(32500) << " +- is a list..." << list;
        }
    }
#endif
}

void TextTool::debugTextStyles()
{
#ifndef NDEBUG
    if (!m_textShapeData)
        return;
    KoTextDocument document(m_textShapeData->document());
    KoStyleManager *styleManager = document.styleManager();

    QSet<int> seenStyles;

    foreach (KoParagraphStyle *style, styleManager->paragraphStyles()) {
        kDebug(32500) << style->styleId() << style->name() << (styleManager->defaultParagraphStyle() == style ? "[Default]" : "");
        KoCharacterStyle *cs = style->characterStyle();
        seenStyles << style->styleId();
        if (cs) {
            kDebug(32500) << "  +- CharStyle: " << cs->styleId() << cs->name();
            kDebug(32500) << "  |  " << cs->font();
            seenStyles << cs->styleId();
        } else {
            kDebug(32500) << "  +- ERROR; no char style found!" << endl;
        }
        KoListStyle *ls = style->listStyle();
        if (ls) { // optional ;)
            kDebug(32500) << "  +- ListStyle: " << ls->styleId() << ls->name()
                << (ls == styleManager->defaultListStyle() ? "[Default]":"");
            foreach (int level, ls->listLevels()) {
                KoListLevelProperties llp = ls->levelProperties(level);
                kDebug(32500) << "  |  level" << llp.level() << " style (enum):" << llp.style();
                if (llp.bulletCharacter().unicode() != 0) {
                    kDebug(32500) << "  |  bullet" << llp.bulletCharacter();
                }
            }
            seenStyles << ls->styleId();
        }
    }

    bool first = true;
    foreach (KoCharacterStyle *style, styleManager->characterStyles()) {
        if (seenStyles.contains(style->styleId()))
            continue;
        if (first) {
            kDebug(32500) << "--- Character styles ---";
            first = false;
        }
        kDebug(32500) << style->styleId() << style->name();
        kDebug(32500) << style->font();
    }

    first = true;
    foreach (KoListStyle *style, styleManager->listStyles()) {
        if (seenStyles.contains(style->styleId()))
            continue;
        if (first) {
            kDebug(32500) << "--- List styles ---";
            first = false;
        }
        kDebug(32500) << style->styleId() << style->name()
                << (style == styleManager->defaultListStyle() ? "[Default]":"");
    }
#endif
}

#include <TextTool.moc>
