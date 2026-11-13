#pragma once

#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QVariantList>

#include "glyphmodel.h"
#include "themesource.h"

class InputMethod;
class QQuickWindow;
class VirtualKeyboard;

class Picker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int section READ section WRITE setSection NOTIFY sectionChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int tone READ tone WRITE setTone NOTIFY toneChanged)
    Q_PROPERTY(GlyphModel *model READ model CONSTANT)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    Q_PROPERTY(int selected READ selected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectedChanged)
    Q_PROPERTY(bool opened READ opened NOTIFY openedChanged)
    Q_PROPERTY(bool overlay READ overlay NOTIFY openedChanged)
    Q_PROPERTY(QStringList toneSamples READ toneSamples CONSTANT)
    Q_PROPERTY(ThemeSource *theme READ theme CONSTANT)
    Q_PROPERTY(QString emojiFamily READ emojiFamily CONSTANT)
    Q_PROPERTY(bool options READ options WRITE setOptions NOTIFY optionsChanged)

public:
    enum Section { Emoji, Kaomoji, Symbols };
    Q_ENUM(Section)

    explicit Picker(QObject *parent = nullptr);
    ~Picker() override;

    void setWindows(QQuickWindow *popup, QQuickWindow *overlay);
    void handleCommand(const QStringList &args);

    int section() const { return m_section; }
    void setSection(int section);
    QString query() const { return m_query; }
    void setQuery(const QString &query);
    int tone() const { return m_tone; }
    void setTone(int tone);
    GlyphModel *model() { return &m_model; }
    QVariantList categories() const { return m_categories; }
    int selected() const { return m_selected; }
    void setSelected(int selected);
    QString selectedName() const;
    bool opened() const { return m_mode != Closed; }
    bool overlay() const { return m_mode == Overlay; }
    QStringList toneSamples() const;
    ThemeSource *theme() const { return m_theme; }
    QString emojiFamily() const { return m_emojiFamily; }
    bool options() const { return m_options; }
    void setOptions(bool options);

    Q_INVOKABLE void insert(int cell, bool keepOpen);
    Q_INVOKABLE void move(int dx, int dy);
    Q_INVOKABLE void close();

signals:
    void sectionChanged();
    void queryChanged();
    void toneChanged();
    void categoriesChanged();
    void selectedChanged();
    void openedChanged();
    void optionsChanged();
    // Emitted on every open so the UI can reset scroll and focus.
    void shown();

private:
    enum Mode { Closed, Popup, Overlay };

    void open(int section, bool options = false);
    void rebuild();
    QList<GlyphSection> emojiSections();
    QList<GlyphSection> kaomojiSections();
    QList<GlyphSection> symbolSections();
    GlyphCell emojiCell(int index) const;
    GlyphCell symbolCell(char32_t codepoint) const;
    QStringList recent(int section) const;
    void remember(int section, const QString &text);
    void deliver(const QString &text);

    QSettings m_settings;
    InputMethod *m_inputMethod;
    VirtualKeyboard *m_keyboard;
    ThemeSource *m_theme;
    QPointer<QQuickWindow> m_popup;
    QPointer<QQuickWindow> m_overlay;
    GlyphModel m_model;
    QList<int> m_visibleEmoji;
    QString m_emojiFamily;
    QVariantList m_categories;
    QString m_query;
    Mode m_mode = Closed;
    int m_section = Emoji;
    int m_tone = 0;
    int m_selected = 0;
    bool m_options = false;
};
