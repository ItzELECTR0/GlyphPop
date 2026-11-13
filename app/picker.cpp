#include "picker.h"

#include "core.h"
#include "inputmethod.h"
#include "popupshell.h"
#include "themesource.h"
#include "virtualkeyboard.h"

#include <LayerShellQt/Window>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QQuickWindow>
#include <QTimer>
#include <algorithm>
#include <qpa/qwindowsysteminterface.h>

static constexpr int RecentCategory = 999;
static constexpr int SearchCategory = -1;

static const char *const EmojiIcons[] = {"😀", "👋", "🐻", "🍔", "🚗", "⚽", "💡", "🔣", "🏳️"};
static const char *const SymbolIcons[] = {"§", "€", "∑", "→", "◆", "™", "Æ", "Ω", "☂", "┼"};

Picker::Picker(QObject *parent)
    : QObject(parent)
    , m_settings("GlyphPop", "GlyphPop")
    , m_inputMethod(new InputMethod(this))
    , m_keyboard(new VirtualKeyboard(this))
    , m_theme(new ThemeSource(&m_settings, this))
    , m_tone(qBound(0, m_settings.value("tone", 0).toInt(), 5))
{
    if (!gp_init())
        qFatal("glyphpop: could not load the emoji data");

    // Qt's Unicode tables can lag the emoji data, so newer emoji only render when the emoji font is asked for by name.
    QFont emoji;
    emoji.setFamilies({QStringLiteral("emoji")});
    m_emojiFamily = QFontInfo(emoji).family();
    const QFontMetricsF metrics{QFont(m_emojiFamily)};
    for (uint32_t i = 0; i < gp_emoji_count(); ++i) {
        const auto ucs4 = QString::fromUtf8(gp_emoji_text(i, 0)).toUcs4();
        if (!ucs4.isEmpty() && metrics.inFontUcs4(ucs4.first()))
            m_visibleEmoji.append(int(i));
    }
    // A missing "emoji" fontconfig alias resolves to a text font; then trust fallback rendering instead.
    if (m_visibleEmoji.size() < int(gp_emoji_count()) / 2) {
        m_emojiFamily.clear();
        m_visibleEmoji.clear();
        for (uint32_t i = 0; i < gp_emoji_count(); ++i)
            m_visibleEmoji.append(int(i));
    }

    connect(m_inputMethod, &InputMethod::activeChanged, this, [this] {
        if (m_mode == Popup && !m_inputMethod->active())
            close();
    });
    connect(m_inputMethod, &InputMethod::externalChange, this, [this] {
        if (m_mode == Popup)
            close();
    });
    rebuild();
}

Picker::~Picker() = default;

void Picker::setWindows(QQuickWindow *popup, QQuickWindow *overlay)
{
    m_popup = popup;
    m_overlay = overlay;
    PopupShellIntegration::attach(popup, m_inputMethod);

    auto *layer = LayerShellQt::Window::get(overlay);
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setAnchors({LayerShellQt::Window::AnchorTop, LayerShellQt::Window::AnchorBottom,
                       LayerShellQt::Window::AnchorLeft, LayerShellQt::Window::AnchorRight});
    layer->setExclusiveZone(-1);
    layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
    layer->setWantsToBeOnActiveScreen(true);
    layer->setScope("glyphpop");
}

void Picker::handleCommand(const QStringList &args)
{
    const QString command = args.value(0, "toggle");
    int section = m_section;
    if (command == "emoji")
        section = Emoji;
    else if (command == "kaomoji")
        section = Kaomoji;
    else if (command == "symbols")
        section = Symbols;
    else if (command == "settings") {
        if (opened())
            setOptions(true);
        else
            open(m_section, true);
        return;
    } else if (command == "close") {
        close();
        return;
    } else if (command != "toggle") {
        qWarning("glyphpop: unknown command %s", qPrintable(command));
        return;
    }

    if (opened() && (command == "toggle" || section == m_section))
        close();
    else if (opened())
        setSection(section);
    else
        open(command == "toggle" ? Emoji : section);
}

void Picker::open(int section, bool options)
{
    setOptions(options);
    m_query.clear();
    emit queryChanged();
    m_section = section;
    emit sectionChanged();
    rebuild();

    if (m_inputMethod->active()) {
        m_mode = Popup;
        m_popup->show();
        m_inputMethod->grabKeyboard(m_popup);
        QWindowSystemInterface::handleFocusWindowChanged(m_popup, Qt::PopupFocusReason);
    } else {
        m_mode = Overlay;
        m_overlay->show();
        m_overlay->requestActivate();
    }
    emit openedChanged();
    emit shown();
}

