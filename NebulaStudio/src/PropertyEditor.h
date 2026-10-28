// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <utility>

class PlaceholderStack;
class QTableWidget;
class QTableWidgetItem;

namespace Nebula
{
class Component;
class ComponentFactory;
class Galaxy;
struct PropertyObject;
class Stargate;
class Universe;
} // namespace Nebula

// Edits the properties of a component instantiated in the application, then those of the objects it added with
// Component::addPropertiesForQObject(): with its own when they were added without a name, under the name they were
// added with otherwise, one group per name. Those of a galaxy are the ones its stargates expose, under the names they
// expose them as: the same properties, on the same objects, as those of its components. It only shows the properties
// it can edit: they can be written and their type has an editor. It follows the changes the component and its objects
// notify, and the objects it adds or loses.
// The name of a property a settings file of the application holds is in italics: its value is the default one. The
// one of a property a stargate where the component is exposes has the icon of the Stargate: the galaxy of the project,
// or the galaxy the component is in, exposes it.
// While the application runs, it only shows the values: they are edited once it is stopped, which gives them back the
// values of the edition.
class PropertyEditor : public QWidget
{
    Q_OBJECT

  public:
    // The components are those of application, or of its galaxies.
    PropertyEditor(const Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                   QWidget* parent = nullptr);

    // Shows the editable properties of component; none when it is nullptr.
    void setComponent(Nebula::Component* component);

  protected:
    // Gives the titles of the groups the background of the palette when it changes, as the color scheme does.
    void changeEvent(QEvent* event) override;

  private slots:
    // Shows the values the properties have now. A slot: their notify signals are only known as QMetaMethod.
    void refreshValues();

  private:
    // A row of m_table: a property of object, or the title of a group, which has no property.
    struct Row
    {
        QPointer<QObject> object;
        QMetaProperty property;
        QString title;
        QString name;                          // What the property is shown as: its own name when empty
        QPointer<Nebula::Component> component; // Of the property: m_component, or one of a galaxy
    };

    // Rebuilds it as component changes its objects, or, a galaxy, is read again; tells it as component is renamed.
    void follow(Nebula::Component* component);
    // Gives the titles of the groups the background of the palette again: it has no role for them.
    void colorTitles();
    // Shows the properties m_component and its objects have now.
    void rebuild();
    // No longer listens to the objects shown until now, but the component, should it still be the one shown: more
    // than its properties is followed. Then forgets them.
    void forgetRows();
    // Rebuilds it when what the stargates where it is expose changes, which the names tell.
    void followStargates();
    // The rows of m_component: those of what a galaxy exposes, or those of the properties of a component and of its
    // objects.
    [[nodiscard]] QList<Row> componentRows() const;
    // What the stargates of galaxy expose, under the names they expose them as.
    [[nodiscard]] static QList<Row> exposedRows(const Nebula::Galaxy& galaxy);
    // The properties of component, then those of its objects: those without a name with its own, those of each name,
    // the first time it came, under a title.
    [[nodiscard]] QList<Row> propertyRows(Nebula::Component& component) const;
    // The rows of the objects among objects named name, under a title when they have a name and some rows.
    [[nodiscard]] static QList<Row> groupRows(const QList<Nebula::PropertyObject>& objects, const QString& name,
                                              Nebula::Component& component);
    // The rows of the objects among objects named name, of component.
    [[nodiscard]] static QList<Row> objectRows(const QList<Nebula::PropertyObject>& objects, const QString& name,
                                               Nebula::Component& component);
    // The rows of those of properties of object, of component, that can be edited here: the other properties of a
    // component are in the viewer of its component.
    [[nodiscard]] static QList<Row> editableRows(QObject* object, const QList<QMetaProperty>& properties,
                                                 Nebula::Component& component);
    // Shows m_rows in the table.
    void showRows();
    // Empties the table, closing the editor of a value shown until now; its values are edited while the application
    // is.
    void clearTable();
    // Shows the row row of m_rows.
    void showRow(int row);
    // Shows title in row, over the whole of it.
    void showTitle(int row, const QString& title);
    // The name of the property of shown, telling where it is, which file holds it and which stargate exposes it.
    [[nodiscard]] QTableWidgetItem* nameItem(const Row& shown) const;
    // Tells in the tooltip of item where the property of shown is, when it is one a galaxy exposes.
    static void tellPlace(QTableWidgetItem& item, const Row& shown);
    // Writes the name of item in italics, and tells in its tooltip, when a settings file holds the property of shown.
    void tellHolder(QTableWidgetItem& item, const Row& shown) const;
    // Gives item the icon of the Stargate, and tells in its tooltip, when a stargate exposes the property of shown.
    void tellExposure(QTableWidgetItem& item, const Row& shown) const;
    // Shows the values again when the property of shown changes.
    void followValue(const Row& shown);
    // Tells why no property is shown, when none is. It names the component, which can be renamed meanwhile.
    void refreshPlaceholder();
    // Writes the value edited in item to its property.
    void write(const QTableWidgetItem* item);
    // The stargates where m_component is, the application or the galaxy it is in; none without it.
    [[nodiscard]] QList<const Nebula::Stargate*> stargates() const;
    // The stargate where m_component is that exposes the property of row, and what it exposes it as; nullptr when none
    // does.
    [[nodiscard]] std::pair<const Nebula::Stargate*, QString> exposedBy(const Row& row) const;

    const Nebula::Universe& m_application;
    const Nebula::ComponentFactory& m_factory;
    QPointer<Nebula::Component> m_component;
    QList<Row> m_rows; // One per row of m_table

    QTableWidget* m_table = nullptr;     // Shown when there is a property to show
    PlaceholderStack* m_stack = nullptr; // Of m_table and of what tells why there is none
};
