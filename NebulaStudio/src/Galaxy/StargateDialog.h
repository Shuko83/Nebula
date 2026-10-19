// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QDialog>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QPointer>
#include <QString>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Nebula/Galaxy/Stargate.h"

class QDialogButtonBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace Nebula
{
class ComponentFactory;
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
class StargateDialog : public QDialog
{
    Q_OBJECT

  public:
    // stargate, one of those of application or of its galaxies, outlives the dialog, but for the components where it
    // is, which can go meanwhile.
    StargateDialog(Nebula::Stargate& stargate, const Nebula::Universe& application,
                   const Nebula::ComponentFactory& factory, QWidget* parent = nullptr);

  protected:
    // Gives why it cannot be accepted the warning color again when the palette changes, as the color scheme does.
    void changeEvent(QEvent* event) override;

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

    // Lists what the components of the application have to expose.
    void fill(const Nebula::ComponentFactory& factory);
    // Adds the row of choice under parent, checked as exposedAs tells, disabled when exposedBy, another stargate,
    // exposes it.
    void addChoice(QTreeWidgetItem* parent, const QString& text, const QString& kind, const Choice& choice,
                   const QString& exposedAs, const QString& exposedBy = {});
    // The name a row checked is given: the one of what it exposes, unless another one has it, the same way.
    [[nodiscard]] QString defaultName(const QTreeWidgetItem* row) const;
    // A checked row is given its name when it has none; an unchecked one loses it. Then what can be accepted is told.
    void edited(QTreeWidgetItem* row, int column);
    // Tells why it cannot be accepted, when it cannot: a name is empty, or the name of another one exposed the same
    // way, or of one another stargate exposes the same way.
    void check();
    // Gives what tells why it cannot be accepted the warning color of the palette.
    void colorProblem();
    // Gives the stargate what was chosen.
    void apply();

    // The rows checked, with what they expose, in the order of the tree.
    [[nodiscard]] QList<std::pair<const QTreeWidgetItem*, Choice>> checked() const;
    // How a choice is exposed, seen from the galaxy: the way of its interface; nullopt for a property.
    [[nodiscard]] static std::optional<Nebula::Exposure> exposureOf(const Choice& choice);
    // Whether two choices checked can be exposed under the same name: interfaces the galaxy consumes, of the same
    // type, given what it consumes through the same interface of the stargate.
    [[nodiscard]] static bool canShare(const Choice& first, const Choice& second);
    // Whether another stargate where it is exposes name, the way exposure tells; a property when it is nullopt.
    [[nodiscard]] bool isTaken(std::optional<Nebula::Exposure> exposure, const QString& name) const;
    // The components where the stargate is, the application or one of its galaxies: what it exposes is theirs.
    [[nodiscard]] const std::vector<std::unique_ptr<Nebula::Component>>& components() const;

    Nebula::Stargate& m_stargate;
    const Nebula::Universe& m_application;
    QHash<const QTreeWidgetItem*, Choice> m_choices; // What each row of the tree exposes
    QTreeWidget* m_tree = nullptr;
    QWidget* m_problem = nullptr;    // Why it cannot be accepted, when it cannot
    QLabel* m_problemText = nullptr; // In the warning color
    QDialogButtonBox* m_buttons = nullptr;
};
