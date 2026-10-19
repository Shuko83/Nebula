// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QList>
#include <QMetaProperty>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <utility>

class QLabel;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;

namespace Nebula
{
class Component;
class ComponentFactory;
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
// one of a property a stargate of the application exposes has the icon of the Stargate: the galaxy of the project
// exposes it.
// While the application runs, it only shows the values: they are edited once it is stopped, which gives them back the
// values of the edition.
class PropertyEditor : public QWidget
{
    Q_OBJECT

  public:
    // The components are those of application.
    PropertyEditor(const Nebula::Universe& application, const Nebula::ComponentFactory& factory,
                   QWidget* parent = nullptr);

    // Shows the editable properties of component; none when it is nullptr.
    void setComponent(Nebula::Component* component);
    // Whether the values are only shown, as they are while the application runs: those of the components of a
    // galaxy, the ones of its file, are. They are edited until told otherwise.
    void setReadOnly(bool readOnly);

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

    // Shows the properties m_component and its objects have now.
    void rebuild();
    // Tells why no property is shown, when none is. It names the component, which can be renamed meanwhile.
    void refreshPlaceholder();
    // Writes the value edited in item to its property.
    void write(const QTableWidgetItem* item);
    // The stargate of the application that exposes the property of row, and what it exposes it as; nullptr when none
    // does.
    [[nodiscard]] std::pair<const Nebula::Stargate*, QString> exposedBy(const Row& row) const;

    const Nebula::Universe& m_application;
    const Nebula::ComponentFactory& m_factory;
    QPointer<Nebula::Component> m_component;
    bool m_readOnly = false;
    QList<Row> m_rows; // One per row of m_table

    QStackedWidget* m_stack = nullptr;
    QLabel* m_placeholder = nullptr; // Shown when there is no property to show, and tells why
    QTableWidget* m_table = nullptr; // Shown otherwise
};
