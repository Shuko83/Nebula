// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QHash>
#include <QList>
#include <QMetaProperty>
#include <QMetaType>
#include <QPointer>
#include <QString>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "EditDialog.h"
#include "Nebula/Galaxy/Stargate.h"

class QTreeWidget;
class QTreeWidgetItem;
class WarningLabel;

namespace Nebula
{
class ComponentFactory;
class Galaxy;
class Universe;
} // namespace Nebula

// Chooses what a stargate of the application, or of one of its galaxies, exposes, as the dialog of a settings file
// chooses what it holds: under each component where the stargate is, its interfaces, then its properties, those a
// project holds, or those a galaxy among them exposes, which it exposes again. Next to what is checked, the name it is
// exposed as: the one of the interface or of the property until another one is typed. Double-clicking a row checks it
// and edits that name. Two interfaces consumed, of the same type, checked under the same name, are given what the
// galaxy consumes through the same interface of the stargate. What another stargate exposes is listed with its name,
// and cannot be checked; what the stargate exposes that is bound to none of the components listed is listed last, to
// keep or not. What was chosen is given to the stargate, which is bound accordingly, once the dialog is accepted.
class StargateDialog : public EditDialog
{
    Q_OBJECT

  public:
    // stargate, one of those of application or of its galaxies, outlives the dialog, but for the components where it
    // is, which can go meanwhile.
    StargateDialog(Nebula::Stargate& stargate, const Nebula::Universe& application,
                   const Nebula::ComponentFactory& factory, QWidget* parent = nullptr);

  private:
    enum class Kind : std::uint8_t
    {
        Provided, // An interface a component provides, which the galaxy provides through the stargate
        Consumed, // An interface a component consumes, which the galaxy consumes through the stargate
        Property,
        Kept, // An interface the stargate exposes, bound to none of the components listed
    };

    // What a row of the tree exposes.
    struct Choice
    {
        Kind kind = Kind::Property;
        QPointer<Nebula::Component> component;                  // nullptr for one kept
        QPointer<QObject> object;                               // Of a property: the component, or one of its objects
        QString name;                                           // Of the interface, or of the property
        QMetaType type;                                         // Of an interface
        Nebula::Exposure exposure = Nebula::Exposure::Provided; // Of one kept
    };

    // The rows checked, with what they expose, in the order of the tree.
    using Rows = QList<std::pair<const QTreeWidgetItem*, Choice>>;

    // Who exposes a choice: the stargate, under a name, or another stargate.
    struct Exposed
    {
        QString as; // The name the stargate exposes it as
        QString by; // The other stargate that exposes it

        // stargate, among those where self is, exposes it as name.
        void record(const Nebula::Stargate& stargate, const Nebula::Stargate& self, const QString& name);
    };

    // The header tells its name, and what it is for.
    void tellAbout();
    // Under the header: the tree, why it cannot be accepted, and the buttons.
    [[nodiscard]] QWidget* newContent();
    // Checks row, when it is a choice, then edits the name it is exposed as.
    void editName(QTreeWidgetItem* row);

    // Lists what the components of the application have to expose.
    void fill(const Nebula::ComponentFactory& factory);
    // The row of component, with what it has to expose under it.
    [[nodiscard]] QTreeWidgetItem* componentRow(Nebula::Component& component, const Nebula::ComponentFactory& factory);
    // The interfaces component consumes, under parent, as its item shows them.
    void addConsumedChoices(QTreeWidgetItem* parent, Nebula::Component& component);
    void addConsumedChoice(QTreeWidgetItem* parent, Nebula::Component& component, const Nebula::InterfaceInfo& info);
    // The interfaces component provides, under parent.
    void addProvidedChoices(QTreeWidgetItem* parent, Nebula::Component& component);
    // The properties of component under parent, then those of its objects.
    void addPropertyChoices(QTreeWidgetItem* parent, Nebula::Component& component,
                            const Nebula::ComponentFactory& factory);
    // Those galaxy exposes, which a stargate of its own exposes again.
    void addGalaxyProperties(QTreeWidgetItem* parent, Nebula::Galaxy& galaxy);
    // Those of properties of object, component or one of its objects, that a project holds.
    void addHeldProperties(Nebula::Component& component, QTreeWidgetItem* parent, QObject* object,
                           const QList<QMetaProperty>& properties);
    // The property name, of type, of object, component or one of its objects, under parent.
    void addPropertyChoice(QTreeWidgetItem* parent, Nebula::Component& component, QObject* object, const QString& name,
                           const QString& type);
    // What the stargate exposes that none of the rows of components tells: bound to nothing, as the component it was
    // bound to went.
    [[nodiscard]] QTreeWidgetItem* keptRow();
    void addKeptChoice(QTreeWidgetItem* parent, const Nebula::ExposedInterface& exposed);
    // Adds the row of choice under parent, checked as exposed.as tells, disabled when exposed.by, another stargate,
    // exposes it; returns it.
    QTreeWidgetItem* addChoice(QTreeWidgetItem* parent, const QString& text, const QString& kind, const Choice& choice,
                               const Exposed& exposed);

    // Who exposes name, which component provides.
    [[nodiscard]] Exposed providedExposure(const Nebula::Component& component, const QString& name) const;
    // Who exposes target, a property, whatever its name.
    [[nodiscard]] Exposed propertyExposure(const Nebula::ExposedProperty& target) const;
    // What the rows of components tell the stargate exposes: the way, and the name.
    [[nodiscard]] QList<std::pair<Nebula::Exposure, QString>> shownInterfaces() const;

