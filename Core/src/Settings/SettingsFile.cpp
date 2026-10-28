// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#include "Nebula/Settings/SettingsFile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <optional>
#include <utility>

#include "Application/JsonFile.h"
#include "Application/ProjectJson.h"
#include "Component/Components.h"
#include "Component/Connections.h"
#include "Nebula/Application/Universe.h"
#include "Nebula/Component/Component.h"
#include "Nebula/Galaxy/Galaxy.h"

namespace Nebula
{

namespace
{

Q_LOGGING_CATEGORY(lcSettings, "nebula.settings")

// The property of entry; not valid when its object has none of that name.
QMetaProperty propertyOf(const SettingsEntry& entry)
{
    const QMetaObject* metaObject = entry.object->metaObject();
    return metaObject->property(metaObject->indexOfProperty(entry.property.toLatin1().constData()));
}

// The value of the property of entry, as a file holds it; null when JSON cannot hold it.
QJsonValue currentValue(const SettingsEntry& entry)
{
    return ProjectJson::toJson(propertyOf(entry).read(entry.object)).value_or(QJsonValue());
}

// Whether an object of the "objects" of a component, as a project holds them, is named name.
auto isNamed(const QString& name)
{
    return [&name](const QJsonValue& object) { return object.toObject().value(u"name").toString() == name; };
}

// The position, among objects, of the one key finds: the key->index-th of those named key->name; -1 when there is
// none, or no key.
qsizetype objectPosition(const QJsonArray& objects, const std::optional<ProjectJson::ObjectKey>& key)
{
    qsizetype index = 0; // Of the objects of its name met so far
    for(qsizetype position = 0; key && position < objects.size(); ++position)
    {
        if(isNamed(key->name)(objects.at(position)) && index++ == key->index)
        {
            return position;
        }
    }
    return -1;
}

// objectPosition(), the object added when missing, after as many objects of its name as were added before it.
qsizetype addedObjectPosition(QJsonArray& objects, const ProjectJson::ObjectKey& key)
{
    for(qsizetype count = std::count_if(objects.begin(), objects.end(), isNamed(key.name)); count <= key.index; ++count)
    {
        objects.append(QJsonObject{{QStringLiteral("name"), key.name}, {QStringLiteral("properties"), QJsonObject()}});
    }
    return objectPosition(objects, key);
}

// The object of content that holds the properties of the object of entry, as a project holds those of a component:
// the component itself, or one of its "objects", found by its name, then in the order they were added. Empty when
// there is none.
QJsonObject holderOf(const QJsonObject& content, const SettingsEntry& entry)
{
    const QJsonObject component = content.value(entry.component->objectName()).toObject();
    if(entry.object == entry.component)
    {
        return component;
    }
    const QJsonArray objects = component.value(u"objects").toArray();
    const qsizetype position = objectPosition(objects, ProjectJson::objectKey(*entry.component, entry.object));
    return position < 0 ? QJsonObject() : objects.at(position).toObject();
}

// The value content holds for entry, in the "properties" of holderOf(). Undefined when it holds none.
QJsonValue valueOf(const QJsonObject& content, const SettingsEntry& entry)
{
    return holderOf(content, entry).value(u"properties").toObject().value(entry.property);
}

// holder, a component or an object, with value for property in its "properties".
QJsonObject withProperty(QJsonObject holder, const QString& property, const QJsonValue& value)
{
    QJsonObject properties = holder.value(u"properties").toObject();
    properties.insert(property, value);
    holder.insert(u"properties", properties);
    return holder;
}

// component with value for property of its object key finds, the object added when missing.
QJsonObject withObjectProperty(QJsonObject component, const ProjectJson::ObjectKey& key, const QString& property,
                               const QJsonValue& value)
{
    QJsonArray objects = component.value(u"objects").toArray();
    const qsizetype position = addedObjectPosition(objects, key);
    objects.replace(position, withProperty(objects.at(position).toObject(), property, value));
    component.insert(u"objects", objects);
    return component;
}

// component, as content holds the one of entry, with value for entry; nullopt when the object of entry is not one of
// the component.
std::optional<QJsonObject> withValue(const QJsonObject& component, const SettingsEntry& entry, const QJsonValue& value)
{
    if(entry.object == entry.component)
    {
        return withProperty(component, entry.property, value);
    }
    const std::optional<ProjectJson::ObjectKey> key = ProjectJson::objectKey(*entry.component, entry.object);
    return key ? std::optional(withObjectProperty(component, *key, entry.property, value)) : std::nullopt;
}

// Sets the value content holds for entry, where valueOf() finds it, to value, adding what is missing for it.
void setValue(QJsonObject& content, const SettingsEntry& entry, const QJsonValue& value)
{
    const QString name = entry.component->objectName();
    if(const std::optional<QJsonObject> component = withValue(content.value(name).toObject(), entry, value))
    {
        content.insert(name, *component);
    }
}

// content with the current values of entries.
QJsonObject withCurrentValues(QJsonObject content, const QList<SettingsEntry>& entries)
{
    for(const SettingsEntry& entry : entries)
    {
        setValue(content, entry, currentValue(entry));
    }
    return content;
}

// The JSON object the file filePath holds; nullopt, with why set, when it cannot be read or holds none.
std::optional<QJsonObject> readObject(const QString& filePath, QString& why)
{
    const std::optional<QJsonDocument> document = JsonFile::read(filePath, why);
    if(document && !document->isObject())
    {
        why = QStringLiteral("it does not hold a JSON object");
    }
    return document && document->isObject() ? std::optional(document->object()) : std::nullopt;
}

// Sets aside the settings file filePath, which cannot be read for why, a ".bak" added to its name, for writing it again
// not to lose what is in it, and problem tells it. An empty object, what to write instead; nullopt when it cannot be
// set aside: it must not be written then.
std::optional<QJsonObject> setAside(const QString& filePath, const QString& why, QString& problem)
{
    const QString backup = filePath + QStringLiteral(".bak");
    QFile::remove(backup); // The one of a former time
    const bool setAside = QFile::rename(filePath, backup);
    problem = setAside ? QStringLiteral("%1 cannot be read (%2): it is set aside as %3 and written again")
                             .arg(QDir::toNativeSeparators(filePath), why, QDir::toNativeSeparators(backup))
                       : QStringLiteral("%1 cannot be read (%2), nor set aside: it is not written")
                             .arg(QDir::toNativeSeparators(filePath), why);
    return setAside ? std::optional(QJsonObject()) : std::nullopt;
}

// What the settings file filePath holds: nothing when it does not exist. A file that cannot be read is set aside, see
// setAside().
std::optional<QJsonObject> readSettings(const QString& filePath, QString& problem)
{
    if(!QFile::exists(filePath))
    {
        return QJsonObject();
    }
    QString why;
    const std::optional<QJsonObject> content = readObject(filePath, why);
    return content ? content : setAside(filePath, why, problem);
}

// Writes content to the settings file filePath, creating its directory when needed; false, with error set, when it
// cannot.
bool writeSettings(const QString& filePath, const QJsonObject& content, QString& error)
{
    if(JsonFile::write(filePath, content, error))
    {
        return true;
    }
    error = QStringLiteral("%1 cannot be written: %2").arg(QDir::toNativeSeparators(filePath), error);
    return false;
}

} // namespace

SettingsFile::SettingsFile(Universe& application, QString fileName, Galaxy* galaxy, QString name)
    : m_application(application), m_galaxy(galaxy), m_fileName(std::move(fileName)),
      m_name(name.isEmpty() ? m_fileName : std::move(name))
{
}

SettingsFile::~SettingsFile()
{
    if(m_running)
    {
        stop(); // What the user changed is written, and the properties have the values of the project again
    }
}

QString SettingsFile::fileName() const
{
    return m_fileName;
}

bool SettingsFile::setFileName(const QString& fileName)
{
    return m_galaxy == nullptr && rename(fileName, fileName);
}

QString SettingsFile::name() const
{
    return m_name;
}

bool SettingsFile::setName(const QString& name)
{
    // After the same galaxies: what its file name starts with.
    return rename(m_fileName.chopped(m_name.size()) + name, name);
}

Galaxy* SettingsFile::galaxy() const
{
    return m_galaxy;
}

bool SettingsFile::rename(const QString& fileName, const QString& name)
{
    if(name.isEmpty() || !isFreeFileName(fileName))
    {
        return false;
    }
    const bool renamed = std::exchange(m_fileName, fileName) != fileName;
    if(std::exchange(m_name, name) != name || renamed)
    {
        emit changed();
    }
    return true;
}

bool SettingsFile::isFreeFileName(const QString& fileName) const
{
    const SettingsFile* named = m_application.settingsFile(fileName);
    return named == nullptr || named == this; // Its own name in another case is its own
}

QString SettingsFile::directory() const
{
    return m_directory;
}

void SettingsFile::setDirectory(const QString& directory)
{
    if(directory == m_directory)
    {
        return;
    }
    m_directory = directory;
    emit changed();
}

QString SettingsFile::filePath() const
{
    return filePath(m_fileName, m_directory);
}

QString SettingsFile::filePath(const QString& fileName, const QString& directory)
{
    if(directory.isEmpty())
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + u'/' + fileName;
    }
    // As it is when it is absolute.
    return QDir::cleanPath(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(directory) + u'/' + fileName);
}

