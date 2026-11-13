#pragma once

#include <QObject>
#include <QWaylandClientExtensionTemplate>

#include "qwayland-virtual-keyboard-unstable-v1.h"

class VirtualKeyboardManager : public QWaylandClientExtensionTemplate<VirtualKeyboardManager>,
                               public QtWayland::zwp_virtual_keyboard_manager_v1
{
public:
    VirtualKeyboardManager();
};

// Types any text into whatever has keyboard focus, by uploading a keymap that maps one key per codepoint.
class VirtualKeyboard : public QObject
{
    Q_OBJECT

public:
    explicit VirtualKeyboard(QObject *parent = nullptr);

    bool available() const { return m_manager.isActive(); }
    void type(const QString &text);

private:
    VirtualKeyboardManager m_manager;
};
