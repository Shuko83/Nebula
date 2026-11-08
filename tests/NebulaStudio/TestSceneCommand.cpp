// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <memory>
#include <vector>

#include "AnchorItem.h"
#include "ComponentItem.h"
#include "EditionScene.h"
#include "InterfaceItem.h"
#include "LinkItem.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/ComponentFactory.h"
#include "Nebula/Standard/IHistory.h"
#include "SceneCommand.h"
#include "SceneItems.h"
#include "TestComponents.h"
#include "ZoneItem.h"

using namespace Qt::StringLiterals;

namespace
{

// Where the components are shown, far enough from each other for their link to be clicked between them.
constexpr QPointF SourceAt(120, 80);
constexpr QPointF SinkAt(540, 160);
// Where a zone is added, below them, and where a component is dropped.
constexpr QPointF ZoneAt(120, 320);
constexpr QPointF DroppedAt(400, 420);
// The type of what the list of components drags, see ComponentModel::MimeType.
constexpr auto ComponentMimeType = "application/x-nebula-component";

// A history as small as the one of HistoryComponent, which the test cannot link: the commands pushed, done in their
// order, then those undone.
class Stack : public Nebula::IHistory
{
  public:
    void push(std::unique_ptr<Nebula::Command> command) override
    {
        m_commands.resize(m_done);
        command->execute();
        m_commands.push_back(std::move(command));
        ++m_done;
    }
    bool undo() override
    {
        if(m_done == 0)
        {
            return false;
        }
        m_commands.at(--m_done)->undo();
        return true;
    }
    bool redo() override
    {
        if(m_done == m_commands.size())
        {
            return false;
        }
        m_commands.at(m_done++)->execute();
        return true;
    }
    void clear() override
    {
        m_commands.clear();
        m_done = 0;
    }
    [[nodiscard]] bool isUndoActive() const override
    {
        return true;
    }
    [[nodiscard]] bool isRedoActive() const override
    {
        return true;
    }

    // The labels of the commands done, the oldest first.
    [[nodiscard]] QStringList done() const
    {
        QStringList labels;
        for(std::size_t index = 0; index < m_done; ++index)
        {
            labels.append(m_commands.at(index)->label());
        }
        return labels;
    }
    // The last command done; nullptr when there is none.
    [[nodiscard]] Nebula::Command* last() const
    {
        return m_done > 0 ? m_commands.at(m_done - 1).get() : nullptr;
    }