const QList<SettingsEntry>& SettingsFile::entries() const
{
    return m_entries;
}

bool SettingsFile::contains(const SettingsEntry& entry) const
{
    return m_entries.contains(entry);
}

bool SettingsFile::add(const SettingsEntry& entry)
{
    if(!canAdd(entry))
    {
        return false;
    }
    m_entries.append(entry);
    connect(entry.component, &QObject::destroyed, this, &SettingsFile::forget, Qt::UniqueConnection);
    connect(entry.object, &QObject::destroyed, this, &SettingsFile::forget, Qt::UniqueConnection);
    emit changed();
    return true;
}

bool SettingsFile::canAdd(const SettingsEntry& entry) const
{
    return entry.component != nullptr && isAmong(entry.component, m_application.components(m_galaxy)) &&
           canHold(*entry.component, entry.object, entry.property) && m_application.settingsFile(entry) == nullptr;
}

void SettingsFile::remove(const SettingsEntry& entry)
{
    if(m_entries.removeAll(entry) > 0)
    {
        emit changed();
    }
}

bool SettingsFile::writeDefaults()
{
    if(m_running)
    {
        return refuse(QStringLiteral("The default values cannot be written while the application runs"));
    }
    QString problem;
    const std::optional<QJsonObject> content = readSettings(filePath(), problem);
    return content ? writeSettings(filePath(), withCurrentValues(*content, m_entries), m_errorString) : refuse(problem);
}

