// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFocusEvent>
#include <QFontDatabase>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <cstdint>
#include <functional>
#include <memory>

#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Galaxy/Galaxy.h"
#include "Nebula/Settings/SettingsFile.h"
#include "Outlets.h"
#include "Probe.h"
#include "PropertyEditor.h"
#include "Settings/SettingsValueEditor.h"
#include "TestComponents.h"
#include "ValueEditors.h"

namespace
{

enum Column : std::uint8_t
{
    NameColumn,
    ValueColumn,
};

// A galaxy whose stargate exposes the mode of its Probe, as "speed", and its color, as "tint".
const QByteArray Styled = R"({"version": 1,
    "components": [{"name": "Probe", "type": "Test.Probe", "properties": {"mode": "Fast"}},
                   {"name": "Gate", "type": "Nebula.Stargate",
                    "exposed": [{"name": "speed", "component": "Probe", "property": "mode"},
                                {"name": "tint", "component": "Probe", "property": "color"}]}]})";

// The row of the property named name in table; -1 when it shows none.
int rowOf(const QTableWidget& table, const QString& name)
{
    for(int row = 0; row < table.rowCount(); ++row)
    {
        const QTableWidgetItem* item = table.item(row, NameColumn);
        if(item != nullptr && item->text() == name)
        {
            return row;
        }
    }
    return -1;
}

// What the cell of the value of row of table shows, as its delegate writes it.
QString shownValue(const QTableWidget& table, int row)
{
    const auto* delegate = qobject_cast<QStyledItemDelegate*>(table.itemDelegateForColumn(ValueColumn));
    return delegate->displayText(table.item(row, ValueColumn)->data(Qt::EditRole), QLocale());
}

// The cell of the value of row of table, painted by its delegate.
QImage paintedValue(const QTableWidget& table, int row)
{
    QStyleOptionViewItem option;
    option.initFrom(&table);
    option.widget = &table;
    option.rect = QRect(0, 0, 200, 30);
    option.decorationSize = QSize(16, 16);
    QImage image(option.rect.size(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    table.itemDelegateForColumn(ValueColumn)->paint(&painter, option, table.model()->index(row, ValueColumn));
    return image;
}

// Whether image has a pixel of color, opaque, in 8 bits a channel.
bool paints(const QImage& image, const QColor& color)
{
    for(int y = 0; y < image.height(); ++y)
    {
        for(int x = 0; x < image.width(); ++x)
        {
            if(image.pixel(x, y) == color.rgba()) // In 8 bits a channel, as a palette may not be
            {
                return true;
            }
        }
    }
    return false;
}

// What the cells of the column column of table show, row by row.
QStringList column(const QTableWidget& table, int column)
{
    QStringList texts;
    for(int row = 0; row < table.rowCount(); ++row)
    {
        const QTableWidgetItem* item = table.item(row, column);
        texts.append(item != nullptr ? item->text() : QString());
    }
    return texts;
}

// The table of the properties of a component in editor, rather than the one of the values of a settings file: the page
// shown is the last child of their stack, as it is raised.
QTableWidget* componentTable(const PropertyEditor& editor)
{
    const auto* settings = editor.findChild<SettingsValueEditor*>();
    for(QTableWidget* table : editor.findChildren<QTableWidget*>())
    {
        if(!settings->isAncestorOf(table))
        {
            return table;
        }
    }
    return nullptr;
}

} // namespace

// An object a component adds, whose properties are of the types PropertyDelegate edits.
class Look : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Probe::Mode mode MEMBER m_mode NOTIFY modeChanged)
    Q_PROPERTY(QColor color MEMBER m_color NOTIFY colorChanged)

  public:
    explicit Look(QObject* parent = nullptr) : QObject(parent) {}

  signals:
    void modeChanged();
    void colorChanged();

  private:
    Probe::Mode m_mode = Probe::Mode::Normal;
    QColor m_color{0x12, 0xab, 0x34};
};

