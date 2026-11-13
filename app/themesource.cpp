#include "themesource.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

static const char *PortalService = "org.freedesktop.portal.Desktop";
static const char *PortalPath = "/org/freedesktop/portal/desktop";
static const char *PortalSettings = "org.freedesktop.portal.Settings";
static const QString Material = QStringLiteral("material");

struct Accent {
    const char *id;
    const char *label;
    const char *dark;
    const char *light;
};

// "default" and "system" resolve at runtime; "custom" uses the stored colour.
static const Accent Accents[] = {
    {"default", QT_TR_NOOP("Default"), nullptr, nullptr},
    {"system", QT_TR_NOOP("System"), nullptr, nullptr},
    {"orange", QT_TR_NOOP("Orange"), "#ff9433", "#c8650a"},
    {"red", QT_TR_NOOP("Red"), "#ff6b6b", "#c62828"},
    {"pink", QT_TR_NOOP("Pink"), "#f48fb1", "#c2185b"},
    {"purple", QT_TR_NOOP("Purple"), "#b39ddb", "#6a3fb5"},
    {"blue", QT_TR_NOOP("Blue"), "#64b5f6", "#1565c0"},
    {"teal", QT_TR_NOOP("Teal"), "#4dd0c4", "#00796b"},
    {"green", QT_TR_NOOP("Green"), "#81c784", "#2e7d32"},
    {"yellow", QT_TR_NOOP("Yellow"), "#ffd54f", "#a37800"},
    {"custom", QT_TR_NOOP("Custom"), nullptr, nullptr},
};

static QColor mix(const QColor &a, const QColor &b, double t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}

static QColor contrastOn(const QColor &c)
{
    const double luminance = 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
    return luminance > 0.55 ? QColor("#121212") : QColor("#ffffff");
}

// Prismora's Material: every ground and line is a mix of the surface and the text colour.
static QVariantMap material(const QColor &surface, const QColor &text, const QColor &primary)
{
    return {{"surface", surface.name()},
            {"surface_container_low", mix(surface, text, 0.04).name()},
            {"surface_container", mix(surface, text, 0.07).name()},
            {"surface_container_high", mix(surface, text, 0.10).name()},
            {"surface_container_highest", mix(surface, text, 0.15).name()},
            {"on_surface", text.name()},
            {"on_surface_variant", mix(text, surface, 0.38).name()},
            {"outline", mix(surface, text, 0.32).name()},
            {"outline_variant", mix(surface, text, 0.17).name()},
            {"primary", primary.name()},
            {"on_primary", contrastOn(primary).name()},
            {"shadow", "#000000"}};
}

static QVariantMap materialFor(bool dark)
{
    return dark ? material(QColor("#121212"), QColor("#e6e6e6"), Qt::white)
                : material(QColor("#f4f4f4"), QColor("#161616"), Qt::black);
}

ThemeSource::ThemeSource(QSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_theme = m_settings->value("theme", Material).toString();
    m_mode = m_settings->value("mode", "system").toString();
    // Before the options page, "theme" held the mode; read those values as such without rewriting them.
    if (m_theme == "auto" || m_theme == "dark" || m_theme == "light") {
        if (!m_settings->contains("mode"))
            m_mode = m_theme == "auto" ? "system" : m_theme;
        m_theme = Material;
    }
    m_accent = m_settings->value("accent", "default").toString();
    m_customAccent = QColor(m_settings->value("customAccent", "#8ecfd4").toString());

    QDBusConnection::sessionBus().connect(PortalService, PortalPath, PortalSettings, "SettingChanged", this,
                                          SLOT(portalSettingChanged(QString, QString, QDBusVariant)));
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &ThemeSource::rebuild);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] {
        scanThemes();
        rebuild();
    });
    readPortal();
    scanThemes();
    rebuild();
}

QString ThemeSource::themeFile(const QString &name) const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/GlyphPop/themes/" + name
        + ".json";
}

void ThemeSource::scanThemes()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/GlyphPop/themes";
    if (QFileInfo::exists(dir) && !m_watcher.directories().contains(dir))
        m_watcher.addPath(dir);
    QStringList themes{Material};
    for (const QFileInfo &file : QDir(dir).entryInfoList({"*.json"}, QDir::Files, QDir::Name))
        themes.append(file.completeBaseName());
    if (themes != m_themes) {
        m_themes = themes;
        emit themesChanged();
    }
}

void ThemeSource::rescan()
{
    scanThemes();
}

void ThemeSource::setTheme(const QString &theme)
{
    if (theme == m_theme)
        return;
    m_theme = theme;
    m_settings->setValue("theme", theme);
    rebuild();
}

void ThemeSource::setMode(const QString &mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;
    m_settings->setValue("mode", mode);
    rebuild();
}

void ThemeSource::setAccent(const QString &accent)
{
    if (accent == m_accent)
        return;
    m_accent = accent;
    m_settings->setValue("accent", accent);
    rebuild();
}

void ThemeSource::setCustomAccent(const QColor &color)
{
    if (!color.isValid() || color == m_customAccent)
        return;
    m_customAccent = color;
    m_settings->setValue("customAccent", color.name());
    rebuild();
}