void Picker::close()
{
    if (m_mode == Closed)
        return;
    if (m_mode == Popup) {
        m_inputMethod->releaseKeyboard();
        QWindowSystemInterface::handleFocusWindowChanged(nullptr);
        m_popup->hide();
    } else {
        m_overlay->hide();
    }
    m_mode = Closed;
    emit openedChanged();
}

void Picker::setOptions(bool options)
{
    if (options == m_options)
        return;
    m_options = options;
    if (options)
        m_theme->rescan();
    emit optionsChanged();
}

void Picker::setSection(int section)
{
    if (section == m_section)
        return;
    m_section = section;
    m_query.clear();
    emit queryChanged();
    emit sectionChanged();
    rebuild();
}

void Picker::setQuery(const QString &query)
{
    if (query == m_query)
        return;
    m_query = query;
    emit queryChanged();
    rebuild();
}

void Picker::setTone(int tone)
{
    tone = qBound(0, tone, 5);
    if (tone == m_tone)
        return;
    m_tone = tone;
    m_settings.setValue("tone", tone);
    emit toneChanged();
    if (m_section == Emoji)
        rebuild();
}

void Picker::setSelected(int selected)
{
    selected = qBound(0, selected, qMax(0, m_model.cellCount() - 1));
    if (selected == m_selected)
        return;
    m_selected = selected;
    emit selectedChanged();
}

QString Picker::selectedName() const
{
    return m_selected < m_model.cellCount() ? m_model.cell(m_selected).name : QString();
}

QStringList Picker::toneSamples() const
{
    const int hand = gp_emoji_find("✋");
    QStringList out;
    for (uint32_t tone = 0; tone <= 5; ++tone)
        out.append(hand < 0 ? QString() : QString::fromUtf8(gp_emoji_text(uint32_t(hand), tone)));
    return out;
}

void Picker::move(int dx, int dy)
{
    if (m_model.cellCount() == 0)
        return;
    if (dx != 0) {
        setSelected(m_selected + dx);
    } else if (dy != 0) {
        const int next = m_model.verticalNeighbour(m_selected, dy);
        if (next >= 0)
            setSelected(next);
    }
}

void Picker::insert(int cell, bool keepOpen)
{
    if (m_mode == Closed || cell < 0 || cell >= m_model.cellCount())
        return;
    const GlyphCell picked = m_model.cell(cell);
    remember(m_section, picked.key);
    if (m_mode == Popup) {
        m_inputMethod->commitText(picked.text);
        if (!keepOpen && !(m_inputMethod->grabbedModifiers() & Qt::ShiftModifier))
            close();
        return;
    }
    close();
    deliver(picked.text);
}

void Picker::deliver(const QString &text)
{
    // The overlay held keyboard focus; give the compositor a moment to hand it back first.
    QTimer::singleShot(150, this, [this, text] {
        if (m_inputMethod->active())
            m_inputMethod->commitText(text);
        else if (m_keyboard->available())
            m_keyboard->type(text);
        else
            qWarning("glyphpop: nowhere to insert, the compositor offers no virtual keyboard");
    });
}

QStringList Picker::recent(int section) const
{
    static const char *const keys[] = {"recent/emoji", "recent/kaomoji", "recent/symbols"};
    return m_settings.value(keys[section]).toStringList();
}

void Picker::remember(int section, const QString &text)
{
    static const char *const keys[] = {"recent/emoji", "recent/kaomoji", "recent/symbols"};
    static const int limits[] = {16, 6, 16};
    QStringList list = recent(section);
    list.removeAll(text);
    list.prepend(text);
    while (list.size() > limits[section])
        list.removeLast();
    m_settings.setValue(keys[section], list);
}

GlyphCell Picker::emojiCell(int index) const
{
    return {QString::fromUtf8(gp_emoji_text(uint32_t(index), uint32_t(m_tone))),
            QString::fromUtf8(gp_emoji_name(uint32_t(index))), QString::fromUtf8(gp_emoji_text(uint32_t(index), 0))};
}

GlyphCell Picker::symbolCell(char32_t codepoint) const
{
    const QString text = QString::fromUcs4(&codepoint, 1);
    QString name = QString::fromUtf8(gp_symbol_name(codepoint));
    if (name.isEmpty())
        name = QStringLiteral("U+%1").arg(uint(codepoint), 4, 16, QLatin1Char('0')).toUpper();
    return {text, name, text};
}

