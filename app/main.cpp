#include "picker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <cstdio>

static const char *Service = "net.electris.GlyphPop";
static const char *ObjectPath = "/net/electris/GlyphPop";

static const char *Usage = R"(Usage: glyphpop [command]

  (none), toggle   open the picker, or close it if it is open
  emoji            open on emoji
  kaomoji          open on kaomoji
  symbols          open on symbols
  settings         open the options
  close            close the picker
  quit             stop the running instance
  --daemon         start in the background without opening
)";

class Instance : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "net.electris.GlyphPop")

public:
    explicit Instance(Picker *picker)
        : m_picker(picker)
    {
    }

public slots:
    void Activate(const QStringList &args)
    {
        if (args.value(0) == "quit")
            QCoreApplication::quit();
        else
            m_picker->handleCommand(args);
    }

private:
    Picker *m_picker;
};

int main(int argc, char **argv)
{
    QStringList args;
    for (int i = 1; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));
    const bool daemon = args.removeAll("--daemon") > 0;
    if (args.contains("--help") || args.contains("-h")) {
        std::fputs(Usage, stdout);
        return 0;
    }
    if (args.contains("--version")) {
        std::puts("glyphpop " GLYPHPOP_VERSION);
        return 0;
    }

    // Only Wayland has the input-method protocol. Our own search field must not reach our own input method, so
    // Qt may only use its own text-input protocol, which wlroots compositors and Hyprland do not offer.
    qputenv("QT_QPA_PLATFORM", "wayland");
    qputenv("QT_WAYLAND_TEXT_INPUT_PROTOCOL", "qt_text_input_method_v1");
    qunsetenv("QT_IM_MODULE");
    qunsetenv("QT_IM_MODULES");

    QGuiApplication app(argc, argv);
    app.setApplicationName("GlyphPop");
    app.setDesktopFileName("net.electris.GlyphPop");
    app.setQuitOnLastWindowClosed(false);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(Service)) {
        QDBusMessage call = QDBusMessage::createMethodCall(Service, ObjectPath, Service, "Activate");
        call << args;
        const QDBusMessage reply = bus.call(call, QDBus::Block, 3000);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            std::fprintf(stderr, "glyphpop: the running instance did not answer: %s\n", qPrintable(reply.errorMessage()));
            return 1;
        }
        return 0;
    }
    if (args.value(0) == "quit")
        return 0;

    Picker picker;
    qmlRegisterSingletonInstance("GlyphPop.Core", 1, 0, "Picker", &picker);
    QQmlApplicationEngine engine;
    engine.loadFromModule("GlyphPop", "App");
    if (engine.rootObjects().isEmpty())
        return 1;
    QObject *root = engine.rootObjects().first();
    picker.setWindows(root->property("popupWindow").value<QQuickWindow *>(),
                      root->property("overlayWindow").value<QQuickWindow *>());

    Instance instance(&picker);
    bus.registerObject(ObjectPath, &instance, QDBusConnection::ExportAllSlots);
    if (!daemon)
        picker.handleCommand(args);
    return app.exec();
}

#include "main.moc"
