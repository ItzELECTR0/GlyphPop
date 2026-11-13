#include "inputmethod.h"

#include <QGuiApplication>
#include <QWindow>
#include <QtGui/private/qxkbcommon_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

// zwp_text_input_v3.change_cause
constexpr uint32_t CauseInputMethod = 0;
constexpr uint32_t CauseOther = 1;

InputMethodManager::InputMethodManager()
    : QWaylandClientExtensionTemplate<InputMethodManager>(1)
{
    initialize();
}

KeyboardGrab::KeyboardGrab(struct ::zwp_input_method_keyboard_grab_v2 *grab, QWindow *target)
    : QtWayland::zwp_input_method_keyboard_grab_v2(grab)
    , m_target(target)
    , m_context(xkb_context_new(XKB_CONTEXT_NO_FLAGS))
{
    m_repeat.setSingleShot(false);
    QObject::connect(&m_repeat, &QTimer::timeout, [this] {
        m_repeat.setInterval(1000 / qMax(1, m_repeatRate));
        deliver(m_repeatKey, true, true);
    });
}

KeyboardGrab::~KeyboardGrab()
{
    stop();
    if (m_state)
        xkb_state_unref(m_state);
    if (m_keymap)
        xkb_keymap_unref(m_keymap);
    xkb_context_unref(m_context);
}

void KeyboardGrab::zwp_input_method_keyboard_grab_v2_keymap(uint32_t format, int32_t fd, uint32_t size)
{
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }
    void *map = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (map == MAP_FAILED)
        return;
    xkb_keymap *keymap = xkb_keymap_new_from_string(m_context, static_cast<const char *>(map),
                                                     XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map, size);
    if (!keymap)
        return;
    if (m_state)
        xkb_state_unref(m_state);
    if (m_keymap)
        xkb_keymap_unref(m_keymap);
    m_keymap = keymap;
    m_state = xkb_state_new(m_keymap);
}

void KeyboardGrab::zwp_input_method_keyboard_grab_v2_key(uint32_t, uint32_t, uint32_t key, uint32_t state)
{
    const bool press = state == WL_KEYBOARD_KEY_STATE_PRESSED;
    if (press) {
        m_down.append(key);
    } else if (!m_down.removeOne(key)) {
        // Keys held while the grab started (the shortcut that opened us) release into nothing.
        return;
    }
    deliver(key, press, false);
    if (!isInitialized())
        return;

    if (press && m_keymap && xkb_keymap_key_repeats(m_keymap, key + 8) && m_repeatRate > 0) {
        m_repeatKey = key;
        m_repeat.start(m_repeatDelay);
    } else if (!press && key == m_repeatKey) {
        m_repeat.stop();
    }
}

void KeyboardGrab::zwp_input_method_keyboard_grab_v2_modifiers(uint32_t, uint32_t depressed, uint32_t latched,
                                                               uint32_t locked, uint32_t group)
{
    if (m_state)
        xkb_state_update_mask(m_state, depressed, latched, locked, 0, 0, group);
}

void KeyboardGrab::zwp_input_method_keyboard_grab_v2_repeat_info(int32_t rate, int32_t delay)
{
    m_repeatRate = rate;
    m_repeatDelay = delay;
}

void KeyboardGrab::stop()
{
    m_repeat.stop();
    m_target = nullptr;
    if (isInitialized())
        release();
}

Qt::KeyboardModifiers KeyboardGrab::modifiers() const
{
    return m_state ? QXkbCommon::modifiers(m_state) : Qt::NoModifier;
}

void KeyboardGrab::deliver(uint32_t key, bool press, bool repeat)
{
    if (!m_state || !m_target)
        return;
    const xkb_keycode_t code = key + 8;
    const xkb_keysym_t sym = xkb_state_key_get_one_sym(m_state, code);
    const Qt::KeyboardModifiers mods = QXkbCommon::modifiers(m_state, sym);
    const int qtKey = QXkbCommon::keysymToQtKey(sym, mods, m_state, code);
    const QString text = QXkbCommon::lookupString(m_state, code);
    QWindowSystemInterface::handleExtendedKeyEvent(m_target, press ? QEvent::KeyPress : QEvent::KeyRelease, qtKey,
                                                   mods, code, sym, 0, text, repeat);
    QWindowSystemInterface::flushWindowSystemEvents();
}

InputMethod::InputMethod(QObject *parent)
    : QObject(parent)
{
    connect(&m_manager, &QWaylandClientExtension::activeChanged, this, &InputMethod::bind);
    bind();
}

InputMethod::~InputMethod()
{
    releaseKeyboard();
    if (isInitialized())
        destroy();
}

void InputMethod::bind()
{
    if (isInitialized() || m_unavailable || !m_manager.isActive())
        return;
    auto *wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!wayland || !wayland->seat())
        return;
    init(m_manager.get_input_method(wayland->seat()));
}

void InputMethod::commitText(const QString &text)
{
    if (!isInitialized() || !m_active)
        return;
    commit_string(text);
    commit(m_serial);
}

void InputMethod::grabKeyboard(QWindow *target)
{
    if (!isInitialized() || !m_active)
        return;
    m_grab = std::make_unique<KeyboardGrab>(grab_keyboard(), target);
}

void InputMethod::releaseKeyboard()
{
    if (!m_grab)
        return;
    // A grabbed key can be what closes the picker, so the grab may still be on the stack: free it later.
    m_grab->stop();
    QTimer::singleShot(0, [grab = m_grab.release()] { delete grab; });
}

Qt::KeyboardModifiers InputMethod::grabbedModifiers() const
{
    return m_grab ? m_grab->modifiers() : Qt::NoModifier;
}

void InputMethod::zwp_input_method_v2_activate()
{
    m_pendingActive = true;
}

void InputMethod::zwp_input_method_v2_deactivate()
{
    m_pendingActive = false;
}

void InputMethod::zwp_input_method_v2_text_change_cause(uint32_t cause)
{
    m_cause = cause;
}

void InputMethod::zwp_input_method_v2_done()
{
    ++m_serial;
    const bool wasActive = m_active;
    m_active = m_pendingActive;
    const uint32_t cause = m_cause;
    m_cause = CauseInputMethod;
    if (m_active != wasActive)
        emit activeChanged();
    else if (m_active && cause == CauseOther)
        emit externalChange();
}

void InputMethod::zwp_input_method_v2_unavailable()
{
    qWarning("glyphpop: another input method owns this seat, so emoji go in through the virtual keyboard");
    m_unavailable = true;
    releaseKeyboard();
    destroy();
    if (m_active) {
        m_active = false;
        emit activeChanged();
    }
}