QList<GlyphSection> Picker::emojiSections()
{
    QList<GlyphSection> sections;
    const QByteArray query = m_query.trimmed().toUtf8();
    if (!query.isEmpty()) {
        const uint32_t *hits = nullptr;
        const uint32_t count = gp_emoji_search(query.constData(), &hits);
        GlyphSection results{{}, {}, SearchCategory};
        for (uint32_t i = 0; i < count; ++i) {
            if (std::binary_search(m_visibleEmoji.begin(), m_visibleEmoji.end(), int(hits[i])))
                results.cells.append(emojiCell(int(hits[i])));
        }
        return {results};
    }

    GlyphSection recentSection{tr("Recently used"), {}, RecentCategory};
    for (const QString &text : recent(Emoji)) {
        const int index = gp_emoji_find(text.toUtf8().constData());
        if (index >= 0)
            recentSection.cells.append(emojiCell(index));
    }
    sections.append(recentSection);
    for (uint32_t g = 0; g < gp_emoji_group_count(); ++g)
        sections.append({QString::fromUtf8(gp_emoji_group_name(g)), {}, int(g)});
    for (int index : m_visibleEmoji)
        sections[1 + int(gp_emoji_group(uint32_t(index)))].cells.append(emojiCell(index));
    return sections;
}

QList<GlyphSection> Picker::kaomojiSections()
{
    QList<GlyphSection> sections;
    const QString query = m_query.trimmed();
    if (query.isEmpty()) {
        GlyphSection recentSection{tr("Recently used"), {}, RecentCategory};
        for (const QString &text : recent(Kaomoji))
            recentSection.cells.append({text, tr("Kaomoji"), text});
        sections.append(recentSection);
    }
    for (uint32_t g = 0; g < gp_kaomoji_group_count(); ++g) {
        const QString title = QString::fromUtf8(gp_kaomoji_group_name(g));
        if (!query.isEmpty() && !title.contains(query, Qt::CaseInsensitive))
            continue;
        GlyphSection section{title, {}, int(g)};
        for (uint32_t i = 0; i < gp_kaomoji_group_size(g); ++i) {
            const QString text = QString::fromUtf8(gp_kaomoji_text(g, i));
            section.cells.append({text, title, text});
        }
        sections.append(section);
    }
    return sections;
}

QList<GlyphSection> Picker::symbolSections()
{
    const auto usable = [](char32_t cp) { return QChar::category(cp) != QChar::Other_NotAssigned; };
    const QByteArray query = m_query.trimmed().toUtf8();
    if (!query.isEmpty()) {
        const uint32_t *hits = nullptr;
        const uint32_t count = gp_symbol_search(query.constData(), &hits);
        GlyphSection results{{}, {}, SearchCategory};
        for (uint32_t i = 0; i < count; ++i) {
            if (usable(hits[i]))
                results.cells.append(symbolCell(hits[i]));
        }
        return {results};
    }

    QList<GlyphSection> sections;
    GlyphSection recentSection{tr("Recently used"), {}, RecentCategory};
    for (const QString &text : recent(Symbols)) {
        const auto ucs4 = text.toUcs4();
        if (!ucs4.isEmpty())
            recentSection.cells.append(symbolCell(ucs4.first()));
    }
    sections.append(recentSection);
    for (uint32_t g = 0; g < gp_symbol_group_count(); ++g) {
        GlyphSection section{QString::fromUtf8(gp_symbol_group_name(g)), {}, int(g)};
        const uint32_t *cps = nullptr;
        const uint32_t count = gp_symbol_group_codepoints(g, &cps);
        for (uint32_t i = 0; i < count; ++i) {
            if (usable(cps[i]))
                section.cells.append(symbolCell(cps[i]));
        }
        sections.append(section);
    }
    return sections;
}

void Picker::rebuild()
{
    QList<GlyphSection> sections;
    int columns = 8;
    switch (m_section) {
    case Emoji:
        sections = emojiSections();
        break;
    case Kaomoji:
        sections = kaomojiSections();
        columns = 3;
        break;
    default:
        sections = symbolSections();
        break;
    }
    m_model.reset(sections, columns);

    QVariantList categories;
    for (const GlyphSection &section : sections) {
        if (section.cells.isEmpty() || section.category == SearchCategory)
            continue;
        QString icon;
        if (section.category == RecentCategory)
            icon = QStringLiteral("🕒");
        else if (m_section == Emoji && section.category < int(std::size(EmojiIcons)))
            icon = QString::fromUtf8(EmojiIcons[section.category]);
        else if (m_section == Symbols && section.category < int(std::size(SymbolIcons)))
            icon = QString::fromUtf8(SymbolIcons[section.category]);
        categories.append(QVariantMap{{"id", section.category}, {"title", section.title}, {"icon", icon}});
    }
    m_categories = categories;
    emit categoriesChanged();

    m_selected = 0;
    emit selectedChanged();
}