QString SettingsFile::errorString() const
{
    return m_errorString;
}

bool SettingsFile::canHold(const Component& component, const QObject* object, const QString& property)
{
    const QList<QMetaProperty> properties = ProjectJson::savedProperties(component, object);
    return std::ranges::any_of(properties, [&property](const QMetaProperty& saved)
                               { return QLatin1StringView(saved.name()) == property; });
}

void SettingsFile::start()
{
    m_running = true;
    m_startedPath = filePath();
    readContent();
    if(!giveValues())
    {
        writeContent(); // With the values it missed
    }
    followChanges(); // Once their values are given: those are not changes to write
}

void SettingsFile::readContent()
{
    QString problem;
    const std::optional<QJsonObject> content = readSettings(m_startedPath, problem);
    if(!problem.isEmpty())
    {
        fail(problem);
    }
    m_writable = content.has_value();
    m_content = content.value_or(QJsonObject());
}

bool SettingsFile::giveValues()
{
    bool complete = true;
    for(const SettingsEntry& entry : std::as_const(m_entries))
    {
        m_started.append(StartedEntry{.entry = entry, .defaultValue = propertyOf(entry).read(entry.object)});
        complete = giveValue(entry) && complete;
    }
    return complete;
}

bool SettingsFile::giveValue(const SettingsEntry& entry)
{
    const QJsonValue value = valueOf(m_content, entry);
    if(value.isUndefined() || !propertyOf(entry).write(entry.object, value.toVariant()))
    {
        tellWrongValue(entry, value);
        setValue(m_content, entry, currentValue(entry));
        return false;
    }
    return true;
}