// PropertyEditor: the properties it shows of a component and of its objects, as they come and go, and the values of a
// settings file in their place.
class TestPropertyEditor : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void objectsThatFollowAProperty();
    void writtenThroughItsEdit();
    void settingsFile();

    void typedValuesAreShown();
    void enumeration();
    void flags();
    void color();
    void url();
    void urlChosenInADialog();
    void nothingIsEditedWhileRunning();
    void typedPropertiesOfObjectsAndGalaxies();

  private:
    // Shows a Probe1 of m_application in m_editor, mode Fast, styles Bold and Italic, a translucent color and a URL;
    // returns it.
    Probe* showProbe();
    // The editor of the value of row of m_table, opened as a click opens it.
    template <typename Editor> Editor* openEditor(int row);

    // An application with an Outlets1, shown by m_editor, whose table is m_table.
    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<PropertyEditor> m_editor;
    Outlets* m_outlets = nullptr;
    QTableWidget* m_table = nullptr;
};

void TestPropertyEditor::initTestCase()
{
    registerTestComponents();
    // Once, before any test fails on a warning: the offscreen platform warns that it has no fonts the first time a text
    // is laid out, and the table lays out the names and values of the properties.
    QFontDatabase::families();
}

void TestPropertyEditor::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    m_outlets = qobject_cast<Outlets*>(m_application->instantiate(testComponentId(QStringLiteral("Outlets"))));
    QVERIFY(m_outlets != nullptr);
    m_editor = std::make_unique<PropertyEditor>(*m_application, Nebula::ComponentFactory::instance());
    m_editor->setComponent(m_outlets);
    m_table = componentTable(*m_editor);
    QVERIFY(m_table != nullptr);
}

void TestPropertyEditor::cleanup()
{
    m_table = nullptr;
    m_editor.reset();
    m_application.reset();
}

// Its count, edited in the table, adds or destroys objects: their groups follow once the events are processed, the
// cell edited living until then.
void TestPropertyEditor::objectsThatFollowAProperty()
{
    QCOMPARE(column(*m_table, NameColumn), (QStringList{"count", "active"}));

    QTableWidgetItem* count = m_table->item(0, ValueColumn);
    count->setData(Qt::EditRole, 2);
    QCOMPARE(m_outlets->count(), 2);
    QCOMPARE(m_table->item(0, ValueColumn), count);
    QTRY_COMPARE(column(*m_table, NameColumn),
                 (QStringList{"count", "active", "object1", "level", "enabled", "object2", "level", "enabled"}));

    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 1);
    QCOMPARE(m_outlets->count(), 1);
    QTRY_COMPARE(column(*m_table, NameColumn), (QStringList{"count", "active", "object1", "level", "enabled"}));
    QCOMPARE(column(*m_table, ValueColumn).at(0), QStringLiteral("1"));
}

// A value edited is written through the edit it is given, a step of the history of NebulaStudio, named after the
// property and the component; one the edit does not write is not written.
void TestPropertyEditor::writtenThroughItsEdit()
{
    m_outlets->setObjectName(QStringLiteral("Outlets1"));
    QStringList steps;
    bool writes = true;
    m_editor->setEdit(
        [&steps, &writes](const QString& what, const std::function<void()>& change)
        {
            steps.append(what);
            if(writes)
            {
                change();
            }
        });
    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 2);
    QCOMPARE(steps, QStringList{QStringLiteral("Change count of Outlets1")});
    QCOMPARE(m_outlets->count(), 2);

    writes = false;
    m_table->item(0, ValueColumn)->setData(Qt::EditRole, 5);
    QCOMPARE(steps.size(), 2);
    QCOMPARE(m_outlets->count(), 2);
    QCOMPARE(column(*m_table, ValueColumn).at(0), QStringLiteral("2")); // What it holds, shown again
}

// A settings file selected is shown in place of a component, what it refuses to write told by the editor.
void TestPropertyEditor::settingsFile()
{
    const QTemporaryDir directory;
    Nebula::SettingsFile& file = *m_application->addSettingsFile();
    file.setDirectory(directory.path());
    QVERIFY(file.add({.component = m_outlets, .object = m_outlets, .property = QStringLiteral("count")}));
    const auto* pages = m_editor->findChild<QStackedWidget*>();
    auto* settings = m_editor->findChild<SettingsValueEditor*>();
    const QSignalSpy refused(m_editor.get(), &PropertyEditor::refused);

    m_editor->showSelection(m_outlets, &file);
    QCOMPARE(pages->currentWidget(), settings);
    QCOMPARE(settings->file(), &file);
    auto* values = settings->findChild<QTableWidget*>();
    values->item(1, ValueColumn)->setData(Qt::EditRole, QStringLiteral("two"));
    QCOMPARE(refused.count(), 1);

    m_editor->showSelection(m_outlets, nullptr);
    QVERIFY(pages->currentWidget()->isAncestorOf(m_table));
    QCOMPARE(settings->file(), nullptr);
    QCOMPARE(column(*m_table, NameColumn), (QStringList{"count", "active"}));

    // Nothing selected.
    m_editor->setSettingsFile(&file);
    m_editor->setSettingsFile(nullptr);
    QVERIFY(pages->currentWidget()->isAncestorOf(m_table));
    QCOMPARE(m_table->rowCount(), 0);
}

