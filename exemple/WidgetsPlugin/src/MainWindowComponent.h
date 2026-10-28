// Copyright (C) 2026 Fabien Di Grande
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <QMainWindow>
#include <QString>

#include "Nebula/Component/Component.h"

class QWidget;

// The main window of an application: a QMainWindow it creates and provides as the interface "mainWindow", for other
// components to fill, a TopBarComponent say, and shows while it runs. The widget it consumes as "centralWidget" fills
// its middle, for as long as it has it. It needs a QApplication.
class MainWindowComponent : public Nebula::Component
{
    Q_OBJECT
    Q_CLASSINFO("description", "The main window of the application, which other components fill")
    Q_CLASSINFO("category", "Widgets")
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

  public:
    MainWindowComponent();

    [[nodiscard]] QString title() const;
    void setTitle(const QString& title);
    // Its size each time it starts, in pixels.
    [[nodiscard]] int width() const;
    void setWidth(int width);
    [[nodiscard]] int height() const;
    void setHeight(int height);

  signals:
    void titleChanged(const QString& title);
    void widthChanged(int width);
    void heightChanged(int height);

  protected:
    bool onStart() override;
    void onStop() override;

  private:
    // Shows the widget it consumes in the middle of the window.
    void showCentralWidget();
    // Takes the widget it consumes out of the window, without a parent, as it was provided.
    void giveBackCentralWidget();

    QMainWindow m_window;
    QWidget* m_centralWidget = nullptr; // Consumed
    int m_width = 800;
    int m_height = 600;
};
