// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QLabel>
#include <QObject>
#include <QString>

// One of the labels of a LabelsComponent: the QLabel it provides, and its text, the one property of it edited and
// saved, in place of the many of a QLabel.
class Label : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

  public:
    explicit Label(const QString& text);

    // What is provided.
    [[nodiscard]] QLabel& widget();
    [[nodiscard]] QString text() const;
    void setText(const QString& text);

  signals:
    void textChanged(const QString& text);

  private:
    QLabel m_label;
};