  private:
    std::vector<std::unique_ptr<Nebula::Command>> m_commands;
    std::size_t m_done = 0;
};

// What scene shows of the component named name; nullptr when it does not show it.
ComponentItem* componentItem(const QGraphicsScene& scene, const QString& name)
{
    const QList<ComponentItem*> shown = objectsAmong<ComponentItem>(scene.items());
    const auto found = std::ranges::find_if(shown, [&name](const ComponentItem* item)
                                            { return item->component().objectName() == name; });
    return found != shown.end() ? *found : nullptr;
}

// The interface named name of the component named component that scene shows; nullptr when there is none.
InterfaceItem* interfaceItem(const QGraphicsScene& scene, const QString& component, const QString& name)
{
    const ComponentItem* item = componentItem(scene, component);
    if(item == nullptr)
    {
        return nullptr;
    }
    const QList<InterfaceItem*> interfaces = item->consumedInterfaces() + item->providedInterfaces();
    const auto found = std::ranges::find(interfaces, name, &InterfaceItem::name);
    return found != interfaces.end() ? *found : nullptr;
}

// Has project, as NebulaStudio holds it, show the component named name at at.
void place(QJsonObject& project, const QString& name, QPointF at)
{
    QJsonArray components = project.value(u"components").toArray();
    for(QJsonValueRef value : components)
    {
        QJsonObject component = value.toObject();
        if(component.value(u"name").toString() == name)
        {
            component.insert(u"x", at.x());
            component.insert(u"y", at.y());
            value = component;
        }
    }
    project.insert(u"components", components);
}

// Sends the viewport of view the mouse event type at at, a position in its scene, as the window system would.
void send(QGraphicsView& view, QEvent::Type type, QPointF at, Qt::MouseButton button, Qt::MouseButtons buttons,
          Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    const QPoint position = view.mapFromScene(at);
    QMouseEvent event(type, position, view.viewport()->mapToGlobal(position), button, buttons, modifiers);
    QApplication::sendEvent(view.viewport(), &event);
}

// Drags from from to to, positions in the scene of view, with the left button, through several moves.
void drag(QGraphicsView& view, QPointF from, QPointF to)
{
    send(view, QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    for(const qreal part : {0.25, 0.5, 0.75, 1.0})
    {
        send(view, QEvent::MouseMove, from + (to - from) * part, Qt::NoButton, Qt::LeftButton);
    }
    send(view, QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
}

// Clicks view at at, a position in its scene, with the left button.
void clickAt(QGraphicsView& view, QPointF at)
{
    send(view, QEvent::MouseButtonPress, at, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton);
}

// Double-clicks view at at, a position in its scene, as the window system does: a click, then a double click.
void doubleClick(QGraphicsView& view, QPointF at)
{
    clickAt(view, at);
    send(view, QEvent::MouseButtonDblClick, at, Qt::LeftButton, Qt::LeftButton);
    send(view, QEvent::MouseButtonRelease, at, Qt::LeftButton, Qt::NoButton);
}

// Opens the context menu of view at at, a position in its scene, and chooses the entry telling chosen, which closes it.
void chooseFromMenu(QGraphicsView& view, QPointF at, const QString& chosen)
{
    // What the offscreen platform cannot do for a menu.
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()");
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support grabbing the keyboard");
    QTimer::singleShot(0, &view,
                       [&chosen]
                       {
                           auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
                           QVERIFY(menu != nullptr);
                           const QList<QAction*> actions = menu->actions();
                           const auto entry = std::ranges::find(actions, chosen, &QAction::text);
                           QVERIFY2(entry != actions.end(), qPrintable(chosen));
                           menu->setActiveAction(*entry);
                           QTest::keyClick(menu, Qt::Key_Return);
                       });
    const QPoint position = view.mapFromScene(at);
    QContextMenuEvent event(QContextMenuEvent::Mouse, position, view.viewport()->mapToGlobal(position));
    QApplication::sendEvent(view.viewport(), &event);
}

// Drops on view at at, a position in its scene, what the list of components drags of componentId.
void dropComponent(QGraphicsView& view, QPointF at, const QString& componentId)
{
    QMimeData data;
    data.setData(QString::fromLatin1(ComponentMimeType), componentId.toUtf8());
    const QPoint position = view.mapFromScene(at);
    QDragEnterEvent enter(position, Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &enter);
    QDropEvent drop(position, Qt::CopyAction, &data, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(view.viewport(), &drop);
}

// Types text in place of the name edited in place among the children of item, then presses Enter; false when none is.
bool typeName(QGraphicsScene& scene, const QGraphicsItem& item, const QString& text)
{
    const QList<QGraphicsItem*> children = item.childItems();
    const auto editor =
        std::ranges::find_if(children, [](const QGraphicsItem* child)
                             { return child->type() == QGraphicsTextItem::Type && child->isVisible(); });
    if(editor == children.end())
    {
        return false;
    }
    auto* nameEditor = static_cast<QGraphicsTextItem*>(*editor);
    nameEditor->setPlainText(text);
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    return scene.sendEvent(nameEditor, &enter);
}

// The middle of the line of link, in the scene.
QPointF middle(const LinkItem& link)
{
    return link.path().pointAtPercent(0.5);
}

// A galaxy of a Source and a Sink bound to it.
const QByteArray Pair = R"({"version": 1,
    "components": [{"name": "Source", "type": "Test.Source", "x": 120, "y": 80},
                   {"name": "Sink", "type": "Test.Sink", "x": 540, "y": 160}],
    "bindings": [{"consumer": "Sink", "consumed": "object", "provider": "Source", "provided": "object"}]})";

} // namespace

// The steps of the history of an EditionScene: each edit, by the mouse, its menus, its keys or edit(), is one
// SceneCommand, which undoing gives the scene exactly what it showed before, and executing again what it showed after.
class TestSceneCommand : public QObject
{
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void cleanup();