Probe* TestPropertyEditor::showProbe()
{
    auto* probe = qobject_cast<Probe*>(m_application->instantiate(testComponentId(QStringLiteral("Probe"))));
    probe->setProperty("mode", QVariant::fromValue(Probe::Mode::Fast));
    probe->setStyles(Probe::Bold | Probe::Italic);
    probe->setProperty("color", QColor(0x12, 0x34, 0x56, 0x80));
    probe->setProperty("url", QUrl(QStringLiteral("https://example.com/a b")));
    m_editor->setComponent(probe);
    return probe;
}

template <typename Editor> Editor* TestPropertyEditor::openEditor(int row)
{
    const QModelIndex index = m_table->model()->index(row, ValueColumn);
    m_table->setCurrentIndex(index); // Closes the editor of another value, as a click does
    m_table->edit(index);            // As the trigger CurrentChanged does, in a table shown
    return qobject_cast<Editor*>(m_table->indexWidget(index));
}

// An enumeration, flags, a color and a URL are shown by their names, or their text, which follow the values they
// notify.
void TestPropertyEditor::typedValuesAreShown()
{
    Probe* probe = showProbe();
    const QStringList names = column(*m_table, NameColumn);
    for(const char* name : {"mode", "styles", "color", "url"})
    {
        QVERIFY2(names.contains(QLatin1StringView(name)), name);
    }
    QVERIFY(!names.contains(u"point")); // No editor for it
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("mode"))), QStringLiteral("Fast"));
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("styles"))), QStringLiteral("Bold|Italic"));
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("color"))), QStringLiteral("#80123456"));
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("url"))), QStringLiteral("https://example.com/a b"));

    probe->setProperty("mode", QVariant::fromValue(Probe::Mode::Slow));
    probe->setStyles({});
    probe->setProperty("color", QColor(0x12, 0x34, 0x56));
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("mode"))), QStringLiteral("Slow"));
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("styles"))), QString());                // No flag
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("color"))), QStringLiteral("#123456")); // Opaque
    probe->setProperty("color", QColor());
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("color"))), QString()); // Not valid

    // Its swatch, edged in the color of the edges of the palette, next to its name.
    probe->setProperty("color", QColor(0x12, 0xab, 0x34));
    const QImage painted = paintedValue(*m_table, rowOf(*m_table, QStringLiteral("color")));
    QVERIFY(paints(painted, QColor(0x12, 0xab, 0x34)));
    QVERIFY(paints(painted, m_table->palette().color(QPalette::Mid)));
    QVERIFY(!paints(paintedValue(*m_table, rowOf(*m_table, QStringLiteral("mode"))), QColor(0x12, 0xab, 0x34)));
}

// An enumeration is chosen in a list of the names of its values, written as soon as it is chosen.
void TestPropertyEditor::enumeration()
{
    Probe* probe = showProbe();
    const int row = rowOf(*m_table, QStringLiteral("mode"));
    auto* editor = openEditor<EnumEditor>(row);
    QVERIFY(editor != nullptr);
    QStringList names;
    for(int index = 0; index < editor->count(); ++index)
    {
        names.append(editor->itemText(index));
    }
    QCOMPARE(names, (QStringList{"Slow", "Normal", "Fast"}));
    QCOMPARE(editor->currentText(), QStringLiteral("Fast"));

    editor->setCurrentIndex(1);
    emit editor->activated(1);
    QCOMPARE(probe->property("mode").value<Probe::Mode>(), Probe::Mode::Normal);
    QCOMPARE(shownValue(*m_table, row), QStringLiteral("Normal"));
}

