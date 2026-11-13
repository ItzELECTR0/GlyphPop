#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QWaylandClientExtensionTemplate>
#include <memory>

#include "qwayland-input-method-unstable-v2.h"

struct xkb_context;
struct xkb_keymap;
struct xkb_state;
class QWindow;

class InputMethodManager : public QWaylandClientExtensionTemplate<InputMethodManager>,
                           public QtWayland::zwp_input_method_manager_v2
{
public:
    InputMethodManager();
};

// Turns grabbed keys into Qt key events for one window, which never has real keyboard focus.
class KeyboardGrab : public QtWayland::zwp_input_method_keyboard_grab_v2
{
public:
    KeyboardGrab(struct ::zwp_input_method_keyboard_grab_v2 *grab, QWindow *target);
    ~KeyboardGrab() override;

    Qt::KeyboardModifiers modifiers() const;
    void stop();

protected:
    void zwp_input_method_keyboard_grab_v2_keymap(uint32_t format, int32_t fd, uint32_t size) override;
    void zwp_input_method_keyboard_grab_v2_key(uint32_t serial, uint32_t time, uint32_t key, uint32_t state) override;
    void zwp_input_method_keyboard_grab_v2_modifiers(uint32_t serial, uint32_t depressed, uint32_t latched,
                                                     uint32_t locked, uint32_t group) override;
    void zwp_input_method_keyboard_grab_v2_repeat_info(int32_t rate, int32_t delay) override;

private:
    void deliver(uint32_t key, bool press, bool repeat);

    QPointer<QWindow> m_target;
    xkb_context *m_context = nullptr;
    xkb_keymap *m_keymap = nullptr;
    xkb_state *m_state = nullptr;
    QList<uint32_t> m_down;
    QTimer m_repeat;
    uint32_t m_repeatKey = 0;
    int m_repeatRate = 25;
    int m_repeatDelay = 600;
};

class InputMethod : public QObject, public QtWayland::zwp_input_method_v2
{
    Q_OBJECT

public:
    explicit InputMethod(QObject *parent = nullptr);
    ~InputMethod() override;

    bool bound() const { return isInitialized(); }
    bool active() const { return m_active; }
    void commitText(const QString &text);
    void grabKeyboard(QWindow *target);
    void releaseKeyboard();
    // Qt reads pointer modifiers from a keyboard focus the popup never has, so Shift+click asks here.
    Qt::KeyboardModifiers grabbedModifiers() const;

signals:
    void activeChanged();
    // The focused field changed for a reason other than our own commit, e.g. a click moved the caret.
    void externalChange();

protected:
    void zwp_input_method_v2_activate() override;
    void zwp_input_method_v2_deactivate() override;
    void zwp_input_method_v2_text_change_cause(uint32_t cause) override;
    void zwp_input_method_v2_done() override;
    void zwp_input_method_v2_unavailable() override;

private:
    void bind();

    InputMethodManager m_manager;
    std::unique_ptr<KeyboardGrab> m_grab;
    bool m_pendingActive = false;
    bool m_active = false;
    bool m_unavailable = false;
    uint32_t m_cause = 0;
    uint32_t m_serial = 0;
};
