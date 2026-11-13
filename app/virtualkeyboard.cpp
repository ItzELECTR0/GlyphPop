#include "virtualkeyboard.h"

#include <QGuiApplication>
#include <QTimer>
#include <iterator>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

VirtualKeyboardManager::VirtualKeyboardManager()
    : QWaylandClientExtensionTemplate<VirtualKeyboardManager>(1)
{
    initialize();
}

VirtualKeyboard::VirtualKeyboard(QObject *parent)
    : QObject(parent)
{
}

// Evdev codes no keyboard defines and X11 can still see. A client that ignores our keymap then gets keys
// that do nothing, rather than Escape or a letter.
static constexpr uint32_t SafeKeys[] = {84, 195, 196, 197, 198, 199};
static constexpr int SafeKeyCount = int(std::size(SafeKeys));

static QByteArray keymapFor(const QList<char32_t> &codepoints)
{
    QByteArray keycodes;
    QByteArray symbols;
    for (int i = 0; i < codepoints.size(); ++i) {
        char name[64];
        xkb_keysym_get_name(xkb_utf32_to_keysym(codepoints[i]), name, sizeof name);
        keycodes += "<K" + QByteArray::number(i) + "> = " + QByteArray::number(SafeKeys[i] + 8) + ";\n";
        symbols += "key <K" + QByteArray::number(i) + "> {[ " + name + " ]};\n";
    }
    return "xkb_keymap {\n"
           "xkb_keycodes \"glyphpop\" { minimum = 8; maximum = 255;\n"
        + keycodes
        + "};\n"
          "xkb_types \"glyphpop\" { include \"complete\" };\n"
          "xkb_compatibility \"glyphpop\" { include \"complete\" };\n"
          "xkb_symbols \"glyphpop\" {\n"
        + symbols + "};\n};\n";
}

static bool sendKeymap(QtWayland::zwp_virtual_keyboard_v1 *keyboard, const QList<char32_t> &codepoints)
{
    const QByteArray keymap = keymapFor(codepoints);
    const int fd = memfd_create("glyphpop-keymap", MFD_CLOEXEC);
    if (fd < 0)
        return false;
    const bool written = write(fd, keymap.constData(), keymap.size() + 1) == keymap.size() + 1;
    if (written)
        keyboard->keymap(WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, fd, keymap.size() + 1);
    close(fd);
    return written;
}

void VirtualKeyboard::type(const QString &text)
{
    auto *wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!m_manager.isActive() || !wayland || !wayland->seat() || text.isEmpty())
        return;

    auto *keyboard = new QtWayland::zwp_virtual_keyboard_v1(m_manager.create_virtual_keyboard(wayland->seat()));
    // Some clients drop keys that arrive in the same frame as a keymap change, so pace everything.
    int delay = 0;
    const QList<uint> ucs4 = text.toUcs4();
    const QList<char32_t> all(ucs4.begin(), ucs4.end());
    for (int start = 0; start < all.size();) {
        QList<char32_t> unique;
        QList<int> sequence;
        int end = start;
        for (; end < all.size(); ++end) {
            int index = unique.indexOf(all[end]);
            if (index < 0) {
                if (unique.size() == SafeKeyCount)
                    break;
                index = unique.size();
                unique.append(all[end]);
            }
            sequence.append(index);
        }
        QTimer::singleShot(delay, this, [keyboard, unique] { sendKeymap(keyboard, unique); });
        delay += 20;
        for (int index : sequence) {
            QTimer::singleShot(delay, this, [keyboard, index] {
                keyboard->key(0, SafeKeys[index], WL_KEYBOARD_KEY_STATE_PRESSED);
                keyboard->key(0, SafeKeys[index], WL_KEYBOARD_KEY_STATE_RELEASED);
            });
            delay += 8;
        }
        start = end;
    }
    QTimer::singleShot(delay + 100, this, [keyboard] {
        keyboard->destroy();
        delete keyboard;
    });
}