// Flags are checked in a list of the names of their values, each written as soon as it is checked or unchecked.
void TestPropertyEditor::flags()
{
    Probe* probe = showProbe();
    const int row = rowOf(*m_table, QStringLiteral("styles"));
    auto* editor = openEditor<FlagsEditor>(row);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->count(), 3);
    QCOMPARE(editor->item(0)->text(), QStringLiteral("Bold"));
    QCOMPARE(editor->item(0)->checkState(), Qt::Checked);
    QCOMPARE(editor->item(1)->checkState(), Qt::Checked);
    QCOMPARE(editor->item(2)->checkState(), Qt::Unchecked);

    editor->item(2)->setCheckState(Qt::Checked);
    QCOMPARE(probe->styles(), Probe::Bold | Probe::Italic | Probe::Underline);
    editor->item(0)->setCheckState(Qt::Unchecked);
    QCOMPARE(probe->styles(), Probe::Italic | Probe::Underline);
    QCOMPARE(shownValue(*m_table, row), QStringLiteral("Italic|Underline"));
    editor->item(1)->setCheckState(Qt::Unchecked);
    editor->item(2)->setCheckState(Qt::Unchecked);
    QCOMPARE(probe->styles(), Probe::Styles());
}

// A color is chosen in a dialog, which a double-click, F2 or Space opens, alpha channel included.
void TestPropertyEditor::color()
{
    Probe* probe = showProbe();
    const int row = rowOf(*m_table, QStringLiteral("color"));
    QCOMPARE(openEditor<QWidget>(row), nullptr); // Not in the cell

    // Each dialog shown asks for the size of its contents, which the offscreen platform does not follow.
    const auto shown = [] { QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()"); };
    shown();
    QTest::keyClick(m_table, Qt::Key_F2);
    auto* dialog = m_table->findChild<QColorDialog*>();
    QVERIFY(dialog != nullptr);
    QVERIFY(dialog->testOption(QColorDialog::ShowAlphaChannel));
    QCOMPARE(dialog->currentColor(), QColor(0x12, 0x34, 0x56, 0x80));
    dialog->setCurrentColor(QColor(0xfe, 0xdc, 0xba, 0x40));
    dialog->accept();
    QCOMPARE(probe->property("color").value<QColor>(), QColor(0xfe, 0xdc, 0xba, 0x40));
    QCOMPARE(shownValue(*m_table, row), QStringLiteral("#40fedcba"));
    QTRY_COMPARE(m_table->findChild<QColorDialog*>(), nullptr); // Destroyed once closed

    // Cancelled: nothing changes.
    shown();
    QTest::keyClick(m_table, Qt::Key_Space);
    dialog = m_table->findChild<QColorDialog*>();
    QVERIFY(dialog != nullptr);
    dialog->setCurrentColor(Qt::black);
    dialog->reject();
    QCOMPARE(probe->property("color").value<QColor>(), QColor(0xfe, 0xdc, 0xba, 0x40));
}

// A URL is typed in a field, written once it is done.
void TestPropertyEditor::url()
{
    Probe* probe = showProbe();
    const int row = rowOf(*m_table, QStringLiteral("url"));
    auto* editor = openEditor<UrlEditor>(row);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->text(), QStringLiteral("https://example.com/a b"));

    editor->setText(QStringLiteral("https://nebula.example/page"));
    QTest::keyClick(editor, Qt::Key_Return); // Written, and the editor closed, once the events are processed
    QTRY_COMPARE(probe->property("url").toUrl(), QUrl(QStringLiteral("https://nebula.example/page")));
    QCOMPARE(shownValue(*m_table, row), QStringLiteral("https://nebula.example/page"));
    QTRY_COMPARE(m_table->indexWidget(m_table->model()->index(row, ValueColumn)), nullptr);

    // An absolute path is a local file, though its drive would be a scheme.
    const QString path = QDir::currentPath() + QStringLiteral("/file.txt");
    editor = openEditor<UrlEditor>(row);
    QVERIFY(editor != nullptr);
    editor->setText(path);
    QTest::keyClick(editor, Qt::Key_Return);
    QTRY_COMPARE(probe->property("url").toUrl(), QUrl::fromLocalFile(path));
}