    void dragIsOneStep();
    void clickChangesNothing();
    void removedComponentComesBackWithItsLinks();
    void removedLinkComesBack();
    void linkDrawnIsOneStep();
    void renamedInPlace();
    void droppedComponent();
    void menuEntryIsOneStep();
    void pasteIsOneStep();
    void cutIsOneStep();
    void duplicateIsOneStep();
    void zoneMovedResizedAndRenamed();
    void anchorAddedMovedAndRemoved();
    void propertyEditedThroughEdit();
    void descriptionKeepsTheComponents();
    void nothingKeptWithoutHistory();
    void notWhileRunning();
    void stepsWhereTheProjectIsSaved();
    void galaxyScene();

  private:
    // Checks that the last step of the history is named what, and that it goes from before, a snapshot of the scene,
    // to after, which undoing it, then executing it again, gives the scene each time.
    void checkStep(const QString& what, const QJsonObject& before);

    std::unique_ptr<Nebula::Universe> m_application;
    std::unique_ptr<EditionScene> m_scene;
    std::unique_ptr<QGraphicsView> m_view;
    Stack m_history;
};

void TestSceneCommand::initTestCase()
{
    registerTestComponents();
    QFontDatabase::families(); // The offscreen platform warns, once, that it has no fonts: before failOnWarning()
}

void TestSceneCommand::init()
{
    QTest::failOnWarning(); // What is expected is told with QTest::ignoreMessage()
    m_application = std::make_unique<Nebula::Universe>();
    Nebula::Component* source = m_application->instantiate(testComponentId(u"Source"_s));
    Nebula::Component* sink = m_application->instantiate(testComponentId(u"Sink"_s));
    QVERIFY(sink->bindInterface(u"object"_s, *source, u"object"_s));
    QJsonObject project = m_application->save();
    place(project, u"Source1"_s, SourceAt);
    place(project, u"Sink1"_s, SinkAt);
    m_scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance());
    QCOMPARE(m_scene->load(project), QStringList());
    m_history.clear();
    m_scene->setHistory(&m_history);

    m_view = std::make_unique<QGraphicsView>(m_scene.get());
    m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setAcceptDrops(true);
    m_view->resize(800, 600);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
}

void TestSceneCommand::cleanup()
{
    m_view.reset();
    m_history.clear();
    m_scene.reset();
    m_application.reset();
}

void TestSceneCommand::checkStep(const QString& what, const QJsonObject& before)
{
    const QJsonObject after = m_scene->snapshot();
    QVERIFY(after != before);
    QCOMPARE(m_history.done().size(), 1);
    QCOMPARE(m_history.done().constLast(), what);
    const QSignalSpy told(m_scene.get(), &EditionScene::told);

    QVERIFY(m_history.undo());
    QCOMPARE(m_scene->snapshot(), before);
    QCOMPARE(told.size(), 1);
    QCOMPARE(told.constLast().constFirst().toString(), u"Undone: "_s + what);
    QVERIFY(m_history.redo());
    QCOMPARE(m_scene->snapshot(), after);
    QCOMPARE(told.constLast().constFirst().toString(), u"Redone: "_s + what);
    // Once more, back and forth: the same again.
    QVERIFY(m_history.undo());
    QCOMPARE(m_scene->snapshot(), before);
    QVERIFY(m_history.redo());
    QCOMPARE(m_scene->snapshot(), after);
}

void TestSceneCommand::dragIsOneStep()
{
    const QJsonObject before = m_scene->snapshot();
    const QPointer<Nebula::Component> source = m_application->component(u"Source1"_s);
    drag(*m_view, SourceAt, SourceAt + QPointF(100, 140));
    QCOMPARE(componentItem(*m_scene, u"Source1"_s)->pos(), SourceAt + QPointF(100, 140));
    checkStep(u"Move Source1"_s, before);
    // Only its item moved: the component stays the same.
    QVERIFY(!source.isNull());
    QCOMPARE(m_application->component(u"Source1"_s), source.data());
}

void TestSceneCommand::clickChangesNothing()
{
    clickAt(*m_view, SourceAt);
    clickAt(*m_view, QPointF(700, 500)); // Nothing there
    QVERIFY(m_history.done().isEmpty());
    QVERIFY(!m_history.undo());
}