void SettingsFile::tellWrongValue(const SettingsEntry& entry, const QJsonValue& value)
{
    if(value.isUndefined())
    {
        return;
    }
    fail(QStringLiteral("%1 holds for %2 of %3 a value it cannot have: it is given its default value")
             .arg(QDir::toNativeSeparators(m_startedPath), entry.property, entry.component->objectName()));
}

void SettingsFile::followChanges()
{
    const QMetaMethod take = metaObject()->method(metaObject()->indexOfSlot("takeValues()"));
    for(const StartedEntry& started : std::as_const(m_started))
    {
        follow(started.entry, take);
    }
}

void SettingsFile::follow(const SettingsEntry& entry, const QMetaMethod& take)
{
    const QMetaProperty property = propertyOf(entry);
    const QMetaObject::Connection followed =
        property.hasNotifySignal() ? connect(entry.object, property.notifySignal(), this, take, Qt::UniqueConnection)
                                   : QMetaObject::Connection();
    if(followed) // Not when another property of the object has the same notify signal
    {
        m_followed.append(followed);
    }
}

void SettingsFile::stop()
{
    disconnectAll(m_followed);
    takeAllValues();
    m_writePending = false;
    writeContent();
    giveBackDefaults();
    m_content = {};
    m_running = false;
}

void SettingsFile::takeValue(const StartedEntry& started)
{
    setValue(m_content, started.entry, currentValue(started.entry));
}

void SettingsFile::takeAllValues()
{
    for(const StartedEntry& started : std::as_const(m_started))
    {
        takeValue(started);
    }
}

void SettingsFile::giveBackDefaults()
{
    for(const StartedEntry& started : std::as_const(m_started))
    {
        propertyOf(started.entry).write(started.entry.object, started.defaultValue);
    }
    m_started.clear();
}

void SettingsFile::scheduleWrite()
{
    if(!m_writePending)
    {
        m_writePending = true;
        QTimer::singleShot(0, this, &SettingsFile::writeTaken);
    }
}

void SettingsFile::writeContent()
{
    if(m_writable && !writeSettings(m_startedPath, m_content, m_errorString))
    {
        m_writable = false; // Told once
        fail(m_errorString);
    }
}

void SettingsFile::fail(const QString& why)
{
    m_errorString = why;
    qCWarning(lcSettings).noquote() << why;
    emit failed(why);
}

bool SettingsFile::refuse(const QString& why)
{
    m_errorString = why;
    return false;
}

void SettingsFile::takeValues()
{
    const QObject* changed = sender();
    for(const StartedEntry& started : std::as_const(m_started))
    {
        if(started.entry.object == changed)
        {
            takeValue(started);
        }
    }
    scheduleWrite();
}

void SettingsFile::writeTaken()
{
    if(!m_writePending) // Written meanwhile, as the application stopped
    {
        return;
    }
    m_writePending = false;
    writeContent();
}

void SettingsFile::forget(QObject* object)
{
    const auto of = [object](const SettingsEntry& entry)
    { return entry.object == object || entry.component == object; };
    m_started.removeIf([&of](const StartedEntry& started) { return of(started.entry); });
    if(m_entries.removeIf(of) > 0)
    {
        emit changed();
    }
}

} // namespace Nebula