// The button of the field of a URL chooses a local file, written as soon as it is chosen: the field stays while the
// dialog has the focus.
void TestPropertyEditor::urlChosenInADialog()
{
    Probe* probe = showProbe();
    const int row = rowOf(*m_table, QStringLiteral("url"));
    auto* editor = openEditor<UrlEditor>(row);
    QVERIFY(editor != nullptr);
    QCOMPARE(editor->actions().size(), 1);

    // The dialog shown asks for the size of its contents, which the offscreen platform does not follow.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support propagateSizeHints()");
    editor->actions().first()->trigger();
    auto* dialog = editor->findChild<QFileDialog*>();
    QVERIFY(dialog != nullptr);
    QVERIFY(editor->isBrowsing());
    QFocusEvent focusOut(QEvent::FocusOut, Qt::ActiveWindowFocusReason);
    QApplication::sendEvent(editor, &focusOut);
    QApplication::processEvents();
    QCOMPARE(m_table->indexWidget(m_table->model()->index(row, ValueColumn)), editor);

    const QUrl chosen = QUrl::fromLocalFile(QDir::currentPath() + QStringLiteral("/chosen.txt"));
    emit dialog->urlSelected(chosen);
    dialog->reject();
    QCOMPARE(probe->property("url").toUrl(), chosen);
    QCOMPARE(editor->value().toUrl(), chosen);
    QVERIFY(!editor->isBrowsing());
}

// While the application runs, the values are only shown: no editor opens, nor the dialog of a color.
void TestPropertyEditor::nothingIsEditedWhileRunning()
{
    Probe* probe = showProbe();
    QVERIFY(m_application->start());
    QCOMPARE(m_table->editTriggers(), QAbstractItemView::NoEditTriggers);
    m_table->setCurrentIndex(m_table->model()->index(rowOf(*m_table, QStringLiteral("mode")), ValueColumn));
    QTest::keyClick(m_table, Qt::Key_F2);
    QCOMPARE(m_table->indexWidget(m_table->currentIndex()), nullptr);
    m_table->setCurrentIndex(m_table->model()->index(rowOf(*m_table, QStringLiteral("color")), ValueColumn));
    QTest::keyClick(m_table, Qt::Key_F2);
    QTest::keyClick(m_table, Qt::Key_Space);
    QCOMPARE(m_table->findChild<QColorDialog*>(), nullptr);
    // Shown still.
    QCOMPARE(shownValue(*m_table, rowOf(*m_table, QStringLiteral("mode"))), QStringLiteral("Fast"));
    m_application->stop();
    QCOMPARE(probe->property("mode").value<Probe::Mode>(), Probe::Mode::Fast);
}

// The properties of these types an object of a component has, and those a galaxy exposes, are edited as those of a
// component.
void TestPropertyEditor::typedPropertiesOfObjectsAndGalaxies()
{
    auto* probe = qobject_cast<Probe*>(m_application->instantiate(testComponentId(QStringLiteral("Probe"))));
    auto* look = new Look(probe);
    probe->addObject(look, QStringLiteral("look"));
    m_editor->setComponent(probe);
    const int title = rowOf(*m_table, QStringLiteral("look"));
    QVERIFY(title >= 0);
    QCOMPARE(column(*m_table, NameColumn).mid(title), (QStringList{"look", "mode", "color"}));
    auto* editor = openEditor<EnumEditor>(title + 1);
    QVERIFY(editor != nullptr);
    editor->setCurrentIndex(2);
    emit editor->activated(2);
    QCOMPARE(look->property("mode").value<Probe::Mode>(), Probe::Mode::Fast);
    m_table->setCurrentIndex({}); // Its editor closed, as a click elsewhere closes it

    const QTemporaryDir directory;
    const QString filePath = directory.filePath(QStringLiteral("Styled.nbp"));
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(Styled);
    }
    Nebula::Galaxy* galaxy = m_application->instantiateGalaxy(filePath);
    QVERIFY(galaxy != nullptr);
    m_editor->setComponent(galaxy);
    QCOMPARE(column(*m_table, NameColumn), (QStringList{"speed", "tint"}));
    QCOMPARE(shownValue(*m_table, 0), QStringLiteral("Fast"));
    editor = openEditor<EnumEditor>(0);
    QVERIFY(editor != nullptr);
    editor->setCurrentIndex(0);
    emit editor->activated(0);
    QCOMPARE(galaxy->exposedProperty(QStringLiteral("speed"))->object->property("mode").value<Probe::Mode>(),
             Probe::Mode::Slow);
}

QTEST_MAIN(TestPropertyEditor)
#include "TestPropertyEditor.moc"