QColor ThemeSource::accentColor(const QString &id, bool dark, const QColor &themePrimary) const
{
    if (id == "system" && m_portalAccent.isValid())
        return m_portalAccent;
    if (id == "custom" && m_customAccent.isValid())
        return m_customAccent;
    for (const Accent &accent : Accents) {
        if (id == accent.id && accent.dark)
            return QColor(dark ? accent.dark : accent.light);
    }
    return themePrimary;
}

QVariantList ThemeSource::accents() const
{
    QVariantList out;
    for (const Accent &accent : Accents) {
        const QString id = QString::fromLatin1(accent.id);
        if (id == "system" && !m_portalAccent.isValid())
            continue;
        const QColor color = accentColor(id, m_dark, m_themePrimary);
        out.append(QVariantMap{{"id", id}, {"label", tr(accent.label)}, {"color", color.name()}});
    }
    return out;
}

static QVariant unwrap(QVariant v)
{
    while (v.metaType() == QMetaType::fromType<QDBusVariant>())
        v = v.value<QDBusVariant>().variant();
    return v;
}

static QColor portalAccentFrom(const QVariant &value)
{
    const QVariant v = unwrap(value);
    if (v.metaType() != QMetaType::fromType<QDBusArgument>())
        return {};
    const QDBusArgument arg = v.value<QDBusArgument>();
    double r = -1, g = -1, b = -1;
    arg.beginStructure();
    arg >> r >> g >> b;
    arg.endStructure();
    const auto unit = [](double c) { return c >= 0.0 && c <= 1.0; };
    return unit(r) && unit(g) && unit(b) ? QColor::fromRgbF(r, g, b) : QColor();
}

void ThemeSource::readPortal()
{
    auto read = [](const char *key) {
        QDBusMessage call = QDBusMessage::createMethodCall(PortalService, PortalPath, PortalSettings, "ReadOne");
        call << QStringLiteral("org.freedesktop.appearance") << QString::fromLatin1(key);
        const QDBusMessage reply = QDBusConnection::sessionBus().call(call, QDBus::Block, 500);
        return reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty() ? reply.arguments().first()
                                                                                         : QVariant();
    };
    m_portalScheme = unwrap(read("color-scheme")).toInt();
    m_portalAccent = portalAccentFrom(read("accent-color"));
}

void ThemeSource::portalSettingChanged(const QString &ns, const QString &key, const QDBusVariant &value)
{
    if (ns != "org.freedesktop.appearance")
        return;
    if (key == "color-scheme")
        m_portalScheme = unwrap(value.variant()).toInt();
    else if (key == "accent-color")
        m_portalAccent = portalAccentFrom(value.variant());
    else
        return;
    rebuild();
}

static void applyColors(QVariantMap &colors, const QJsonObject &overrides)
{
    for (auto it = overrides.begin(); it != overrides.end(); ++it) {
        if (colors.contains(it.key()) && QColor::isValidColorName(it.value().toString()))
            colors[it.key()] = it.value().toString();
    }
}

void ThemeSource::rebuild()
{
    // Portal color-scheme: 1 prefers dark, 2 prefers light, 0 has no preference (we default to dark).
    bool dark = m_mode == "dark" || (m_mode != "light" && m_portalScheme != 2);
    bool fixed = false;
    int radius = 10;
    QVariantMap colors;

    if (m_theme != Material) {
        QFile f(themeFile(m_theme));
        QJsonObject json;
        if (!m_watcher.files().contains(f.fileName()) && f.exists())
            m_watcher.addPath(f.fileName());
        if (f.open(QIODevice::ReadOnly))
            json = QJsonDocument::fromJson(f.readAll()).object();
        else
            qWarning("glyphpop: theme %s not found, using Material", qPrintable(f.fileName()));
        if (json.contains("base")) {
            dark = json.value("base").toString() != "light";
            fixed = true;
        }
        QJsonObject overrides = json.value("colors").toObject();
        const QJsonObject perMode = json.value(dark ? "dark" : "light").toObject();
        for (auto it = perMode.begin(); it != perMode.end(); ++it)
            overrides.insert(it.key(), it.value());
        // A file that only sets surface and text still gets matching containers and lines.
        const QVariantMap base = materialFor(dark);
        const auto pick = [&](const char *key) {
            const QString v = overrides.value(key).toString();
            return QColor::isValidColorName(v) ? QColor(v) : QColor(base.value(key).toString());
        };
        colors = material(pick("surface"), pick("on_surface"), pick("primary"));
        applyColors(colors, overrides);
        radius = json.value("radius").toInt(radius);
    } else {
        colors = materialFor(dark);
    }

    m_themePrimary = QColor(colors.value("primary").toString());
    if (m_accent != "default") {
        const QColor accent = accentColor(m_accent, dark, QColor(colors.value("primary").toString()));
        colors["primary"] = accent.name();
        colors["on_primary"] = contrastOn(accent).name();
    }

    m_colors = colors;
    m_dark = dark;
    m_modeFixed = fixed;
    m_radius = radius;
    emit changed();
}