void TestSceneCommand::removedComponentComesBackWithItsLinks()
{
    const QJsonObject before = m_scene->snapshot();
    clickAt(*m_view, SinkAt);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QVERIFY(m_application->component(u"Sink1"_s) == nullptr);
    QVERIFY(itemsAmong<LinkItem>(m_scene->items()).isEmpty());
    checkStep(u"Remove Sink1"_s, before);

    QVERIFY(m_history.undo());
    // The same name, bound as it was, its link drawn again, where it was.
    const Nebula::Component* sink = m_application->component(u"Sink1"_s);
    QVERIFY(sink != nullptr);
    QCOMPARE(sink->binding(u"object"_s).provider, m_application->component(u"Source1"_s));
    QCOMPARE(itemsAmong<LinkItem>(m_scene->items()).size(), 1);
    QCOMPARE(componentItem(*m_scene, u"Sink1"_s)->pos(), SinkAt);
}

void TestSceneCommand::removedLinkComesBack()
{
    const QJsonObject before = m_scene->snapshot();
    LinkItem* link = itemsAmong<LinkItem>(m_scene->items()).constFirst();
    link->setSelected(true);
    QTest::keyClick(m_view.get(), Qt::Key_Delete);
    QVERIFY(m_application->component(u"Sink1"_s)->binding(u"object"_s).provider == nullptr);
    checkStep(u"Remove link"_s, before);
}

void TestSceneCommand::linkDrawnIsOneStep()
{
    m_application->component(u"Sink1"_s)->unbindInterface(u"object"_s);
    m_scene->load(m_scene->save()); // Shown without the link
    const QJsonObject before = m_scene->snapshot();
    const QPointF from = interfaceItem(*m_scene, u"Source1"_s, u"object"_s)->scenePos();
    const QPointF to = interfaceItem(*m_scene, u"Sink1"_s, u"object"_s)->scenePos();
    QTest::ignoreMessage(QtWarningMsg, "This plugin does not support raise()"); // The tooltip of the dot hovered
    drag(*m_view, from, to);
    QCOMPARE(m_application->component(u"Sink1"_s)->binding(u"object"_s).provider,
             m_application->component(u"Source1"_s));
    checkStep(u"Bind Sink1.object to Source1.object"_s, before);
}

void TestSceneCommand::renamedInPlace()
{
    const QJsonObject before = m_scene->snapshot();
    ComponentItem* item = componentItem(*m_scene, u"Source1"_s);
    item->editName();
    QVERIFY(typeName(*m_scene, *item, u"Sensor"_s));
    QVERIFY(m_application->component(u"Sensor"_s) != nullptr);
    checkStep(u"Rename Source1 to Sensor"_s, before);
    QVERIFY(m_history.undo());
    // Its name again, bound as it was.
    QCOMPARE(m_application->component(u"Sink1"_s)->binding(u"object"_s).provider,
             m_application->component(u"Source1"_s));
}

void TestSceneCommand::droppedComponent()
{
    const QJsonObject before = m_scene->snapshot();
    dropComponent(*m_view, DroppedAt, testComponentId(u"Probe"_s));
    QVERIFY(m_application->component(u"Probe1"_s) != nullptr);
    checkStep(u"Add Probe1"_s, before);
    QVERIFY(m_history.undo());
    QVERIFY(m_application->component(u"Probe1"_s) == nullptr);
    QVERIFY(m_history.redo());
    QCOMPARE(componentItem(*m_scene, u"Probe1"_s)->pos(), DroppedAt);
}

void TestSceneCommand::menuEntryIsOneStep()
{
    const QJsonObject before = m_scene->snapshot();
    chooseFromMenu(*m_view, SourceAt, u"Remove component"_s);
    QVERIFY(m_application->component(u"Source1"_s) == nullptr);
    checkStep(u"Remove component (Source1)"_s, before);
}

void TestSceneCommand::pasteIsOneStep()
{
    componentItem(*m_scene, u"Source1"_s)->setSelected(true);
    componentItem(*m_scene, u"Sink1"_s)->setSelected(true);
    m_scene->copy();
    QVERIFY(m_history.done().isEmpty()); // Copying changes nothing
    const QJsonObject before = m_scene->snapshot();
    m_scene->paste();
    QCOMPARE(m_application->components().size(), std::size_t(4));
    checkStep(u"Paste 2 items"_s, before); // Those pasted, selected alone
}

void TestSceneCommand::cutIsOneStep()
{
    const QJsonObject before = m_scene->snapshot();
    componentItem(*m_scene, u"Sink1"_s)->setSelected(true);
    m_scene->cut();
    QVERIFY(m_application->component(u"Sink1"_s) == nullptr);
    checkStep(u"Cut Sink1"_s, before);
}

