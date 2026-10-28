// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QJsonObject>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariant>

#include "Core_export.h"

class QMetaMethod;

namespace Nebula
{

class Component;
class Galaxy;
class Universe;

// A property a settings file holds: one of a component of the application, or of one of the objects the component
// added with Component::addPropertiesForQObject().
struct SettingsEntry
{
    Component* component = nullptr;
    QObject* object = nullptr; // The component itself, or one of its propertyObjects()
    QString property;

    [[nodiscard]] bool operator==(const SettingsEntry& other) const = default;
};

// A file of settings of an application, see Universe::addSettingsFile(): the values of properties of its
// components, its entries, which the user of the application changes while it runs and finds again the next time it
// starts. The value a project gives such a property is its default value.
// When the application starts, the file gives its values to the properties it holds, once their default values are
// written to it when it does not exist or misses some; while the application runs, the file follows the changes they
// notify; when it stops, the file is written a last time, and the properties have their default values again.
// It holds the properties of any number of components, under their names, as a project holds them; a property is held
// by a single file of the application. What else is in the file is kept: another application can share it.
// Its name is its own among the files of the application, whatever their case and their directories, as two files of
// the same name would be written over each other.
class CORE_EXPORT SettingsFile : public QObject
{
    Q_OBJECT

  public:
    ~SettingsFile() override;

    // As it was added, settings.json, or settings2.json, settings3.json... when another file has that name, until it
    // is given another one. The one of a file of a galaxy is its name() after the galaxies it is in.
    [[nodiscard]] QString fileName() const;
    // Returns false, changing nothing, when fileName is empty or the name of another file of the application, whatever
    // its case, and for a file of a galaxy, named by setName(). Taken into account the next time the application
    // starts.
    bool setFileName(const QString& fileName);
    // The name its project gives it: its fileName() in the application; in a galaxy, the one its file gives it, after
    // which each instance of the galaxy names its own: settings.json for Clock1.settings.json, see
    // Universe::instantiateGalaxy().
    [[nodiscard]] QString name() const;
    // Names it name in its project: its fileName() is name in the application, name after the galaxies it is in in a
    // galaxy. Returns false, changing nothing, when name is empty or its fileName() would be the one of another file of
    // the application, whatever its case. Taken into account the next time the application starts.
    bool setName(const QString& name);
    // The galaxy whose components it holds the properties of; nullptr for a file of the application.
    [[nodiscard]] Galaxy* galaxy() const;
    // Where the file is, see filePath(); empty until it is set.
    [[nodiscard]] QString directory() const;
    // Taken into account the next time the application starts.
    void setDirectory(const QString& directory);
    // filePath() of its file name and its directory.
    [[nodiscard]] QString filePath() const;
    // fileName in directory: in the configuration directory of the user for the application that runs,
    // QStandardPaths::AppConfigLocation, when directory is empty; from the directory of that application when it is
    // relative.
    [[nodiscard]] static QString filePath(const QString& fileName, const QString& directory);

    // In the order they were added.
    [[nodiscard]] const QList<SettingsEntry>& entries() const;
    [[nodiscard]] bool contains(const SettingsEntry& entry) const;
    // Holds entry from the next time the application starts. Returns false, changing nothing, when entry is not a
    // property it can hold, see canHold(), of a component of the application, or of the galaxy the file is of, see
    // Universe::instantiateGalaxy(), or when a file of the application holds it already. An entry leaves once its
    // object, or its component, is destroyed.
    bool add(const SettingsEntry& entry);
    // No longer holds entry from the next time the application starts.
    void remove(const SettingsEntry& entry);

    // Writes to its file the values its entries have, those of the project as the application does not run, in place
    // of those it holds for them. Returns false, with errorString() set, while the application runs, nothing being
    // written then, and when the file cannot be written.
    bool writeDefaults();
    // Why writeDefaults() returned false, or failed() was last emitted.
    [[nodiscard]] QString errorString() const;

    // Whether a settings file can hold property of object, component itself or one of its objects: as a project, see
    // Universe::save(), the property can be written and JSON holds its value.
    [[nodiscard]] static bool canHold(const Component& component, const QObject* object, const QString& property);