    // The name a row checked is given: the one of what it exposes, unless another one has it, the same way.
    [[nodiscard]] QString defaultName(const QTreeWidgetItem* row) const;
    // Whether name is taken for row, which exposes choice: by another stargate, or by another row of rows exposed the
    // same way, unless they share it.
    [[nodiscard]] bool isNameTaken(const Rows& rows, const QTreeWidgetItem* row, const Choice& choice,
                                   const QString& name) const;
    // A checked row is given its name when it has none; an unchecked one loses it. Then what can be accepted is told.
    void edited(QTreeWidgetItem* row, int column);
    // Gives row the name it has once column is edited, see nameAfterEdit().
    void tidyName(QTreeWidgetItem& row, int column);
    // The name row is given once column is edited: its default one once checked without one, none once unchecked,
    // on one line once typed; nullopt when it keeps its own.
    [[nodiscard]] std::optional<QString> nameAfterEdit(const QTreeWidgetItem& row, int column) const;
    // Tells why it cannot be accepted, when it cannot: a name is empty, or the name of another one exposed the same
    // way, or of one another stargate exposes the same way.
    void check();
    // What keeps the first row of rows that cannot be exposed from being so; empty when they all can.
    [[nodiscard]] QString firstProblem() const;
    // What keeps row, which exposes choice, from being exposed: no name, or the name of what another stargate
    // exposes the same way; empty when nothing does.
    [[nodiscard]] QString ownProblem(const QTreeWidgetItem& row, const Choice& choice) const;
    // What keeps rows.at(index) from being exposed under its name: a row after it exposed the same way under the same
    // name, which it cannot share; empty when none is.
    [[nodiscard]] static QString clashProblem(const Rows& rows, qsizetype index);

    // Gives the stargate what was chosen.
    void apply() override;
    // The properties no longer exposed, or under another name, first: a name can pass from one to another.
    void applyProperties(const Rows& rows);
    // Withdraws the properties the stargate exposes, but kept.
    void removePropertiesBut(const QList<Nebula::ExposedProperty>& kept);
    // Has the stargate expose properties, those it does not yet.
    void exposeProperties(const QList<Nebula::ExposedProperty>& properties);
    // What consumes the stargate, and no longer should under that name, is unbound from it.
    void unbindDropped();
    // Unbinds what row, of choice, an interface consumed, no longer wants of the stargate.
    void unbindIfDropped(const QTreeWidgetItem& row, const Choice& choice);
    // Its interfaces no longer wanted, or wanted of another type, go.
    void removeUnwanted(const Rows& rows);
    // Those wanted are exposed, in the order of the tree, and bound.
    void exposeWanted(const Rows& rows);
    // Exposes name, of choice, the way exposure tells, and binds it to the interface of choice.
    void exposeAndBind(Nebula::Exposure exposure, const QString& name, const Choice& choice);
    // Binds name, which the stargate exposes, to the interface of choice; nothing for one kept, bound to no component.
    void bind(const QString& name, const Choice& choice);
    void bindProvided(const QString& name, const Choice& choice);
    void bindConsumed(const QString& name, const Choice& choice);

    // The rows checked, with what they expose, in the order of the tree.
    [[nodiscard]] Rows checked() const;
    // How a choice is exposed, seen from the galaxy: the way of its interface; nullopt for a property.
    [[nodiscard]] static std::optional<Nebula::Exposure> exposureOf(const Choice& choice);
    // Whether two choices checked can be exposed under the same name: interfaces the galaxy consumes, of the same
    // type, given what it consumes through the same interface of the stargate.
    [[nodiscard]] static bool canShare(const Choice& first, const Choice& second);
    // Whether other, a row checked, keeps choice from being exposed as name: exposed the same way under that name,
    // without sharing it.
    [[nodiscard]] static bool clashes(const Choice& choice, const QString& name,
                                      const std::pair<const QTreeWidgetItem*, Choice>& other);
    // Whether rows want exposed, of the same type.
    [[nodiscard]] static bool isWanted(const Rows& rows, const Nebula::ExposedInterface& exposed);
    // The property a row of a property, choice, chooses, exposed under the name row tells.
    [[nodiscard]] static Nebula::ExposedProperty chosenProperty(const QTreeWidgetItem& row, const Choice& choice);
    // The properties rows choose.
    [[nodiscard]] static QList<Nebula::ExposedProperty> chosenProperties(const Rows& rows);
    // Whether another stargate where it is exposes name, the way exposure tells; a property when it is nullopt.
    [[nodiscard]] bool isTaken(std::optional<Nebula::Exposure> exposure, const QString& name) const;
    // The components where the stargate is, the application or one of its galaxies: what it exposes is theirs.
    [[nodiscard]] const std::vector<std::unique_ptr<Nebula::Component>>& components() const;
    // Those of components() that can expose something: all but the stargates, bound to one another.
    [[nodiscard]] QList<Nebula::Component*> exposable() const;
    // The stargates where it is, itself among them.
    [[nodiscard]] QList<const Nebula::Stargate*> stargates() const;

    Nebula::Stargate& m_stargate;
    const Nebula::Universe& m_application;
    QHash<const QTreeWidgetItem*, Choice> m_choices; // What each row of the tree exposes
    QTreeWidget* m_tree = nullptr;
    WarningLabel* m_problem = nullptr; // Why it cannot be accepted, when it cannot
};