void TestSceneCommand::duplicateIsOneStep()
{
    const QJsonObject before = m_scene->snapshot();
    componentItem(*m_scene, u"Source1"_s)->setSelected(true);
    m_scene->duplicate();
    QVERIFY(m_application->component(u"Source2"_s) != nullptr);
    checkStep(u"Duplicate Source1"_s, before);
}

void TestSceneCommand::zoneMovedResizedAndRenamed()
{
    const QJsonObject empty = m_scene->snapshot();
    chooseFromMenu(*m_view, ZoneAt, u"Add zone"_s);
    QTest::keyClick(m_view.get(), Qt::Key_Escape); // Its title, edited at once, is left as it is
    QCOMPARE(itemsAmong<ZoneItem>(m_scene->items()).size(), 1);
    checkStep(u"Add zone"_s, empty);

    m_history.clear();
    const QJsonObject added = m_scene->snapshot();
    drag(*m_view, ZoneAt + QPointF(60, 14), ZoneAt + QPointF(100, 34)); // By its title
    QCOMPARE(itemsAmong<ZoneItem>(m_scene->items()).constFirst()->pos(), ZoneAt + QPointF(40, 20));
    checkStep(u"Move zone Zone"_s, added);

    m_history.clear();
    const QJsonObject moved = m_scene->snapshot();
    const ZoneItem* zone = itemsAmong<ZoneItem>(m_scene->items()).constFirst();
    const QPointF rightEdge = zone->pos() + QPointF(zone->boundingRect().width() - 2, 100);
    drag(*m_view, rightEdge, rightEdge + QPointF(80, 0));
    checkStep(u"Resize zone Zone"_s, moved);

    m_history.clear();
    const QJsonObject resized = m_scene->snapshot();
    ZoneItem* shown = itemsAmong<ZoneItem>(m_scene->items()).constFirst();
    shown->editTitle();
    QVERIFY(typeName(*m_scene, *shown, u"Sensors"_s));
    QCOMPARE(itemsAmong<ZoneItem>(m_scene->items()).constFirst()->title(), u"Sensors"_s);
    checkStep(u"Rename zone Zone to Sensors"_s, resized);
}

void TestSceneCommand::anchorAddedMovedAndRemoved()
{
    const QJsonObject straight = m_scene->snapshot();
    const QPointF at = middle(*itemsAmong<LinkItem>(m_scene->items()).constFirst());
    doubleClick(*m_view, at);
    QCOMPARE(itemsAmong<AnchorItem>(m_scene->items()).size(), 1);
    checkStep(u"Add anchor point"_s, straight);

    m_history.clear();
    const QJsonObject anchored = m_scene->snapshot();
    const QPointF anchor = itemsAmong<AnchorItem>(m_scene->items()).constFirst()->pos();
    drag(*m_view, anchor, anchor + QPointF(0, 80));
    checkStep(u"Move anchor point"_s, anchored);

    m_history.clear();
    const QJsonObject moved = m_scene->snapshot();
    doubleClick(*m_view, itemsAmong<AnchorItem>(m_scene->items()).constFirst()->pos());
    QVERIFY(itemsAmong<AnchorItem>(m_scene->items()).isEmpty());
    checkStep(u"Remove anchor point"_s, moved);
}

void TestSceneCommand::propertyEditedThroughEdit()
{
    const QJsonObject before = m_scene->snapshot();
    const QVariant label = m_application->component(u"Source1"_s)->property("label");
    m_scene->edit(u"Change label of Source1"_s,
                  [this] { m_application->component(u"Source1"_s)->setProperty("label", u"tock"_s); });
    checkStep(u"Change label of Source1"_s, before);
    QVERIFY(m_history.undo());
    QCOMPARE(m_application->component(u"Source1"_s)->property("label"), label);

    // Nothing changed, nothing kept.
    m_scene->edit(u"Nothing"_s, [] {});
    QCOMPARE(m_history.done(), QStringList());
}

void TestSceneCommand::descriptionKeepsTheComponents()
{
    const QJsonObject before = m_scene->snapshot();
    const QPointer<Nebula::Component> source = m_application->component(u"Source1"_s);
    m_scene->edit(u"Edit description"_s, [this] { m_application->setDescription(u"Ticks"_s); });
    checkStep(u"Edit description"_s, before);
    QVERIFY(m_history.undo());
    QCOMPARE(m_application->description(), QString());
    QVERIFY(!source.isNull()); // Not loaded again
}