  signals:
    // Its file name, its directory or its entries changed.
    void changed();
    // While the application starts or runs, its file could not be read or written; why is a sentence to show. The
    // application runs all the same: without the values it holds, or without saving those the user changes.
    void failed(const QString& why);

  private:
    friend class Universe; // Creates it, starts and stops it

    // An entry, as it was when the application started.
    struct StartedEntry
    {
        SettingsEntry entry;
        QVariant defaultValue; // The value of the project
    };

    // fileName is one no other file of application has. It holds the properties of the components of galaxy, one of
    // those of application, or of the application itself when galaxy is nullptr. name is its name(): fileName ends
    // with it, and is it when it is empty.
    SettingsFile(Universe& application, QString fileName, Galaxy* galaxy = nullptr, QString name = {});

    // Names it fileName, and name in its project. Returns false, changing nothing, as setName() does.
    bool rename(const QString& fileName, const QString& name);
    // Whether fileName is free for it: no other file of the application has it, whatever its case.
    [[nodiscard]] bool isFreeFileName(const QString& fileName) const;
    // Whether entry can be added: of a component it holds the properties of, held by a project, and by no file yet.
    [[nodiscard]] bool canAdd(const SettingsEntry& entry) const;
    // Gives the properties it holds the values of its file, writes their default values to it first when it does not
    // exist or misses some, and follows their changes.
    void start();
    // Reads what its file holds as it starts; it is not written when it cannot be set aside.
    void readContent();
    // Gives each entry the value its file holds for it, keeping the value of the project to give back; returns
    // whether the file held one for each.
    bool giveValues();
    // Gives entry the value its file holds for it. When it holds none, or one the property cannot have, which is
    // told, the file is given the current value instead and false is returned.
    bool giveValue(const SettingsEntry& entry);
    // Tells, unless value is undefined, that its file holds value for entry, which its property cannot have.
    void tellWrongValue(const SettingsEntry& entry, const QJsonValue& value);
    // Follows the notify signals of the properties of its entries: what they change is written.
    void followChanges();
    // Has take called when the property of entry changes, unless it has no notify signal or one already followed.
    void follow(const SettingsEntry& entry, const QMetaMethod& take);
    // Writes the values of the properties it holds to its file, and gives them their default values again.
    void stop();
    // Has its content hold the current value of started.
    void takeValue(const StartedEntry& started);
    // takeValue() of each entry: those no notify signal tells the changes of as well.
    void takeAllValues();
    // Gives back to the properties of its entries the values of the project, kept when it started.
    void giveBackDefaults();
    // Writes its content to its file once for the values taken meanwhile: a property can change at every event.
    void scheduleWrite();
    // Writes its content to its file, unless writing it failed before; tells it once when it fails.
    void writeContent();
    // Tells why through the log and failed().
    void fail(const QString& why);
    // Sets errorString() to why and returns false.
    bool refuse(const QString& why);

  private slots:
    // Takes the values the sender, an object of its entries, has now: one of its properties changed. A slot: their
    // notify signals are only known as QMetaMethod.
    void takeValues();
    // Writes the values taken since it last wrote them.
    void writeTaken();
    // Forgets the entries of object, or of its component, being destroyed.
    void forget(QObject* object);

  private:
    Universe& m_application;
    Galaxy* m_galaxy = nullptr; // Whose components it holds the properties of; nullptr for those of the application
    QString m_fileName;
    QString m_name; // As its project names it: m_fileName ends with it
    QString m_directory;
    QList<SettingsEntry> m_entries;
    QString m_errorString;

    // While the application runs.
    bool m_running = false;
    QString m_startedPath;                     // Where it is written
    bool m_writable = false;                   // Whether it is written: false once that failed
    QJsonObject m_content;                     // What it writes there
    QList<StartedEntry> m_started;             // Its entries when it started: those it follows
    QList<QMetaObject::Connection> m_followed; // To their notify signals
    bool m_writePending = false;               // Whether values were taken since it last wrote them
};

} // namespace Nebula