void TestSceneCommand::nothingKeptWithoutHistory()
{
    m_scene->setHistory(nullptr);
    drag(*m_view, SourceAt, SourceAt + QPointF(100, 140));
    QVERIFY(m_history.done().isEmpty());
}

void TestSceneCommand::notWhileRunning()
{
    const QJsonObject before = m_scene->snapshot();
    drag(*m_view, SourceAt, SourceAt + QPointF(100, 140));
    QVERIFY(m_application->start());
    const QSignalSpy refused(m_scene.get(), &EditionScene::refused);
    QVERIFY(!m_scene->restore(before));
    QCOMPARE(refused.size(), 1);
    QVERIFY(m_scene->snapshot() != before);
    m_application->stop();
    QVERIFY(m_scene->restore(before));
    QCOMPARE(m_scene->snapshot(), before);
}

void TestSceneCommand::stepsWhereTheProjectIsSaved()
{
    // A galaxy placed, whose file the project is then saved away from: the steps hold it whole.
    const QTemporaryDir directory;
    const QString galaxyPath = directory.filePath(u"Pair.nbp"_s);
    QFile galaxyFile(galaxyPath);
    QVERIFY(galaxyFile.open(QIODevice::WriteOnly) && galaxyFile.write(Pair) == Pair.size());
    galaxyFile.close();
    m_application->setFilePath(directory.filePath(u"project/Project.nbp"_s));
    QVERIFY(m_application->instantiateGalaxy(galaxyPath) != nullptr);
    QCOMPARE(m_scene->load(m_scene->snapshot()), QStringList()); // Which shows it
    const QJsonObject before = m_scene->snapshot();
    m_scene->edit(u"Remove Pair1"_s, [this] { m_scene->remove({m_application->component(u"Pair1"_s)}); });

    m_application->setFilePath(directory.filePath(u"elsewhere/deeper/Project.nbp"_s));
    QVERIFY(m_history.undo());
    QCOMPARE(m_scene->snapshot(), before);
    const auto* galaxy = qobject_cast<Nebula::Galaxy*>(m_application->component(u"Pair1"_s));
    QVERIFY(galaxy != nullptr);
    QCOMPARE(galaxy->components().size(), std::size_t(2));
}

void TestSceneCommand::galaxyScene()
{
    const QTemporaryDir directory;
    const QString galaxyPath = directory.filePath(u"Pair.nbp"_s);
    QFile galaxyFile(galaxyPath);
    QVERIFY(galaxyFile.open(QIODevice::WriteOnly) && galaxyFile.write(Pair) == Pair.size());
    galaxyFile.close();
    Nebula::Galaxy* galaxy = m_application->instantiateGalaxy(galaxyPath);
    QVERIFY(galaxy != nullptr);
    auto scene = std::make_unique<EditionScene>(*m_application, Nebula::ComponentFactory::instance(), *galaxy);
    scene->setHistory(&m_history);
    QVERIFY(!scene->isModified());

    // Its components replaced: the galaxy is given what it held, not read from its file.
    const QJsonObject before = scene->snapshot();
    scene->edit(u"Remove Sink"_s, [&scene, galaxy] { scene->remove({galaxy->component(u"Sink"_s)}); });
    QVERIFY(scene->isModified());
    QVERIFY(m_history.undo());
    QCOMPARE(scene->snapshot(), before);
    QVERIFY(!scene->isModified()); // As its file holds it
    QVERIFY(galaxy->component(u"Sink"_s) != nullptr);
    QCOMPARE(galaxy->component(u"Sink"_s)->binding(u"object"_s).provider, galaxy->component(u"Source"_s));
    QVERIFY(m_history.redo());
    QVERIFY(galaxy->component(u"Sink"_s) == nullptr);
    QVERIFY(scene->isModified());

    // Once its scene is closed, its steps are obsolete: they act on nothing.
    QVERIFY(m_history.last() != nullptr && !m_history.last()->isObsolete());
    scene.reset();
    QVERIFY(m_history.last()->isObsolete());
    QVERIFY(m_history.undo()); // Which does nothing
    QVERIFY(galaxy->component(u"Sink"_s) == nullptr);
}

QTEST_MAIN(TestSceneCommand)
#include "TestSceneCommand.moc"
